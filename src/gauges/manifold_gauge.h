#pragma once

#include "gauge_base.h"
#include "../config.h"

class ManifoldGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {10, 25, COLOR_ARC_GREEN  },   // Normal operating
            {25, 35, COLOR_ARC_YELLOW },   // Caution
            {35, 40, COLOR_ARC_RED    },   // Over-boost
        };

        static const GaugeConfig cfg = {
            .title        = "MAN PRESS",
            .units        = "inHg",
            .dataref      = "sim/cockpit2/engine/indicators/manifold_pressure_inhg[0]",
            .minValue     = 10,
            .maxValue     = 40,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 5, .minorInterval = 1, .labelDivisor = 1, .showLabels = true },
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
