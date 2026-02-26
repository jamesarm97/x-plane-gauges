#pragma once

#include "gauge_base.h"
#include <M5Dial.h>
#include <cmath>
#include "../config.h"

class LonGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "LONGITUDE",
            .units        = "DEG",
            .dataref      = "sim/flightmodel/position/longitude",
            .minValue     = -180,
            .maxValue     = 180,
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

    bool customRender(void* spritePtr, float lon) override {
        M5Canvas* canvas = static_cast<M5Canvas*>(spritePtr);

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_BG);
        canvas->drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Title
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.0);
        canvas->drawString("LONGITUDE", CENTER_X, CENTER_Y - 50);

        // E/W indicator
        char ew = (lon >= 0) ? 'E' : 'W';
        float absLon = fabsf(lon);

        // Degrees and decimal minutes
        int deg = (int)absLon;
        float minutes = (absLon - deg) * 60.0f;

        // Large degree display
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.8);
        char buf[16];
        snprintf(buf, sizeof(buf), "%c %d", ew, deg);
        canvas->drawString(buf, CENTER_X, CENTER_Y - 10);

        // Minutes below
        canvas->setTextSize(1.3);
        canvas->setTextColor(COLOR_LABEL);
        snprintf(buf, sizeof(buf), "%06.3f'", minutes);
        canvas->drawString(buf, CENTER_X, CENTER_Y + 25);

        return true;
    }
};
