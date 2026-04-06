#pragma once

#include <WebServer.h>
#include "../config/config.h"
#include "../camera/camera.h"
#include <stddef.h>

// HTTP server: serves embedded web GUI (assets copied to PSRAM at startup)
// and exposes the REST API.
class AppWebServer {
public:
    // Copy web assets to PSRAM (when BOARD_HAS_PSRAM), register routes, start on port 80.
    void begin(Config& config, Camera& camera);

    // Process pending HTTP requests; call from loop().
    void handle();

private:
    WebServer _server{80};
    Config*   _config{nullptr};
    Camera*   _camera{nullptr};
    uint32_t  _lightStatusAt{0}; // seconds uptime until which capture light is enabled
    bool      _lightPwmReady{false};

    const uint8_t* _htmlBuf{nullptr};  // index.html: PSRAM copy or flash pointer
    size_t         _htmlSize{0};
    const uint8_t* _favBuf{nullptr};   // favicon.ico: PSRAM copy or flash pointer
    size_t         _favSize{0};

    // GET /             → index.html
    void handleRoot();
    // GET /favicon.ico  → favicon
    void handleFavicon();
    // GET /photo        → JPEG snapshot
    void handlePhoto();
    // GET /status       → JSON: { wifi, ip, quality, framesize }
    void handleStatus();
    // POST /settings    → update config, reboot
    void handleSaveSettings();

    // Send response from any memory region (flash PROGMEM or PSRAM).
    void sendBuffer(int code, const char* mime, const uint8_t* buf, size_t len);

    // Control white capture illumination (LED_FLASH_PIN).
    void setCaptureLight(bool on);

    // Setup PWM for white illumination output.
    void ensureLightPwm();

    // Heuristic darkness check based on JPEG payload density.
    bool isFrameDark(const camera_fb_t* fb) const;

    // Log completed request to Serial: method URI clientIP -> code (sizeB).
    void logRequest(int code, size_t bodyBytes);
};
