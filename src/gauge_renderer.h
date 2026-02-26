#pragma once

#include <M5Dial.h>
#include "gauges/gauge_base.h"

class GaugeRenderer {
public:
    void begin();
    void render(GaugeBase* gauge, float rawValue, float smoothedValue, bool xplaneConnected = false);
    void push(M5GFX& display);
    M5Canvas& getCanvas() { return _canvas; }

private:
    M5Canvas _canvas;       // Off-screen sprite buffer
    M5Canvas _needle;       // Small needle sprite for rotation
    bool _initialized = false;

    void drawBackground();
    void drawArcs(const GaugeConfig& cfg);
    void drawTicks(const GaugeConfig& cfg);
    void drawNeedle(float angle);
    void drawHub();
    void drawTitle(const GaugeConfig& cfg);
    void drawValue(const GaugeConfig& cfg, float value);
    void drawConnectionStatus(bool connected);

    float valueToAngle(const GaugeConfig& cfg, float value);
};
