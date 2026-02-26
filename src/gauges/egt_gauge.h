#pragma once

#include "gauge_base.h"
#include "../config.h"

class EGTGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {  0,  500, COLOR_ARC_WHITE  },   // Cold
            {500,  800, COLOR_ARC_GREEN  },   // Normal
            {800,  900, COLOR_ARC_YELLOW },   // Caution
            {900, 1000, COLOR_ARC_RED    },   // Danger
        };

        static const GaugeConfig cfg = {
            .title        = "EGT",
            .units        = "DEG C",
            .dataref      = "sim/cockpit2/engine/indicators/EGT_deg_C[0]",
            .minValue     = 0,
            .maxValue     = 1000,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 200, .minorInterval = 50, .labelDivisor = 1, .showLabels = true },
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
