#pragma once

#include "gauge_base.h"
#include "../config.h"

class FuelFlowGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            { 0,  8, COLOR_ARC_GREEN  },   // Normal cruise
            { 8, 15, COLOR_ARC_YELLOW },   // High consumption
            {15, 25, COLOR_ARC_RED    },   // Maximum
        };

        static const GaugeConfig cfg = {
            .title        = "FUEL FLOW",
            .units        = "GPH",
            .dataref      = "sim/cockpit2/engine/indicators/fuel_flow_kg_sec[0]",
            .minValue     = 0,
            .maxValue     = 25,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 5, .minorInterval = 1, .labelDivisor = 1, .showLabels = true },
            .arcs         = arcs,
            .arcCount     = 3,
            .valueMultiplier = 792.5f,   // kg/sec to GPH (x3600/4.536 ≈ 792.5 for avgas)
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = false,
        };
        return cfg;
    }
};
