#pragma once

#include "gauge_base.h"
#include "../config.h"

class VSIGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "VERT SPEED",
            .units        = "FPM",
            .dataref      = "sim/cockpit2/gauges/indicators/vvi_fpm_pilot",
            .minValue     = -2000,
            .maxValue     = 2000,
            .startAngle   = 200,        // Down-left for -2000
            .endAngle     = 520,        // Down-right for +2000 (through top at 360=0)
            .ticks        = { .majorInterval = 500, .minorInterval = 100, .labelDivisor = 100, .showLabels = true },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = false,
        };
        return cfg;
    }
};
