#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include "config.h"
#include "dashboard_layout.h"
#include "gauge_renderer.h"

class GaugePicker {
public:
    void begin(DashboardLayout* layout);

    // Process touch input. Returns true if a gauge was selected (layout changed).
    bool handleTouch(int x, int y, bool pressed);

    // Draw the picker overlay onto the framebuffer (if open)
    void draw(LGFX_Sprite& fb);

    // Must be called each frame to check timeout
    void update();

    bool isOpen() const { return _open; }
    int selectedCell() const { return _selectedCell; }
    void open(int cellIndex);

private:
    DashboardLayout* _layout = nullptr;
    bool _open = false;
    int _selectedCell = -1;         // Which cell is being configured
    int _scrollOffset = 0;          // Scroll position in pixels
    int _maxScroll = 0;             // Computed max scroll
    unsigned long _openTime = 0;    // For auto-close timeout
    bool _wasTouched = false;       // For edge detection
    int _lastTouchY = 0;           // For scroll drag tracking
    bool _isDragging = false;       // True if actively dragging to scroll
    int _touchStartY = 0;          // Y at touch start for drag detection

    // Picker popup geometry (centered on screen)
    static constexpr int PANEL_W = PICKER_WIDTH;
    static constexpr int PANEL_H = PICKER_HEIGHT;
    static constexpr int PANEL_X = (SCREEN_WIDTH - PANEL_W) / 2;
    static constexpr int PANEL_Y = (SCREEN_HEIGHT - PANEL_H) / 2;
    static constexpr int HEADER_H = 60;
    static constexpr int CAT_HEADER_H = 32;
    static constexpr int DRAG_THRESHOLD = 8;    // Pixels before drag starts

    // Category info for grouped display
    struct Category {
        const char* name;
        int startIdx;   // First gauge index in this category
        int count;       // Number of gauges
    };

    static constexpr Category CATEGORIES[] = {
        {"FLIGHT",     0,  7},
        {"CONTROLS",   7,  2},
        {"ENGINE",     9,  6},
        {"FUEL/ELEC", 15,  3},
        {"NAV",       18,  3},
        {"ADVANCED",  21,  8},
    };
    static constexpr int CATEGORY_COUNT = 6;

    void close();
    int hitTestCell(int x, int y);
    int hitTestPickerRow(int x, int y);
    int computeContentHeight() const;
};
