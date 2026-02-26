#pragma once

#include "gauge_base.h"
#include "../config.h"

class RPMGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {   0, 2000, COLOR_ARC_GREEN  },   // Normal
            {2000, 2500, COLOR_ARC_YELLOW },   // Caution
            {2500, 2800, COLOR_ARC_RED    },   // Redline
        };

        static const GaugeConfig cfg = {
            .title        = "RPM",
            .units        = "RPM",
            .dataref      = "sim/cockpit2/engine/indicators/engine_speed_rpm[0]",
            .minValue     = 0,
            .maxValue     = 2800,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 400, .minorInterval = 100, .labelDivisor = 1, .showLabels = true },
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
