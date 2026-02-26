#pragma once

#include "gauge_base.h"
#include "../config.h"

class FlapGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {0.0f, 0.25f, COLOR_ARC_GREEN  },   // Takeoff
            {0.25f, 0.75f, COLOR_ARC_WHITE },   // Approach
            {0.75f, 1.0f,  COLOR_ARC_YELLOW },  // Full
        };

        static const GaugeConfig cfg = {
            .title        = "FLAPS",
            .units        = "%",
            .dataref      = "sim/cockpit2/controls/flap_ratio",
            .minValue     = 0.0f,
            .maxValue     = 1.0f,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 0.25f, .minorInterval = 0.125f, .labelDivisor = 0.01f, .showLabels = true },
            .arcs         = arcs,
            .arcCount     = 3,
            .valueMultiplier = 100.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = false,
        };
        return cfg;
    }
};
