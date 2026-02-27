#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

class HeadingGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "HEADING",
            .units        = "DEG",
            .dataref      = "sim/cockpit2/gauges/indicators/heading_AHARS_deg_mag_pilot",
            .minValue     = 0,
            .maxValue     = 360,
            .startAngle   = 0,
            .endAngle     = 360,
            .ticks        = { .majorInterval = 30, .minorInterval = 10, .labelDivisor = 1, .showLabels = false },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = true,     // Uses rotating compass card
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        float heading = values[0];
        (void)valueCount;
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);

        // Background
        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_BG);
        canvas->drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);

        // Draw rotating compass card
        for (int deg = 0; deg < 360; deg += 5) {
            float dispAngle = (float)deg - heading;
            float rad = dispAngle * DEG_TO_RAD;
            float sinA = sinf(rad);
            float cosA = cosf(rad);

            bool isMajor = (deg % 30 == 0);
            bool isMedium = (deg % 10 == 0);

            int innerR = isMajor ? TICK_MAJOR_R : (isMedium ? TICK_MINOR_R : TICK_MINOR_R + 3);

            int x1 = cx + (int)(innerR * sinA);
            int y1 = cy - (int)(innerR * cosA);
            int x2 = cx + (int)(TICK_OUTER_R * sinA);
            int y2 = cy - (int)(TICK_OUTER_R * cosA);

            canvas->drawLine(x1, y1, x2, y2, COLOR_TICK);
            if (isMajor) {
                canvas->drawLine(x1 + 1, y1, x2 + 1, y2, COLOR_TICK);
            }

            // Cardinal and ordinal labels
            if (isMajor) {
                int lx = cx + (int)(LABEL_RADIUS * sinA);
                int ly = cy - (int)(LABEL_RADIUS * cosA);
                canvas->setTextDatum(middle_center);
                canvas->setTextColor(COLOR_LABEL);
                canvas->setTextSize(1.0);

                const char* label = nullptr;
                switch (deg) {
                    case 0:   label = "N"; break;
                    case 30:  label = "3"; break;
                    case 60:  label = "6"; break;
                    case 90:  label = "E"; break;
                    case 120: label = "12"; break;
                    case 150: label = "15"; break;
                    case 180: label = "S"; break;
                    case 210: label = "21"; break;
                    case 240: label = "24"; break;
                    case 270: label = "W"; break;
                    case 300: label = "30"; break;
                    case 330: label = "33"; break;
                }
                if (label) canvas->drawString(label, lx, ly);
            }
        }

        // Fixed aircraft index triangle at top
        canvas->fillTriangle(
            cx, cy - TICK_OUTER_R - 5,
            cx - 6, cy - TICK_OUTER_R + 8,
            cx + 6, cy - TICK_OUTER_R + 8,
            COLOR_ARC_YELLOW
        );

        // Title and value
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.0);
        canvas->drawString("HDG", cx, cy - 18);

        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.3);
        char buf[8];
        snprintf(buf, sizeof(buf), "%03d", (int)heading % 360);
        canvas->drawString(buf, cx, cy + 10);

        // Center dot
        canvas->fillCircle(cx, cy, 3, COLOR_HUB);

        return true;
    }
};
