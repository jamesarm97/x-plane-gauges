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

private:
    DashboardLayout* _layout = nullptr;
    bool _open = false;
    int _selectedCell = -1;         // Which cell is being configured
    int _scrollOffset = 0;          // Scroll position in picker list
    unsigned long _openTime = 0;    // For auto-close timeout
    bool _wasTouched = false;       // For edge detection

    // Picker panel geometry
    static constexpr int PANEL_X = SCREEN_WIDTH - PICKER_WIDTH;
    static constexpr int PANEL_Y = 0;
    static constexpr int PANEL_W = PICKER_WIDTH;
    static constexpr int PANEL_H = SCREEN_HEIGHT;
    static constexpr int HEADER_H = 40;
    static constexpr int MAX_VISIBLE_ROWS = (PANEL_H - HEADER_H) / PICKER_ROW_HEIGHT;

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
    };
    static constexpr int CATEGORY_COUNT = 5;

    void open(int cellIndex);
    void close();
    int hitTestCell(int x, int y);
    int hitTestPickerRow(int x, int y);
};
