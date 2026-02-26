#pragma once

#include "gauge_base.h"
#include "../config.h"

class ThrottleGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {0.0f,  0.25f, COLOR_ARC_WHITE  },   // Idle range
            {0.25f, 0.75f, COLOR_ARC_GREEN  },   // Normal operating
            {0.75f, 1.0f,  COLOR_ARC_YELLOW },   // High power
        };

        static const GaugeConfig cfg = {
            .title        = "THROTTLE",
            .units        = "%",
            .dataref      = "sim/cockpit2/engine/actuators/throttle_ratio[0]",
            .minValue     = 0.0f,
            .maxValue     = 1.0f,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 0.2f, .minorInterval = 0.1f, .labelDivisor = 0.01f, .showLabels = true },
            .arcs         = arcs,
            .arcCount     = 3,
            .valueMultiplier = 100.0f,   // Display 0-1 as 0-100%
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = false,
        };
        return cfg;
    }
};
