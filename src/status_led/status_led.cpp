#include "status_led.h"

#include <Arduino.h>

#if __has_include("esp32-hal-rgb-led.h")
#include <esp32-hal-rgb-led.h>
#endif

namespace StatusLed {

namespace {

bool s_wifiConnected = false;
bool s_cameraReady = false;
bool s_captureRequested = false;
uint32_t s_captureBlinkUntilMs = 0;
uint32_t s_blueBlinkUntilMs = 0;
uint32_t s_nextBlueBlinkMs = 0;
uint16_t s_captureDurationMs = 250;
uint8_t s_captureBrightnessPct = 100;
uint8_t s_blueBrightnessPct = 10;

uint8_t pctToByte(uint8_t pct) {
    if (pct > 100) pct = 100;
    return static_cast<uint8_t>((static_cast<uint16_t>(pct) * 255u) / 100u);
}

// Write one RGB color to on-board WS LED.
void writeRgb(uint8_t r, uint8_t g, uint8_t b) {
#ifdef RGB_LED_PIN
    neopixelWrite(RGB_LED_PIN, r, g, b);
#else
    (void)r;
    (void)g;
    (void)b;
#endif
}

}  // namespace

void begin() {
#ifdef RGB_LED_PIN
    pinMode(RGB_LED_PIN, OUTPUT);
#endif
    writeRgb(0, 0, 0);
}

void setWifiConnected(bool connected) {
    s_wifiConnected = connected;
}

void setCameraReady(bool ready) {
    s_cameraReady = ready;
}

void notifyCapture() {
    // Set capture trigger flag; actual pulse timing is handled in tick() from loop.
    s_captureRequested = true;
}

void setBlueBrightnessPercent(uint8_t percent) {
    if (percent > 100) percent = 100;
    s_blueBrightnessPct = percent;
}

void setCapturePulse(uint16_t durationMs, uint8_t brightnessPercent) {
    if (durationMs < 20) durationMs = 20;
    if (durationMs > 2000) durationMs = 2000;
    if (brightnessPercent > 100) brightnessPercent = 100;
    s_captureDurationMs = durationMs;
    s_captureBrightnessPct = brightnessPercent;
}

void tick() {
    uint32_t now = millis();

    // Wi-Fi missing: force solid red while disconnected.
    if (!s_wifiConnected) {
        writeRgb(255, 0, 0);
        return;
    }

    // Start green pulse from loop when a capture trigger is raised.
    if (s_captureRequested) {
        s_captureRequested = false;
        s_captureBlinkUntilMs = now + s_captureDurationMs;
    }

    // Capture event has priority: short green pulse.
    if (now < s_captureBlinkUntilMs) {
        writeRgb(0, pctToByte(s_captureBrightnessPct), 0);
        return;
    }

    // Camera healthy heartbeat: rare short blue blink.
    if (s_cameraReady) {
        if (s_nextBlueBlinkMs == 0) {
            s_nextBlueBlinkMs = now + 7000;
        }
        if (now >= s_nextBlueBlinkMs && s_blueBlinkUntilMs == 0) {
            s_blueBlinkUntilMs = now + 60;
            s_nextBlueBlinkMs = now + 7000;
        }
        if (s_blueBlinkUntilMs != 0 && now < s_blueBlinkUntilMs) {
            writeRgb(0, 0, pctToByte(s_blueBrightnessPct));
            return;
        }
        if (s_blueBlinkUntilMs != 0 && now >= s_blueBlinkUntilMs) {
            s_blueBlinkUntilMs = 0;
        }
    }

    // Idle connected state: LED off.
    writeRgb(0, 0, 0);
}

}  // namespace StatusLed
