#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

class HSIGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "HSI",
            .units        = "",
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
            .customRenderer = true,
            .extraDatarefs = {
                "sim/cockpit/radios/nav1_obs_degm",
                "sim/cockpit/radios/nav1_hdef_dot",
                "sim/cockpit/radios/nav1_vdef_dot",
            },
            .extraDatarefCount = 3,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);
        float heading = values[0];
        float obs = (valueCount > 1) ? values[1] : 0.0f;
        float cdi = (valueCount > 2) ? values[2] : 0.0f;   // dots, -2.5 to +2.5
        float gs  = (valueCount > 3) ? values[3] : 0.0f;    // dots, -2.5 to +2.5

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_BG);

        // Rotating compass card
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
                int lx = cx + (int)(LABEL_RADIUS * sinA);
                int ly = cy - (int)(LABEL_RADIUS * cosA);
                canvas->setTextDatum(middle_center);
                canvas->setTextColor(COLOR_LABEL);
                canvas->setTextSize(1.2);
                char lbl[4];
                snprintf(lbl, sizeof(lbl), "%d", deg / 10);
                if (deg == 0) canvas->drawString("N", lx, ly);
                else if (deg == 90) canvas->drawString("E", lx, ly);
                else if (deg == 180) canvas->drawString("S", lx, ly);
                else if (deg == 270) canvas->drawString("W", lx, ly);
                else canvas->drawString(lbl, lx, ly);
            }
        }

        // Course needle (OBS) — line rotated to OBS relative to heading
        float courseAngle = (obs - heading) * DEG_TO_RAD;
        float cSin = sinf(courseAngle);
        float cCos = cosf(courseAngle);
        int needleLen = DIAL_RADIUS - 25;

        // Course arrow from center outward
        int nx1 = cx + (int)(needleLen * cSin);
        int ny1 = cy - (int)(needleLen * cCos);
        int nx2 = cx - (int)(needleLen * cSin);
        int ny2 = cy + (int)(needleLen * cCos);
        canvas->drawLine(nx1, ny1, nx2, ny2, COLOR_ARC_GREEN);
        canvas->drawLine(nx1 + 1, ny1, nx2 + 1, ny2, COLOR_ARC_GREEN);

        // Arrow head at course end
        float arrowRad1 = courseAngle - 0.2f;
        float arrowRad2 = courseAngle + 0.2f;
        int arrowLen = needleLen - 12;
        canvas->drawLine(nx1, ny1, cx + (int)(arrowLen * sinf(arrowRad1)), cy - (int)(arrowLen * cosf(arrowRad1)), COLOR_ARC_GREEN);
        canvas->drawLine(nx1, ny1, cx + (int)(arrowLen * sinf(arrowRad2)), cy - (int)(arrowLen * cosf(arrowRad2)), COLOR_ARC_GREEN);

        // CDI deviation bar — perpendicular to course, offset by cdi dots
        // Each dot = 8 pixels offset
        float cdiOffset = cdi * 8.0f;
        float perpAngle = courseAngle + 1.5708f;  // +90°
        float perpSin = sinf(perpAngle);
        float perpCos = cosf(perpAngle);

        int barHalf = 30;
        int bx = cx + (int)(cdiOffset * perpSin);
        int by = cy - (int)(cdiOffset * perpCos);
        int bx1 = bx + (int)(barHalf * cSin);
        int by1 = by - (int)(barHalf * cCos);
        int bx2 = bx - (int)(barHalf * cSin);
        int by2 = by + (int)(barHalf * cCos);
        canvas->drawLine(bx1, by1, bx2, by2, COLOR_ARC_GREEN);
        canvas->drawLine(bx1 + 1, by1, bx2 + 1, by2, COLOR_ARC_GREEN);

        // CDI reference dots (at ±1, ±2 dot positions)
        for (int d = -2; d <= 2; d++) {
            if (d == 0) continue;
            float dotOff = d * 8.0f;
            int dx = cx + (int)(dotOff * perpSin);
            int dy = cy - (int)(dotOff * perpCos);
            canvas->fillCircle(dx, dy, 2, COLOR_TICK);
        }

        // Glideslope dots on right side
        int gsX = cx + DIAL_RADIUS - 15;
        for (int d = -2; d <= 2; d++) {
            if (d == 0) continue;
            int dotY = cy + d * 10;
            canvas->fillCircle(gsX, dotY, 2, COLOR_TICK);
        }
        // GS diamond
        int gsY = cy + (int)(gs * 10.0f);
        if (gsY < cy - 25) gsY = cy - 25;
        if (gsY > cy + 25) gsY = cy + 25;
        canvas->fillTriangle(gsX, gsY - 5, gsX - 4, gsY, gsX, gsY + 5, COLOR_ARC_GREEN);
        canvas->fillTriangle(gsX, gsY - 5, gsX + 4, gsY, gsX, gsY + 5, COLOR_ARC_GREEN);

        // Fixed aircraft index
        canvas->fillTriangle(cx, cy - TICK_OUTER_R - 5, cx - 6, cy - TICK_OUTER_R + 8,
                            cx + 6, cy - TICK_OUTER_R + 8, COLOR_ARC_YELLOW);

        // Rim
        canvas->drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);

        // Center dot
        canvas->fillCircle(cx, cy, 3, COLOR_HUB);

        // Heading readout
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.3);
        char buf[8];
        snprintf(buf, sizeof(buf), "%03d", (int)heading % 360);
        canvas->drawString(buf, cx, cy + DIAL_RADIUS - 18);

        return true;
    }
};
