#pragma once

#include <Preferences.h>
#include <Arduino.h>

// Persistent device configuration backed by NVS (Preferences).
class Config {
public:
    // Open NVS namespace and load saved values into memory.
    void begin();

    // WiFi credentials.
    String getSSID()     const;
    String getPassword() const;
    // Set credentials (call save() to persist).
    void   setWifi(const String& ssid, const String& password);
    // True if SSID has been saved.
    bool   isWifiConfigured() const;

    // JPEG quality: 1–100 (higher = better image), default 90.
    uint8_t getJpegQuality() const;
    void    setJpegQuality(uint8_t quality);

    // Frame size as esp32-camera framesize_t value; default 13 (FRAMESIZE_UXGA).
    uint8_t getFrameSize() const;
    void    setFrameSize(uint8_t frameSize);

    // Enable adaptive white illumination on photo capture, default false.
    bool getIlluminationEnabled() const;
    void setIlluminationEnabled(bool enabled);

    // White illumination brightness percent (0..100), default 100.
    uint8_t getIlluminationBrightness() const;
    void    setIlluminationBrightness(uint8_t brightnessPercent);

    // Capture duration threshold in milliseconds for enabling assist mode, default 300.
    uint16_t getIlluminationCaptureThresholdMs() const;
    void     setIlluminationCaptureThresholdMs(uint16_t thresholdMs);

    // Blue heartbeat brightness percent (0..100), default 10.
    uint8_t getLedBlueBrightness() const;
    void    setLedBlueBrightness(uint8_t brightnessPercent);

    // Green capture pulse duration in milliseconds, default 250.
    uint16_t getLedCaptureDurationMs() const;
    void     setLedCaptureDurationMs(uint16_t durationMs);

    // Green capture pulse brightness percent (0..100), default 100.
    uint8_t getLedCaptureBrightness() const;
    void    setLedCaptureBrightness(uint8_t brightnessPercent);

    // Horizontal mirror (flip image left-right), default false.
    bool getMirrorEnabled() const;
    void setMirrorEnabled(bool enabled);

    // Write all current values to NVS.
    void save();

    // Erase all NVS keys and reset in-memory values to defaults.
    void reset();

private:
    Preferences _prefs;

    String  _ssid;
    String  _password;
    uint8_t _jpegQuality{90};
    uint8_t _frameSize{13};   // FRAMESIZE_UXGA
    bool    _illuminationEnabled{false};
    uint8_t _illuminationBrightness{100};
    uint16_t _illuminationCaptureThresholdMs{300};
    uint8_t  _ledBlueBrightness{10};
    uint16_t _ledCaptureDurationMs{250};
    uint8_t  _ledCaptureBrightness{100};
    bool     _mirrorEnabled{false};
};
