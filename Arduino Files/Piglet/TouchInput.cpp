#include "TouchInput.h"
#include "Globals.h"
#include "MeshNode.h"

// ---- On-screen button geometry (landscape 320x240 panel coordinates) ----
// Upper-right corner, beside the 2x page area (0..TFT_PAGE_W-1 across,
// 0..TFT_PAGE_H-1 down) and the same height. The strip below both is free.
static const int16_t BTN_X = TFT_PAGE_W + 4;            // 260
static const int16_t BTN_Y = 0;
static const int16_t BTN_W = TFT_PANEL_W - BTN_X;       // 60
static const int16_t BTN_H = TFT_PAGE_H;                // 128

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

  // The button touches the right and top edges of the panel, so don't bound
  // it there: uncalibrated readings can map a little past the panel.
  return (px >= BTN_X && py <= (BTN_Y + BTN_H));
}

// Set once the button has been drawn at boot, so refreshTouchButton() never
// touches the panel when TFT init failed.
static bool btnDrawn = false;

// Two hint lines under NEXT: the extra gestures available on the current page.
// Size-1 text is 6 px per char; the 60 px button fits 9 chars per line.
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
  gfx->fillRect(BTN_X, BTN_Y, BTN_W, BTN_H, RGB565_BLACK);
  gfx->drawRect(BTN_X, BTN_Y, BTN_W, BTN_H, RGB565_WHITE);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  drawCenteredInButton("NEXT", 2, BTN_Y + 40);
  drawCenteredInButton(hint1, 1, BTN_Y + 72);
  drawCenteredInButton(hint2, 1, BTN_Y + 84);
  btnDrawn = true;
}

void refreshTouchButton() {
  if (btnDrawn) drawTouchButton();
}
