#pragma once

#include "display_driver.h"
#include "gauges/gauge_base.h"
#include "config.h"

class GaugeRenderer {
public:
    void begin(LGFX* display);

    // Render a single gauge into its cell position on the framebuffer
    void renderCell(int cellIndex, GaugeBase* gauge, float rawValue,
                    float smoothedValue, bool xplaneConnected);

    // Draw the grid borders onto the framebuffer
    void drawGrid();

    // Draw a highlight border around a cell (e.g., selected cell)
    void drawCellHighlight(int cellIndex, uint16_t color);

    // Push the entire framebuffer to the display
    void push();

    // Access the framebuffer for overlay drawing (picker, mute indicator, etc.)
    LGFX_Sprite& getFramebuffer() { return _framebuffer; }

    // Get cell center coordinates
    static void cellCenter(int cellIndex, int& cx, int& cy);

    // Get cell top-left coordinates
    static void cellOrigin(int cellIndex, int& x, int& y);

    // Get cell dimensions
    static void cellSize(int cellIndex, int& w, int& h);

private:
    LGFX* _display = nullptr;
    LGFX_Sprite _framebuffer;   // Full-screen 800x480 sprite (PSRAM)
    LGFX_Sprite _cellSprite;    // Reusable per-cell sprite (266x240)
    LGFX_Sprite _needle;        // Small needle sprite for rotation
    bool _initialized = false;

    void drawBackground(int cx, int cy);
    void drawArcs(const GaugeConfig& cfg, int cx, int cy);
    void drawTicks(const GaugeConfig& cfg, int cx, int cy);
    void drawNeedle(float angle, int cx, int cy);
    void drawHub(int cx, int cy);
    void drawTitle(const GaugeConfig& cfg, int cx, int cy);
    void drawValue(const GaugeConfig& cfg, float value, int cx, int cy);
    void drawConnectionStatus(bool connected, int cx, int cy);

    float valueToAngle(const GaugeConfig& cfg, float value);
};
