#pragma once

#include "gauge_base.h"
#include <M5Dial.h>
#include <cmath>
#include "../config.h"

// Secondary value stored by main.cpp when position gauge is active
extern float g_secondaryValue;

class PositionGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "POSITION",
            .units        = "",
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

    // Secondary dataref for longitude
    static const char* secondaryDataref() { return "sim/flightmodel/position/longitude"; }
    static constexpr int SECONDARY_INDEX = 100;

    bool customRender(void* spritePtr, float lat) override {
        M5Canvas* canvas = static_cast<M5Canvas*>(spritePtr);
        float lon = g_secondaryValue;

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_BG);
        canvas->drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Title
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.0);
        canvas->drawString("POSITION", CENTER_X, CENTER_Y - 65);

        // ── Latitude ──
        char ns = (lat >= 0) ? 'N' : 'S';
        float absLat = fabsf(lat);
        int latDeg = (int)absLat;
        float latMin = (absLat - latDeg) * 60.0f;

        canvas->setTextColor(COLOR_LABEL);
        canvas->setTextSize(0.8);
        canvas->drawString("LAT", CENTER_X, CENTER_Y - 40);

        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.2);
        char buf[24];
        snprintf(buf, sizeof(buf), "%c %d %05.2f'", ns, latDeg, latMin);
        canvas->drawString(buf, CENTER_X, CENTER_Y - 18);

        // ── Longitude ──
        char ew = (lon >= 0) ? 'E' : 'W';
        float absLon = fabsf(lon);
        int lonDeg = (int)absLon;
        float lonMin = (absLon - lonDeg) * 60.0f;

        canvas->setTextColor(COLOR_LABEL);
        canvas->setTextSize(0.8);
        canvas->drawString("LON", CENTER_X, CENTER_Y + 8);

        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.2);
        snprintf(buf, sizeof(buf), "%c %d %05.2f'", ew, lonDeg, lonMin);
        canvas->drawString(buf, CENTER_X, CENTER_Y + 30);

        // ── Decimal readout at bottom ──
        canvas->setTextSize(0.7);
        canvas->setTextColor(COLOR_DIAL_RIM);
        snprintf(buf, sizeof(buf), "%.4f, %.4f", lat, lon);
        canvas->drawString(buf, CENTER_X, CENTER_Y + 58);

        return true;
    }
};
