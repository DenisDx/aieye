#pragma once

#include "esp_camera.h"
#include "../config/config.h"

// Camera hardware abstraction: initialisation and single-frame capture.
class Camera {
public:
    // Configure and init esp32-camera driver using board pins (from -D macros)
    // and image settings from Config. Returns true on success.
    bool begin(const Config& config);

    // Capture one JPEG frame. Returns nullptr on failure.
    // Must call releaseFrame() after the buffer is consumed.
    camera_fb_t* captureFrame();

    // Return frame buffer ownership to the driver.
    void releaseFrame(camera_fb_t* fb);

    bool isReady() const { return _ready; }

private:
    bool _ready{false};

    // Map quality percent (1–100) → esp32-camera parameter (0–63, 0 = best).
    static uint8_t percentToEspQuality(uint8_t qualityPercent);
};
