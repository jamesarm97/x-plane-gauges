#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

class TrimGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "TRIM",
            .units        = "",
            .dataref      = "sim/cockpit2/controls/elevator_trim",
            .minValue     = -1,
            .maxValue     = 1,
            .startAngle   = 0,
            .endAngle     = 360,
            .ticks        = { .majorInterval = 0.5f, .minorInterval = 0.25f, .labelDivisor = 1, .showLabels = false },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = true,
            .extraDatarefs = {
                "sim/cockpit2/controls/aileron_trim",
                "sim/cockpit2/controls/rudder_trim",
            },
            .extraDatarefCount = 2,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);
        float elevator = values[0];                            // -1 to +1
        float aileron  = (valueCount > 1) ? values[1] : 0.0f;
        float rudder   = (valueCount > 2) ? values[2] : 0.0f;

        canvas->fillSprite(COLOR_BG);

        // Title
        canvas->setTextDatum(top_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.2);
        canvas->drawString("TRIM", cx, 2);

        int w = cx * 2;  // cell width
        int h = cy * 2;  // cell height

        // ── Elevator: tall vertical bar on left side ──
        {
            int barX = 20;
            int barW = 30;
            int barY = 30;
            int barH = h - 50;
            int centerY = barY + barH / 2;

            canvas->fillRect(barX, barY, barW, barH, COLOR_DIAL_BG);
            canvas->drawRect(barX, barY, barW, barH, COLOR_DIAL_RIM);

            // Green center zone
            int greenH = barH / 6;
            canvas->fillRect(barX + 1, centerY - greenH / 2, barW - 2, greenH, 0x0320);

            // Center mark
            canvas->drawFastHLine(barX, centerY, barW, COLOR_TICK);

            // Indicator position
            int indY = centerY - (int)(elevator * barH / 2);
            if (indY < barY + 3) indY = barY + 3;
            if (indY > barY + barH - 3) indY = barY + barH - 3;
            canvas->fillRect(barX + 2, indY - 3, barW - 4, 6, COLOR_ARC_YELLOW);

            // Labels
            canvas->setTextDatum(middle_center);
            canvas->setTextColor(COLOR_LABEL);
            canvas->setTextSize(0.8);
            canvas->drawString("ELEV", barX + barW / 2, barY - 8);
            canvas->drawString("ND", barX + barW / 2, barY + 10);
            canvas->drawString("NU", barX + barW / 2, barY + barH - 10);
        }

        // ── Aileron: horizontal bar upper right ──
        {
            int barX = 70;
            int barW = w - 90;
            int barY = 45;
            int barH = 30;
            int centerX = barX + barW / 2;

            canvas->fillRect(barX, barY, barW, barH, COLOR_DIAL_BG);
            canvas->drawRect(barX, barY, barW, barH, COLOR_DIAL_RIM);

            int greenW = barW / 6;
            canvas->fillRect(centerX - greenW / 2, barY + 1, greenW, barH - 2, 0x0320);

            canvas->drawFastVLine(centerX, barY, barH, COLOR_TICK);

            int indX = centerX + (int)(aileron * barW / 2);
            if (indX < barX + 3) indX = barX + 3;
            if (indX > barX + barW - 3) indX = barX + barW - 3;
            canvas->fillRect(indX - 3, barY + 2, 6, barH - 4, COLOR_ARC_YELLOW);

            canvas->setTextDatum(middle_center);
            canvas->setTextColor(COLOR_LABEL);
            canvas->setTextSize(0.8);
            canvas->drawString("AIL", barX + barW / 2, barY - 8);
            canvas->drawString("L", barX + 6, barY + barH / 2);
            canvas->drawString("R", barX + barW - 6, barY + barH / 2);
        }

        // ── Rudder: horizontal bar lower right ──
        {
            int barX = 70;
            int barW = w - 90;
            int barY = h - 75;
            int barH = 30;
            int centerX = barX + barW / 2;

            canvas->fillRect(barX, barY, barW, barH, COLOR_DIAL_BG);
            canvas->drawRect(barX, barY, barW, barH, COLOR_DIAL_RIM);

            int greenW = barW / 6;
            canvas->fillRect(centerX - greenW / 2, barY + 1, greenW, barH - 2, 0x0320);

            canvas->drawFastVLine(centerX, barY, barH, COLOR_TICK);

            int indX = centerX + (int)(rudder * barW / 2);
            if (indX < barX + 3) indX = barX + 3;
            if (indX > barX + barW - 3) indX = barX + barW - 3;
            canvas->fillRect(indX - 3, barY + 2, 6, barH - 4, COLOR_ARC_YELLOW);

            canvas->setTextDatum(middle_center);
            canvas->setTextColor(COLOR_LABEL);
            canvas->setTextSize(0.8);
            canvas->drawString("RUD", barX + barW / 2, barY - 8);
            canvas->drawString("L", barX + 6, barY + barH / 2);
            canvas->drawString("R", barX + barW - 6, barY + barH / 2);
        }

        // ── Numeric readouts in center area ──
        {
            int textX = 70 + (w - 90) / 2;
            int textY = 100;

            canvas->setTextDatum(middle_center);
            canvas->setTextColor(COLOR_VALUE);
            canvas->setTextSize(1.1);

            char buf[12];
            snprintf(buf, sizeof(buf), "E %+.0f%%", elevator * 100);
            canvas->drawString(buf, textX, textY);

            snprintf(buf, sizeof(buf), "A %+.0f%%", aileron * 100);
            canvas->drawString(buf, textX, textY + 25);

            snprintf(buf, sizeof(buf), "R %+.0f%%", rudder * 100);
            canvas->drawString(buf, textX, textY + 50);
        }

        return true;
    }
};
