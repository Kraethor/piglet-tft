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
// OLED-sized (128x64) Arduino_GFX framebuffer that is flushed to a region
// *centered* on the 240x320 TFT. A full-screen 240x320 redesign of the pages
// is a later pass (see FORK.md). PigletDisplay is that framebuffer plus the
// three SSD1306-only methods the ported code still calls.

// SPI clock for the display (shared bus; matches WifiScanner_v1).
#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY 40000000
#endif

// Panel geometry and the centered OLED-sized canvas placed on it.
#define TFT_PANEL_W  240
#define TFT_PANEL_H  320
#define TFT_CANVAS_W 128
#define TFT_CANVAS_H 64
#define TFT_CANVAS_X ((TFT_PANEL_W - TFT_CANVAS_W) / 2)  // 56
#define TFT_CANVAS_Y ((TFT_PANEL_H - TFT_CANVAS_H) / 2)  // 128

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
    : Arduino_Canvas(w, h, output, x, y) {}

  // SSD1306 buffer semantics -> canvas framebuffer semantics.
  inline void clearDisplay() { fillScreen(RGB565_BLACK); }
  inline void display()      { flush(); }

  // No panel command interface on a color TFT; swallow (e.g. DISPLAYOFF).
  inline void ssd1306_command(uint8_t) {}
};
