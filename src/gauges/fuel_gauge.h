#pragma once

#include "gauge_base.h"
#include "../config.h"

class FuelGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            { 0,  8, COLOR_ARC_RED    },   // Critical low
            { 8, 13, COLOR_ARC_YELLOW },   // Low fuel warning
            {13, 50, COLOR_ARC_GREEN  },   // Normal
        };

        static const GaugeConfig cfg = {
            .title        = "FUEL L",
            .units        = "GAL",
            .dataref      = "sim/cockpit2/fuel/fuel_level_indicated_left",
            .minValue     = 0,
            .maxValue     = 50,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 10, .minorInterval = 5, .labelDivisor = 1, .showLabels = true },
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
