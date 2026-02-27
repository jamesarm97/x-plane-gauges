#include "wifi_manager.h"
#include "config.h"

void WiFiManager::begin(const WiFiCredential* networks, int count) {
    WiFi.mode(WIFI_STA);
    for (int i = 0; i < count; i++) {
        _wifiMulti.addAP(networks[i].ssid, networks[i].password);
    }
    _wifiMulti.run();
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
        fb.drawString(WiFi.localIP().toString().c_str(), cx, cy + 25);
    } else {
        fb.setTextColor(COLOR_ARC_YELLOW);
        fb.setTextSize(3.0);
        fb.drawString("Scanning WiFi...", cx, cy - 30);
        fb.setTextColor(COLOR_LABEL);
        fb.setTextSize(2.5);
        fb.drawString(WiFi.SSID().c_str(), cx, cy + 35);
    }
}
