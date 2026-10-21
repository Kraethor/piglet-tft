#pragma once
#include <Arduino.h>

struct PinMap {
  int sda, scl;  // OLED I2C (-1 on TFT builds: OLED removed)
  int sd_cs, sd_sck, sd_miso, sd_mosi; // SD (SPI; the bus is shared with the TFT + touch on TFT builds)
  int gps_rx, gps_tx; // GPS
  int btn; // Physical button (-1 = none; TFT build uses an on-screen touch button instead)
  // --- TFT + touch (fork additions) ---
  // On TFT builds the ILI9341 and the XPT2046 touch controller share the SD SPI
  // bus (sd_sck / sd_mosi / sd_miso) and each have their own CS line. Only the
  // non-bus control lines live here. -1 means "not present on this board".
  int tft_cs, tft_dc, tft_rst;  // ILI9341 control lines
  int tch_cs, tch_irq;          // XPT2046 chip-select + interrupt
  const char* name;  // "S3" / "C6" / etc
};

// --- S3 default pins (your original) ---
static const PinMap PINS_S3 = {
  5, 6,
  4, 7, 8, 9,
  44, 43,
  2,
  -1, -1, -1,   // no TFT
  -1, -1,       // no touch
  "S3"
};

// --- C6 default pins ---
static const PinMap PINS_C6 = {
  22, 23,
  21, 19, 20, 18,
  17, 16,
  1,
  -1, -1, -1,   // no TFT
  -1, -1,       // no touch
  "C6"
};

// --- C5 default pins (FORK: TFT + touch, OLED + physical button removed) ---
// Shared SPI bus: SCK=D8/GPIO8, MOSI=D10/GPIO10, MISO=D9/GPIO9.
// SD CS=D3/GPIO7, TFT CS=D0/GPIO1, Touch CS=D4/GPIO23 — one bus, three CS lines.
// TFT DC=D1/GPIO0, TFT RESET=D2/GPIO25, Touch IRQ=D5/GPIO24.
// GPS unchanged: RX=D7/GPIO12 (from GPS TX), TX=D6/GPIO11 (to GPS RX).
// Matches the WifiScanner_v1 wiring (ai-knowledge: Projects/wifi-scanner-poc.md).
static const PinMap PINS_C5 = {
  -1, -1,        // OLED removed (D4/D5 now drive the XPT2046 touch CS/IRQ)
  7, 8, 9, 10,   // SD: cs(D3), sck(D8), miso(D9), mosi(D10) — shared bus
  12, 11,        // GPS: rx(D7), tx(D6)
  -1,            // physical button removed — on-screen touch button replaces it
  1, 0, 25,      // TFT: cs(D0/GPIO1), dc(D1/GPIO0), rst(D2/GPIO25)
  23, 24,        // Touch: cs(D4/GPIO23), irq(D5/GPIO24)
  "C5"
};

// --- S3 + Expansion Base ---
static const PinMap PINS_S3_EXP_BASE = {
  5, 6,
  3, 7, 8, 10,
  12, 11,
  2,
  -1, -1, -1,   // no TFT
  -1, -1,       // no touch
  "EXP_BASE"
};

// --- XIAO ESP32-C3 ---
// Headless (no built-in OLED), 2.4 GHz only.
// SDA/SCL (D4/D5) are exposed and can drive an external OLED if attached.
// SD on SPI2 (D8/D9/D10); GPS on UART1 (D7=RX, D6=TX).
// btn=-1: GPIO9 (D9) conflicts with SPI MISO so no dedicated button pin.
static const PinMap PINS_XIAO_C3 = {
  6,  7,           // SDA (D4/GPIO6), SCL (D5/GPIO7) — OLED if attached
  2,  8, 9,  10,  // SD:  cs(D0), sck(D8), miso(D9), mosi(D10)
  20, 21,          // GPS: rx(D7/GPIO20 from GPS TX), tx(D6/GPIO21 to GPS RX)
  -1,              // BTN: none (GPIO9=SPI MISO conflict; wire external if needed)
  -1, -1, -1,      // no TFT
  -1, -1,          // no touch
  "C3"
};
