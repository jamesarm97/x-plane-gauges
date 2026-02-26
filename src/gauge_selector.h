#pragma once

#include <M5Dial.h>
#include <Preferences.h>
#include "gauges/gauge_base.h"
#include "gauges/gauge_registry.h"
#include "xplane_client.h"

class GaugeSelector {
public:
    void begin(XPlaneClient* client);
    void update();                    // Call each frame to process encoder input
    void draw(M5Canvas& canvas);     // Draw selection overlay into sprite if active

    int  currentIndex() const { return _currentIndex; }
    GaugeBase* currentGauge() const { return g_gauges[_currentIndex]; }
    bool selectionChanged();          // Returns true once after gauge changes
    bool isSelecting() const { return _selecting; }

private:
    XPlaneClient* _client = nullptr;
    Preferences _prefs;
    int  _currentIndex = 0;
    int  _pendingIndex = 0;
    bool _selecting = false;
    bool _changed = false;
    unsigned long _lastInputTime = 0;
    long _lastEncoderPos = 0;

    void confirmSelection();
    void saveIndex(int index);
    int  loadIndex();
};
