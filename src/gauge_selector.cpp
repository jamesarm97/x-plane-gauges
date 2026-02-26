#include "gauge_selector.h"
#include "config.h"

void GaugeSelector::begin(XPlaneClient* client) {
    _client = client;
    _currentIndex = loadIndex();
    _pendingIndex = _currentIndex;
    _selecting = false;
    _changed = true;  // Trigger initial subscription
    _lastEncoderPos = 0;
}

void GaugeSelector::update() {
    // Read rotary encoder position from M5Dial
    long pos = M5Dial.Encoder.read();
    long delta = pos - _lastEncoderPos;

    // Require a full detent click to register (M5Dial detents are every 4 ticks)
    if (abs(delta) >= 4) {
        _lastEncoderPos = pos;
        int step = (delta > 0) ? 1 : -1;

        _pendingIndex = (_pendingIndex + step + GAUGE_COUNT) % GAUGE_COUNT;
        _selecting = true;
        _lastInputTime = millis();
    }

    // Button press immediately confirms
    M5Dial.update();
    if (M5Dial.BtnA.wasPressed()) {
        if (_selecting) {
            confirmSelection();
        }
    }

    // Auto-confirm after timeout
    if (_selecting && (millis() - _lastInputTime > SELECTOR_TIMEOUT_MS)) {
        confirmSelection();
    }
}

void GaugeSelector::confirmSelection() {
    if (_pendingIndex != _currentIndex) {
        // Unsubscribe from old dataref
        const GaugeConfig& oldCfg = g_gauges[_currentIndex]->getConfig();
        _client->unsubscribe(oldCfg.dataref, _currentIndex);

        _currentIndex = _pendingIndex;
        _changed = true;

        // Subscribe to new dataref
        const GaugeConfig& newCfg = g_gauges[_currentIndex]->getConfig();
        _client->subscribe(newCfg.dataref, XPLANE_FREQ, _currentIndex);

        // Persist to NVS
        saveIndex(_currentIndex);
    }
    _selecting = false;
}

void GaugeSelector::saveIndex(int index) {
    _prefs.begin("gauge", false);
    _prefs.putInt("idx", index);
    _prefs.end();
}

int GaugeSelector::loadIndex() {
    _prefs.begin("gauge", true);
    int idx = _prefs.getInt("idx", 0);
    _prefs.end();
    if (idx < 0 || idx >= GAUGE_COUNT) idx = 0;
    return idx;
}

bool GaugeSelector::selectionChanged() {
    if (_changed) {
        _changed = false;
        return true;
    }
    return false;
}

void GaugeSelector::draw(M5Canvas& canvas) {
    if (!_selecting) return;

    // Overlay bar at bottom of sprite (raised well above round display edge)
    canvas.fillRect(0, SCREEN_HEIGHT - 70, SCREEN_WIDTH, 50, COLOR_SELECTOR_BG);
    canvas.drawLine(0, SCREEN_HEIGHT - 70, SCREEN_WIDTH, SCREEN_HEIGHT - 70, COLOR_SELECTOR_FG);

    // Show pending gauge name
    const GaugeConfig& cfg = g_gauges[_pendingIndex]->getConfig();
    canvas.setTextDatum(middle_center);
    canvas.setTextColor(COLOR_SELECTOR_FG);
    canvas.setTextSize(1.0);

    char buf[32];
    snprintf(buf, sizeof(buf), "< %s >", cfg.title);
    canvas.drawString(buf, CENTER_X, SCREEN_HEIGHT - 52);

    // Index indicator
    canvas.setTextSize(1.0);
    canvas.setTextColor(COLOR_DIAL_RIM);
    snprintf(buf, sizeof(buf), "%d / %d", _pendingIndex + 1, GAUGE_COUNT);
    canvas.drawString(buf, CENTER_X, SCREEN_HEIGHT - 30);
}
