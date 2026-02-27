#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cstdio>
#include "../config.h"

class EngineClusterGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "ENGINE",
            .units        = "",
            .dataref      = "sim/flightmodel/engine/ENGN_tacrad[0]",
            .minValue     = 0,
            .maxValue     = 3000,
            .startAngle   = 0,
            .endAngle     = 360,
            .ticks        = { .majorInterval = 500, .minorInterval = 100, .labelDivisor = 1, .showLabels = false },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = true,
            .extraDatarefs = {
                "sim/flightmodel/engine/ENGN_MPR[0]",
                "sim/flightmodel/engine/ENGN_oil_temp_[0]",
                "sim/flightmodel/engine/ENGN_oil_press_[0]",
                "sim/flightmodel/engine/ENGN_egt[0]",
                "sim/flightmodel/engine/ENGN_FF_[0]",
            },
            .extraDatarefCount = 5,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);

        // RPM from tacrad: rad/s → RPM = val * 60 / (2π)
        float rpm      = values[0] * 9.5493f;
        float manifold = (valueCount > 1) ? values[1] : 0.0f;
        float oilTemp  = (valueCount > 2) ? values[2] : 0.0f;
        float oilPress = (valueCount > 3) ? values[3] : 0.0f;
        float egt      = (valueCount > 4) ? values[4] : 0.0f;
        float fuelFlow = (valueCount > 5) ? values[5] : 0.0f;

        canvas->fillSprite(COLOR_BG);

        // Title
        canvas->setTextDatum(top_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.5);
        canvas->drawString("ENGINE", cx, 2);

        struct BarParam {
            const char* label;
            float value;
            float minVal;
            float maxVal;
            float greenLo;
            float greenHi;
            float yellowHi;
            const char* fmt;
        };

        BarParam params[6] = {
            { "RPM",  rpm,      0, 3000, 500, 2500, 2700, "%.0f" },
            { "MAP",  manifold, 0, 40,   10,  30,   35,   "%.1f" },
            { "OilT", oilTemp,  0, 130,  50,  100,  110,  "%.0f" },
            { "OilP", oilPress, 0, 120,  25,  80,   95,   "%.0f" },
            { "EGT",  egt,      0, 1000, 200, 800,  900,  "%.0f" },
            { "FF",   fuelFlow, 0, 25,   0,   20,   22,   "%.1f" },
        };

        int barX = 55;
        int barW = cx * 2 - 120;
        int barH = 26;
        int startY = 28;
        int rowH = 35;

        for (int i = 0; i < 6; i++) {
            int by = startY + i * rowH;

            // Label
            canvas->setTextDatum(middle_left);
            canvas->setTextColor(COLOR_LABEL);
            canvas->setTextSize(1.2);
            canvas->drawString(params[i].label, 4, by + barH / 2);

            // Bar background
            canvas->fillRect(barX, by, barW, barH, COLOR_DIAL_BG);
            canvas->drawRect(barX, by, barW, barH, COLOR_DIAL_RIM);

            // Fill bar
            float range = params[i].maxVal - params[i].minVal;
            float frac = (params[i].value - params[i].minVal) / range;
            if (frac < 0) frac = 0;
            if (frac > 1) frac = 1;
            int fillW = (int)(frac * barW);

            uint16_t barColor;
            if (params[i].value < params[i].greenLo || params[i].value > params[i].yellowHi) {
                barColor = COLOR_ARC_RED;
            } else if (params[i].value > params[i].greenHi) {
                barColor = COLOR_ARC_YELLOW;
            } else {
                barColor = COLOR_ARC_GREEN;
            }
            canvas->fillRect(barX + 1, by + 1, fillW - 2, barH - 2, barColor);

            // Value text
            canvas->setTextDatum(middle_right);
            canvas->setTextColor(COLOR_VALUE);
            canvas->setTextSize(1.2);
            char buf[12];
            snprintf(buf, sizeof(buf), params[i].fmt, params[i].value);
            canvas->drawString(buf, cx * 2 - 4, by + barH / 2);
        }

        return true;
    }
};
