#include "wifi_manager.h"
#include "config.h"

void WiFiManager::begin(const WiFiCredential* networks, int count) {
    WiFi.mode(WIFI_STA);
    for (int i = 0; i < count; i++) {
        _wifiMulti.addAP(networks[i].ssid, networks[i].password);
    }
    _connectStartTime = millis();
    _wifiMulti.run();
}

void WiFiManager::addNetwork(const char* ssid, const char* password) {
    _wifiMulti.addAP(ssid, password);
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::ensureConnected() {
    if (isConnected()) return;

    unsigned long now = millis();
    if (now - _lastAttempt < RETRY_INTERVAL_MS) return;
    _lastAttempt = now;

    _wifiMulti.run();
}

void WiFiManager::drawStatus(LGFX_Sprite& fb) {
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;

    fb.fillSprite(COLOR_BG);
    fb.setTextDatum(middle_center);

    if (isConnected()) {
        fb.setTextColor(COLOR_ARC_GREEN);
        fb.setTextSize(3.0);
        fb.drawString("WiFi Connected", cx, cy - 40);
        fb.setTextColor(COLOR_LABEL);
        fb.setTextSize(2.5);
        fb.drawString(WiFi.localIP().toString().c_str(), cx, cy + 10);

        // Show web UI URL
        fb.setTextColor(COLOR_DIAL_RIM);
        fb.setTextSize(1.8);
        char urlBuf[48];
        snprintf(urlBuf, sizeof(urlBuf), "Web UI: http://%s", WiFi.localIP().toString().c_str());
        fb.drawString(urlBuf, cx, cy + 55);
    } else {
        fb.setTextColor(COLOR_ARC_YELLOW);
        fb.setTextSize(3.0);
        fb.drawString("Scanning WiFi...", cx, cy - 30);
        fb.setTextColor(COLOR_LABEL);
        fb.setTextSize(2.5);
        fb.drawString(WiFi.SSID().c_str(), cx, cy + 35);
    }
}
