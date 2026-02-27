#pragma once

#include "config.h"

// ── Predefined view layouts ────────────────────────────────────────
struct ViewDef {
    const char* name;
    int gauges[CELL_COUNT];  // Gauge indices for each cell
};

static constexpr int VIEW_COUNT = 4;
static constexpr int VIEW_CUSTOM = 3;  // Index of CUSTOM view

static const ViewDef g_views[VIEW_COUNT] = {
    // FLIGHT: airspeed(0), attitude(21), altitude(1), HSI(22), heading(3), VSI(2)
    { "FLIGHT",  { 0, 21, 1, 22, 3, 2 } },
    // ENGINE: RPM(9), manifold(10), oil_temp(13), oil_press(14), EGT(11), fuel_flow(16)
    { "ENGINE",  { 9, 10, 13, 14, 11, 16 } },
    // SYSTEMS: annunciator(23), trim(28), gmeter(26), fuel_endurance(27), wind(25), volts(17)
    { "SYSTEMS", { 23, 28, 26, 27, 25, 17 } },
    // CUSTOM: loaded from NVS (user's saved layout — placeholder, not applied directly)
    { "CUSTOM",  { 0, 3, 1, 9, 4, 2 } },
};

class ViewManager {
public:
    int currentView() const { return _currentView; }
    const char* currentViewName() const { return g_views[_currentView].name; }

    void setView(int view) {
        if (view < 0 || view >= VIEW_COUNT) return;
        _currentView = view;
        _showNameUntil = millis() + 2000;
    }

    void nextView() {
        setView((_currentView + 1) % VIEW_COUNT);
    }

    void prevView() {
        setView((_currentView - 1 + VIEW_COUNT) % VIEW_COUNT);
    }

    bool shouldShowName() const {
        return millis() < _showNameUntil;
    }

    unsigned long showNameUntil() const { return _showNameUntil; }
    void clearOverlay() { _showNameUntil = 0; }

private:
    int _currentView = VIEW_CUSTOM;  // Start with user's custom layout
    unsigned long _showNameUntil = 0;
};
