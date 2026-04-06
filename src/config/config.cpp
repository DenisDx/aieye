#include "config.h"

static constexpr char NVS_NS[]        = "aieye";
static constexpr char KEY_SSID[]      = "ssid";
static constexpr char KEY_PASS[]      = "pass";
static constexpr char KEY_QUALITY[]   = "quality";
static constexpr char KEY_FRAMESIZE[] = "framesize";
static constexpr char KEY_LIGHT_EN[]  = "light_en";
static constexpr char KEY_LIGHT_BR[]  = "light_br";
static constexpr char KEY_LIGHT_THR[] = "light_thr";
static constexpr char KEY_LED_BLUE[]  = "led_blue";
static constexpr char KEY_LED_CAP_MS[]= "led_cap_ms";
static constexpr char KEY_LED_CAP_BR[]= "led_cap_br";

// Open NVS namespace and populate in-memory state from stored values.
void Config::begin() {
    _prefs.begin(NVS_NS, /*readOnly=*/false);
    _ssid        = _prefs.getString(KEY_SSID,     "");
    _password    = _prefs.getString(KEY_PASS,     "");
    _jpegQuality = _prefs.getUChar(KEY_QUALITY,   90);
    _frameSize   = _prefs.getUChar(KEY_FRAMESIZE, 13);
    _illuminationEnabled = _prefs.getBool(KEY_LIGHT_EN, false);
    _illuminationBrightness = _prefs.getUChar(KEY_LIGHT_BR, 100);
    _illuminationCaptureThresholdMs = _prefs.getUShort(KEY_LIGHT_THR, 300);
    _ledBlueBrightness   = _prefs.getUChar(KEY_LED_BLUE, 10);
    _ledCaptureDurationMs= _prefs.getUShort(KEY_LED_CAP_MS, 250);
    _ledCaptureBrightness= _prefs.getUChar(KEY_LED_CAP_BR, 100);
}

String Config::getSSID()     const { return _ssid; }
String Config::getPassword() const { return _password; }

void Config::setWifi(const String& ssid, const String& password) {
    _ssid     = ssid;
    _password = password;
}

bool Config::isWifiConfigured() const {
    return _ssid.length() > 0;
}

uint8_t Config::getJpegQuality() const   { return _jpegQuality; }
void    Config::setJpegQuality(uint8_t q){ _jpegQuality = q; }

uint8_t Config::getFrameSize() const     { return _frameSize; }
void    Config::setFrameSize(uint8_t fs) { _frameSize = fs; }

bool Config::getIlluminationEnabled() const { return _illuminationEnabled; }
void Config::setIlluminationEnabled(bool v) { _illuminationEnabled = v; }

uint8_t Config::getIlluminationBrightness() const { return _illuminationBrightness; }
void Config::setIlluminationBrightness(uint8_t v) {
    if (v > 100) v = 100;
    _illuminationBrightness = v;
}

uint16_t Config::getIlluminationCaptureThresholdMs() const {
    return _illuminationCaptureThresholdMs;
}

void Config::setIlluminationCaptureThresholdMs(uint16_t v) {
    if (v < 50) v = 50;
    if (v > 5000) v = 5000;
    _illuminationCaptureThresholdMs = v;
}

uint8_t Config::getLedBlueBrightness() const { return _ledBlueBrightness; }
void Config::setLedBlueBrightness(uint8_t v) {
    if (v > 100) v = 100;
    _ledBlueBrightness = v;
}

uint16_t Config::getLedCaptureDurationMs() const { return _ledCaptureDurationMs; }
void Config::setLedCaptureDurationMs(uint16_t v) {
    if (v < 20) v = 20;
    if (v > 2000) v = 2000;
    _ledCaptureDurationMs = v;
}

uint8_t Config::getLedCaptureBrightness() const { return _ledCaptureBrightness; }
void Config::setLedCaptureBrightness(uint8_t v) {
    if (v > 100) v = 100;
    _ledCaptureBrightness = v;
}

// Persist current in-memory state to NVS.
void Config::save() {
    _prefs.putString(KEY_SSID,     _ssid);
    _prefs.putString(KEY_PASS,     _password);
    _prefs.putUChar(KEY_QUALITY,   _jpegQuality);
    _prefs.putUChar(KEY_FRAMESIZE, _frameSize);
    _prefs.putBool(KEY_LIGHT_EN,    _illuminationEnabled);
    _prefs.putUChar(KEY_LIGHT_BR,   _illuminationBrightness);
    _prefs.putUShort(KEY_LIGHT_THR, _illuminationCaptureThresholdMs);
    _prefs.putUChar(KEY_LED_BLUE,   _ledBlueBrightness);
    _prefs.putUShort(KEY_LED_CAP_MS,_ledCaptureDurationMs);
    _prefs.putUChar(KEY_LED_CAP_BR, _ledCaptureBrightness);
}

// Erase all keys in the NVS namespace and reset in-memory state to defaults.
void Config::reset() {
    _prefs.clear();
    _ssid        = "";
    _password    = "";
    _jpegQuality = 90;
    _frameSize   = 13;
    _illuminationEnabled = false;
    _illuminationBrightness = 100;
    _illuminationCaptureThresholdMs = 300;
    _ledBlueBrightness = 10;
    _ledCaptureDurationMs = 250;
    _ledCaptureBrightness = 100;
}
