#pragma once

#include <DNSServer.h>
#include <WiFi.h>
#include "../config/config.h"

// WiFi management and captive portal: STA connection + AP fallback with DNS redirect.
class CaptivePortal {
public:
    // Attempt WPA2 STA connection using saved credentials.
    // Returns true if connected within timeoutMs; false if not configured or timed out.
    bool connectWifi(const Config& config, uint32_t timeoutMs = 10000);

    // Start softAP "AIeye-setup" so any device can reach the configuration page.
    void startAP();

    // Start DNS server on port 53; redirects all queries to softAP IP.
    // Call only after startAP().
    void beginDns();

    // Process pending DNS requests; call from loop(). No-op in STA mode.
    void handleDns();

    // Keep connectivity alive: retry STA connection periodically.
    // If STA is down, AP is started so setup UI remains reachable.
    void maintain(const Config& config, uint32_t retryIntervalMs = 10000);

    // True when STA is connected and has an IP.
    bool isStaConnected() const;

    // True when running in AP (captive portal) mode.
    bool isAPMode() const { return _apMode; }

private:
    DNSServer _dnsServer;
    bool      _apMode{false};
    uint32_t  _lastRetryMs{0};

    static constexpr char     AP_SSID[]  = "AIeye-setup";
    static constexpr uint16_t DNS_PORT   = 53;
};
