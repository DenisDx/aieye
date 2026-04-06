# AI Eye

Lightweight ESP32-S3 firmware that exposes a camera snapshot over HTTP and provides a browser-based configuration interface.

## Features

- **GET /photo** – capture and stream a JPEG frame
- **Web GUI at /** – live preview (1 fps), device status, WiFi & camera settings, LED & illumination control
- **Captive portal** – when WiFi is not configured or unreachable the device raises its own access point `AIeye-setup` and redirects all DNS queries to the setup page
- **NVS persistence** – WiFi credentials, JPEG quality, resolution, LED settings, and illumination parameters survive reboots
- **RGB LED status indicator** – real-time feedback: red (no WiFi), green (capturing), blue (heartbeat), off (idle)
- **Adaptive white illumination** (first board) – automatic LED assist in dark conditions or when capture takes too long; configurable brightness and threshold
- **Embedded web assets** – `index.html` and `favicon.ico` are compiled into the firmware as PROGMEM constants and cached in PSRAM at startup; no flash filesystem required

## Hardware

| Target env           | Board                               | Camera  | LED               | Illumination Assist |
|----------------------|-------------------------------------|---------|-------------------|---------------------|
| `esp32s3cam_n8r8`    | ESP32S3-CAM N8R8 (Emakefun)        | OV3660  | RGB WS2812 (GPIO 48) | White LED (GPIO 3) ✓ |
| `esp32-s3-cam`       | ESP32-S3-CAM (generic)             | OV3660  | RGB WS2812 (GPIO 48) | —                   |

### Board Details

**esp32s3cam_n8r8** ([Emakefun Docs](https://emakefun.github.io/emakefun-docsify/#/zh-cn/esp32/esp32s3-cam/README_zh))
- RAM: 8MB OPI PSRAM
- Flash: 8MB
- Features: onboard white LED flash (2× high-brightness), RGB status indicator, reset button

**esp32-s3-cam** (generic)
- RAM: 8MB (QD-only flash variant)
- Flash: 8MB
- Features: RGB status indicator only

To add a new board, append a new `[env:board_name]` section to `platformio.ini` and define the camera pin macros with `-D` build flags (see existing env as reference).

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- Python 3.8+ (for `convert_html.py`)

### Build & Flash

```bash
# First time or after editing index.html / favicon.ico:
python convert_html.py

# Build and upload firmware (choose target):
pio run -e esp32s3cam_n8r8 -t upload
# or
pio run -e esp32-s3-cam -t upload

# Open serial monitor:
pio device monitor -e esp32s3cam_n8r8
# or
pio device monitor -e esp32-s3-cam
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
| GET    | `/status`   | JSON: device state including WiFi, IP, camera settings, LED & illumination config |
| POST   | `/settings` | Form fields: `ssid`, `password`, `quality`, `framesize`, `light_enabled` (esp32s3cam_n8r8 only), `light_br`, `light_thr_ms`, `led_blue`, `led_cap_ms`, `led_cap_br`. Saves and reboots. |

### Settings Parameters

All parameters are persisted to NVS and survive power cycles.

#### WiFi & Camera
- `ssid` – WiFi network name
- `password` – WiFi password
- `quality` – JPEG quality (0–100, default 40)
- `framesize` – camera resolution (default SVGA)

#### RGB LED (all boards)
- `led_blue` – blue heartbeat brightness (0–100%, default 10)
- `led_cap_ms` – duration of green capture pulse (20–2000 ms, default 250)
- `led_cap_br` – green capture brightness (0–100%, default 100)

#### White Illumination Assist (esp32s3cam_n8r8 only)
- `light_enabled` – enable/disable white LED assist (default false)
- `light_br` – white LED brightness (0–100%, default 100)
- `light_thr_ms` – capture duration threshold for assist trigger (50–5000 ms, default 300)

### LED Status Indicator

The RGB LED (all boards) indicates device state via color:
- **Red** – WiFi disconnected
- **Green pulse** – capturing photo (brief pulse at configured duration)
- **Blue blink** – connected & idle (heartbeat at low brightness)
- **Off** – cold start or error state

### White Illumination Logic (esp32s3cam_n8r8)

When `light_enabled` is true, the white LED automatically activates if:
1. **Dark scene detected** – JPEG payload analysis indicates low light
2. **Slow capture** – snapshot took longer than `light_thr_ms` (suggests sensor exposure struggle)

The assist window remains active for 10 minutes after either condition is met, reducing repeated light toggles. Once active, subsequent captures are pre-lit before the sensor snaps, yielding sharper low-light images.
