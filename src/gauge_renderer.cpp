#include "gauge_renderer.h"
#include <cmath>

#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.017453292519943295f
#endif

void GaugeRenderer::begin(LGFX* display) {
    _display = display;

    // Full-screen framebuffer in PSRAM (800x480 x 2 bytes = 768KB)
    _framebuffer.setPsram(true);
    _framebuffer.setColorDepth(16);
    _framebuffer.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Reusable cell sprite (max cell size)
    _cellSprite.setPsram(true);
    _cellSprite.setColorDepth(16);
    _cellSprite.createSprite(CELL_WIDTH + 2, CELL_HEIGHT);  // +2 for last column (268)

    // Needle sprite: tall narrow rectangle, pivot at bottom center
    _needle.setColorDepth(16);
    _needle.createSprite(NEEDLE_WIDTH + 2, NEEDLE_LENGTH);

    _initialized = true;
}

void GaugeRenderer::cellCenter(int cellIndex, int& cx, int& cy) {
    int col = cellIndex % GRID_COLS;
    int row = cellIndex / GRID_COLS;
    int x, y, w, h;
    cellOrigin(cellIndex, x, y);
    cellSize(cellIndex, w, h);
    cx = x + w / 2;
    cy = y + h / 2;
}

void GaugeRenderer::cellOrigin(int cellIndex, int& x, int& y) {
    int col = cellIndex % GRID_COLS;
    int row = cellIndex / GRID_COLS;
    x = col * CELL_WIDTH;
    y = row * CELL_HEIGHT;
}

void GaugeRenderer::cellSize(int cellIndex, int& w, int& h) {
    int col = cellIndex % GRID_COLS;
    // Last column gets remaining pixels (800 - 266*2 = 268)
    w = (col == GRID_COLS - 1) ? (SCREEN_WIDTH - CELL_WIDTH * (GRID_COLS - 1)) : CELL_WIDTH;
    h = CELL_HEIGHT;
}

void GaugeRenderer::renderCell(int cellIndex, GaugeBase* gauge, float rawValue,
                                float smoothedValue, bool xplaneConnected) {
    if (!_initialized || !gauge) return;

    const GaugeConfig& cfg = gauge->getConfig();

    int cellW, cellH;
    cellSize(cellIndex, cellW, cellH);

    // Center within the cell sprite
    int cx = cellW / 2;
    int cy = cellH / 2;

    // Resize cell sprite if needed (for last column which is 268 wide)
    if (_cellSprite.width() != cellW || _cellSprite.height() != cellH) {
        _cellSprite.deleteSprite();
        _cellSprite.createSprite(cellW, cellH);
    }

    // Check for custom rendering
    if (cfg.customRenderer && gauge->customRender(&_cellSprite, smoothedValue, cx, cy)) {
        // Custom renderer handled it
    } else {
        // Standard gauge rendering
        float displayValue = smoothedValue;
        if (cfg.wrapNeedle && cfg.wrapModulo > 0) {
            displayValue = fmodf(smoothedValue, cfg.wrapModulo);
            if (displayValue < 0) displayValue += cfg.wrapModulo;
        }
        float angle = valueToAngle(cfg, displayValue);

        drawBackground(cx, cy);
        drawArcs(cfg, cx, cy);
        drawTicks(cfg, cx, cy);
        drawTitle(cfg, cx, cy);
        drawValue(cfg, rawValue, cx, cy);
        drawNeedle(angle, cx, cy);
        drawHub(cx, cy);
    }

    // Composite cell sprite onto framebuffer
    int ox, oy;
    cellOrigin(cellIndex, ox, oy);
    _cellSprite.pushSprite(&_framebuffer, ox, oy);
}

void GaugeRenderer::drawGrid() {
    if (!_initialized) return;

    // Vertical lines
    for (int col = 1; col < GRID_COLS; col++) {
        int x = col * CELL_WIDTH;
        _framebuffer.drawFastVLine(x, 0, SCREEN_HEIGHT, COLOR_GRID_LINE);
    }

    // Horizontal line (center)
    _framebuffer.drawFastHLine(0, CELL_HEIGHT, SCREEN_WIDTH, COLOR_GRID_LINE);
}

void GaugeRenderer::drawCellHighlight(int cellIndex, uint16_t color) {
    if (!_initialized) return;

    int ox, oy, w, h;
    cellOrigin(cellIndex, ox, oy);
    cellSize(cellIndex, w, h);

    // Draw 2px border inside the cell
    _framebuffer.drawRect(ox, oy, w, h, color);
    _framebuffer.drawRect(ox + 1, oy + 1, w - 2, h - 2, color);
}

void GaugeRenderer::push() {
    if (_initialized && _display) {
        _framebuffer.pushSprite(_display, 0, 0);
    }
}

// ── Private rendering methods (all work on _cellSprite) ──────────────

void GaugeRenderer::drawBackground(int cx, int cy) {
    _cellSprite.fillSprite(COLOR_BG);
    _cellSprite.fillCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_BG);
    _cellSprite.drawCircle(cx, cy, DIAL_RADIUS, COLOR_DIAL_RIM);
    _cellSprite.drawCircle(cx, cy, DIAL_RADIUS - 1, COLOR_DIAL_RIM);
}

void GaugeRenderer::drawArcs(const GaugeConfig& cfg, int cx, int cy) {
    if (!cfg.arcs || cfg.arcCount == 0) return;

    for (size_t i = 0; i < cfg.arcCount; i++) {
        const ArcSegment& arc = cfg.arcs[i];
        float startAngle = valueToAngle(cfg, arc.startValue);
        float endAngle = valueToAngle(cfg, arc.endValue);

        float gfxStart = startAngle - 90.0f;
        float gfxEnd = endAngle - 90.0f;

        for (int t = 0; t < ARC_THICKNESS; t++) {
            _cellSprite.drawArc(cx, cy,
                           ARC_RADIUS + ARC_THICKNESS / 2 - t,
                           ARC_RADIUS - ARC_THICKNESS / 2 - t,
                           gfxStart, gfxEnd, arc.color);
        }
    }
}

void GaugeRenderer::drawTicks(const GaugeConfig& cfg, int cx, int cy) {
    float range = cfg.maxValue - cfg.minValue;
    if (range <= 0) return;

    _cellSprite.setTextDatum(middle_center);
    _cellSprite.setTextColor(COLOR_LABEL);
    _cellSprite.setTextSize(1.0);

    // Minor ticks
    if (cfg.ticks.minorInterval > 0) {
        for (float v = cfg.minValue; v <= cfg.maxValue + 0.001f; v += cfg.ticks.minorInterval) {
            float angle = valueToAngle(cfg, v);
            float rad = angle * DEG_TO_RAD;
            float sinA = sinf(rad);
            float cosA = cosf(rad);

            int x1 = cx + (int)(TICK_MINOR_R * sinA);
            int y1 = cy - (int)(TICK_MINOR_R * cosA);
            int x2 = cx + (int)(TICK_OUTER_R * sinA);
            int y2 = cy - (int)(TICK_OUTER_R * cosA);

            _cellSprite.drawLine(x1, y1, x2, y2, COLOR_TICK);
        }
    }

    // Major ticks
    if (cfg.ticks.majorInterval > 0) {
        for (float v = cfg.minValue; v <= cfg.maxValue + 0.001f; v += cfg.ticks.majorInterval) {
            float angle = valueToAngle(cfg, v);
            float rad = angle * DEG_TO_RAD;
            float sinA = sinf(rad);
            float cosA = cosf(rad);

            int x1 = cx + (int)(TICK_MAJOR_R * sinA);
            int y1 = cy - (int)(TICK_MAJOR_R * cosA);
            int x2 = cx + (int)(TICK_OUTER_R * sinA);
            int y2 = cy - (int)(TICK_OUTER_R * cosA);

            _cellSprite.drawLine(x1, y1, x2, y2, COLOR_TICK);
            _cellSprite.drawLine(x1 + 1, y1, x2 + 1, y2, COLOR_TICK);

            // Labels
            if (cfg.ticks.showLabels) {
                int lx = cx + (int)(LABEL_RADIUS * sinA);
                int ly = cy - (int)(LABEL_RADIUS * cosA);
                int labelVal = (int)(v / cfg.ticks.labelDivisor);
                _cellSprite.drawNumber(labelVal, lx, ly);
            }
        }
    }
}

void GaugeRenderer::drawNeedle(float angle, int cx, int cy) {
    _needle.fillSprite(TFT_BLACK);

    int midX = (_needle.width()) / 2;
    _needle.fillTriangle(
        midX, 0,
        0, _needle.height() - 1,
        _needle.width() - 1, _needle.height() - 1,
        COLOR_NEEDLE
    );

    // Set pivot at base center and rotate onto cell sprite
    _needle.setPivot(midX, _needle.height() - 1);

    // Temporarily set cell sprite pivot to gauge center for proper rotation target
    _cellSprite.setPivot(cx, cy);
    _needle.pushRotated(&_cellSprite, angle, TFT_BLACK);
}

void GaugeRenderer::drawHub(int cx, int cy) {
    _cellSprite.fillCircle(cx, cy, HUB_RADIUS, COLOR_HUB);
    _cellSprite.drawCircle(cx, cy, HUB_RADIUS, COLOR_DIAL_RIM);
}

void GaugeRenderer::drawTitle(const GaugeConfig& cfg, int cx, int cy) {
    _cellSprite.setTextDatum(middle_center);
    _cellSprite.setTextColor(COLOR_TITLE);
    _cellSprite.setTextSize(1.3);
    _cellSprite.drawString(cfg.title, cx, cy - 50);

    _cellSprite.setTextSize(1.0);
    _cellSprite.drawString(cfg.units, cx, cy - 35);
}

void GaugeRenderer::drawValue(const GaugeConfig& cfg, float value, int cx, int cy) {
    _cellSprite.setTextDatum(middle_center);
    _cellSprite.setTextColor(COLOR_VALUE);
    _cellSprite.setTextSize(1.4);

    float displayVal = value * cfg.valueMultiplier;

    char buf[16];
    if (fabsf(displayVal) >= 100.0f) {
        snprintf(buf, sizeof(buf), "%.0f", displayVal);
    } else if (fabsf(displayVal) >= 10.0f) {
        snprintf(buf, sizeof(buf), "%.1f", displayVal);
    } else {
        snprintf(buf, sizeof(buf), "%.1f", displayVal);
    }
    _cellSprite.drawString(buf, cx, cy + 35);
}

float GaugeRenderer::valueToAngle(const GaugeConfig& cfg, float value) {
    float range = cfg.maxValue - cfg.minValue;
    if (range <= 0) return cfg.startAngle;

    float t = (value - cfg.minValue) / range;
    t = fmaxf(0.0f, fminf(1.0f, t));
    return cfg.startAngle + t * (cfg.endAngle - cfg.startAngle);
}
