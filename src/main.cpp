#include <Arduino.h>
#include <WiFi.h>

#include "config/config.h"
#include "camera/camera.h"
#include "webserver/webserver.h"
#include "captive_portal/captive_portal.h"
#include "button/button.h"
#include "status_led/status_led.h"

static Config        g_config;
static Camera        g_camera;
static AppWebServer  g_webServer;
static CaptivePortal g_captivePortal;
static Button        g_button;

void setup() {
    Serial.begin(115200);

    StatusLed::begin();

    g_button.begin();
    g_config.begin();
    StatusLed::setBlueBrightnessPercent(g_config.getLedBlueBrightness());
    StatusLed::setCapturePulse(
        g_config.getLedCaptureDurationMs(),
        g_config.getLedCaptureBrightness()
    );
    g_camera.begin(g_config);
    StatusLed::setCameraReady(g_camera.isReady());

    // Keep setup AP always available as a fallback access path.
    g_captivePortal.startAP();
    g_captivePortal.beginDns();

    if (!g_captivePortal.connectWifi(g_config)) {
        Serial.println("[wifi] STA not available at boot, using setup AP only");
    }
    StatusLed::setWifiConnected(g_captivePortal.isStaConnected());

    g_webServer.begin(g_config, g_camera);
}

void loop() {
    switch (g_button.poll()) {
        case ButtonEvent::SHORT_PRESS:
            Serial.println("[button] short press – reboot");
            delay(100);
            ESP.restart();
            break;
        case ButtonEvent::LONG_PRESS:
            Serial.println("[button] long press – factory reset");
            g_config.reset();
            delay(100);
            ESP.restart();
            break;
        default:
            break;
    }

    g_captivePortal.maintain(g_config, 10000);
    StatusLed::setWifiConnected(g_captivePortal.isStaConnected());
    StatusLed::setCameraReady(g_camera.isReady());
    StatusLed::tick();

    g_webServer.handle();
    g_captivePortal.handleDns();
}
