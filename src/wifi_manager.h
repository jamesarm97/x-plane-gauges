#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
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
    void drawStatus(LGFX_Sprite& fb);

private:
    WiFiMulti _wifiMulti;
    unsigned long _lastAttempt = 0;
    static constexpr unsigned long RETRY_INTERVAL_MS = 5000;
};
