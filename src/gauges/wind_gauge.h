#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

class WindGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "WIND",
            .units        = "KTS",
            .dataref      = "sim/cockpit2/gauges/indicators/wind_speed_kts",
            .minValue     = 0,
            .maxValue     = 100,
            .startAngle   = 0,
            .endAngle     = 360,
            .ticks        = { .majorInterval = 20, .minorInterval = 5, .labelDivisor = 1, .showLabels = false },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = true,
            .extraDatarefs = {
                "sim/cockpit2/gauges/indicators/wind_heading_deg_mag",
                "sim/cockpit2/gauges/indicators/heading_AHARS_deg_mag_pilot",
            },
            .extraDatarefCount = 2,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);
        float windSpeed = values[0];
        float windDir   = (valueCount > 1) ? values[1] : 0.0f;
        float heading   = (valueCount > 2) ? values[2] : 0.0f;

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_BG);
        canvas->drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(cx, cy, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Compass ring (simplified)
        for (int deg = 0; deg < 360; deg += 30) {
            float dispAngle = (float)deg - heading;
            float rad = dispAngle * DEG_TO_RAD;
            float sinA = sinf(rad);
            float cosA = cosf(rad);

            int x1 = cx + (int)(TICK_MINOR_R * sinA);
            int y1 = cy - (int)(TICK_MINOR_R * cosA);
            int x2 = cx + (int)(TICK_OUTER_R * sinA);
            int y2 = cy - (int)(TICK_OUTER_R * cosA);
            canvas->drawLine(x1, y1, x2, y2, COLOR_TICK);
            canvas->drawLine(x1 + 1, y1, x2 + 1, y2, COLOR_TICK);

            // Cardinal labels
            int lx = cx + (int)((LABEL_RADIUS + 5) * sinA);
            int ly = cy - (int)((LABEL_RADIUS + 5) * cosA);
            canvas->setTextDatum(middle_center);
            canvas->setTextColor(COLOR_LABEL);
            canvas->setTextSize(0.8);
            if (deg == 0) canvas->drawString("N", lx, ly);
            else if (deg == 90) canvas->drawString("E", lx, ly);
            else if (deg == 180) canvas->drawString("S", lx, ly);
            else if (deg == 270) canvas->drawString("W", lx, ly);
        }

        // Aircraft symbol at center
        canvas->fillTriangle(cx, cy - 12, cx - 4, cy + 4, cx + 4, cy + 4, COLOR_ARC_YELLOW);
        canvas->fillRect(cx - 10, cy - 2, 20, 3, COLOR_ARC_YELLOW);

        // Wind arrow — from wind direction relative to aircraft heading
        float relAngle = (windDir - heading) * DEG_TO_RAD;
        float sinW = sinf(relAngle);
        float cosW = cosf(relAngle);

        // Arrow points FROM wind direction toward center
        int arrowLen = DIAL_RADIUS - 30;
        int tipX = cx + (int)(20 * sinW);
        int tipY = cy - (int)(20 * cosW);
        int tailX = cx + (int)(arrowLen * sinW);
        int tailY = cy - (int)(arrowLen * cosW);

        // Shaft
        canvas->drawLine(tipX, tipY, tailX, tailY, COLOR_ARC_GREEN);
        canvas->drawLine(tipX + 1, tipY, tailX + 1, tailY, COLOR_ARC_GREEN);

        // Arrow head at tip
        float headAngle1 = relAngle - 2.8f;
        float headAngle2 = relAngle + 2.8f;
        int headLen = 12;
        canvas->drawLine(tipX, tipY, tipX + (int)(headLen * sinf(headAngle1)), tipY - (int)(headLen * cosf(headAngle1)), COLOR_ARC_GREEN);
        canvas->drawLine(tipX, tipY, tipX + (int)(headLen * sinf(headAngle2)), tipY - (int)(headLen * cosf(headAngle2)), COLOR_ARC_GREEN);

        // Fixed top index
        canvas->fillTriangle(cx, cy - TICK_OUTER_R - 5, cx - 6, cy - TICK_OUTER_R + 8,
                            cx + 6, cy - TICK_OUTER_R + 8, COLOR_ARC_YELLOW);

        // Wind speed and direction text
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(0.9);
        canvas->drawString("WIND", cx, cy - DIAL_RADIUS + 18);

        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.4);
        char buf[8];
        snprintf(buf, sizeof(buf), "%.0f", windSpeed);
        canvas->drawString(buf, cx, cy + 30);

        canvas->setTextSize(0.9);
        canvas->setTextColor(COLOR_LABEL);
        snprintf(buf, sizeof(buf), "%03d", (int)windDir % 360);
        canvas->drawString(buf, cx, cy + 50);
        canvas->drawString("KTS", cx, cy + 65);

        return true;
    }
};
