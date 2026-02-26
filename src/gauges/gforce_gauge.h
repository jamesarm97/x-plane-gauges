#pragma once

#include "gauge_base.h"
#include "../config.h"

class GForceGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {-2.0f,  0.0f, COLOR_ARC_RED    },   // Negative G
            { 0.0f,  1.5f, COLOR_ARC_GREEN  },   // Normal
            { 1.5f,  3.0f, COLOR_ARC_YELLOW },   // Caution
            { 3.0f,  5.0f, COLOR_ARC_RED    },   // Danger
        };

        static const GaugeConfig cfg = {
            .title        = "G-FORCE",
            .units        = "G",
            .dataref      = "sim/flightmodel2/misc/gforce_normal",
            .minValue     = -2.0f,
            .maxValue     = 5.0f,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 1.0f, .minorInterval = 0.5f, .labelDivisor = 1, .showLabels = true },
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
