# Piglet-TFT — fork notes

Fork of [hamspiced/piglet](https://github.com/hamspiced/piglet) (the ESP32 XIAO
wardriver) that swaps the small I²C OLED + physical button for the 2.8" SPI TFT
(ILI9341 240×320) + XPT2046 resistive touch already used by the `WifiScanner_v1`
build. Same MCU as stock Piglet's C5 kit — a Seeed **XIAO ESP32-C5** — so this is
a display/input change, not a board change.

Upstream and this fork share the GPS (ATGM336H, UART) and the SD card module (SPI)
with identical wiring; only the display subsystem differs.

## Goal

- Remove the SSD1306 OLED and the physical button.
- Drive the ILI9341 TFT + XPT2046 touch over the **shared SD SPI bus**.
- Replace the button's gesture vocabulary (single / double / triple / long press)
  with an on-screen **touch button** living in a screen corner (temporary home),
  reusing the existing gesture state machine unchanged.

## Pin map (XIAO ESP32-C5) — see `Arduino Files/Piglet/PinMapDefs.h`

Shared SPI bus: SCK=D8/GPIO8, MOSI=D10/GPIO10, MISO=D9/GPIO9.

| Signal      | XIAO pad | GPIO  | Notes |
|-------------|----------|-------|-------|
| SD CS       | D3       | 7     | shared bus |
| TFT CS      | D0       | 1     | shared bus |
| TFT DC      | D1       | 0     | (was the physical button) |
| TFT RESET   | D2       | 25    | |
| Touch CS    | D4       | 23    | (was OLED SDA) |
| Touch IRQ   | D5       | 24    | (was OLED SCL) — can wake from deep sleep |
| GPS RX      | D7       | 12    | unchanged |
| GPS TX      | D6       | 11    | unchanged |

This is the same wiring as the author's earlier `WifiScanner_v1` TFT build.

## Status — phase-1 implemented, built and running (2026-10-21)

- [x] Repo cloned; `build/` artifacts untracked.
- [x] Pin map: C5 set to TFT+touch; OLED (`sda`/`scl`) and `btn` set to -1.
- [x] Display backend: ILI9341 via Arduino_GFX (`Arduino_HWSPI` on `&SPI`).
      `TftCompat.h` shims the SSD1306 call surface onto an Arduino_Canvas, so the
      existing pages (`Display.cpp`, `MeshNode.cpp`, splash, sleep screen) render
      unchanged — into a **128×64 framebuffer centered on the 240×320 panel**
      (phase-1). Full-screen 240×320 redesign is phase-2.
- [x] Touch input (`TouchInput.cpp`): XPT2046 polled on the shared bus; an
      on-screen "NEXT" button in the bottom-right corner is fed into the existing
      button gesture state machine (`pollButton()`), so single / double / triple /
      long-press all still work from that one corner.
- [x] Deep-sleep wake moved from the button pin to the touch IRQ (`pins.tch_irq`).
- [x] Compile/flash verification — built and flashed onto a XIAO ESP32-C5
      2026-10-21; TFT pages and all touch-button gestures tested on the device.
      SD logging, GPS fix, uploads and touch wake from deep sleep not yet tested.

### Required libraries (add to the build)

- `Arduino_GFX_Library` (moononournation) — replaces TFT_eSPI.
- `XPT2046_Touchscreen` (PaulStoffregen) — touch.
- (`Adafruit_SSD1306` / `Adafruit_GFX` are no longer needed.)

### Phase-1 caveats to check on hardware

- **UI is small and centered**, with black margins — expected for phase-1.
- **Touch mapping is uncalibrated** (`TouchInput.cpp` uses a generous fixed
  raw→pixel range). If the corner button misses, port `WifiScanner_v1`'s 2-point
  calibration (raw min/max persisted to SD).
- **Canvas RAM**: a 128×64×16bpp framebuffer is ~16 KB — fine without PSRAM, but
  keep PSRAM on per upstream build notes. (A full-screen phase-2 canvas would be
  ~150 KB and would need PSRAM.)
- **Flush cadence**: pages redraw at 2 Hz (pig page faster); the corner button is
  painted once at boot and persists because the canvas flush only repaints the
  centered region.

## Reference & proven approach (shared SPI — solved)

The trickiest part is three SPI devices (TFT, SD, touch) sharing one bus with
separate CS lines on the C5. The author's earlier `WifiScanner_v1` TFT build
already solved it.

Proven stack ported from it:

```cpp
#include <Arduino_GFX_Library.h>
#include <XPT2046_Touchscreen.h>
// Arduino_HWSPI rides the standard Arduino SPIClass (&SPI) — THIS is what lets
// the SD card share the bus safely via its own CS line. Do NOT use
// Arduino_ESP32SPI (raw-register) here; it would not share with SD.
Arduino_DataBus *bus = new Arduino_HWSPI(TFT_DC, TFT_CS, TFT_SCK, TFT_MOSI, TFT_MISO, &SPI, true /*shared*/);
Arduino_GFX    *gfx  = new Arduino_ILI9341(bus, TFT_RST, 0 /*portrait*/, false /*not IPS*/);
// IRQ deliberately NOT passed: keeps every touch call a true poll and avoids the
// XPT2046_Touchscreen isrWake dead-touch trap.
XPT2046_Touchscreen touch(TOUCH_CS);
```

Piglet's SD code already calls `SPI.begin(sck, miso, mosi, cs)` on the global
`SPI` and then `SD.begin(cs)`, so TFT + touch on `&SPI` coexist with it directly.

Colors in Arduino_GFX are RGB565 (`RGB565_BLACK`, `RGB565_WHITE`, …), not the
SSD1306 1-bit constants. Touch returns raw 0..4095; `WifiScanner_v1` maps raw→
pixel via a 2-point calibration persisted to the SD card.

## Build note

See **Building Firmware** in `README.md` for board settings and flashing.
