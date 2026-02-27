#pragma once

#include <Arduino.h>
#include "config.h"
#include "dashboard_layout.h"

class BuzzerDriver {
public:
    void begin() {
        // Configure LEDC for PWM tone generation
        ledcSetup(BUZZER_LEDC_CHANNEL, WARNING_BEEP_FREQ, 8);  // 8-bit resolution
        ledcAttachPin(BUZZER_GPIO, BUZZER_LEDC_CHANNEL);
        ledcWrite(BUZZER_LEDC_CHANNEL, 0);  // Start silent

        _inRedZone = false;
        _beepOn = false;
        _muted = false;
        _lastToggleMs = 0;
    }

    void updateAll(DashboardLayout& layout) {
        bool inRed = layout.anyRedZone();

        if (inRed && !_inRedZone) {
            _inRedZone = true;
            if (!_muted) {
                _beepOn = true;
                _lastToggleMs = millis();
                toneOn();
            }
        } else if (!inRed && _inRedZone) {
            stop();
        } else if (_inRedZone && !_muted) {
            unsigned long now = millis();
            unsigned long elapsed = now - _lastToggleMs;
            if (_beepOn && elapsed >= WARNING_BEEP_ON_MS) {
                _beepOn = false;
                _lastToggleMs = now;
                toneOff();
            } else if (!_beepOn && elapsed >= WARNING_BEEP_OFF_MS) {
                _beepOn = true;
                _lastToggleMs = now;
                toneOn();
            }
        }
    }

    void toggleMute() {
        _muted = !_muted;
        if (_muted) {
            _beepOn = false;
            toneOff();
        } else if (_inRedZone) {
            _beepOn = true;
            _lastToggleMs = millis();
            toneOn();
        }
    }

    bool isMuted() const { return _muted; }

    void stop() {
        _inRedZone = false;
        _beepOn = false;
        toneOff();
    }

private:
    bool _inRedZone = false;
    bool _beepOn = false;
    bool _muted = false;
    unsigned long _lastToggleMs = 0;

    void toneOn() {
        ledcWriteTone(BUZZER_LEDC_CHANNEL, WARNING_BEEP_FREQ);
        ledcWrite(BUZZER_LEDC_CHANNEL, 128);  // 50% duty cycle
    }

    void toneOff() {
        ledcWrite(BUZZER_LEDC_CHANNEL, 0);
    }
};
