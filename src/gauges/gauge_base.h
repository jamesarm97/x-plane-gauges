#pragma once

#include <cstdint>
#include <cstddef>

// ── Colored arc segment on the gauge dial ───────────────────────────
struct ArcSegment {
    float startValue;
    float endValue;
    uint16_t color;
};

// ── Tick mark configuration ─────────────────────────────────────────
struct TickConfig {
    float majorInterval;    // Value interval between major ticks (with labels)
    float minorInterval;    // Value interval between minor ticks
    float labelDivisor;     // Divide value by this before displaying (e.g., 1000 for altitude, 0.01 for 0-1 as %)
    bool  showLabels;       // Whether to draw numeric labels at major ticks
};

// ── Complete gauge configuration ────────────────────────────────────
struct GaugeConfig {
    const char* title;          // Gauge name (e.g., "AIRSPEED")
    const char* units;          // Units label (e.g., "KTS")
    const char* dataref;        // X-Plane dataref path

    float minValue;             // Scale minimum
    float maxValue;             // Scale maximum
    float startAngle;           // Needle angle at minValue (degrees, 0=top, CW)
    float endAngle;             // Needle angle at maxValue (degrees, 0=top, CW)

    TickConfig ticks;

    const ArcSegment* arcs;     // Array of colored arc segments (nullptr if none)
    size_t arcCount;            // Number of arc segments

    float valueMultiplier;      // Multiply raw value for display readout (e.g., 100 for 0-1 → 0-100%)

    bool wrapNeedle;            // For altitude: needle wraps every (maxValue) feet
    float wrapModulo;           // Modulo value for wrapping (e.g., 1000 for altitude)
    bool customRenderer;        // If true, gauge provides its own render method
};

// ── Abstract base for gauges ────────────────────────────────────────
class GaugeBase {
public:
    virtual ~GaugeBase() = default;
    virtual const GaugeConfig& getConfig() const = 0;

    // Override for custom rendering (heading gauge compass card, etc.)
    // cx, cy = center coordinates of the cell being rendered into.
    // Returns true if custom rendering was handled, false to use default.
    virtual bool customRender(void* /*sprite*/, float /*value*/, int /*cx*/, int /*cy*/) { return false; }
};
