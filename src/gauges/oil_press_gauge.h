#pragma once

#include "gauge_base.h"
#include "../config.h"

class OilPressGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {  0,  25, COLOR_ARC_RED    },   // Low pressure danger
            { 25,  60, COLOR_ARC_YELLOW },   // Low
            { 60,  95, COLOR_ARC_GREEN  },   // Normal
            { 95, 120, COLOR_ARC_RED    },   // High pressure danger
        };

        static const GaugeConfig cfg = {
            .title        = "OIL PRESS",
            .units        = "PSI",
            .dataref      = "sim/cockpit2/engine/indicators/oil_pressure_psi[0]",
            .minValue     = 0,
            .maxValue     = 120,
            .startAngle   = 225,
            .endAngle     = 495,
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
