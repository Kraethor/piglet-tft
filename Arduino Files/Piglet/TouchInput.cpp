#include "TouchInput.h"
#include "Globals.h"

// ---- On-screen button geometry (full 240x320 panel coordinates) ----
// Bottom-right corner, clear of the centered 128x64 page canvas (which occupies
// y = TFT_CANVAS_Y .. TFT_CANVAS_Y+TFT_CANVAS_H, i.e. 128..191).
static const int16_t BTN_X = 150;
static const int16_t BTN_Y = 256;
static const int16_t BTN_W = 86;
static const int16_t BTN_H = 60;

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
  touch.setRotation(0);      // matches gfx rotation(0); refine with calibration
  Serial.println(ok ? "[TOUCH] XPT2046 init OK"
                    : "[TOUCH] XPT2046 init FAILED (on-screen button won't work)");
}

bool touchButtonPressed() {
  if (!touch.touched()) return false;

  TS_Point p = touch.getPoint();
  if (p.z < TOUCH_Z_MIN) return false;

  int px = map(p.x, RAW_MIN, RAW_MAX, 0, TFT_PANEL_W);
  int py = map(p.y, RAW_MIN, RAW_MAX, 0, TFT_PANEL_H);

  return (px >= BTN_X && px <= (BTN_X + BTN_W) &&
          py >= BTN_Y && py <= (BTN_Y + BTN_H));
}

void drawTouchButton() {
  // Drawn directly on the panel (not the page canvas) so it persists across
  // page redraws -- the canvas flush only repaints the centered page region.
  gfx->fillRect(BTN_X, BTN_Y, BTN_W, BTN_H, RGB565_BLACK);
  gfx->drawRect(BTN_X, BTN_Y, BTN_W, BTN_H, RGB565_WHITE);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setTextSize(2);
  gfx->setCursor(BTN_X + 13, BTN_Y + 22);
  gfx->print("NEXT");
}
