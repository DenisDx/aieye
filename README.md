# AI Eye

Lightweight ESP32-S3 firmware that exposes a camera snapshot over HTTP and provides a browser-based configuration interface.

## Features

- **GET /photo** – capture and stream a JPEG frame
- **Web GUI at /** – live preview (1 fps), device status, WiFi & camera settings
- **Captive portal** – when WiFi is not configured or unreachable the device raises its own access point `AIeye-setup` and redirects all DNS queries to the setup page
- **NVS persistence** – WiFi credentials, JPEG quality and resolution survive reboots
- **Embedded web assets** – `index.html` and `favicon.ico` are compiled into the firmware as PROGMEM constants and cached in PSRAM at startup; no flash filesystem required

## Hardware

| Target env           | Board                         | Camera  |
|----------------------|-------------------------------|---------|
| `esp32s3cam_n8r8`    | ESP32S3-CAM N8R8 (Emakefun)  | OV3660  |

[Board documentation](https://emakefun.github.io/emakefun-docsify/#/zh-cn/esp32/esp32s3-cam/README_zh)

To add a new board, append a new `[env:board_name]` section to `platformio.ini` and define the camera pin macros with `-D` build flags (see existing env as reference).

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- Python 3.8+ (for `convert_html.py`)

### Build & Flash

```bash
# First time or after editing index.html / favicon.ico:
python convert_html.py

# Build and upload firmware:
pio run -e esp32s3cam_n8r8 -t upload

# Open serial monitor:
pio device monitor -e esp32s3cam_n8r8
```

### First-Time Setup

1. Power the board – it starts access point **AIeye-setup**
2. Connect any device to that network
3. Open `http://192.168.4.1` in a browser
4. Enter your WiFi credentials and press **Save & Restart**
5. The board reconnects to your network; the IP is printed on the serial monitor

## Web Asset Workflow

Web assets live in the project root and are embedded in firmware as C byte arrays.

| Source file  | Description                          |
|--------------|--------------------------------------|
| `index.html` | Full HTML/CSS/JS for the web GUI     |
| `favicon.ico`| Browser tab icon (optional)          |

After modifying either file, regenerate the C sources:

```bash
python convert_html.py
```

This writes (or overwrites):

```
src/webui/index.html.c   – byte array (PROGMEM)
src/webui/index.html.h   – extern declarations
src/webui/favicon.ico.c  – byte array (PROGMEM)
src/webui/favicon.ico.h  – extern declarations
```

Then rebuild and reflash the firmware. **Commit the generated files** so the project builds without needing to run the script.

> **PSRAM caching** – on boards where `BOARD_HAS_PSRAM` is defined the webserver copies both assets from flash to PSRAM once at startup, yielding faster HTTP responses.

## Project Structure

```
aieye/
├── index.html           ← web GUI source (edit this)
├── favicon.ico          ← tab icon source (optional, replace with your own)
├── convert_html.py      ← regenerates src/webui/ C files from the sources above
├── platformio.ini
└── src/
    ├── main.cpp
    ├── webui/           ← generated files – do not edit manually
    │   ├── index.html.h / index.html.c
    │   └── favicon.ico.h / favicon.ico.c
    ├── camera/          ← esp32-camera driver wrapper
    ├── captive_portal/  ← WiFi STA/AP management + DNS redirect
    ├── config/          ← NVS-backed settings (Preferences)
    └── webserver/       ← HTTP server + REST API
```

## HTTP API

| Method | Path        | Description                                              |
|--------|-------------|----------------------------------------------------------|
| GET    | `/`         | Web GUI (HTML)                                           |
| GET    | `/favicon.ico` | Browser icon                                          |
| GET    | `/photo`    | JPEG snapshot (`image/jpeg`)                             |
| GET    | `/status`   | JSON: `{ wifi, ip, quality, framesize }`                 |
| POST   | `/settings` | Form fields: `ssid`, `password`, `quality`, `framesize`. Saves and reboots. |
