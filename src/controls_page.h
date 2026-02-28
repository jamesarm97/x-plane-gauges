#pragma once

#include "xplane_client.h"

// Forward declare LovyanGFX sprite
namespace lgfx { inline namespace v1 { class LGFX_Sprite; } }
using LGFX_Sprite = lgfx::v1::LGFX_Sprite;

struct ControlDef {
    const char* label;
    const char* cmdOn;
    const char* cmdOff;   // nullptr = toggle (cmdOn used for both)
    bool isMultiCmd;      // true for DOORS (sends 4 commands)
    const char* category;
};

// ── Control Registry ─────────────────────────────────────────────
static constexpr int CTRL_REGISTRY_COUNT = 22;
static constexpr int CTRL_SLOT_COUNT = 18;
static constexpr int CTRL_COLS = 6;
static constexpr int CTRL_ROWS = 3;

// Defined in controls_page.cpp
extern const ControlDef g_controlRegistry[CTRL_REGISTRY_COUNT];
extern const int g_defaultAssignments[CTRL_SLOT_COUNT];

class ControlsPage {
public:
    void begin(XPlaneClient* xplane);
    void draw(LGFX_Sprite& fb);
    bool handleTouch(int x, int y, bool pressed, bool justPressed);
    void resetStates();
    void toggle(int slot);

    // Slot assignment
    void setSlot(int slot, int registryIndex);
    int getAssignment(int slot) const;
    bool getState(int slot) const;
    const char* getSlotLabel(int slot) const;

    void saveToNVS();
    void loadFromNVS();

private:
    static constexpr int HEADER_H = 50;

    XPlaneClient* _xplane = nullptr;
    int  _assignments[CTRL_SLOT_COUNT];
    bool _states[CTRL_SLOT_COUNT] = {};
    int  _pressedIdx = -1;
    bool _wasTouching = false;

    int hitTest(int x, int y);
    void sendControl(int slot, bool on);
};
