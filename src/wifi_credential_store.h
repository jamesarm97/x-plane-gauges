#pragma once

#include <Preferences.h>
#include "config.h"

struct StoredWiFiNetwork {
    char ssid[33];
    char password[65];
};

class WiFiCredentialStore {
public:
    void load() {
        Preferences prefs;
        prefs.begin("wifi", true);
        _count = prefs.getUChar("count", 0);
        if (_count > WIFI_NVS_MAX) _count = WIFI_NVS_MAX;
        for (int i = 0; i < _count; i++) {
            char sk[4], pk[4];
            snprintf(sk, sizeof(sk), "s%d", i);
            snprintf(pk, sizeof(pk), "p%d", i);
            String s = prefs.getString(sk, "");
            String p = prefs.getString(pk, "");
            strncpy(_networks[i].ssid, s.c_str(), 32);
            _networks[i].ssid[32] = '\0';
            strncpy(_networks[i].password, p.c_str(), 64);
            _networks[i].password[64] = '\0';
        }
        prefs.end();
    }

    bool save(const char* ssid, const char* password) {
        // Update existing entry if SSID matches
        for (int i = 0; i < _count; i++) {
            if (strcmp(_networks[i].ssid, ssid) == 0) {
                strncpy(_networks[i].password, password, 64);
                _networks[i].password[64] = '\0';
                _persist();
                return true;
            }
        }
        // Add new entry
        if (_count >= WIFI_NVS_MAX) return false;
        strncpy(_networks[_count].ssid, ssid, 32);
        _networks[_count].ssid[32] = '\0';
        strncpy(_networks[_count].password, password, 64);
        _networks[_count].password[64] = '\0';
        _count++;
        _persist();
        return true;
    }

    bool remove(const char* ssid) {
        for (int i = 0; i < _count; i++) {
            if (strcmp(_networks[i].ssid, ssid) == 0) {
                // Shift remaining entries down
                for (int j = i; j < _count - 1; j++) {
                    _networks[j] = _networks[j + 1];
                }
                _count--;
                _persist();
                return true;
            }
        }
        return false;
    }

    int count() const { return _count; }
    const StoredWiFiNetwork& network(int i) const { return _networks[i]; }

private:
    StoredWiFiNetwork _networks[WIFI_NVS_MAX];
    int _count = 0;

    void _persist() {
        Preferences prefs;
        prefs.begin("wifi", false);
        prefs.putUChar("count", _count);
        for (int i = 0; i < _count; i++) {
            char sk[4], pk[4];
            snprintf(sk, sizeof(sk), "s%d", i);
            snprintf(pk, sizeof(pk), "p%d", i);
            prefs.putString(sk, _networks[i].ssid);
            prefs.putString(pk, _networks[i].password);
        }
        // Clear old slots
        for (int i = _count; i < WIFI_NVS_MAX; i++) {
            char sk[4], pk[4];
            snprintf(sk, sizeof(sk), "s%d", i);
            snprintf(pk, sizeof(pk), "p%d", i);
            prefs.remove(sk);
            prefs.remove(pk);
        }
        prefs.end();
    }
};
