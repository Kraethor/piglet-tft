#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// =============================================================================
//  TftCompat.h  --  fork shim: run Piglet's OLED UI on the ILI9341 TFT
// =============================================================================
// The stock Piglet pages (Display.cpp, MeshNode.cpp) and the deep-sleep screen
// are written against the Adafruit_SSD1306 call surface: clearDisplay(),
// display(), ssd1306_command(), and the SSD1306_WHITE / SSD1306_BLACK color
// constants, all on a 128x64 monochrome canvas.
//
// Phase-1 of this fork keeps all of that code unchanged and renders it into an
// OLED-height (160x64) Arduino_GFX framebuffer. The panel runs in landscape
// (320x240), and display() flushes the framebuffer at 2x scale (320x128) across
// the top. Below a separator line, the strip across the bottom holds the status
// lines (StatusStrip.cpp) and, at its right end, the touch button. A
// full-screen redesign of the pages is a later pass (see FORK.md).
// PigletDisplay is that framebuffer plus the three SSD1306-only methods the
// ported code still calls.

// SPI clock for the display (shared bus; matches WifiScanner_v1).
#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY 40000000
#endif

// Panel geometry (landscape: gfx rotation 1, or 3 with rotateScreen180) and
// the page canvas, drawn at TFT_CANVAS_SCALE across the top of the panel.
//
// The canvas is 160 wide rather than the OLED's 128 so it fills the panel at
// 2x. The stock pages still lay out for 128 (OLED_W), so they sit at the left
// with spare room on the right -- which is what keeps long size-2 text such as
// the "Wardrift UL" upload header from wrapping onto the next line.
#define TFT_PANEL_W  320
#define TFT_PANEL_H  240
#define TFT_CANVAS_W 160
#define TFT_CANVAS_H 64
#define TFT_CANVAS_SCALE 2
#define TFT_CANVAS_X 0
#define TFT_CANVAS_Y 0
#define TFT_PAGE_W   (TFT_CANVAS_W * TFT_CANVAS_SCALE)  // 320 px on the panel
#define TFT_PAGE_H   (TFT_CANVAS_H * TFT_CANVAS_SCALE)  // 128 px on the panel

// Below the page: a separator, then the bottom strip. The touch button takes
// the right end of the strip; the status lines use the rest.
#define TFT_SEP_Y    (TFT_PAGE_H + 2)                   // 130
#define TFT_SEP_H    3
#define TFT_STRIP_Y  (TFT_SEP_Y + TFT_SEP_H + 2)        // 135
#define TFT_STRIP_H  (TFT_PANEL_H - TFT_STRIP_Y)        // 105
#define TFT_BTN_W    64
#define TFT_BTN_X    (TFT_PANEL_W - TFT_BTN_W)          // 256

// SSD1306 color / command names the ported page code still uses, mapped onto
// RGB565. White text on a black background reads correctly; SSD1306_BLACK is
// used by the page code to carve black detail out of white shapes, which lands
// as black pixels on the (black) background -- same visual intent.
#ifndef SSD1306_WHITE
#define SSD1306_WHITE 0xFFFF
#endif
#ifndef SSD1306_BLACK
#define SSD1306_BLACK 0x0000
#endif
#ifndef SSD1306_SWITCHCAPVCC
#define SSD1306_SWITCHCAPVCC 0x02
#endif
#ifndef SSD1306_DISPLAYOFF
#define SSD1306_DISPLAYOFF 0xAE
#endif

// Framebuffered canvas that adds the SSD1306-only call surface on top of
// Arduino_Canvas. Everything else (drawRect/fillCircle/print/getTextBounds/
// setTextSize/setCursor/setRotation/...) is inherited from Arduino_GFX.
class PigletDisplay : public Arduino_Canvas {
public:
  PigletDisplay(int16_t w, int16_t h, Arduino_GFX *output, int16_t x, int16_t y)
    : Arduino_Canvas(w, h, output, x, y), out_(output), outX_(x), outY_(y) {}

  // SSD1306 buffer semantics -> canvas framebuffer semantics.
  inline void clearDisplay() { fillScreen(RGB565_BLACK); }
  inline void display()      { flushScaled(); }

  // No panel command interface on a color TFT; swallow (e.g. DISPLAYOFF).
  inline void ssd1306_command(uint8_t) {}

private:
  Arduino_GFX *out_;
  int16_t outX_, outY_;

  // Arduino_Canvas::flush() copies the framebuffer 1:1. This writes each
  // canvas row TFT_CANVAS_SCALE times, each pixel TFT_CANVAS_SCALE wide, one
  // scaled row band per transfer.
  void flushScaled() {
    const uint16_t *fb = getFramebuffer();
    if (!fb || !out_) return;
    static uint16_t band[TFT_PAGE_W * TFT_CANVAS_SCALE];
    for (int16_t y = 0; y < TFT_CANVAS_H; y++) {
      const uint16_t *src = fb + (int32_t)y * TFT_CANVAS_W;
      uint16_t *dst = band;
      for (int16_t x = 0; x < TFT_CANVAS_W; x++) {
        for (uint8_t s = 0; s < TFT_CANVAS_SCALE; s++) *dst++ = src[x];
      }
      for (uint8_t r = 1; r < TFT_CANVAS_SCALE; r++) {
        memcpy(band + r * TFT_PAGE_W, band, TFT_PAGE_W * sizeof(uint16_t));
      }
      out_->draw16bitRGBBitmap(outX_, outY_ + y * TFT_CANVAS_SCALE,
                               band, TFT_PAGE_W, TFT_CANVAS_SCALE);
    }
  }
};
