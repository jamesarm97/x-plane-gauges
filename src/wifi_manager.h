#pragma once

#include <M5Dial.h>
#include <WiFi.h>
#include <WiFiMulti.h>

struct WiFiCredential {
    const char* ssid;
    const char* password;
};

class WiFiManager {
public:
    void begin(const WiFiCredential* networks, int count);
    bool isConnected();
    void ensureConnected();
    void drawStatus(M5GFX& display);

private:
    WiFiMulti _wifiMulti;
    unsigned long _lastAttempt = 0;
    static constexpr unsigned long RETRY_INTERVAL_MS = 5000;
};
