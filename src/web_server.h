#pragma once

#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <ArduinoJson.h>
#include <Update.h>
#include "config.h"

// ── Thread-safe command queue (web → main loop) ────────────────────
enum WebCommandType {
    CMD_NONE,
    CMD_SET_GAUGE,        // cell + gaugeIndex
    CMD_SWITCH_VIEW,      // viewIndex
    CMD_RESTART,
    CMD_WIFI_SCAN,        // trigger WiFi scan from Core 1
    CMD_TOGGLE_CONTROL,   // controlIndex (slot index)
    CMD_SET_CONTROL,      // cell (slot) + controlIndex (registry index)
};

struct WebCommand {
    WebCommandType type = CMD_NONE;
    int cell = 0;
    int gaugeIndex = 0;
    int viewIndex = 0;
    int controlIndex = 0;
};

class WebServerManager {
public:
    void begin(bool apMode = false);
    void stop();

    // Call from main loop (Core 1) to drain commands
    bool dequeueCommand(WebCommand& cmd);

    // DNS processing for captive portal (call in AP mode loop)
    void processDNS();

    // OTA state
    bool otaInProgress() const { return _otaInProgress; }
    int otaProgress() const { return _otaProgress; }
    bool otaComplete() const { return _otaComplete; }

    // Store WiFi scan results (called from main loop on Core 1)
    void setScanResults(const String& json) {
        portENTER_CRITICAL(&_scanMux);
        _scanJson = json;
        portEXIT_CRITICAL(&_scanMux);
        _scanDone = true;
    }

private:
    AsyncWebServer _server{80};
    DNSServer _dns;
    bool _apMode = false;
    bool _running = false;

    // Thread-safe command queue (single slot with spinlock)
    portMUX_TYPE _cmdMux = portMUX_INITIALIZER_UNLOCKED;
    WebCommand _pendingCmd;
    bool _hasPendingCmd = false;

    // OTA state (volatile for cross-core visibility)
    volatile bool _otaInProgress = false;
    volatile int _otaProgress = 0;
    volatile bool _otaComplete = false;
    size_t _otaTotal = 0;

    // WiFi scan results (written by main loop, read by web handler)
    volatile bool _scanDone = false;
    String _scanJson;
    portMUX_TYPE _scanMux = portMUX_INITIALIZER_UNLOCKED;

    void enqueueCommand(const WebCommand& cmd);
    void setupRoutes();
    void setupCaptivePortal();
};
