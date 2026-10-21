#include "TouchInput.h"
#include "Globals.h"
#include "MeshNode.h"

// ---- On-screen button geometry (landscape 320x240 panel coordinates) ----
// Bottom-right corner: the right end of the status strip, below the separator
// under the full-width 2x page (see TftCompat.h).
static const int16_t BTN_X = TFT_BTN_X;                 // 256
static const int16_t BTN_Y = TFT_STRIP_Y;               // 135
static const int16_t BTN_W = TFT_BTN_W;                 // 64
static const int16_t BTN_H = TFT_STRIP_H;               // 105

// ---- Raw -> pixel mapping ----
// Uncalibrated. XPT2046 raw readings run ~0..4095; these are typical usable
// extents. Phase-1 only needs "is the corner tapped?", so the zone is generous
// and tolerates the imprecision. If the corner misses on hardware, port
// WifiScanner_v1's 2-point calibration (raw min/max persisted to SD).
static const int RAW_MIN = 300;
static const int RAW_MAX = 3800;

// Minimum pressure to count as a real touch (XPT2046 z).
static const int16_t TOUCH_Z_MIN = 300;

void touchInit() {
  bool ok = touch.begin();   // uses the shared &SPI bus (default argument)
  // XPT2046_Touchscreen's rotations line up with the panel's: portrait 0 was
  // verified on hardware, and 1 / 3 are its 90-degree / 270-degree turns.
  touch.setRotation(gfx->getRotation());
  Serial.println(ok ? "[TOUCH] XPT2046 init OK"
                    : "[TOUCH] XPT2046 init FAILED (on-screen button won't work)");
}

bool touchButtonPressed() {
  if (!touch.touched()) return false;

  TS_Point p = touch.getPoint();
  if (p.z < TOUCH_Z_MIN) return false;

  int px = map(p.x, RAW_MIN, RAW_MAX, 0, TFT_PANEL_W);
  int py = map(p.y, RAW_MIN, RAW_MAX, 0, TFT_PANEL_H);

  // The button touches the right and bottom edges of the panel, so don't
  // bound it there: uncalibrated readings can map a little past the panel.
  return (px >= BTN_X && py >= BTN_Y);
}

// Set once the button has been drawn at boot, so refreshTouchButton() never
// touches the panel when TFT init failed.
static bool btnDrawn = false;

// Two hint lines under NEXT: the extra gestures available on the current page.
// Size-1 text is 6 px per char; the 64 px button fits 9 chars per line
// inside its border.
static void buttonHint(const char*& line1, const char*& line2) {
  switch (currentPage) {
    case 0:  line1 = "2x:";     line2 = statusPagePaused ? "resume" : "pause"; break;
    case 4:  line1 = "2x oink"; line2 = "3x sq"; break;
    case 5:  line1 = "hold:";   line2 = meshCoreActive ? "node" : "core"; break;
    default: line1 = "hold:";   line2 = "sleep"; break;
  }
}

static void drawCenteredInButton(const char* txt, uint8_t size, int16_t y) {
  int16_t w = (int16_t)strlen(txt) * 6 * size;
  gfx->setTextSize(size);
  gfx->setCursor(BTN_X + (BTN_W - w) / 2, y);
  gfx->print(txt);
}

void drawTouchButton() {
  // Drawn directly on the panel (not the page canvas) so it persists across
  // page redraws -- the canvas flush only repaints the 2x page area.
  const char *hint1, *hint2;
  buttonHint(hint1, hint2);
  // 2 px border so it reads in daylight like the separator above it.
  gfx->fillRect(BTN_X, BTN_Y, BTN_W, BTN_H, RGB565_BLACK);
  gfx->drawRect(BTN_X, BTN_Y, BTN_W, BTN_H, RGB565_WHITE);
  gfx->drawRect(BTN_X + 1, BTN_Y + 1, BTN_W - 2, BTN_H - 2, RGB565_WHITE);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  drawCenteredInButton("NEXT", 2, BTN_Y + 26);
  drawCenteredInButton(hint1, 1, BTN_Y + 60);
  drawCenteredInButton(hint2, 1, BTN_Y + 72);
  btnDrawn = true;
}

void refreshTouchButton() {
  if (btnDrawn) drawTouchButton();
}
