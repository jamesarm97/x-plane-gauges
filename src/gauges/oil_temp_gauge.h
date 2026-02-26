#pragma once

#include "gauge_base.h"
#include "../config.h"

class OilTempGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {  0,  50, COLOR_ARC_YELLOW },   // Cold
            { 50, 110, COLOR_ARC_GREEN  },   // Normal
            {110, 150, COLOR_ARC_RED    },   // Hot
        };

        static const GaugeConfig cfg = {
            .title        = "OIL TEMP",
            .units        = "DEG C",
            .dataref      = "sim/cockpit2/engine/indicators/oil_temperature_deg_C[0]",
            .minValue     = 0,
            .maxValue     = 150,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 25, .minorInterval = 5, .labelDivisor = 1, .showLabels = true },
            .arcs         = arcs,
            .arcCount     = 3,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = false,
        };
        return cfg;
    }
};
