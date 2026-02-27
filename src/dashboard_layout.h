#pragma once

#include <Preferences.h>
#include "config.h"
#include "gauges/gauge_base.h"
#include "gauges/gauge_registry.h"

// ── Per-cell state ──────────────────────────────────────────────────
struct CellState {
    int gaugeIndex = 0;         // Index into g_gauges[]
    float rawValue = 0.0f;
    float smoothedValue = 0.0f;
    bool hasData = false;
    unsigned long lastReceiveTime = 0;

    // For position gauge secondary dataref (longitude)
    float secondaryValue = 0.0f;
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
        _cells[cellIndex].rawValue = 0.0f;
        _cells[cellIndex].smoothedValue = 0.0f;
        _cells[cellIndex].hasData = false;
        _cells[cellIndex].secondaryValue = 0.0f;
        saveToNVS();
    }

    // Check if any cell has a gauge in the red zone
    bool anyRedZone() const {
        for (int i = 0; i < CELL_COUNT; i++) {
            if (!_cells[i].hasData) continue;
            const GaugeConfig& cfg = g_gauges[_cells[i].gaugeIndex]->getConfig();
            if (cfg.customRenderer) continue;
            float val = _cells[i].smoothedValue;
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

    // Check if a specific gauge type needs a secondary dataref (position gauge)
    bool needsSecondary(int cellIndex) const {
        const GaugeConfig& cfg = g_gauges[_cells[cellIndex].gaugeIndex]->getConfig();
        // Position gauge has latitude as primary, needs longitude as secondary
        return (strcmp(cfg.title, "POSITION") == 0);
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
        }
        _prefs.end();
    }
};
