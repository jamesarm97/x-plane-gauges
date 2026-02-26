#pragma once

#include "gauge_base.h"
#include "../config.h"

class AirspeedGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {  0,  40, COLOR_ARC_WHITE  },   // Vs0 flap range low
            { 40,  85, COLOR_ARC_GREEN  },   // Normal operating
            { 85, 130, COLOR_ARC_YELLOW },   // Caution
            {130, 165, COLOR_ARC_RED    },   // Never exceed approach
        };

        static const GaugeConfig cfg = {
            .title       = "AIRSPEED",
            .units        = "KTS",
            .dataref      = "sim/cockpit2/gauges/indicators/airspeed_kts_pilot",
            .minValue     = 0,
            .maxValue     = 200,
            .startAngle   = 220,    // ~7 o'clock
            .endAngle     = 500,    // ~5 o'clock (past 360 for >270 sweep)
            .ticks        = { .majorInterval = 20, .minorInterval = 10, .labelDivisor = 1, .showLabels = true },
            .arcs         = arcs,
            .arcCount     = 4,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = false,
        };
        return cfg;
    }
};
