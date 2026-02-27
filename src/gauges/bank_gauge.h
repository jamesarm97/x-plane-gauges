#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

class BankGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "BANK",
            .units        = "DEG",
            .dataref      = "sim/cockpit2/gauges/indicators/roll_AHARS_deg_pilot",
            .minValue     = -60,
            .maxValue     = 60,
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

    bool customRender(void* spritePtr, float roll, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);

        // Clamp roll for display
        if (roll > 90.0f) roll = 90.0f;
        if (roll < -90.0f) roll = -90.0f;

        float rad = roll * DEG_TO_RAD;
        float sinR = sinf(rad);
        float cosR = cosf(rad);

        // Background - dark
        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_BG);

        // Draw sky/ground split rotated by bank angle
        uint16_t skyColor = 0x2B5F;
        uint16_t gndColor = 0x8A22;

        canvas->fillCircle(cx, cy, DIAL_RADIUS - 2, skyColor);

        // Ground: a large rotated rectangle below the horizon line
        int hw = DIAL_RADIUS + 20;
        int gx[4], gy[4];
        gx[0] = cx + (int)(-hw * cosR);
        gy[0] = cy + (int)(-hw * sinR);
        gx[1] = cx + (int)(hw * cosR);
        gy[1] = cy + (int)(hw * sinR);
        gx[2] = cx + (int)(hw * cosR + hw * sinR);
        gy[2] = cy + (int)(hw * sinR - hw * cosR);
        gx[3] = cx + (int)(-hw * cosR + hw * sinR);
        gy[3] = cy + (int)(-hw * sinR - hw * cosR);

        canvas->fillTriangle(gx[0], gy[0], gx[1], gy[1], gx[2], gy[2], gndColor);
        canvas->fillTriangle(gx[0], gy[0], gx[2], gy[2], gx[3], gy[3], gndColor);

        // Re-clip to circle (draw rim ring to mask corners)
        for (int r = DIAL_RADIUS - 1; r <= DIAL_RADIUS + 20; r++) {
            canvas->drawCircle(cx, cy, r, COLOR_BG);
        }
        canvas->drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(cx, cy, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Horizon line (white)
        int hx1 = cx + (int)(-DIAL_RADIUS * cosR);
        int hy1 = cy + (int)(-DIAL_RADIUS * sinR);
        int hx2 = cx + (int)(DIAL_RADIUS * cosR);
        int hy2 = cy + (int)(DIAL_RADIUS * sinR);
        canvas->drawLine(hx1, hy1, hx2, hy2, COLOR_TICK);
        canvas->drawLine(hx1, hy1 + 1, hx2, hy2 + 1, COLOR_TICK);

        // Bank angle tick marks at top of dial (fixed position)
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

        // Fixed aircraft wings symbol (center)
        int wingLen = 35;
        int wingY = cy;
        canvas->fillRect(cx - wingLen - 5, wingY - 2, wingLen, 4, COLOR_ARC_YELLOW);
        canvas->fillRect(cx + 5, wingY - 2, wingLen, 4, COLOR_ARC_YELLOW);
        canvas->fillCircle(cx, cy, 4, COLOR_ARC_YELLOW);

        // Bank pointer triangle at top (rotates with bank)
        float ptrRad = (-roll - 90.0f) * DEG_TO_RAD;
        float pSin = sinf(ptrRad);
        float pCos = cosf(ptrRad);
        int pTipR = DIAL_RADIUS - 18;
        int pBaseR = DIAL_RADIUS - 8;
        int px = cx + (int)(pTipR * pCos);
        int py = cy + (int)(pTipR * pSin);
        float perpRad = ptrRad + 1.5708f;
        int bx1 = cx + (int)(pBaseR * pCos + 5 * sinf(perpRad));
        int by1 = cy + (int)(pBaseR * pSin - 5 * cosf(perpRad));
        int bx2 = cx + (int)(pBaseR * pCos - 5 * sinf(perpRad));
        int by2 = cy + (int)(pBaseR * pSin + 5 * cosf(perpRad));
        canvas->fillTriangle(px, py, bx1, by1, bx2, by2, COLOR_ARC_YELLOW);

        // Value readout
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.3);
        char buf[8];
        snprintf(buf, sizeof(buf), "%.0f", roll);
        canvas->drawString(buf, cx, cy + 45);

        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.0);
        canvas->drawString("BANK", cx, cy + 62);

        return true;
    }
};
