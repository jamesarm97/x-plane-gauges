#pragma once

#include <Preferences.h>
#include "config.h"
#include "gauges/gauge_base.h"
#include "gauges/gauge_registry.h"

// ── Per-cell state ──────────────────────────────────────────────────
static constexpr int MAX_VALUES = 8;

struct CellState {
    int gaugeIndex = 0;         // Index into g_gauges[]
    float values[MAX_VALUES] = {};          // Raw values (slot 0 = primary, 1-7 = extra)
    float smoothedValues[MAX_VALUES] = {};  // Smoothed values for rendering
    int valueCount = 1;                     // Number of active dataref slots
    bool hasData = false;
    unsigned long lastReceiveTime = 0;
};

// ── Dashboard: manages 6 gauge cells ─────────────────────────────────
class DashboardLayout {
public:
    void begin() {
        loadFromNVS();
    }

    CellState& cell(int i) { return _cells[i]; }
    const CellState& cell(int i) const { return _cells[i]; }

    int gaugeIndex(int cellIndex) const { return _cells[cellIndex].gaugeIndex; }

    GaugeBase* gauge(int cellIndex) const {
        return g_gauges[_cells[cellIndex].gaugeIndex];
    }

    void setGauge(int cellIndex, int gaugeIdx) {
        if (gaugeIdx < 0 || gaugeIdx >= GAUGE_COUNT) return;
        _cells[cellIndex].gaugeIndex = gaugeIdx;
        for (int s = 0; s < MAX_VALUES; s++) {
            _cells[cellIndex].values[s] = 0.0f;
            _cells[cellIndex].smoothedValues[s] = 0.0f;
        }
        _cells[cellIndex].valueCount = 1 + g_gauges[gaugeIdx]->getConfig().extraDatarefCount;
        _cells[cellIndex].hasData = false;
        saveToNVS();
    }

    // Check if any cell has a gauge in the red zone
    bool anyRedZone() const {
        for (int i = 0; i < CELL_COUNT; i++) {
            if (!_cells[i].hasData) continue;
            const GaugeConfig& cfg = g_gauges[_cells[i].gaugeIndex]->getConfig();
            if (cfg.customRenderer) continue;
            float val = _cells[i].smoothedValues[0];
            for (size_t a = 0; a < cfg.arcCount; a++) {
                if (cfg.arcs[a].color == COLOR_ARC_RED &&
                    val >= cfg.arcs[a].startValue &&
                    val <= cfg.arcs[a].endValue) {
                    return true;
                }
            }
        }
        return false;
    }

private:
    CellState _cells[CELL_COUNT];
    Preferences _prefs;

    // Default gauge layout: 6 most useful flight instruments
    int defaultGauge(int i) const {
        static const int defaults[] = { 0, 3, 1, 9, 4, 2 };
        return defaults[i];
    }

    void saveToNVS() {
        _prefs.begin("dash", false);
        for (int i = 0; i < CELL_COUNT; i++) {
            char key[8];
            snprintf(key, sizeof(key), "c%d", i);
            _prefs.putInt(key, _cells[i].gaugeIndex);
        }
        _prefs.end();
    }

    void loadFromNVS() {
        _prefs.begin("dash", true);
        for (int i = 0; i < CELL_COUNT; i++) {
            char key[8];
            snprintf(key, sizeof(key), "c%d", i);
            int idx = _prefs.getInt(key, defaultGauge(i));
            if (idx < 0 || idx >= GAUGE_COUNT) idx = defaultGauge(i);
            _cells[i].gaugeIndex = idx;
            _cells[i].valueCount = 1 + g_gauges[idx]->getConfig().extraDatarefCount;
        }
        _prefs.end();
    }
};
