#pragma once

#include "gauge_base.h"
#include <M5Dial.h>
#include <cmath>
#include "../config.h"

class LatGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "LATITUDE",
            .units        = "DEG",
            .dataref      = "sim/flightmodel/position/latitude",
            .minValue     = -90,
            .maxValue     = 90,
            .startAngle   = 0,
            .endAngle     = 360,
            .ticks        = { .majorInterval = 30, .minorInterval = 10, .labelDivisor = 1, .showLabels = false },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = true,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, float lat) override {
        M5Canvas* canvas = static_cast<M5Canvas*>(spritePtr);

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_BG);
        canvas->drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Title
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.0);
        canvas->drawString("LATITUDE", CENTER_X, CENTER_Y - 50);

        // N/S indicator
        char ns = (lat >= 0) ? 'N' : 'S';
        float absLat = fabsf(lat);

        // Degrees and decimal minutes
        int deg = (int)absLat;
        float minutes = (absLat - deg) * 60.0f;

        // Large degree display
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.8);
        char buf[16];
        snprintf(buf, sizeof(buf), "%c %d", ns, deg);
        canvas->drawString(buf, CENTER_X, CENTER_Y - 10);

        // Minutes below
        canvas->setTextSize(1.3);
        canvas->setTextColor(COLOR_LABEL);
        snprintf(buf, sizeof(buf), "%06.3f'", minutes);
        canvas->drawString(buf, CENTER_X, CENTER_Y + 25);

        return true;
    }
};
