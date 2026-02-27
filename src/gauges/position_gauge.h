#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

class PositionGauge : public GaugeBase {
public:
    // Secondary value (longitude) stored per-cell by dashboard
    float secondaryValue = 0.0f;

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
    static constexpr int SECONDARY_INDEX_BASE = 100;  // + cellIndex for per-cell secondary

    bool customRender(void* spritePtr, float lat, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);
        float lon = secondaryValue;

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_BG);
        canvas->drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(cx, cy, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Title
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.0);
        canvas->drawString("POSITION", cx, cy - 65);

        // ── Latitude ──
        char ns = (lat >= 0) ? 'N' : 'S';
        float absLat = fabsf(lat);
        int latDeg = (int)absLat;
        float latMin = (absLat - latDeg) * 60.0f;

        canvas->setTextColor(COLOR_LABEL);
        canvas->setTextSize(0.8);
        canvas->drawString("LAT", cx, cy - 40);

        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.2);
        char buf[24];
        snprintf(buf, sizeof(buf), "%c %d %05.2f'", ns, latDeg, latMin);
        canvas->drawString(buf, cx, cy - 18);

        // ── Longitude ──
        char ew = (lon >= 0) ? 'E' : 'W';
        float absLon = fabsf(lon);
        int lonDeg = (int)absLon;
        float lonMin = (absLon - lonDeg) * 60.0f;

        canvas->setTextColor(COLOR_LABEL);
        canvas->setTextSize(0.8);
        canvas->drawString("LON", cx, cy + 8);

        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.2);
        snprintf(buf, sizeof(buf), "%c %d %05.2f'", ew, lonDeg, lonMin);
        canvas->drawString(buf, cx, cy + 30);

        return true;
    }
};
