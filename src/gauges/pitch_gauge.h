#pragma once

#include "gauge_base.h"
#include "../config.h"

class PitchGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const ArcSegment arcs[] = {
            {-30, -10, COLOR_ARC_RED    },   // Steep dive
            {-10,   0, COLOR_ARC_YELLOW },   // Descent
            {  0,  10, COLOR_ARC_GREEN  },   // Normal flight
            { 10,  20, COLOR_ARC_YELLOW },   // Climb
            { 20,  30, COLOR_ARC_RED    },   // Steep climb / stall risk
        };

        static const GaugeConfig cfg = {
            .title        = "PITCH",
            .units        = "DEG",
            .dataref      = "sim/cockpit2/gauges/indicators/pitch_AHARS_deg_pilot",
            .minValue     = -30,
            .maxValue     = 30,
            .startAngle   = 225,
            .endAngle     = 495,
            .ticks        = { .majorInterval = 10, .minorInterval = 5, .labelDivisor = 1, .showLabels = true },
            .arcs         = arcs,
            .arcCount     = 5,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = false,
        };
        return cfg;
    }
};
