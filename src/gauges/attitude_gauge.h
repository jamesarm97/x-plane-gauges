#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

class AttitudeGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "ATTITUDE",
            .units        = "",
            .dataref      = "sim/cockpit2/gauges/indicators/pitch_AHARS_deg_pilot",
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
            .extraDatarefs = { "sim/cockpit2/gauges/indicators/roll_AHARS_deg_pilot" },
            .extraDatarefCount = 1,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);
        float pitch = values[0];
        float roll = (valueCount > 1) ? values[1] : 0.0f;

        // Clamp
        if (pitch > 90.0f) pitch = 90.0f;
        if (pitch < -90.0f) pitch = -90.0f;
        if (roll > 90.0f) roll = 90.0f;
        if (roll < -90.0f) roll = -90.0f;

        uint16_t skyColor = 0x2B5F;   // Blue
        uint16_t gndColor = 0x8A22;   // Brown

        float rad = roll * DEG_TO_RAD;
        float sinR = sinf(rad);
        float cosR = cosf(rad);

        // Pitch offset: pixels per degree
        float pitchPx = pitch * 2.0f;

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(cx, cy, DIAL_RADIUS, skyColor);

        // Ground fill — shifted by pitch, rotated by roll
        int hw = DIAL_RADIUS + 30;
        // Horizon center offset by pitch
        float hcx = cx - pitchPx * sinR;
        float hcy = cy + pitchPx * cosR;

        int gx[4], gy[4];
        gx[0] = (int)(hcx - hw * cosR);
        gy[0] = (int)(hcy - hw * sinR);
        gx[1] = (int)(hcx + hw * cosR);
        gy[1] = (int)(hcy + hw * sinR);
        gx[2] = (int)(hcx + hw * cosR + hw * sinR);
        gy[2] = (int)(hcy + hw * sinR - hw * cosR);
        gx[3] = (int)(hcx - hw * cosR + hw * sinR);
        gy[3] = (int)(hcy - hw * sinR - hw * cosR);

        canvas->fillTriangle(gx[0], gy[0], gx[1], gy[1], gx[2], gy[2], gndColor);
        canvas->fillTriangle(gx[0], gy[0], gx[2], gy[2], gx[3], gy[3], gndColor);

        // Horizon line
        int hx1 = (int)(hcx - DIAL_RADIUS * cosR);
        int hy1 = (int)(hcy - DIAL_RADIUS * sinR);
        int hx2 = (int)(hcx + DIAL_RADIUS * cosR);
        int hy2 = (int)(hcy + DIAL_RADIUS * sinR);
        canvas->drawLine(hx1, hy1, hx2, hy2, COLOR_TICK);
        canvas->drawLine(hx1, hy1 + 1, hx2, hy2 + 1, COLOR_TICK);

        // Pitch ladder lines at 5° intervals
        for (int deg = -20; deg <= 20; deg += 5) {
            if (deg == 0) continue;
            float ladderPx = (pitch - deg) * 2.0f;
            float lhcx = cx - ladderPx * sinR;
            float lhcy = cy + ladderPx * cosR;

            int halfW = (deg % 10 == 0) ? 25 : 12;
            int lx1 = (int)(lhcx - halfW * cosR);
            int ly1 = (int)(lhcy - halfW * sinR);
            int lx2 = (int)(lhcx + halfW * cosR);
            int ly2 = (int)(lhcy + halfW * sinR);
            canvas->drawLine(lx1, ly1, lx2, ly2, COLOR_TICK);

            // Labels for 10° intervals
            if (deg % 10 == 0) {
                canvas->setTextDatum(middle_center);
                canvas->setTextColor(COLOR_TICK);
                canvas->setTextSize(0.8);
                char buf[8];
                snprintf(buf, sizeof(buf), "%d", abs(deg));
                canvas->drawString(buf, lx2 + (int)(10 * cosR), ly2 + (int)(10 * sinR));
            }
        }

        // Clip to circle
        for (int r = DIAL_RADIUS - 1; r <= DIAL_RADIUS + 30; r++) {
            canvas->drawCircle(cx, cy, r, COLOR_BG);
        }
        canvas->drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(cx, cy, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Roll arc with tick marks at top
        for (int deg = -60; deg <= 60; deg += 10) {
            bool isMajor = (deg % 30 == 0);
            float tickRad = ((float)deg - 90.0f) * DEG_TO_RAD;
            float sa = sinf(tickRad);
            float ca = cosf(tickRad);
            int innerR = isMajor ? (DIAL_RADIUS - 15) : (DIAL_RADIUS - 8);
            int x1 = cx + (int)(innerR * ca);
            int y1 = cy + (int)(innerR * sa);
            int x2 = cx + (int)((DIAL_RADIUS - 2) * ca);
            int y2 = cy + (int)((DIAL_RADIUS - 2) * sa);
            canvas->drawLine(x1, y1, x2, y2, COLOR_TICK);
        }

        // Roll pointer (rotates with roll)
        float ptrRad = (-roll - 90.0f) * DEG_TO_RAD;
        int pTipR = DIAL_RADIUS - 18;
        int pBaseR = DIAL_RADIUS - 8;
        float pSin = sinf(ptrRad);
        float pCos = cosf(ptrRad);
        int px = cx + (int)(pTipR * pCos);
        int py = cy + (int)(pTipR * pSin);
        float perpRad = ptrRad + 1.5708f;
        int bx1 = cx + (int)(pBaseR * pCos + 5 * sinf(perpRad));
        int by1 = cy + (int)(pBaseR * pSin - 5 * cosf(perpRad));
        int bx2 = cx + (int)(pBaseR * pCos - 5 * sinf(perpRad));
        int by2 = cy + (int)(pBaseR * pSin + 5 * cosf(perpRad));
        canvas->fillTriangle(px, py, bx1, by1, bx2, by2, COLOR_ARC_YELLOW);

        // Fixed aircraft wings symbol
        int wingLen = 30;
        canvas->fillRect(cx - wingLen - 5, cy - 2, wingLen, 4, COLOR_ARC_YELLOW);
        canvas->fillRect(cx + 5, cy - 2, wingLen, 4, COLOR_ARC_YELLOW);
        canvas->fillCircle(cx, cy, 4, COLOR_ARC_YELLOW);

        // Numeric readout
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.0);
        char buf[16];
        snprintf(buf, sizeof(buf), "P%+.0f R%+.0f", pitch, roll);
        canvas->drawString(buf, cx, cy + DIAL_RADIUS - 20);

        return true;
    }
};
