#include "gauge_renderer.h"
#include "config.h"
#include <cmath>

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

void GaugeRenderer::begin() {
    _canvas.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
    _canvas.setColorDepth(16);

    // Needle sprite: tall narrow rectangle, pivot at bottom center
    _needle.createSprite(NEEDLE_WIDTH + 2, NEEDLE_LENGTH);
    _needle.setColorDepth(16);

    _initialized = true;
}

void GaugeRenderer::render(GaugeBase* gauge, float rawValue, float smoothedValue, bool xplaneConnected) {
    if (!_initialized || !gauge) return;

    const GaugeConfig& cfg = gauge->getConfig();

    // Check for custom rendering (e.g., heading compass card)
    if (cfg.customRenderer && gauge->customRender(&_canvas, smoothedValue)) {
        drawConnectionStatus(xplaneConnected);
        return;
    }

    // Compute needle angle from smoothed value
    float displayValue = smoothedValue;
    if (cfg.wrapNeedle && cfg.wrapModulo > 0) {
        displayValue = fmodf(smoothedValue, cfg.wrapModulo);
        if (displayValue < 0) displayValue += cfg.wrapModulo;
    }
    float angle = valueToAngle(cfg, displayValue);

    // Draw all layers into sprite
    drawBackground();
    drawArcs(cfg);
    drawTicks(cfg);
    drawTitle(cfg);
    drawValue(cfg, rawValue);
    drawNeedle(angle);
    drawHub();
    drawConnectionStatus(xplaneConnected);
}

void GaugeRenderer::push(M5GFX& display) {
    if (_initialized) {
        _canvas.pushSprite(&display, 0, 0);
    }
}

void GaugeRenderer::drawBackground() {
    _canvas.fillSprite(COLOR_BG);

    // Dial face circle
    _canvas.fillCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_BG);

    // Rim
    _canvas.drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS, COLOR_DIAL_RIM);
    _canvas.drawCircle(CENTER_X, CENTER_Y, DIAL_RADIUS - 1, COLOR_DIAL_RIM);
}

void GaugeRenderer::drawArcs(const GaugeConfig& cfg) {
    if (!cfg.arcs || cfg.arcCount == 0) return;

    for (size_t i = 0; i < cfg.arcCount; i++) {
        const ArcSegment& arc = cfg.arcs[i];
        float startAngle = valueToAngle(cfg, arc.startValue);
        float endAngle = valueToAngle(cfg, arc.endValue);

        // Convert from our angle system (0=top, CW) to M5GFX arc (0=right, CW)
        // Our 0 degrees = top = M5GFX -90
        float gfxStart = startAngle - 90.0f;
        float gfxEnd = endAngle - 90.0f;

        for (int t = 0; t < ARC_THICKNESS; t++) {
            _canvas.drawArc(CENTER_X, CENTER_Y,
                           ARC_RADIUS + ARC_THICKNESS / 2 - t,
                           ARC_RADIUS - ARC_THICKNESS / 2 - t,
                           gfxStart, gfxEnd, arc.color);
        }
    }
}

void GaugeRenderer::drawTicks(const GaugeConfig& cfg) {
    float range = cfg.maxValue - cfg.minValue;
    if (range <= 0) return;

    _canvas.setTextDatum(middle_center);
    _canvas.setTextColor(COLOR_LABEL);
    _canvas.setTextSize(1.0);

    // Minor ticks
    if (cfg.ticks.minorInterval > 0) {
        for (float v = cfg.minValue; v <= cfg.maxValue + 0.001f; v += cfg.ticks.minorInterval) {
            float angle = valueToAngle(cfg, v);
            float rad = angle * DEG_TO_RAD;
            float sinA = sinf(rad);
            float cosA = cosf(rad);

            int x1 = CENTER_X + (int)(TICK_MINOR_R * sinA);
            int y1 = CENTER_Y - (int)(TICK_MINOR_R * cosA);
            int x2 = CENTER_X + (int)(TICK_OUTER_R * sinA);
            int y2 = CENTER_Y - (int)(TICK_OUTER_R * cosA);

            _canvas.drawLine(x1, y1, x2, y2, COLOR_TICK);
        }
    }

    // Major ticks
    if (cfg.ticks.majorInterval > 0) {
        for (float v = cfg.minValue; v <= cfg.maxValue + 0.001f; v += cfg.ticks.majorInterval) {
            float angle = valueToAngle(cfg, v);
            float rad = angle * DEG_TO_RAD;
            float sinA = sinf(rad);
            float cosA = cosf(rad);

            int x1 = CENTER_X + (int)(TICK_MAJOR_R * sinA);
            int y1 = CENTER_Y - (int)(TICK_MAJOR_R * cosA);
            int x2 = CENTER_X + (int)(TICK_OUTER_R * sinA);
            int y2 = CENTER_Y - (int)(TICK_OUTER_R * cosA);

            // Thicker major ticks
            _canvas.drawLine(x1, y1, x2, y2, COLOR_TICK);
            _canvas.drawLine(x1 + 1, y1, x2 + 1, y2, COLOR_TICK);

            // Labels
            if (cfg.ticks.showLabels) {
                int lx = CENTER_X + (int)(LABEL_RADIUS * sinA);
                int ly = CENTER_Y - (int)(LABEL_RADIUS * cosA);
                int labelVal = (int)(v / cfg.ticks.labelDivisor);
                _canvas.drawNumber(labelVal, lx, ly);
            }
        }
    }
}

void GaugeRenderer::drawNeedle(float angle) {
    // Draw needle using rotated sprite for clean anti-aliased appearance
    _needle.fillSprite(TFT_BLACK);
    _needle.setColorDepth(16);

    // Needle shape: narrow at tip, wider at base
    int midX = (_needle.width()) / 2;
    _needle.fillTriangle(
        midX, 0,                          // Tip (top)
        0, _needle.height() - 1,          // Base left
        _needle.width() - 1, _needle.height() - 1, // Base right
        COLOR_NEEDLE
    );

    // Set pivot at base center and rotate onto main canvas
    _needle.setPivot(midX, _needle.height() - 1);
    _needle.pushRotated(&_canvas, angle, TFT_BLACK);
}

void GaugeRenderer::drawHub() {
    _canvas.fillCircle(CENTER_X, CENTER_Y, HUB_RADIUS, COLOR_HUB);
    _canvas.drawCircle(CENTER_X, CENTER_Y, HUB_RADIUS, COLOR_DIAL_RIM);
}

void GaugeRenderer::drawTitle(const GaugeConfig& cfg) {
    _canvas.setTextDatum(middle_center);
    _canvas.setTextColor(COLOR_TITLE);
    _canvas.setTextSize(1.0);
    _canvas.drawString(cfg.title, CENTER_X, CENTER_Y - 45);

    _canvas.setTextSize(0.8);
    _canvas.drawString(cfg.units, CENTER_X, CENTER_Y - 30);
}

void GaugeRenderer::drawValue(const GaugeConfig& cfg, float value) {
    _canvas.setTextDatum(middle_center);
    _canvas.setTextColor(COLOR_VALUE);
    _canvas.setTextSize(1.1);

    // Apply display multiplier if set (e.g., throttle 0-1 → 0-100%)
    float displayVal = value * cfg.valueMultiplier;

    char buf[16];
    if (fabsf(displayVal) >= 100.0f) {
        snprintf(buf, sizeof(buf), "%.0f", displayVal);
    } else if (fabsf(displayVal) >= 10.0f) {
        snprintf(buf, sizeof(buf), "%.1f", displayVal);
    } else {
        snprintf(buf, sizeof(buf), "%.1f", displayVal);
    }
    _canvas.drawString(buf, CENTER_X, CENTER_Y + 35);
}

void GaugeRenderer::drawConnectionStatus(bool connected) {
    // Small indicator dot in top-right area of the dial
    uint16_t color = connected ? COLOR_ARC_GREEN : COLOR_ARC_RED;
    int dotX = CENTER_X + 25;
    int dotY = CENTER_Y + 50;
    _canvas.fillCircle(dotX, dotY, 4, color);
}

float GaugeRenderer::valueToAngle(const GaugeConfig& cfg, float value) {
    float range = cfg.maxValue - cfg.minValue;
    if (range <= 0) return cfg.startAngle;

    float t = (value - cfg.minValue) / range;
    t = fmaxf(0.0f, fminf(1.0f, t));  // Clamp 0-1
    return cfg.startAngle + t * (cfg.endAngle - cfg.startAngle);
}
