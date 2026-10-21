#include "StatusStrip.h"
#include "Globals.h"
#include "Config.h"
#include "Scanner.h"

// ---- Geometry (landscape 320x240 panel coordinates, see TftCompat.h) ----
// The strip runs from TFT_STRIP_Y to the bottom of the panel; the touch button
// takes its right end, so the status text stops short of TFT_BTN_X.
static const int16_t TEXT_X = 4;
static const int16_t TEXT_W = TFT_BTN_X - 4 - TEXT_X;   // 248

// Five size-2 rows (16 px tall, 20 px apart). Size 2 is 12 px per char, so a
// row holds 20 chars. Everything is size 2 and white (state in colour) -- size
// 1 and grey were unreadable outdoors.
static const uint8_t TEXT_SIZE = 2;
static const int16_t ROW_Y0    = TFT_STRIP_Y + 3;
static const int16_t ROW_STEP  = 20;

// No new GPS bytes for this long counts as "no data".
static const uint32_t GPS_SILENT_MS = 3000;

static const uint16_t COLOR_FIX = RGB565(0, 220, 0);  // RGB565_GREEN is too dark

// Set by statusStripInit(), so nothing draws on the panel if TFT init failed.
static bool stripReady = false;

// Draw one row padded to the text width. Text is drawn with a background
// colour, so overwriting the previous contents needs no clear (and no flicker).
static void drawRow(uint8_t row, uint16_t color, const char* txt) {
  const int cols = TEXT_W / (6 * TEXT_SIZE);
  char buf[32];
  snprintf(buf, sizeof(buf), "%-*.*s", cols, cols, txt);
  gfx->setTextSize(TEXT_SIZE);
  gfx->setTextColor(color, RGB565_BLACK);
  gfx->setCursor(TEXT_X, ROW_Y0 + row * ROW_STEP);
  gfx->print(buf);
}

// "45s", "3m12s", "1h05m"
static void formatAge(char* out, size_t n, uint32_t ms) {
  uint32_t s = ms / 1000;
  if (s < 60)        snprintf(out, n, "%lus", (unsigned long)s);
  else if (s < 3600) snprintf(out, n, "%lum%02lus", (unsigned long)(s / 60), (unsigned long)(s % 60));
  else               snprintf(out, n, "%luh%02lum", (unsigned long)(s / 3600), (unsigned long)((s % 3600) / 60));
}

// "48213", then "3456k" once it no longer fits a 20-char row comfortably
static void formatCount(char* out, size_t n, uint32_t v) {
  if (v < 100000) snprintf(out, n, "%lu", (unsigned long)v);
  else            snprintf(out, n, "%luk", (unsigned long)(v / 1000));
}

void statusStripInit() {
  // 3 px white so it shows outdoors; a 1 px grey line didn't.
  gfx->fillRect(0, TFT_SEP_Y, TFT_PANEL_W, TFT_SEP_H, RGB565_WHITE);
  stripReady = true;
}

void statusStripClear() {
  if (!stripReady) return;
  gfx->fillRect(0, TFT_STRIP_Y, TFT_BTN_X - 1, TFT_STRIP_H, RGB565_BLACK);
}

void statusStripDrawGps() {
  if (!stripReady) return;

  const uint32_t now    = millis();
  const uint32_t chars  = gps.charsProcessed();
  const uint32_t ok     = gps.passedChecksum();
  const uint32_t failed = gps.failedChecksum();

  // Track when bytes last arrived, and the checksum counts since the last draw
  // (the strip is redrawn every 500 ms), so an old burst of errors doesn't
  // keep the state stuck on "garbled".
  static uint32_t prevChars = 0, prevOk = 0, prevFailed = 0;
  static uint32_t lastByteMs = 0;
  if (chars != prevChars) lastByteMs = now;
  const uint32_t dOk     = ok - prevOk;
  const uint32_t dFailed = failed - prevFailed;
  prevChars = chars; prevOk = ok; prevFailed = failed;

  const bool silent = (chars == 0) || (now - lastByteMs > GPS_SILENT_MS);
  const int  sats   = gps.satellites.isValid() ? (int)gps.satellites.value() : -1;

  char line[32], a[16], b[16], age[16];

  // Rows 0-1: state and what it means
  if (silent) {
    drawRow(0, RGB565_RED, "GPS: NO DATA");
    snprintf(line, sizeof(line), "Check TX->GPIO%d", pins.gps_rx);
  } else if (dFailed > dOk) {
    drawRow(0, RGB565_ORANGE, "GPS: GARBLED");
    snprintf(line, sizeof(line), "Check baud %lu", (unsigned long)cfg.gpsBaud);
  } else if (!gpsHasFix) {
    drawRow(0, RGB565_YELLOW, "GPS: SEARCHING");
    if (sats >= 0) snprintf(line, sizeof(line), "%d sats in view", sats);
    else           snprintf(line, sizeof(line), "Sats: no data yet");
  } else {
    drawRow(0, COLOR_FIX, "GPS: FIX");
    if (gps.hdop.isValid())
      snprintf(line, sizeof(line), "%d sats HDOP %.1f", sats < 0 ? 0 : sats, gps.hdop.hdop());
    else
      snprintf(line, sizeof(line), "%d sats", sats < 0 ? 0 : sats);
  }
  drawRow(1, RGB565_WHITE, line);

  // Rows 2-3: raw parser counters (the same ones the serial log prints).
  // ok / bad = NMEA sentences passing / failing checksum, rx = bytes received.
  formatCount(a, sizeof(a), ok);
  formatCount(b, sizeof(b), failed);
  snprintf(line, sizeof(line), "ok %s bad %s", a, b);
  drawRow(2, RGB565_WHITE, line);

  if (chars == 0) {
    snprintf(line, sizeof(line), "rx 0 since boot");
  } else {
    formatCount(a, sizeof(a), chars);
    formatAge(age, sizeof(age), now - lastByteMs);
    snprintf(line, sizeof(line), "rx %s last %s", a, age);
  }
  drawRow(3, RGB565_WHITE, line);

  // Row 4: position, or how long without one
  if (gpsHasFix) {
    snprintf(line, sizeof(line), "%.5f,%.5f", gps.location.lat(), gps.location.lng());
  } else if (lastGpsValid) {
    formatAge(age, sizeof(age), now - lastGpsValidMs);
    snprintf(line, sizeof(line), "Last fix %s ago", age);
  } else {
    formatAge(age, sizeof(age), now);
    snprintf(line, sizeof(line), "No fix yet %s", age);
  }
  drawRow(4, RGB565_WHITE, line);
}
