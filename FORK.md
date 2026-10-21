# Piglet-TFT — developer notes

How the fork works inside, and how to keep it in step with upstream. For what
the fork is, the wiring, the touch-button gestures and the build settings, see
[`README.md`](README.md).

All fork changes are in the main sketch, `Arduino Files/Piglet/`. Every other
folder is unchanged from upstream.

## Branches and upstream

| Branch | Contents |
|---|---|
| `tft` *(default)* | The fork: upstream plus the TFT/touch changes |
| `main` | An untouched mirror of [hamspiced/piglet](https://github.com/hamspiced/piglet) `main` |

To pick up a new upstream release:

```sh
git remote add upstream https://github.com/hamspiced/piglet.git   # once
git fetch upstream
git checkout main && git merge --ff-only upstream/main && git push origin main
git checkout tft  && git merge main
```

Expect conflicts in the files the fork has changed: `Display.cpp`, `Globals.*`,
`Piglet.ino`, `PinMapDefs.h`, `TouchInput.*`, `TftCompat.h`. Upstream changes to
the page-drawing code in `Display.cpp` and `MeshNode.cpp` usually carry straight
over, because those pages still draw through the SSD1306 call surface (see
below). After merging, set `FIRMWARE_VERSION` in `Globals.h` to
`v<upstream>-tft` (for example `v2.8-tft`) and add a CHANGELOG entry. Further
fork-only releases on the same upstream base add a counter: `v2.7-tft.2`,
`v2.7-tft.3`, ….

## Display: phase 1

The stock pages (`Display.cpp`, `MeshNode.cpp`, splash and sleep screens) are
written against the Adafruit_SSD1306 API on a 128×64 monochrome buffer. Phase 1
leaves that code alone and runs it on the TFT through a small shim:

- **`TftCompat.h`** defines `PigletDisplay`, an `Arduino_Canvas` of 160×64 that
  adds the three SSD1306-only calls the pages use: `clearDisplay()` →
  `fillScreen(black)`, `display()` → a 2× scaled flush, and `ssd1306_command()` → no-op.
  `SSD1306_WHITE` / `SSD1306_BLACK` map to RGB565 white / black.
- The panel runs in **landscape, 320×240** (`gfx->setRotation(1)`, or 3 with
  `rotateScreen180`). `display()` calls `flushScaled()` rather than
  `Arduino_Canvas::flush()`: each canvas pixel is written as a 2×2 block, one
  scaled row band per transfer, so the page fills **the top 320×128** of the
  panel (`TFT_CANVAS_SCALE`, `TFT_PAGE_W/H`).
- **The canvas is 160 wide, not the OLED's 128,** so it fills the panel width at
  2×. The stock pages still lay out for `OLED_W` (128), so they sit at the left
  with 32 logical px spare on the right. That spare room is what stops long
  size-2 text from wrapping: the Status page's `"Wardrift UL"` upload header is
  132 px, wider than 128, and wraps its last letter on stock Piglet too.
- Below the page: a 3 px white separator (`TFT_SEP_Y`), then the **status strip**
  (`TFT_STRIP_Y` to the bottom), with the NEXT button at its right end
  (`TFT_BTN_X`). All of this layout lives in `TftCompat.h`.
- Each redraw sends four times the pixels of a 1:1 flush; on hardware the pig
  animation and page changes still look smooth (2026-10-21).
- Everything else (`drawRect`, `print`, `setTextSize`, `getTextBounds`, …) is
  inherited from Arduino_GFX, so the page code compiles unchanged.

**RAM:** the 160×64×16 bpp canvas is about 20 KB, plus a 1.3 KB row band for
the scaled flush, which fits comfortably without PSRAM. The fork is built and run with PSRAM disabled.

## Shared SPI bus

The SD card, the ILI9341 and the XPT2046 share one SPI bus, each with its own CS
line. What makes this work, in `Globals.cpp`:

```cpp
// Arduino_HWSPI rides the standard Arduino SPIClass (&SPI) — this is what lets
// the SD card share the bus via its own CS line. Do NOT use Arduino_ESP32SPI
// (raw-register); it would not share with SD.
Arduino_DataBus *tftBus = new Arduino_HWSPI(
    PINS_C5.tft_dc, PINS_C5.tft_cs,
    PINS_C5.sd_sck, PINS_C5.sd_mosi, PINS_C5.sd_miso,
    &SPI, true /* shared */);
Arduino_GFX *gfx = new Arduino_ILI9341(tftBus, PINS_C5.tft_rst, 0, false);
// IRQ deliberately NOT passed: every touch call is a true poll, which avoids
// XPT2046_Touchscreen's isrWake "dead touch" trap.
XPT2046_Touchscreen touch(PINS_C5.tch_cs);
```

Piglet's SD code already calls `SPI.begin(sck, miso, mosi, cs)` on the global
`SPI` and then `SD.begin(cs)`, so the TFT and touch controller coexist with it.
Before the display starts, `setup()` drives all three CS lines high so a
floating CS can't corrupt the init transfers.

**The display and touch objects are built from `PINS_C5` at static
initialisation**, not from the runtime pin map. Changing pins in the config or
web UI does not move the TFT or touch pins; edit `PINS_C5` in `PinMapDefs.h`.

Arduino_GFX colours are RGB565 (`RGB565_BLACK`, `RGB565_WHITE`, …).

## Touch input

- **`TouchInput.cpp`** polls the XPT2046 and reports whether the **NEXT** button
  zone (bottom-right, 64×105 px at x = 256, y = 135) is being touched, with a
  minimum pressure of z ≥ 300. The zone isn't bounded at the panel's right and
  bottom edges, because uncalibrated readings can map slightly past them.
- **Touch rotation follows the panel:** `touch.setRotation(gfx->getRotation())`.
  XPT2046_Touchscreen's rotations line up with Arduino_GFX's — portrait 0 and
  landscape 1 are both verified on hardware.
- **Uncalibrated:** raw readings are mapped linearly from 300–3800 to panel
  pixels. That is accurate enough for one large corner button. Smaller phase-2
  touch targets will need a two-point calibration, with raw min/max kept on the
  SD card.
- `pollButton()` in `Piglet.ino` feeds that pressed / not-pressed signal into
  upstream's button gesture state machine as if it were the old `INPUT_PULLUP`
  button (pressed = LOW). A 1.5 s settle window after boot ignores spurious
  touches while the bus and panel come up.
- **The NEXT button and its hint line are drawn directly on the panel**, outside
  the canvas, so canvas flushes never overwrite them. `refreshTouchButton()`
  redraws the button when its hint changes: on a page change, a status-page
  pause toggle, and a mesh Core/Node switch.

## Status strip (`StatusStrip.cpp`)

Drawn directly on the panel, outside the page canvas, so canvas flushes never
touch it. `statusStripInit()` draws the separator at boot; `loop()` calls
`statusStripDrawGps()` after each 500 ms page refresh while on the Navigation
page (2), and `onPageChange()` clears the strip when leaving it.

The GPS state comes from TinyGPSPlus's own counters — the same ones as the
serial `[GPS] chars=… ok=… fail=…` diagnostic:

- **No data:** no new bytes (`charsProcessed()`) for 3 s, or none since boot.
- **Garbled:** more checksum failures than passes *since the previous redraw*,
  so an old burst of errors doesn't stick.
- **Searching / fix:** `gpsHasFix`, with `satellites` and `hdop`.
- Fix history uses `lastGpsValid` / `lastGpsValidMs` from the position cache.

Rows are size-2 text, 20 characters each, drawn with a background colour and
padded to full width, so they overwrite in place without clearing or flicker.
Size-1 grey text was tried first and was unreadable outdoors.

### Changes from upstream's gesture handling

- Pages without multi-press gestures (1, 2, 3, 5) act on release instead of
  waiting out `DOUBLE_PRESS_MS`, the 350 ms window (`pageHasMultiPress()`).
- The multi-press window is measured from the **latest** release, not the first,
  and isn't evaluated while a follow-up press is held. Upstream measures from the
  first release, which makes a triple press almost impossible with touch.
- A page change sets `pageRedrawPending`, so `loop()` redraws at once instead of
  waiting for the next 500 ms refresh.
- The status page's play/pause glyph includes `statusPagePaused`. Upstream
  leaves it out, so the double-press pause never showed on screen.

Both upstream bugs, the triple press and the glyph, also affect stock Piglet.

### Deep sleep

A 2 s hold on any page except the mesh page enters deep sleep. The wake source
is the XPT2046 IRQ line (`pins.tch_irq`, D5 / GPIO 24), which goes low while the
panel is touched. *Not yet tested on hardware.*

## Status

- [x] Pin map: C5 set to TFT + touch; OLED (`sda` / `scl`) and `btn` set to -1.
- [x] ILI9341 via Arduino_GFX on the shared bus; stock pages render through `TftCompat.h`.
- [x] Touch NEXT button driving the gesture state machine, with a page-specific hint line.
- [x] Built and flashed on a XIAO ESP32-C5 (core 3.3.10); TFT pages and all
      touch gestures tested on the device (2026-10-21).
- [x] Uploads work on the fork, with PSRAM disabled (2026-10-21; service not recorded).
- [x] Landscape layout, full-width 2× pages, divider, NEXT bottom-right; touch
      works in landscape (2026-10-21).
- [x] GPS fix, and the GPS status strip on the Navigation page (2026-10-21).
- [ ] SD logging — not yet confirmed on the fork.
- [ ] Touch wake from deep sleep — not yet tested.
- [ ] `rotateScreen180` — not yet tested. It now turns the whole panel (rotation
      3), and the touch mapping follows.

## Phase 2 (planned)

Redesign the pages for the full 320×240 colour panel instead of the 2×-scaled
128×64 canvas.

- A full-screen 16 bpp canvas is about 150 KB. That likely needs PSRAM, or
  partial / unbuffered redraws instead of a full framebuffer.
- More, smaller touch targets will need the two-point touch calibration
  described above.
- The page code would move off the SSD1306 call surface. Upstream changes to
  `Display.cpp` would then stop merging cleanly, so plan for upstream merges to
  become manual.
