#pragma once

#include "gauge_base.h"
#include "../config.h"

class VoltsGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            { 0, 11, COLOR_ARC_RED    },   // Dead / low
            {11, 13, COLOR_ARC_GREEN  },   // Normal (12V system)
            {13, 15, COLOR_ARC_YELLOW },   // High
            {15, 18, COLOR_ARC_RED    },   // Over-voltage
        };

        static const GaugeConfig cfg = {
            .title        = "VOLTS",
            .units        = "V",
            .dataref      = "sim/cockpit2/electrical/bus_volts[0]",
            .minValue     = 0,
            .maxValue     = 18,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 3, .minorInterval = 1, .labelDivisor = 1, .showLabels = true },
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
