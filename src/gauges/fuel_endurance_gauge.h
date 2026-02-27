#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

class FuelEnduranceGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "ENDURANCE",
            .units        = "",
            .dataref      = "sim/flightmodel/weight/m_fuel[0]",
            .minValue     = 0,
            .maxValue     = 200,
            .startAngle   = 0,
            .endAngle     = 360,
            .ticks        = { .majorInterval = 50, .minorInterval = 10, .labelDivisor = 1, .showLabels = false },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = true,
            .extraDatarefs = { "sim/flightmodel/engine/ENGN_FF_[0]" },
            .extraDatarefCount = 1,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);

        // fuel quantity in kg, fuel flow in kg/s
        float fuelKg = values[0];
        float fuelFlowKgS = (valueCount > 1) ? values[1] : 0.0f;

        // Convert: kg → lbs → gallons (avgas ~6 lbs/gal)
        float fuelLbs = fuelKg * 2.20462f;
        float fuelGal = fuelLbs / 6.0f;

        // Flow: kg/s → gal/hr
        float flowGPH = fuelFlowKgS * 2.20462f / 6.0f * 3600.0f;

        // Endurance calculation
        float enduranceHrs = 0.0f;
        if (flowGPH > 0.1f) {
            enduranceHrs = fuelGal / flowGPH;
        }
        int hours = (int)enduranceHrs;
        int minutes = (int)((enduranceHrs - hours) * 60.0f);
        float enduranceMin = enduranceHrs * 60.0f;

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_BG);
        canvas->drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(cx, cy, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Title
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.3);
        canvas->drawString("ENDURANCE", cx, cy - 70);

        // Time remaining — large display
        uint16_t timeColor;
        if (enduranceMin < 30.0f && flowGPH > 0.1f) {
            timeColor = COLOR_ARC_RED;
        } else if (enduranceMin < 60.0f && flowGPH > 0.1f) {
            timeColor = COLOR_ARC_YELLOW;
        } else {
            timeColor = COLOR_VALUE;
        }

        canvas->setTextColor(timeColor);
        canvas->setTextSize(3.0);
        char timeBuf[12];
        if (flowGPH < 0.1f) {
            canvas->drawString("--:--", cx, cy - 18);
        } else {
            snprintf(timeBuf, sizeof(timeBuf), "%d:%02d", hours, minutes);
            canvas->drawString(timeBuf, cx, cy - 18);
        }

        // Fuel quantity
        canvas->setTextColor(COLOR_LABEL);
        canvas->setTextSize(1.2);
        canvas->drawString("FUEL", cx - 40, cy + 22);
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.4);
        char buf[12];
        snprintf(buf, sizeof(buf), "%.1f", fuelGal);
        canvas->drawString(buf, cx - 40, cy + 42);
        canvas->setTextColor(COLOR_LABEL);
        canvas->setTextSize(1.0);
        canvas->drawString("GAL", cx - 40, cy + 62);

        // Fuel flow
        canvas->setTextColor(COLOR_LABEL);
        canvas->setTextSize(1.2);
        canvas->drawString("FLOW", cx + 40, cy + 22);
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.4);
        snprintf(buf, sizeof(buf), "%.1f", flowGPH);
        canvas->drawString(buf, cx + 40, cy + 42);
        canvas->setTextColor(COLOR_LABEL);
        canvas->setTextSize(1.0);
        canvas->drawString("GPH", cx + 40, cy + 62);

        return true;
    }
};
