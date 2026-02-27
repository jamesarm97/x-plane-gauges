#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "../config.h"

class AnnunciatorGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "ANNUNCIATOR",
            .units        = "",
            .dataref      = "sim/cockpit2/controls/gear_handle_down",
            .minValue     = 0,
            .maxValue     = 1,
            .startAngle   = 0,
            .endAngle     = 360,
            .ticks        = { .majorInterval = 1, .minorInterval = 1, .labelDivisor = 1, .showLabels = false },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = true,
            .extraDatarefs = {
                "sim/cockpit2/controls/flap_ratio",
                "sim/cockpit2/annunciators/stall_warning",
                "sim/cockpit2/annunciators/speedbrake",
            },
            .extraDatarefCount = 3,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);
        float gear  = values[0];
        float flaps = (valueCount > 1) ? values[1] : 0.0f;
        float stall = (valueCount > 2) ? values[2] : 0.0f;
        float ovspd = (valueCount > 3) ? values[3] : 0.0f;

        canvas->fillSprite(COLOR_BG);

        // Title
        canvas->setTextDatum(top_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.5);
        canvas->drawString("ANNUNCIATOR", cx, 4);

        // 2x2 grid of status boxes
        int boxW = 110;
        int boxH = 85;
        int gap = 8;
        int startX = cx - boxW - gap / 2;
        int startY = 32;

        struct Annunc {
            const char* label;
            float value;
            uint16_t activeColor;
            const char* stateOn;
            const char* stateOff;
        };

        Annunc items[4] = {
            { "GEAR",  gear,  COLOR_ARC_GREEN, "DOWN", "UP" },
            { "FLAPS", flaps, COLOR_ARC_YELLOW, nullptr, "UP" },  // special: show ratio
            { "STALL", stall, COLOR_ARC_RED, "WARN", "OK" },
            { "OVSPD", ovspd, COLOR_ARC_RED, "WARN", "OK" },
        };

        for (int i = 0; i < 4; i++) {
            int col = i % 2;
            int row = i / 2;
            int bx = startX + col * (boxW + gap);
            int by = startY + row * (boxH + gap);

            bool active;
            if (i == 1) {
                active = (flaps > 0.01f);  // Flaps deployed
            } else {
                active = (items[i].value > 0.5f);
            }

            uint16_t bgCol = active ? items[i].activeColor : COLOR_DIAL_BG;
            uint16_t textCol = active ? COLOR_BG : COLOR_TICK;

            canvas->fillRoundRect(bx, by, boxW, boxH, 6, bgCol);
            canvas->drawRoundRect(bx, by, boxW, boxH, 6, COLOR_DIAL_RIM);

            // Label
            canvas->setTextDatum(middle_center);
            canvas->setTextColor(textCol);
            canvas->setTextSize(1.5);
            canvas->drawString(items[i].label, bx + boxW / 2, by + 28);

            // State text
            canvas->setTextSize(1.3);
            if (i == 1) {
                // Flaps: show percentage
                char buf[8];
                snprintf(buf, sizeof(buf), "%d%%", (int)(flaps * 100));
                canvas->drawString(buf, bx + boxW / 2, by + 55);
            } else {
                const char* state = active ? items[i].stateOn : items[i].stateOff;
                canvas->drawString(state, bx + boxW / 2, by + 55);
            }
        }

        return true;
    }
};
