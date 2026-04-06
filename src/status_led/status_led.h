#pragma once

#include <stdint.h>
#include <stdbool.h>

namespace StatusLed {

// Initialize RGB status LED output (no-op if RGB_LED_PIN is not defined).
void begin();

// Update current Wi-Fi connectivity state.
void setWifiConnected(bool connected);

// Update camera readiness state (true after successful camera init).
void setCameraReady(bool ready);

// Request a short green blink indicating a frame capture event.
void notifyCapture();

// Set blue heartbeat brightness in percent (0..100).
void setBlueBrightnessPercent(uint8_t percent);

// Set capture pulse properties: duration in ms and green brightness (0..100).
void setCapturePulse(uint16_t durationMs, uint8_t brightnessPercent);

// Advance LED state machine; call frequently from loop().
void tick();

}  // namespace StatusLed
