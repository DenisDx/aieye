#include "webserver.h"
#include <WiFi.h>
#include <Arduino.h>
#ifdef BOARD_HAS_PSRAM
#include <esp_heap_caps.h>
#endif
#include "../webui/index.html.h"
#include "../webui/favicon.ico.h"
#include "../status_led/status_led.h"

namespace {
constexpr float DARK_JPEG_DENSITY_THRESHOLD = 0.035f;
constexpr uint8_t LIGHT_PWM_CH = 2;
constexpr uint16_t LIGHT_PWM_FREQ = 5000;
constexpr uint8_t LIGHT_PWM_RES_BITS = 8;
}

// Copy web assets to PSRAM if available; fall back to flash pointer.
void AppWebServer::begin(Config& config, Camera& camera) {
    _config = &config;
    _camera = &camera;

    auto cacheAsset = [](const uint8_t* src, size_t len) -> const uint8_t* {
#ifdef BOARD_HAS_PSRAM
        void* p = heap_caps_malloc(len, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (p) {
            memcpy(p, src, len);
            return (const uint8_t*)p;
        }
        Serial.println("[webserver] PSRAM alloc failed, serving from flash");
#endif
        return src;
    };

    _htmlBuf  = cacheAsset(index_html_data, index_html_size);
    _htmlSize = index_html_size;
    _favBuf   = cacheAsset(favicon_ico_data, favicon_ico_size);
    _favSize  = favicon_ico_size;

    _server.on("/",            HTTP_GET,  [this] { handleRoot();         });
    _server.on("/favicon.ico", HTTP_GET,  [this] { handleFavicon();      });
    _server.on("/photo",       HTTP_GET,  [this] { handlePhoto();        });
    _server.on("/status",      HTTP_GET,  [this] { handleStatus();       });
    _server.on("/settings",    HTTP_POST, [this] { handleSaveSettings(); });
    _server.onNotFound([this] {
        _server.send(404, "text/plain", "Not found");
        logRequest(404, 0);
    });

    _server.begin();
    setCaptureLight(false);
    Serial.printf("[webserver] started  html=%uB  fav=%uB\n",
                  (unsigned)_htmlSize, (unsigned)_favSize);
}

void AppWebServer::handle() {
    _server.handleClient();
}

// Send response from any memory region (PSRAM, DRAM, or PROGMEM flash pointer).
void AppWebServer::sendBuffer(int code, const char* mime,
                               const uint8_t* buf, size_t len) {
    // Force short-lived connections to avoid keep-alive stalls on some clients/APs.
    _server.sendHeader("Connection", "close");
    _server.setContentLength(len);
    _server.send(code, mime, "");
    static const size_t CHUNK = 1440;  // fits one TCP segment
    for (size_t offset = 0; offset < len; ) {
        size_t chunk = min(CHUNK, len - offset);
        _server.sendContent((const char*)(buf + offset), chunk);
        offset += chunk;
    }
    logRequest(code, len);
}

// Log completed request: method URI clientIP -> code (sizeB).
void AppWebServer::logRequest(int code, size_t bodyBytes) {
    const char* method = "???";
    String uri = _server.uri();
    String remote = _server.client().remoteIP().toString();
    switch (_server.method()) {
        case HTTP_GET:  method = "GET";  break;
        case HTTP_POST: method = "POST"; break;
        default: break;
    }
    Serial.printf("[webserver] %s %s %s -> %d (%uB)\n",
        method,
        uri.c_str(),
        remote.c_str(),
        code,
        (unsigned)bodyBytes);
}

void AppWebServer::handleRoot() {
    sendBuffer(200, "text/html; charset=utf-8", _htmlBuf, _htmlSize);
}

void AppWebServer::handleFavicon() {
    sendBuffer(200, "image/x-icon", _favBuf, _favSize);
}

// Capture and stream a single JPEG frame.
void AppWebServer::handlePhoto() {
    uint32_t captureStartMs = millis();
    uint32_t nowSec = millis() / 1000u;
    bool lightOnForThisCapture = _config->getIlluminationEnabled() &&
                                 _lightStatusAt > 0 &&
                                 _lightStatusAt > nowSec;
    if (lightOnForThisCapture) {
        setCaptureLight(true);
        delay(40); // give LEDs/sensor a short settling time
    }

    camera_fb_t* fb = _camera->captureFrame();

    // If capture failed in dark without active assist window, enable assist and retry once.
    if (!fb && _config->getIlluminationEnabled() && !lightOnForThisCapture) {
        _lightStatusAt = nowSec + 600;
        lightOnForThisCapture = true;
        setCaptureLight(true);
        delay(80); // first assisted frame may need slightly longer exposure settle

        // Retry a few times: first frame after enabling LED may still fail.
        for (int attempt = 0; attempt < 3 && !fb; ++attempt) {
            fb = _camera->captureFrame();
            if (!fb) delay(40);
        }

        if (fb) {
            Serial.printf("[light] recovery capture succeeded, assist window until t=%lu\n",
                          static_cast<unsigned long>(_lightStatusAt));
        }
    }

    if (!fb) {
        if (lightOnForThisCapture) setCaptureLight(false);
        _server.send(503, "text/plain", "Camera unavailable");
        logRequest(503, 0);
        return;
    }

    if (_config->getIlluminationEnabled() && !lightOnForThisCapture) {
        // If current scene is too dark, schedule assisted capture mode for next 10 minutes.
        const uint32_t captureMs = millis() - captureStartMs;
        const bool darkScene = isFrameDark(fb);
        const bool slowCapture = captureMs > _config->getIlluminationCaptureThresholdMs();
        if (darkScene || slowCapture) {
            _lightStatusAt = nowSec + 600;
            Serial.printf("[light] assist window set (dark=%s slow=%s capture=%lums thr=%ums) until t=%lu\n",
                          darkScene ? "yes" : "no",
                          slowCapture ? "yes" : "no",
                          static_cast<unsigned long>(captureMs),
                          static_cast<unsigned>(_config->getIlluminationCaptureThresholdMs()),
                          static_cast<unsigned long>(_lightStatusAt));
        }
    }

    StatusLed::notifyCapture();
    sendBuffer(200, "image/jpeg", fb->buf, fb->len);
    _camera->releaseFrame(fb);
    if (lightOnForThisCapture) setCaptureLight(false);
}

// Return current device state as JSON.
void AppWebServer::handleStatus() {
    String json = "{";
    json += "\"wifi\":\""    + WiFi.SSID()               + "\",";
    json += "\"ip\":\""      + WiFi.localIP().toString()  + "\",";
    json += "\"gw\":\""      + WiFi.gatewayIP().toString() + "\",";
    json += "\"mask\":\""    + WiFi.subnetMask().toString() + "\",";
    json += "\"rssi\":"       + String(WiFi.RSSI()) + ",";
    json += "\"sta\":"        + String(WiFi.status() == WL_CONNECTED ? 1 : 0) + ",";
    json += "\"quality\":"   + String(_config->getJpegQuality()) + ",";
    json += "\"framesize\":" + String(_config->getFrameSize()) + ",";
    json += "\"light_enabled\":" + String(_config->getIlluminationEnabled() ? 1 : 0) + ",";
    json += "\"light_bri\":" + String(_config->getIlluminationBrightness()) + ",";
    json += "\"light_thr_ms\":" + String(_config->getIlluminationCaptureThresholdMs()) + ",";
    json += "\"light_status_at\":" + String(_lightStatusAt) + ",";
    json += "\"led_blue\":" + String(_config->getLedBlueBrightness()) + ",";
    json += "\"led_cap_ms\":" + String(_config->getLedCaptureDurationMs()) + ",";
    json += "\"led_cap_bri\":" + String(_config->getLedCaptureBrightness()) + ",";
    json += "\"mirror_enabled\":" + String(_config->getMirrorEnabled() ? 1 : 0);
    json += "}";
    _server.sendHeader("Connection", "close");
    _server.send(200, "application/json", json);
    logRequest(200, json.length());
}

// Validate and persist settings from POST body, then reboot.
void AppWebServer::handleSaveSettings() {
    if (_server.hasArg("ssid") && _server.arg("ssid").length() > 0) {
        _config->setWifi(_server.arg("ssid"), _server.arg("password"));
    }
    if (_server.hasArg("quality")) {
        int q = _server.arg("quality").toInt();
        if (q >= 10 && q <= 100) {
            _config->setJpegQuality(static_cast<uint8_t>(q));
        }
    }
    if (_server.hasArg("framesize")) {
        _config->setFrameSize(static_cast<uint8_t>(_server.arg("framesize").toInt()));
    }
    if (_server.hasArg("light_enabled")) {
        _config->setIlluminationEnabled(_server.arg("light_enabled").toInt() != 0);
        if (!_config->getIlluminationEnabled()) {
            _lightStatusAt = 0;
            setCaptureLight(false);
        }
    }
    if (_server.hasArg("light_bri")) {
        _config->setIlluminationBrightness(static_cast<uint8_t>(_server.arg("light_bri").toInt()));
    }
    if (_server.hasArg("light_thr_ms")) {
        _config->setIlluminationCaptureThresholdMs(static_cast<uint16_t>(_server.arg("light_thr_ms").toInt()));
    }
    if (_server.hasArg("led_blue")) {
        _config->setLedBlueBrightness(static_cast<uint8_t>(_server.arg("led_blue").toInt()));
    }
    if (_server.hasArg("led_cap_ms")) {
        _config->setLedCaptureDurationMs(static_cast<uint16_t>(_server.arg("led_cap_ms").toInt()));
    }
    if (_server.hasArg("led_cap_bri")) {
        _config->setLedCaptureBrightness(static_cast<uint8_t>(_server.arg("led_cap_bri").toInt()));
    }
    if (_server.hasArg("mirror_enabled")) {
        _config->setMirrorEnabled(_server.arg("mirror_enabled").toInt() != 0);
    }
    _config->save();
    _server.send(200, "text/plain", "Saved. Rebooting...");
    logRequest(200, 0);
    delay(500);
    ESP.restart();
}

void AppWebServer::setCaptureLight(bool on) {
#ifdef LED_FLASH_PIN
    ensureLightPwm();
    uint8_t duty = 0;
    if (on) {
        const uint8_t pct = _config ? _config->getIlluminationBrightness() : 100;
        duty = static_cast<uint8_t>((static_cast<uint16_t>(pct) * 255u) / 100u);
        if (pct > 0 && duty < 16) duty = 16; // avoid invisible/ineffective ultra-low duty
    }
    ledcWrite(LIGHT_PWM_CH, duty);
#else
    (void)on;
#endif
}

void AppWebServer::ensureLightPwm() {
#ifdef LED_FLASH_PIN
    if (_lightPwmReady) return;
    ledcSetup(LIGHT_PWM_CH, LIGHT_PWM_FREQ, LIGHT_PWM_RES_BITS);
    ledcAttachPin(LED_FLASH_PIN, LIGHT_PWM_CH);
    ledcWrite(LIGHT_PWM_CH, 0);
    _lightPwmReady = true;
#endif
}

bool AppWebServer::isFrameDark(const camera_fb_t* fb) const {
    if (!fb || fb->width == 0 || fb->height == 0) return false;
    const float pixels = static_cast<float>(fb->width) * static_cast<float>(fb->height);
    const float density = static_cast<float>(fb->len) / pixels;
    return density < DARK_JPEG_DENSITY_THRESHOLD;
}
