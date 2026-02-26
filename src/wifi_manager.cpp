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

void WiFiManager::drawStatus(M5GFX& display) {
    display.fillScreen(COLOR_BG);
    display.setTextDatum(middle_center);

    if (isConnected()) {
        display.setTextColor(COLOR_ARC_GREEN);
        display.setTextSize(1.5);
        display.drawString("WiFi Connected", CENTER_X, CENTER_Y - 25);
        display.setTextColor(COLOR_LABEL);
        display.setTextSize(1.1);
        display.drawString(WiFi.localIP().toString().c_str(), CENTER_X, CENTER_Y + 15);
    } else {
        display.setTextColor(COLOR_ARC_YELLOW);
        display.setTextSize(1.4);
        display.drawString("Scanning WiFi...", CENTER_X, CENTER_Y - 15);
        display.setTextColor(COLOR_LABEL);
        display.setTextSize(1.1);
        display.drawString(WiFi.SSID().c_str(), CENTER_X, CENTER_Y + 20);
    }
}
