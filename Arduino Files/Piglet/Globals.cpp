#include "Globals.h"

// ---- Global objects ----
PinMap pins = PINS_C5; // fork targets XIAO ESP32-C5 (re-detected in setup())
Config cfg;

// Display: ILI9341 on the shared SPI bus. Arduino_HWSPI rides the standard
// Arduino &SPI object, which is what lets the SD card and the XPT2046 touch
// controller share this bus via their own CS lines (per WifiScanner_v1). The
// bus pins are the SD SPI pins; only DC/CS/RST are display-specific. Phase-1
// draws the pages into a 128x64 framebuffer centered on the 240x320 panel.
Arduino_DataBus *tftBus = new Arduino_HWSPI(
    PINS_C5.tft_dc, PINS_C5.tft_cs,
    PINS_C5.sd_sck, PINS_C5.sd_mosi, PINS_C5.sd_miso,
    &SPI, true /* shared interface w/ SD + touch */);
Arduino_GFX *gfx = new Arduino_ILI9341(
    tftBus, PINS_C5.tft_rst, 0 /* rotation: portrait */, false /* not IPS */);
PigletDisplay display(TFT_CANVAS_W, TFT_CANVAS_H, gfx, TFT_CANVAS_X, TFT_CANVAS_Y);

// Touch: TOUCH_IRQ deliberately NOT passed -> every call is a true poll, which
// avoids XPT2046_Touchscreen's isrWake "dead touch" trap (see FORK.md).
XPT2046_Touchscreen touch(PINS_C5.tch_cs);

TinyGPSPlus gps;
HardwareSerial GPSSerial(1);
WebServer server(80);

// ---- State flags ----
bool sdOk = false;
bool scanningEnabled = true;
bool gpsHasFix = false;
bool allowScanForOled = false;
bool userScanOverride = false;
bool autoPaused = false;

// ---- OLED page system ----
uint8_t currentPage = 0;
bool statusPagePaused = false;

// ---- AP state ----
uint32_t apStartMs = 0;
bool apClientSeen = false;
bool apWindowActive = false;
const uint32_t AP_WINDOW_MS = 60000UL;

bool apExtended = false;
uint32_t apExtendedStartMs = 0;
bool apForceClose = false;
const uint32_t AP_EXTENDED_WINDOW_MS = 5UL * 60UL * 1000UL;  // 5 minutes
const uint32_t AP_EXTEND_PROMPT_LEAD_MS = 30000UL;            // prompt 30 s before expiry

// ---- Counters ----
uint32_t networksFound2G = 0;
uint32_t networksFound5G = 0;

// ---- Log state ----
File logFile;
String currentCsvPath;
wl_status_t lastStaStatus = WL_IDLE_STATUS;

// ---- Upload state ----
bool     uploading = false;
bool     uploadPausedScanWasEnabled = false;
uint32_t uploadTotalFiles = 0;
uint32_t uploadDoneFiles  = 0;
String   uploadCurrentFile = "";
String   uploadLastResult  = "";
String   uploadTargetName  = "";
uint32_t uploadFailedFiles = 0;
int      wigleTokenStatus  = 0;
int      wigleLastHttpCode = 0;

// ---- WiGLE constants ----
const char* WIGLE_HOST = "api.wigle.net";
const uint16_t WIGLE_PORT = 443;

// ---- Wardrift constants ----
const char* WARDRIFT_HOST = "wardrift.net";
const uint16_t WARDRIFT_PORT = 443;
