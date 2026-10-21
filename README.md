
# Piglet-TFT

**An unofficial fork of [Piglet Wardriver](https://github.com/hamspiced/piglet) by Midwest Gadgets LLC** that replaces Piglet's small I²C OLED and physical button with a **2.8" ILI9341 SPI TFT (240×320) and XPT2046 resistive touch**, on the same **Seeed XIAO ESP32-C5**.

Everything else — Wi-Fi scanning, GPS, WiGLE CSV logging to SD, the web UI, uploads, and ESP-Now mesh — is upstream Piglet firmware. This fork changes only the display and input. It is not affiliated with or supported by Midwest Gadgets; please report fork issues here, not upstream.

## What's different from upstream

- **Display:** ILI9341 TFT via [Arduino_GFX](https://github.com/moononournation/Arduino_GFX), sharing the SD card's SPI bus. The panel runs in landscape (320×240). The stock 128×64 pages are drawn unchanged at 2× size (256×128) in the upper-left corner (phase 1), leaving a strip across the bottom for future status lines; a full-screen colour layout is planned (phase 2).
- **Input:** the physical button is replaced by an on-screen **NEXT** button in the upper-right corner, beside the page. It drives the same single / double / triple / long-press gestures, and a two-line hint under NEXT shows what the current page's extra gestures do.
- **Faster taps:** pages without multi-tap gestures change on release, with no double-tap wait.
- **Two inherited button bugs fixed:** the status page's play/pause icon now shows a double-tap pause, and triple-tap is reachable (the multi-press window now runs from the latest tap, not the first).
- **Wake from deep sleep** moves from the button to a screen touch (the XPT2046 IRQ line).

Implementation notes and the phase-1/phase-2 plan are in [`FORK.md`](FORK.md). Changes are on the **`tft`** branch; `main` mirrors upstream.

### Supported hardware

**XIAO ESP32-C5 only.** The OLED driver has been replaced by the TFT driver, and only the C5 pin map has TFT pins, so an S3 / C6 / C3 build from this fork has no working display. Use [upstream](https://github.com/hamspiced/piglet) for those boards.

### Tested so far (2026-10-21)

| Area | Status |
|---|---|
| TFT draws all pages | ✅ Tested on hardware |
| Touch button: tap, double / triple tap, hold on the mesh page | ✅ Tested on hardware |
| Uploads | ✅ Working on hardware (service not recorded) |
| SD logging, GPS fix | ⏳ Not yet confirmed — no GPS fix seen yet on the fork |
| Landscape layout, 2× pages, touch in landscape | ✅ Tested on hardware |
| Touch wake from deep sleep, `rotateScreen180` | ⏳ Not yet tested on the fork |
| Mesh Node / Core operation with a coordinator | ⏳ Not yet tested (switching modes works) |


## Features

- 2.4 GHz and 5 GHz Wi-Fi scanning (ESP32-C5)
- GPS position, heading, and speed logging
- SD card logging in WiGLE CSV format
- Web UI for:
  - Start / stop scanning
  - Upload logs to WiGLE, WDGoWars, and Wardrift
  - View device status
  - Manage SD files
  - Edit configuration
- TFT live status display with an on-screen touch button
- Optional battery monitoring (board dependent)
- Automatic STA connect with AP fallback
- **Home network auto-return** — detects your home SSID while wardriving, automatically connects and uploads pending CSVs, then resumes wardriving once you drive out of range again
- Optimized for mobile wardriving or warWalking!
- **ESP-Now Mesh Node mode** — pair with a coordinator device for multi-node wardriving
- **Mesh auto-start on boot** — configure `meshModeOnBoot` to automatically enter Core or Node mode after uploads complete, bypassing the AP window
- **Screen rotation** — `rotateScreen180=true` turns the whole panel 180°, touch button and touch mapping included *(not yet tested on the TFT fork)*
- **Auto-start wardriving after uploads** — set `autoStartAfterUpload=true` to disconnect from home Wi-Fi immediately after boot uploads complete and begin scanning without delay


## ESP-Now Mesh Network Node Mode

Piglet includes a built-in **Mesh Node mode** that lets it act as a wireless wardriving node alongside a compatible coordinator device. In this mode, Piglet scans Wi-Fi networks and forwards results over ESP-Now — no SD card or GPS fix required on Piglet itself. The coordinator handles GPS stamping and data logging.

### Compatible Coordinator Devices

- **Biscuit Pro** by [Hedge / Biscuit Shop](https://biscuitshop.us)
- **JCMK C5 Wardriver** by JustCallMeKoko

### How to Use

1. Power on your coordinator device (Biscuit Pro or JCMK C5 Wardriver)
2. On Piglet, tap **NEXT** to cycle pages until you reach **Mesh Node** (the last page after the pig animation)
3. Piglet automatically searches for a coordinator on ESP-Now channel 6
4. Once connected, it receives a channel range assignment and begins forwarding scan data
5. Tap **NEXT** again to exit Mesh Node mode and return to normal wardriving

### Mesh Node Display

While in Mesh Node mode the screen shows:
- Link status (Searching / Core linked)
- Coordinator MAC address
- Assigned channel range
- Total networks discovered
- Records forwarded to the coordinator

> **Note:** Entering Mesh Node mode suspends normal WiGLE CSV logging. All data is sent live to the coordinator. Exiting the page restores normal scanning automatically.

### Auto-Start Mesh Mode on Boot

Set `meshModeOnBoot` in `/wardriver.cfg` to automatically enter mesh mode after boot uploads are complete, without needing to navigate pages manually:

| Value | Behaviour |
|-------|-----------|
| `none` | Normal wardriving (default) |
| `core` | Enters Mesh Core mode — acts as coordinator, logs records from nodes |
| `node` | Enters Mesh Node mode — forwards scan results to a Core |

When `core` or `node` is set the SoftAP window is **skipped entirely** (ESP-Now owns the WiFi stack and the AP would be non-functional). The device goes straight from boot uploads to the mesh page. Set via the web UI **Mesh Mode On Boot** dropdown or directly in `/wardriver.cfg`.

## Hardware

| Part | Notes |
|---|---|
| [Seeed XIAO ESP32-C5](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C5-p-6609.html) | Required |
| 2.8" ILI9341 SPI TFT, 240×320, with XPT2046 resistive touch | Display and touch share the SD card's SPI bus |
| ATGM336H GPS module | UART, 9600 baud by default |
| SPI microSD card module | FAT32 card |
| Optional LiPo battery | Connected to the XIAO battery inputs |

### Wiring (XIAO ESP32-C5)

One SPI bus is shared by the SD card, TFT and touch controller, each with its own chip-select line: **SCK = D8 / GPIO 8, MOSI = D10 / GPIO 10, MISO = D9 / GPIO 9.**

| Signal | XIAO pad | GPIO | Notes |
|---|---|---|---|
| SD CS | D3 | 7 | Shared bus |
| TFT CS | D0 | 1 | Shared bus |
| TFT DC | D1 | 0 | Was the physical button |
| TFT RESET | D2 | 25 | |
| Touch CS | D4 | 23 | Was OLED SDA |
| Touch IRQ | D5 | 24 | Was OLED SCL — wakes from deep sleep |
| GPS RX | D7 | 12 | From GPS TX — unchanged from upstream |
| GPS TX | D6 | 11 | To GPS RX — unchanged from upstream |

The pin map lives in `Arduino Files/Piglet/PinMapDefs.h` and is selected automatically when the firmware detects a C5.


## Touch Button

The **NEXT** button runs down the upper-right corner of the screen, beside the page. The two small lines under it are a reminder of the current page's extra gestures.

### Gestures

| Gesture | Action |
|---|---|
| **Tap** | Next page |
| **Double tap** *(Status page)* | Pause / resume scanning |
| **Double tap** *(Pig page)* | Pig twerk ("OINK!") |
| **Triple tap** *(Pig page)* | Sasquatch walks across |
| **Hold 2 s** *(Mesh page)* | Switch between Mesh Node and Mesh Core |
| **Hold 2 s** *(any other page)* | Deep sleep — the log file is closed first; touch the screen to wake (full reboot) |

Multi-tap taps must each follow the previous one within 350 ms. Only the Status and Pig pages wait for a possible second tap; on every other page a tap acts as soon as you lift your finger.

### Pages

| Page | Name | What it shows | Hint under NEXT | Scanning |
|---|---|---|---|---|
| 0 | **Status** | Play/pause icon, GPS indicator, STA and SD state, 2.4 / 5 GHz counts, battery, speed, IP or AP countdown, upload progress | `2x:` `pause` / `resume` | ✅ Active unless paused |
| 1 | **Networks** | 2.4 GHz, 5 GHz and total network counts | `hold:` `sleep` | ✅ Active |
| 2 | **Navigation** | Compass arrow, heading, speed; `No Fix` and satellite dots without a fix | `hold:` `sleep` | ✅ Active |
| 3 | **Paused** | `Scan paused` | `hold:` `sleep` | ❌ Stopped while on this page |
| 4 | **Pig** | Walking pig animation 🐷 | `2x oink` `3x sq` | ✅ Active |
| 5 | **Mesh** | Node: link state, coordinator MAC, channel range, ESP-Now channel, Found count. Core: node count and list, GPS state | `hold:` `core` / `node` | ↔ Forwarded via ESP-Now |

Leaving the Status page clears a double-tap pause. Entering the Mesh page always starts in Node mode.


## First Time Run Initialization

The Wardriver functions using a config file located on the root of a FAT32-formatted SD card. On first boot the device will start its own access point (`Piglet-WARDRIVE` / `wardrive1234`) so you can connect and fill in your settings via the web interface — or you can place the config file on the SD card manually before powering on.

**AP Timer & Keep-Alive**

The SoftAP runs for **60 seconds** by default. About **30 seconds before** that window expires, the WebUI shows a *"Stay in WebUI?"* prompt — clicking **Stay** extends the window to a **5 minute** rolling timer so you have room to actually use the WebUI. The same prompt re-appears 30 seconds before the 5 minute timer expires; clicking Stay again resets it.

- **Stay** — extends (or re-extends) the window to 5 minutes.
- **Start Scanning Now** — closes the AP immediately and begins wardriving.
- **Ignored / browser closed** — timer runs out, AP closes, scanning starts.

The Status page shows the live countdown (`AP: 192.168.4.1 60s` initially, `m:ss` once extended). Once your home Wi-Fi is configured, the device will connect to it on subsequent boots and the WebUI is reachable on the STA IP shown on the Status page — the AP only comes up if STA fails.

**Location:** `/wardriver.cfg` on the SD card root

A sample config file is included in `Arduino Files/Piglet/wardriver.cfg`. The full default config with all available keys is shown below:

```ini
# ============================================================
# Piglet Wardriver Configuration
# Format: key=value
# Lines starting with # are comments and ignored.
# ============================================================

# ------------------------------------------------------------
# WiGLE Upload
# ------------------------------------------------------------
# Use only the "Encoded for Use" token from wigle.net/account
# Leave empty to disable WiGLE uploads.

wigleBasicToken=EnterWigleTokenHere

# ------------------------------------------------------------
# WDGoWars
# ------------------------------------------------------------
# Get your API key at: https://wdgwars.pl/profile/
# Leave empty to disable WDGoWars uploads.

wdgwarsApiKey=EnterWDGoWarsAPIKeyHere

# ------------------------------------------------------------
# Wardrift
# ------------------------------------------------------------
# Get your API key at: https://wardrift.net
# Leave empty to disable Wardrift uploads.

wardriftApiKey=EnterWardriftAPIKeyHere

# ------------------------------------------------------------
# Max Automatic Uploads at Boot
# ------------------------------------------------------------
# -1 = Upload ALL pending files every boot
#  0 = Disabled — no auto-upload at boot (use web UI manually)
# 1+ = Upload up to N files per boot

maxBootUploads=-1

# ------------------------------------------------------------
# Device Name (optional)
# ------------------------------------------------------------
# A short label for this device. Used in WiGLE CSV filenames
# and in the WiGLE upload header so you can tell devices apart.
# Spaces become underscores. Max 20 characters.
# Leave empty to use the default (no prefix).

deviceName=

# ------------------------------------------------------------
# Home Wi-Fi (STA mode)
# ------------------------------------------------------------
# If provided, device connects on boot.
# If connection fails, falls back to SoftAP for 60 seconds
# (can be extended via the "Stay in WebUI?" prompt that appears
# ~30 s before the timer expires).
#
# Auto-return while wardriving: if this SSID is seen in a scan result
# while driving, the device stops scanning, connects to it, and uploads
# pending CSVs automatically (same upload flow as boot) -- no need to
# reboot or stop by the web UI when you get home. Normal wardriving
# resumes automatically once the connection is lost (e.g. driving back
# out of range).

homeSsid=EnterWifiHere
homePsk=EnterWifiPasswordHere

# ------------------------------------------------------------
# Wardriver Access Point (SoftAP fallback)
# ------------------------------------------------------------
# SSID and password for the temporary config AP.
# Password must be 8+ characters or AP becomes open.

wardriverSsid=Piglet-WARDRIVE
wardriverPsk=wardrive1234

# ------------------------------------------------------------
# GPS Settings
# ------------------------------------------------------------
# UART baud rate for the GPS module.
# Common values: 9600, 38400, 115200

gpsBaud=9600

# ------------------------------------------------------------
# Wi-Fi Scan Mode
# ------------------------------------------------------------
# aggressive  — scans every ~3 seconds using async mode (faster, more power)
# powersaving — scans every ~12 seconds (slower, less power)

scanMode=aggressive

# ------------------------------------------------------------
# Speed Units (display only)
# ------------------------------------------------------------
# kmh = kilometers per hour
# mph = miles per hour

speedUnits=mph

# ------------------------------------------------------------
# Battery Test
# ------------------------------------------------------------
# true = logs elapsed time on battery to /battery_test.csv
# false = disabled

batteryTest=false

# ------------------------------------------------------------
# Mesh Mode On Boot
# ------------------------------------------------------------
# Automatically enter ESP-Now mesh mode after boot uploads complete.
# Bypasses the SoftAP window and jumps directly to the mesh page.
#
#   none = Normal wardriving mode (default)
#   core = Start as Mesh Core (coordinator) — logs data from nodes
#   node = Start as Mesh Node — forwards scan results to a Core
#
# Requires a compatible coordinator (Biscuit Pro, JCMK C5 Wardriver)
# when using node mode.

meshModeOnBoot=none

# ------------------------------------------------------------
# Screen Rotation
# ------------------------------------------------------------
# Rotate the display 180 degrees for upside-down mounting.
# Values: true or false
# Reboot required after changing.

rotateScreen180=false

# ------------------------------------------------------------
# Auto-Start Wardriving After Uploads
# ------------------------------------------------------------
# When true: disconnects from home Wi-Fi immediately after boot uploads
# complete and begins wardriving without delay. The web UI remains
# accessible if you later connect to the Wardriver AP, but the device
# will not hold the STA link open.
# Values: true or false
# Reboot required after changing.

autoStartAfterUpload=false

# ------------------------------------------------------------
# SD Card SPI Clock Ceiling
# ------------------------------------------------------------
# Boot negotiates the fastest SD-over-SPI clock (descending ladder) up to
# this cap and uses the fastest speed that mounts successfully. Lower this
# if you see SD write errors/corruption on marginal wiring (e.g. long
# breadboard jumpers). Default is a commonly-safe ceiling for SD-over-SPI.
# Reboot required after changing.

sdMaxSpiHz=20000000

# ------------------------------------------------------------
# Network Whitelist
# ------------------------------------------------------------
# Up to 10 SSIDs that will never be written to the WiGLE CSV log.
# Matching is exact and case-sensitive; empty slots never match, so
# blank/hidden SSIDs are never accidentally filtered. Whitelisted
# networks are still scanned and still count toward the Found totals —
# they are simply skipped when writing log rows. Editable from the
# web UI's Configuration card ("Network Whitelist" textarea, one SSID
# per line) or directly in this file.

ssidWhitelist1=
ssidWhitelist2=
ssidWhitelist3=
ssidWhitelist4=
ssidWhitelist5=
ssidWhitelist6=
ssidWhitelist7=
ssidWhitelist8=
ssidWhitelist9=
ssidWhitelist10=
```

### Auto-Start Wardriving After Uploads — How to Disable

> **Important:** When `autoStartAfterUpload=true`, the device disconnects from your home Wi-Fi immediately after uploads finish and goes straight into wardriving. Because it no longer holds the STA connection, the web UI is **not** reachable on your home network after boot.

To disable this setting after it has been enabled, you have two options:

**Option 1 — Connect via the Wardriver AP**

When Piglet is away from the saved home network (or the home network is unavailable), it falls back to its own SoftAP. Connect to it and use the web UI:

1. Power on the device somewhere the home Wi-Fi is not in range (or temporarily forget the home network on the device by clearing `homeSsid` in the config)
2. Connect your phone or laptop to the **Wardriver SSID** (default: `Piglet-WARDRIVE` / `wardrive1234`)
3. Open a browser and go to **`http://192.168.4.1`**
4. In the **Configuration** section, set **Auto-Start Wardriving After Uploads** to **Disabled**
5. Click **Save & Reboot**

**Option 2 — Edit the SD card directly**

1. Remove the SD card from the device
2. Open `wardriver.cfg` in any text editor
3. Change `autoStartAfterUpload=true` to `autoStartAfterUpload=false`
4. Save the file, re-insert the SD card, and reboot


## Building Firmware

### Requirements

- **Arduino IDE 2.x** (or `arduino-cli`)
- **Arduino-ESP32 core** by Espressif — built and tested with **3.3.10**
- Sketch folder: **`Arduino Files/Piglet/`** (open `Piglet.ino`)

### Required Libraries

Install via Arduino Library Manager (`Sketch → Include Library → Manage Libraries`):

| Library | Author | Notes |
|---------|--------|-------|
| GFX Library for Arduino (`Arduino_GFX_Library`) | moononournation | ILI9341 TFT driver — replaces Adafruit SSD1306 |
| XPT2046_Touchscreen | Paul Stoffregen | Touch controller |
| TinyGPSPlus | Mikal Hart | GPS NMEA parsing |
| ArduinoJson | Benoit Blanchon | v6.x or v7.x |

The Adafruit SSD1306, GFX and BusIO libraries that upstream needs are **not** used by this fork. All other headers (`WiFi`, `WebServer`, `WiFiClientSecure`, `HTTPClient`, `SD`, `SPI`, `esp_now.h`, `esp_wifi.h`) come with the ESP32 Arduino core.

### Board Settings

Select **XIAO_ESP32C5**, then under **Tools**:

| Setting | Value |
|---|---|
| USB CDC On Boot | **Enabled** — otherwise serial output goes to the UART pins, not USB |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| Flash Size | 8MB |
| Flash Mode / Flash Frequency | QIO / 80MHz |
| CPU Frequency | 80MHz |
| PSRAM | Disabled — see the note below |

Equivalent `arduino-cli` FQBN:

```
esp32:esp32:XIAO_ESP32C5:UploadSpeed=921600,CDCOnBoot=cdc,CPUFreq=80,FlashFreq=80,FlashMode=qio,FlashSize=8M,PartitionScheme=huge_app,DebugLevel=none,PSRAM=disabled,EraseFlash=none,JTAGAdapter=default,ZigbeeMode=default
```

> **PSRAM note:** upstream's flash steps call PSRAM "critical" for HTTPS uploads, but the settings above — with PSRAM disabled — are the ones the stock Piglet C5 firmware is built with, and stock uploads work with them. This fork is built and run with PSRAM disabled, and **uploads work on the fork with these settings** (2026-10-21).

### Flash Steps

1. Build and upload as usual.
2. **If the upload fails** (`Failed to start stub flasher`, or `No serial data received`): unplug the board, **hold BOOT while plugging it back in**, and upload again. If the stub flasher still fails, flash with `esptool` directly using `--no-stub`. **Leave the flash options as `keep`** — passing explicit `--flash-mode` / `--flash-freq` / `--flash-size` rewrites the bootloader header, and the board then boot-loops.
3. After flashing from download mode the board **stays in the bootloader** (white screen) — tap **RESET** or replug it.
4. Insert a **FAT32-formatted SD card**, add `/wardriver.cfg` (see above), and restart.


## Upstream-only Folders

These folders are kept **unchanged from upstream** so that upstream releases merge cleanly. They do not include the TFT changes — see [upstream](https://github.com/hamspiced/piglet) for their documentation:

- `TDongleC5_Piglet/` — LilyGo T-Dongle C5 standalone firmware
- `PigletNode/` — standalone XIAO ESP32-C5 mesh node (has its own [README](PigletNode/README.md))
- `waveshareDisplayMiniPiglet/` — Waveshare display variant
- `PCB Files/`, `Case Files/` — PCB and 3D-printed case **for the stock OLED build**; they are not designed for the 2.8" TFT. The Piglet case is by **Bread** at [Breadbox Systems](https://breadboxsystems.com) — [MakerWorld](https://makerworld.com/en/models/2708429-piglet-wardriver-case#profileId-3000037).


## License

**Creative Commons Attribution-NonCommercial-ShareAlike 4.0 (CC BY-NC-SA 4.0)** — see [`LICENSE`](LICENSE). This fork is distributed under the same license, as ShareAlike requires.

You may use, modify and share this project for **non-commercial** purposes, with attribution, and any adaptation you share must carry the same license.

https://creativecommons.org/licenses/by-nc-sa/4.0/

*(Upstream's README states CC BY-NC 4.0, but the upstream `LICENSE` file and `CREDITS.md` both say CC BY-NC-SA 4.0; this fork follows the `LICENSE` file.)*

---

**Piglet** was created by **Midwest Gadgets LLC** — [hamspiced/piglet](https://github.com/hamspiced/piglet). See [`CREDITS.md`](CREDITS.md) for third-party acknowledgements.

**Piglet-TFT** is a modified version, maintained by [Kraethor](https://github.com/Kraethor). Changes from upstream: replaced the OLED and button with an ILI9341 TFT and XPT2046 touch, and changed the button handling as described in "What's different from upstream" above. The full history is on the `tft` branch.
