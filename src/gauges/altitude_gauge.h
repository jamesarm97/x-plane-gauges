#pragma once

#include "gauge_base.h"
#include "../config.h"

class AltitudeGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "ALTITUDE",
            .units        = "FT x1000",
            .dataref      = "sim/cockpit2/gauges/indicators/altitude_ft_pilot",
            .minValue     = 0,
            .maxValue     = 1000,       // Needle wraps every 1000 ft
            .startAngle   = 0,          // 12 o'clock = 0
            .endAngle     = 360,        // Full rotation
            .ticks        = { .majorInterval = 100, .minorInterval = 20, .labelDivisor = 100, .showLabels = true },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = true,
            .wrapModulo   = 1000,       // Wrap every 1000 ft
            .customRenderer = false,
        };
        return cfg;
    }
};
