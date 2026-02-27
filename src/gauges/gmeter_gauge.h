#pragma once

#include "gauge_base.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <cmath>
#include "../config.h"

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

class GMeterGauge : public GaugeBase {
public:
    const GaugeConfig& getConfig() const override {
        static const GaugeConfig cfg = {
            .title        = "G-METER",
            .units        = "G",
            .dataref      = "sim/flightmodel/position/g_nrml",
            .minValue     = -2,
            .maxValue     = 5,
            .startAngle   = -140,
            .endAngle     = 140,
            .ticks        = { .majorInterval = 1, .minorInterval = 0.5f, .labelDivisor = 1, .showLabels = true },
            .arcs         = nullptr,
            .arcCount     = 0,
            .valueMultiplier = 1.0f,
            .wrapNeedle   = false,
            .wrapModulo   = 0,
            .customRenderer = true,
            .extraDatarefs = {},
            .extraDatarefCount = 0,
        };
        return cfg;
    }

    bool customRender(void* spritePtr, const float* values, int valueCount, int cx, int cy) override {
        LGFX_Sprite* canvas = static_cast<LGFX_Sprite*>(spritePtr);
        float gForce = values[0];
        (void)valueCount;

        // Track session peaks
        if (gForce > _maxG) _maxG = gForce;
        if (gForce < _minG) _minG = gForce;

        const GaugeConfig& cfg = getConfig();

        canvas->fillSprite(COLOR_BG);
        canvas->fillCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_BG);
        canvas->drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);
        canvas->drawCircle(cx, cy, DIAL_RADIUS - 1, COLOR_DIAL_RIM);

        // Draw colored arcs manually
        // Green: -1 to 3.8G, Yellow: 3.8 to 4.4G, Red: 4.4+ and below -1
        drawArcRange(canvas, cfg, cx, cy, -1.0f, 3.8f, COLOR_ARC_GREEN);
        drawArcRange(canvas, cfg, cx, cy, 3.8f, 4.4f, COLOR_ARC_YELLOW);
        drawArcRange(canvas, cfg, cx, cy, 4.4f, 5.0f, COLOR_ARC_RED);
        drawArcRange(canvas, cfg, cx, cy, -2.0f, -1.0f, COLOR_ARC_RED);

        // Tick marks
        for (float v = cfg.minValue; v <= cfg.maxValue + 0.001f; v += cfg.ticks.minorInterval) {
            float angle = valueToAngle(cfg, v);
            float rad = angle * DEG_TO_RAD;
            float sinA = sinf(rad);
            float cosA = cosf(rad);

            bool isMajor = (fabsf(fmodf(v - cfg.minValue, cfg.ticks.majorInterval)) < 0.01f);
            int innerR = isMajor ? TICK_MAJOR_R : TICK_MINOR_R;

            int x1 = cx + (int)(innerR * sinA);
            int y1 = cy - (int)(innerR * cosA);
            int x2 = cx + (int)(TICK_OUTER_R * sinA);
            int y2 = cy - (int)(TICK_OUTER_R * cosA);
            canvas->drawLine(x1, y1, x2, y2, COLOR_TICK);
            if (isMajor) {
                canvas->drawLine(x1 + 1, y1, x2 + 1, y2, COLOR_TICK);
                int lx = cx + (int)(LABEL_RADIUS * sinA);
                int ly = cy - (int)(LABEL_RADIUS * cosA);
                canvas->setTextDatum(middle_center);
                canvas->setTextColor(COLOR_LABEL);
                canvas->setTextSize(0.9);
                char lbl[6];
                snprintf(lbl, sizeof(lbl), "%d", (int)v);
                canvas->drawString(lbl, lx, ly);
            }
        }

        // Peak markers (small triangles on the arc)
        drawPeakMarker(canvas, cfg, cx, cy, _maxG, COLOR_ARC_RED);
        drawPeakMarker(canvas, cfg, cx, cy, _minG, COLOR_ARC_YELLOW);

        // Main needle
        float angle = valueToAngle(cfg, gForce);
        float rad = angle * DEG_TO_RAD;
        int needleLen = NEEDLE_LENGTH;
        int nx = cx + (int)(needleLen * sinf(rad));
        int ny = cy - (int)(needleLen * cosf(rad));
        canvas->drawLine(cx, cy, nx, ny, COLOR_NEEDLE);
        canvas->drawLine(cx + 1, cy, nx + 1, ny, COLOR_NEEDLE);

        // Hub
        canvas->fillCircle(cx, cy, HUB_RADIUS, COLOR_HUB);
        canvas->drawCircle(cx, cy, HUB_RADIUS, COLOR_DIAL_RIM);

        // Title
        canvas->setTextDatum(middle_center);
        canvas->setTextColor(COLOR_TITLE);
        canvas->setTextSize(1.0);
        canvas->drawString("G-METER", cx, cy - 45);

        // Current value
        canvas->setTextColor(COLOR_VALUE);
        canvas->setTextSize(1.3);
        char buf[12];
        snprintf(buf, sizeof(buf), "%+.1f", gForce);
        canvas->drawString(buf, cx, cy + 35);

        // Min/Max readout
        canvas->setTextSize(0.8);
        canvas->setTextColor(COLOR_ARC_RED);
        snprintf(buf, sizeof(buf), "H%+.1f", _maxG);
        canvas->drawString(buf, cx - 30, cy + 55);

        canvas->setTextColor(COLOR_ARC_YELLOW);
        snprintf(buf, sizeof(buf), "L%+.1f", _minG);
        canvas->drawString(buf, cx + 30, cy + 55);

        return true;
    }

private:
    float _maxG = 1.0f;
    float _minG = 1.0f;

    static float valueToAngle(const GaugeConfig& cfg, float value) {
        float range = cfg.maxValue - cfg.minValue;
        if (range <= 0) return cfg.startAngle;
        float t = (value - cfg.minValue) / range;
        if (t < 0) t = 0;
        if (t > 1) t = 1;
        return cfg.startAngle + t * (cfg.endAngle - cfg.startAngle);
    }

    static void drawArcRange(LGFX_Sprite* canvas, const GaugeConfig& cfg, int cx, int cy,
                              float startVal, float endVal, uint16_t color) {
        float startAngle = valueToAngle(cfg, startVal) - 90.0f;
        float endAngle = valueToAngle(cfg, endVal) - 90.0f;
        for (int t = 0; t < ARC_THICKNESS; t++) {
            canvas->drawArc(cx, cy,
                           ARC_RADIUS + ARC_THICKNESS / 2 - t,
                           ARC_RADIUS - ARC_THICKNESS / 2 - t,
                           startAngle, endAngle, color);
        }
    }

    static void drawPeakMarker(LGFX_Sprite* canvas, const GaugeConfig& cfg, int cx, int cy,
                                float value, uint16_t color) {
        float angle = valueToAngle(cfg, value);
        float rad = angle * DEG_TO_RAD;
        float sinA = sinf(rad);
        float cosA = cosf(rad);
        int r = ARC_RADIUS + ARC_THICKNESS;
        int mx = cx + (int)(r * sinA);
        int my = cy - (int)(r * cosA);
        canvas->fillCircle(mx, my, 3, color);
    }
};
