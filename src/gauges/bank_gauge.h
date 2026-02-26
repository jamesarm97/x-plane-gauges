#pragma once

#include "gauge_base.h"
#include <M5Dial.h>
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

    bool customRender(void* spritePtr, float roll) override {
        M5Canvas* canvas = static_cast<M5Canvas*>(spritePtr);

        // Clamp roll for display
        if (roll > 90.0f) roll = 90.0f;
        if (roll < -90.0f) roll = -90.0f;

        float rad = roll * DEG_TO_RAD;
        float sinR = sinf(rad);
        float cosR = cosf(rad);

        // Background - dark
        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_BG);

        // Draw sky/ground split rotated by bank angle
        // Ground is brown (0x8A22), sky is blue (0x2B5F)
        // Fill sky first, then draw ground polygon
        uint16_t skyColor = 0x2B5F;
        uint16_t gndColor = 0x8A22;

        canvas->fillCircle(CENTER_X, CENTER_Y, DIAL_RADIUS - 2, skyColor);

        // Ground: a large rotated rectangle below the horizon line
        // Horizon passes through center, rotated by roll angle
        // We draw ground as a filled polygon below the tilted horizon
        int hw = DIAL_RADIUS + 20;  // Half-width of ground rectangle (oversized to cover)
        // Four corners of a large rectangle below horizon, rotated
        int gx[4], gy[4];
        // Top-left and top-right of ground (on the horizon line)
        gx[0] = CENTER_X + (int)(-hw * cosR);
        gy[0] = CENTER_Y + (int)(-hw * sinR);
        gx[1] = CENTER_X + (int)(hw * cosR);
        gy[1] = CENTER_Y + (int)(hw * sinR);
        // Bottom-right and bottom-left (far below horizon)
        gx[2] = CENTER_X + (int)(hw * cosR + hw * sinR);
        gy[2] = CENTER_Y + (int)(hw * sinR - hw * cosR);
        gx[3] = CENTER_X + (int)(-hw * cosR + hw * sinR);
        gy[3] = CENTER_Y + (int)(-hw * sinR - hw * cosR);

        // Draw ground as two triangles
        canvas->fillTriangle(gx[0], gy[0], gx[1], gy[1], gx[2], gy[2], gndColor);
        canvas->fillTriangle(gx[0], gy[0], gx[2], gy[2], gx[3], gy[3], gndColor);

        // Re-clip to circle (draw rim ring to mask corners)
        for (int r = DIAL_RADIUS - 1; r <= DIAL_RADIUS + 20; r++) {
            canvas->drawCircle(CENTER_X, CENTER_Y, r, COLOR_BG);
        }
        canvas->drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Horizon line (white)
        int hx1 = CENTER_X + (int)(-DIAL_RADIUS * cosR);
        int hy1 = CENTER_Y + (int)(-DIAL_RADIUS * sinR);
        int hx2 = CENTER_X + (int)(DIAL_RADIUS * cosR);
        int hy2 = CENTER_Y + (int)(DIAL_RADIUS * sinR);
        canvas->drawLine(hx1, hy1, hx2, hy2, COLOR_TICK);
        canvas->drawLine(hx1, hy1 + 1, hx2, hy2 + 1, COLOR_TICK);

        // Bank angle tick marks at top of dial (fixed position)
        for (int deg = -60; deg <= 60; deg += 10) {
            bool isMajor = (deg % 30 == 0);
            float tickRad = ((float)deg - 90.0f) * DEG_TO_RAD;  // -90 to put 0 at top
            float sa = sinf(tickRad);
            float ca = cosf(tickRad);

            int innerR = isMajor ? (DIAL_RADIUS - 15) : (DIAL_RADIUS - 8);
            int x1 = CENTER_X + (int)(innerR * ca);
            int y1 = CENTER_Y + (int)(innerR * sa);
            int x2 = CENTER_X + (int)((DIAL_RADIUS - 2) * ca);
            int y2 = CENTER_Y + (int)((DIAL_RADIUS - 2) * sa);

            canvas->drawLine(x1, y1, x2, y2, COLOR_TICK);
        }

        // Fixed aircraft wings symbol (center)
        int wingLen = 35;
        int wingY = CENTER_Y;
        // Left wing
        canvas->fillRect(CENTER_X - wingLen - 5, wingY - 2, wingLen, 4, COLOR_ARC_YELLOW);
        // Right wing
        canvas->fillRect(CENTER_X + 5, wingY - 2, wingLen, 4, COLOR_ARC_YELLOW);
        // Center dot
        canvas->fillCircle(CENTER_X, CENTER_Y, 4, COLOR_ARC_YELLOW);

        // Bank pointer triangle at top (rotates with bank)
        float ptrRad = (-roll - 90.0f) * DEG_TO_RAD;
        float pSin = sinf(ptrRad);
        float pCos = cosf(ptrRad);
        int pTipR = DIAL_RADIUS - 18;
        int pBaseR = DIAL_RADIUS - 8;
        int px = CENTER_X + (int)(pTipR * pCos);
        int py = CENTER_Y + (int)(pTipR * pSin);
        // Base points offset perpendicular
        float perpRad = ptrRad + 1.5708f;  // +90 degrees
        int bx1 = CENTER_X + (int)(pBaseR * pCos + 5 * sinf(perpRad));
        int by1 = CENTER_Y + (int)(pBaseR * pSin - 5 * cosf(perpRad));
        int bx2 = CENTER_X + (int)(pBaseR * pCos - 5 * sinf(perpRad));
        int by2 = CENTER_Y + (int)(pBaseR * pSin + 5 * cosf(perpRad));
        canvas->fillTriangle(px, py, bx1, by1, bx2, by2, COLOR_ARC_YELLOW);

        // Value readout
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.3);
        char buf[8];
        snprintf(buf, sizeof(buf), "%.0f", roll);
        canvas->drawString(buf, CENTER_X, CENTER_Y + 45);

        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.0);
        canvas->drawString("BANK", CENTER_X, CENTER_Y + 62);

        return true;
    }
};
