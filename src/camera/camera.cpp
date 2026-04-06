#include "camera.h"
#include <Arduino.h>

// quality percent (1–100) → esp32-camera value (0–63, lower = better)
uint8_t Camera::percentToEspQuality(uint8_t pct) {
    if (pct > 100) pct = 100;
    if (pct < 1)   pct = 1;
    return static_cast<uint8_t>((100u - pct) * 63u / 100u);
}

// Build camera_config_t from board pin macros and stored settings, then init driver.
bool Camera::begin(const Config& config) {
    camera_config_t cfg = {};

    cfg.pin_pwdn     = CAM_PIN_PWDN;
    cfg.pin_reset    = CAM_PIN_RESET;
    cfg.pin_xclk     = CAM_PIN_XCLK;
    cfg.pin_sccb_sda = CAM_PIN_SIOD;
    cfg.pin_sccb_scl = CAM_PIN_SIOC;
    cfg.pin_d7       = CAM_PIN_D7;
    cfg.pin_d6       = CAM_PIN_D6;
    cfg.pin_d5       = CAM_PIN_D5;
    cfg.pin_d4       = CAM_PIN_D4;
    cfg.pin_d3       = CAM_PIN_D3;
    cfg.pin_d2       = CAM_PIN_D2;
    cfg.pin_d1       = CAM_PIN_D1;
    cfg.pin_d0       = CAM_PIN_D0;
    cfg.pin_vsync    = CAM_PIN_VSYNC;
    cfg.pin_href     = CAM_PIN_HREF;
    cfg.pin_pclk     = CAM_PIN_PCLK;

    cfg.xclk_freq_hz = 20000000;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.ledc_channel = LEDC_CHANNEL_0;

    cfg.pixel_format = PIXFORMAT_JPEG;
    cfg.frame_size   = static_cast<framesize_t>(config.getFrameSize());
    cfg.jpeg_quality = percentToEspQuality(config.getJpegQuality());
    cfg.fb_count     = 2;
    cfg.fb_location  = CAMERA_FB_IN_PSRAM;
    cfg.grab_mode    = CAMERA_GRAB_LATEST;

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
        Serial.printf("[camera] init failed: 0x%x\n", err);
        return false;
    }

    _ready = true;
    return true;
}

// Acquire a driver-managed JPEG frame buffer.
camera_fb_t* Camera::captureFrame() {
    if (!_ready) return nullptr;
    return esp_camera_fb_get();
}

// Return buffer to the driver pool.
void Camera::releaseFrame(camera_fb_t* fb) {
    if (fb) esp_camera_fb_return(fb);
}
