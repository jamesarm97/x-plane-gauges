#pragma once

#include <M5Dial.h>
#include "config.h"
#include "gauges/gauge_base.h"

class WarningBeeper {
public:
    void begin() {
        M5Dial.Speaker.setVolume(WARNING_VOLUME);
        _inRedZone = false;
        _beepOn = false;
        _muted = false;
        _lastToggleMs = 0;
    }

    void update(const GaugeConfig& cfg, float value, bool hasData) {
        if (!hasData || cfg.customRenderer) {
            if (_inRedZone) stop();
            return;
        }

        bool inRed = false;
        for (size_t i = 0; i < cfg.arcCount; i++) {
            if (cfg.arcs[i].color == COLOR_ARC_RED &&
                value >= cfg.arcs[i].startValue &&
                value <= cfg.arcs[i].endValue) {
                inRed = true;
                break;
            }
        }

        if (inRed && !_inRedZone) {
            _inRedZone = true;
            if (!_muted) {
                _beepOn = true;
                _lastToggleMs = millis();
                M5Dial.Speaker.tone(WARNING_BEEP_FREQ, WARNING_BEEP_ON_MS);
            }
        } else if (!inRed && _inRedZone) {
            stop();
        } else if (_inRedZone && !_muted) {
            unsigned long now = millis();
            unsigned long elapsed = now - _lastToggleMs;
            if (_beepOn && elapsed >= WARNING_BEEP_ON_MS) {
                _beepOn = false;
                _lastToggleMs = now;
            } else if (!_beepOn && elapsed >= WARNING_BEEP_OFF_MS) {
                _beepOn = true;
                _lastToggleMs = now;
                M5Dial.Speaker.tone(WARNING_BEEP_FREQ, WARNING_BEEP_ON_MS);
            }
        }
    }

    void toggleMute() {
        _muted = !_muted;
        if (_muted) {
            _beepOn = false;
            M5Dial.Speaker.stop();
        } else if (_inRedZone) {
            _beepOn = true;
            _lastToggleMs = millis();
            M5Dial.Speaker.tone(WARNING_BEEP_FREQ, WARNING_BEEP_ON_MS);
        }
    }

    bool isMuted() const { return _muted; }

    void stop() {
        _inRedZone = false;
        _beepOn = false;
        M5Dial.Speaker.stop();
    }

private:
    bool _inRedZone = false;
    bool _beepOn = false;
    bool _muted = false;
    unsigned long _lastToggleMs = 0;
};
