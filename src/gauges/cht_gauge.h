#pragma once

#include "gauge_base.h"
#include "../config.h"

class CHTGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {  0, 100, COLOR_ARC_WHITE  },   // Cold
            {100, 230, COLOR_ARC_GREEN  },   // Normal
            {230, 260, COLOR_ARC_YELLOW },   // Caution
            {260, 300, COLOR_ARC_RED    },   // Danger
        };

        static const GaugeConfig cfg = {
            .title        = "CHT",
            .units        = "DEG C",
            .dataref      = "sim/cockpit2/engine/indicators/CHT_deg_C[0]",
            .minValue     = 0,
            .maxValue     = 300,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 50, .minorInterval = 25, .labelDivisor = 1, .showLabels = true },
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
