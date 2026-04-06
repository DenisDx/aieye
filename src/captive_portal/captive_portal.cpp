#include "captive_portal.h"
#include <Arduino.h>

constexpr char     CaptivePortal::AP_SSID[];
constexpr uint16_t CaptivePortal::DNS_PORT;

// Try WPA2 STA connection; poll until connected or timeout.
bool CaptivePortal::connectWifi(const Config& config, uint32_t timeoutMs) {
    if (!config.isWifiConfigured()) return false;

    // Keep AP alive if it is already enabled; otherwise use STA only.
    WiFi.mode(_apMode ? WIFI_AP_STA : WIFI_STA);
    // Disable power-save: without this the kernel may sleep between packets,
    // making the TCP server intermittently unreachable from the LAN.
    WiFi.setSleep(false);
    WiFi.begin(config.getSSID().c_str(), config.getPassword().c_str());

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start > timeoutMs) {
            Serial.println("[wifi] connection timed out");
            // Do not power WiFi off here: AP fallback must remain available.
            WiFi.disconnect(/*wifioff=*/false);
            return false;
        }
        delay(250);
    }

    Serial.printf("[wifi] connected  IP: %s  GW: %s  Mask: %s  RSSI: %d dBm\n",
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str(),
                  WiFi.subnetMask().toString().c_str(),
                  WiFi.RSSI());
    return true;
}

// Raise softAP so unconfigured clients can reach the setup page.
void CaptivePortal::startAP() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID);
    _apMode = true;
    Serial.printf("[captive] AP \"%s\"  IP: %s\n",
                  AP_SSID, WiFi.softAPIP().toString().c_str());
}

// Redirect every DNS query to the AP's own IP (captive portal pattern).
void CaptivePortal::beginDns() {
    if (!_apMode) return;
    _dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
}

// Handle one pending DNS packet; no-op when in STA mode.
void CaptivePortal::handleDns() {
    if (_apMode) _dnsServer.processNextRequest();
}

// True when STA is connected and has a valid IPv4.
bool CaptivePortal::isStaConnected() const {
    return WiFi.status() == WL_CONNECTED && WiFi.localIP() != IPAddress((uint32_t)0);
}

// Keep trying STA and ensure AP fallback exists while disconnected.
void CaptivePortal::maintain(const Config& config, uint32_t retryIntervalMs) {
    if (isStaConnected()) return;

    if (!_apMode) {
        Serial.println("[wifi] STA lost, enabling captive AP fallback");
        startAP();
        beginDns();
    }

    uint32_t now = millis();
    if (now - _lastRetryMs < retryIntervalMs) return;
    _lastRetryMs = now;

    Serial.println("[wifi] retrying STA connection...");
    if (connectWifi(config, 5000)) {
        Serial.println("[wifi] STA restored");
        // Keep AP enabled intentionally: setup UI remains reachable as backup.
    }
}
