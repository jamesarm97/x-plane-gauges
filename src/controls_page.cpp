#include "controls_page.h"
#include "config.h"
#include "display_driver.h"
#include <Preferences.h>
#include <cstring>

// ── Control Registry (22 available controls) ─────────────────────
const ControlDef g_controlRegistry[CTRL_REGISTRY_COUNT] = {
    // Electrical (0-6)
    { "BATTERY 1",  "sim/electrical/battery_1_on",     "sim/electrical/battery_1_off",     false, "Electrical" },
    { "BATTERY 2",  "sim/electrical/battery_2_on",     "sim/electrical/battery_2_off",     false, "Electrical" },
    { "GENERATOR 1","sim/electrical/generator_1_on",   "sim/electrical/generator_1_off",   false, "Electrical" },
    { "GENERATOR 2","sim/electrical/generator_2_on",   "sim/electrical/generator_2_off",   false, "Electrical" },
    { "APU",        "sim/electrical/APU_on",           "sim/electrical/APU_off",           false, "Electrical" },
    { "AVIONICS",   "sim/systems/avionics_on",         "sim/systems/avionics_off",         false, "Electrical" },
    { "FUEL PUMP",  "sim/fuel/fuel_pump_1_on",         "sim/fuel/fuel_pump_1_off",         false, "Electrical" },

    // Anti-Ice (7-10)
    { "PITOT HEAT", "sim/ice/pitot_heat0_on",          "sim/ice/pitot_heat0_off",          false, "Anti-Ice" },
    { "PROP HEAT",  "sim/ice/prop_heat_on",            "sim/ice/prop_heat_off",            false, "Anti-Ice" },
    { "WINDOW HEAT","sim/ice/window_heat_on",          "sim/ice/window_heat_off",          false, "Anti-Ice" },
    { "CARB HEAT",  "sim/engines/carb_heat_toggle",    nullptr,                            false, "Anti-Ice" },

    // Systems (11-14)
    { "DOORS",      "sim/flight_controls/door_close_", "sim/flight_controls/door_open_",   true,  "Systems" },
    { "PARK BRAKE", "sim/flight_controls/brakes_toggle_max", nullptr,                      false, "Systems" },
    { "GEAR",       "sim/flight_controls/landing_gear_toggle", nullptr,                    false, "Systems" },
    { "MAGNETOS",   "sim/magnetos/magnetos_both_1",    "sim/magnetos/magnetos_off_1",      false, "Engine" },

    // Lights (15-21)
    { "NAV LTS",    "sim/lights/nav_lights_toggle",    nullptr,                            false, "Lights" },
    { "BEACON",     "sim/lights/beacon_lights_toggle",  nullptr,                           false, "Lights" },
    { "STROBE",     "sim/lights/strobe_lights_toggle",  nullptr,                           false, "Lights" },
    { "LANDING LT", "sim/lights/landing_lights_toggle", nullptr,                           false, "Lights" },
    { "TAXI LT",    "sim/lights/taxi_lights_on",       "sim/lights/taxi_lights_off",       false, "Lights" },
    { "SPOT LT",    "sim/lights/spot_lights_on",       "sim/lights/spot_lights_off",       false, "Lights" },
    { "PANEL LT",   "sim/lights/panel_lights_toggle",   nullptr,                           false, "Lights" },
};

// Default slot assignments (indices into registry)
const int g_defaultAssignments[CTRL_SLOT_COUNT] = {
    // Row 1 — Electrical
    0,  1,  2,  4,  5,  6,
    // Row 2 — Systems/Anti-Ice
    7,  10, 9,  8,  11, 14,
    // Row 3 — Lights
    15, 16, 17, 18, 19, 20,
};

// ── NVS persistence ──────────────────────────────────────────────
static const char* NVS_NAMESPACE = "controls";

void ControlsPage::loadFromNVS() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);  // read-only
    for (int i = 0; i < CTRL_SLOT_COUNT; i++) {
        char key[8];
        snprintf(key, sizeof(key), "ctrl%d", i);
        _assignments[i] = prefs.getUChar(key, 0xFF);
        if (_assignments[i] >= CTRL_REGISTRY_COUNT) {
            _assignments[i] = g_defaultAssignments[i];
        }
    }
    prefs.end();
}

void ControlsPage::saveToNVS() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);  // read-write
    for (int i = 0; i < CTRL_SLOT_COUNT; i++) {
        char key[8];
        snprintf(key, sizeof(key), "ctrl%d", i);
        prefs.putUChar(key, (uint8_t)_assignments[i]);
    }
    prefs.end();
}

// ── Public API ───────────────────────────────────────────────────

void ControlsPage::begin(XPlaneClient* xplane) {
    _xplane = xplane;
    loadFromNVS();
    resetStates();
}

void ControlsPage::resetStates() {
    memset(_states, 0, sizeof(_states));
    // Parking brake (registry index 12) typically starts set in X-Plane
    for (int i = 0; i < CTRL_SLOT_COUNT; i++) {
        if (_assignments[i] == 12) _states[i] = true;
    }
    _pressedIdx = -1;
}

void ControlsPage::setSlot(int slot, int registryIndex) {
    if (slot < 0 || slot >= CTRL_SLOT_COUNT) return;
    if (registryIndex < 0 || registryIndex >= CTRL_REGISTRY_COUNT) return;
    _assignments[slot] = registryIndex;
    _states[slot] = false;  // Reset state when reassigning
    saveToNVS();
}

int ControlsPage::getAssignment(int slot) const {
    if (slot < 0 || slot >= CTRL_SLOT_COUNT) return -1;
    return _assignments[slot];
}

bool ControlsPage::getState(int slot) const {
    if (slot < 0 || slot >= CTRL_SLOT_COUNT) return false;
    return _states[slot];
}

const char* ControlsPage::getSlotLabel(int slot) const {
    if (slot < 0 || slot >= CTRL_SLOT_COUNT) return "";
    return g_controlRegistry[_assignments[slot]].label;
}

void ControlsPage::toggle(int slot) {
    if (slot < 0 || slot >= CTRL_SLOT_COUNT) return;
    _states[slot] = !_states[slot];
    sendControl(slot, _states[slot]);
}

// ── Touch ────────────────────────────────────────────────────────

int ControlsPage::hitTest(int x, int y) {
    if (y < HEADER_H) return -1;

    int btnW = SCREEN_WIDTH / CTRL_COLS;
    int btnH = (SCREEN_HEIGHT - HEADER_H) / CTRL_ROWS;

    int col = x / btnW;
    int row = (y - HEADER_H) / btnH;

    if (col < 0 || col >= CTRL_COLS || row < 0 || row >= CTRL_ROWS) return -1;
    return row * CTRL_COLS + col;
}

void ControlsPage::sendControl(int slot, bool on) {
    if (!_xplane || slot < 0 || slot >= CTRL_SLOT_COUNT) return;

    const ControlDef& ctrl = g_controlRegistry[_assignments[slot]];

    if (ctrl.isMultiCmd) {
        // DOORS: send 4 commands (door_close_1 through door_close_4 or door_open_1..4)
        const char* base = on ? ctrl.cmdOn : ctrl.cmdOff;
        char cmd[64];
        for (int i = 1; i <= 4; i++) {
            snprintf(cmd, sizeof(cmd), "%s%d", base, i);
            _xplane->sendCMND(cmd);
        }
    } else if (ctrl.cmdOff == nullptr) {
        // Toggle command — always send the same command
        _xplane->sendCMND(ctrl.cmdOn);
    } else {
        // On/off pair
        _xplane->sendCMND(on ? ctrl.cmdOn : ctrl.cmdOff);
    }
}

bool ControlsPage::handleTouch(int x, int y, bool pressed, bool justPressed) {
    if (justPressed) {
        _pressedIdx = hitTest(x, y);
    }

    if (!pressed && _wasTouching && _pressedIdx >= 0) {
        // Release on a button — toggle it
        int releaseIdx = hitTest(x, y);
        if (releaseIdx == _pressedIdx) {
            _states[_pressedIdx] = !_states[_pressedIdx];
            sendControl(_pressedIdx, _states[_pressedIdx]);
        }
        _pressedIdx = -1;
    }

    _wasTouching = pressed;
    return false;
}

// ── Drawing (6x3 grid) ──────────────────────────────────────────

void ControlsPage::draw(LGFX_Sprite& fb) {
    int btnW = SCREEN_WIDTH / CTRL_COLS;
    int btnH = (SCREEN_HEIGHT - HEADER_H) / CTRL_ROWS;
    int pad = 3;

    // Header
    fb.fillRect(0, 0, SCREEN_WIDTH, HEADER_H, COLOR_BG);
    fb.setTextDatum(middle_center);
    fb.setTextColor(COLOR_TITLE);
    fb.setTextSize(2.5);
    fb.drawString("PRE-FLIGHT CONTROLS", SCREEN_WIDTH / 2, HEADER_H / 2);
    fb.drawFastHLine(0, HEADER_H - 1, SCREEN_WIDTH, COLOR_GRID_LINE);

    // Toggle switches
    for (int i = 0; i < CTRL_SLOT_COUNT; i++) {
        int col = i % CTRL_COLS;
        int row = i / CTRL_COLS;
        int bx = col * btnW + pad;
        int by = HEADER_H + row * btnH + pad;
        int bw = btnW - pad * 2;
        int bh = btnH - pad * 2;

        bool on = _states[i];
        bool pressing = (i == _pressedIdx);
        const char* label = g_controlRegistry[_assignments[i]].label;

        // Cell background
        uint16_t bgColor = pressing ? COLOR_CTRL_PRESS : COLOR_CTRL_BG;
        fb.fillRoundRect(bx, by, bw, bh, 6, bgColor);

        // Subtle border
        fb.drawRoundRect(bx, by, bw, bh, 6, COLOR_CTRL_OFF);

        int cx = bx + bw / 2;

        // Label above the toggle
        fb.setTextDatum(middle_center);
        fb.setTextColor(on ? COLOR_CTRL_ON : COLOR_LABEL);
        fb.setTextSize(1.5);
        fb.drawString(label, cx, by + 28);

        // Toggle switch track (pill shape)
        int trackW = 52;
        int trackH = 24;
        int trackX = cx - trackW / 2;
        int trackY = by + bh / 2 + 2;
        int trackR = trackH / 2;

        uint16_t trackColor = on ? COLOR_CTRL_ON : 0x3186;  // green or dark gray
        fb.fillRoundRect(trackX, trackY, trackW, trackH, trackR, trackColor);

        // Knob (slides left/right)
        int knobR = trackH / 2 - 2;
        int knobX = on ? (trackX + trackW - trackR) : (trackX + trackR);
        int knobY = trackY + trackH / 2;
        fb.fillCircle(knobX, knobY, knobR, COLOR_LABEL);

        // ON/OFF text below toggle
        fb.setTextDatum(middle_center);
        fb.setTextColor(on ? COLOR_CTRL_ON : COLOR_CTRL_OFF);
        fb.setTextSize(1.2);
        fb.drawString(on ? "ON" : "OFF", cx, trackY + trackH + 16);
    }
}
