#pragma once

#include "gauge_base.h"
#include "airspeed_gauge.h"
#include "altitude_gauge.h"
#include "vsi_gauge.h"
#include "heading_gauge.h"
#include "bank_gauge.h"
#include "pitch_gauge.h"
#include "throttle_gauge.h"
#include "flap_gauge.h"
#include "gforce_gauge.h"
#include "rpm_gauge.h"
#include "manifold_gauge.h"
#include "egt_gauge.h"
#include "cht_gauge.h"
#include "oil_temp_gauge.h"
#include "oil_press_gauge.h"
#include "fuel_gauge.h"
#include "fuel_flow_gauge.h"
#include "volts_gauge.h"
#include "lat_gauge.h"
#include "lon_gauge.h"
#include "position_gauge.h"

// ── Static gauge instances ──────────────────────────────────────────
// Flight instruments
static AirspeedGauge  g_airspeedGauge;
static AltitudeGauge  g_altitudeGauge;
static VSIGauge       g_vsiGauge;
static HeadingGauge   g_headingGauge;
static BankGauge      g_bankGauge;
static PitchGauge     g_pitchGauge;
static GForceGauge    g_gforceGauge;

// Controls
static ThrottleGauge  g_throttleGauge;
static FlapGauge      g_flapGauge;

// Engine
static RPMGauge       g_rpmGauge;
static ManifoldGauge  g_manifoldGauge;
static EGTGauge       g_egtGauge;
static CHTGauge       g_chtGauge;
static OilTempGauge   g_oilTempGauge;
static OilPressGauge  g_oilPressGauge;

// Fuel & electrical
static FuelGauge      g_fuelGauge;
static FuelFlowGauge  g_fuelFlowGauge;
static VoltsGauge     g_voltsGauge;

// Navigation
static PositionGauge  g_positionGauge;
static LatGauge       g_latGauge;
static LonGauge       g_lonGauge;

// ── Gauge registry array ────────────────────────────────────────────
static GaugeBase* g_gauges[] = {
    // Flight instruments
    &g_airspeedGauge,
    &g_altitudeGauge,
    &g_vsiGauge,
    &g_headingGauge,
    &g_bankGauge,
    &g_pitchGauge,
    &g_gforceGauge,
    // Controls
    &g_throttleGauge,
    &g_flapGauge,
    // Engine
    &g_rpmGauge,
    &g_manifoldGauge,
    &g_egtGauge,
    &g_chtGauge,
    &g_oilTempGauge,
    &g_oilPressGauge,
    // Fuel & electrical
    &g_fuelGauge,
    &g_fuelFlowGauge,
    &g_voltsGauge,
    // Navigation
    &g_positionGauge,
    &g_latGauge,
    &g_lonGauge,
};

static constexpr int GAUGE_COUNT = sizeof(g_gauges) / sizeof(g_gauges[0]);
