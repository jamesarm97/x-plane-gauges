#include "web_server.h"
#include "web_ui.h"
#include "wifi_credential_store.h"
#include "gauges/gauge_registry.h"
#include "gauges/gauge_base.h"
#include "view_manager.h"
#include "dashboard_layout.h"
#include "controls_page.h"
#include <WiFi.h>

// External references (owned by main.cpp)
extern DashboardLayout g_layout;
extern WiFiCredentialStore g_credStore;
extern ViewManager g_viewManager;
extern ControlsPage g_controlsPage;

void WebServerManager::begin(bool apMode) {
    if (_running) return;
    _apMode = apMode;

    setupRoutes();
    if (apMode) {
        setupCaptivePortal();
        _dns.start(53, "*", WiFi.softAPIP());
    }

    _server.begin();
    _running = true;
    Serial.printf("[Web] Server started (%s mode)\n", apMode ? "AP" : "STA");
}

void WebServerManager::stop() {
    if (!_running) return;
    _server.end();
    if (_apMode) _dns.stop();
    _running = false;
}

void WebServerManager::processDNS() {
    if (_apMode) _dns.processNextRequest();
}

void WebServerManager::enqueueCommand(const WebCommand& cmd) {
    portENTER_CRITICAL(&_cmdMux);
    _pendingCmd = cmd;
    _hasPendingCmd = true;
    portEXIT_CRITICAL(&_cmdMux);
}

bool WebServerManager::dequeueCommand(WebCommand& cmd) {
    portENTER_CRITICAL(&_cmdMux);
    if (!_hasPendingCmd) {
        portEXIT_CRITICAL(&_cmdMux);
        return false;
    }
    cmd = _pendingCmd;
    _hasPendingCmd = false;
    portEXIT_CRITICAL(&_cmdMux);
    return true;
}

// ── Route setup ─────────────────────────────────────────────────────

void WebServerManager::setupRoutes() {
    // Serve gzipped web UI
    _server.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
        AsyncWebServerResponse* resp = req->beginResponse(200, "text/html", WEB_UI_HTML_GZ, WEB_UI_HTML_GZ_LEN);
        resp->addHeader("Content-Encoding", "gzip");
        req->send(resp);
    });

    // ── Status ──────────────────────────────────────────────────────
    _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["version"] = FW_VERSION;
        doc["ip"] = _apMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
        doc["ap_mode"] = _apMode;
        doc["uptime"] = millis() / 1000;
        doc["heap"] = ESP.getFreeHeap();
        doc["psram"] = ESP.getFreePsram();
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    // ── Gauge list ──────────────────────────────────────────────────
    _server.on("/api/gauges", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        // Category names matching gauge_picker.h order
        static const char* categories[] = {
            "Flight Instruments", "Controls", "Engine",
            "Fuel & Electrical", "Navigation", "Advanced"
        };
        static const int catStart[] = { 0, 7, 9, 15, 18, 21 };
        static const int catEnd[]   = { 7, 9, 15, 18, 21, GAUGE_COUNT };
        static const int catCount = 6;

        for (int c = 0; c < catCount; c++) {
            JsonObject cat = arr.add<JsonObject>();
            cat["category"] = categories[c];
            JsonArray gauges = cat["gauges"].to<JsonArray>();
            for (int i = catStart[c]; i < catEnd[c]; i++) {
                JsonObject g = gauges.add<JsonObject>();
                g["index"] = i;
                g["name"] = g_gauges[i]->getConfig().title;
            }
        }

        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    // ── Layout (GET) ────────────────────────────────────────────────
    _server.on("/api/layout", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        doc["view"] = g_viewManager.currentView();
        JsonArray cells = doc["cells"].to<JsonArray>();
        for (int i = 0; i < CELL_COUNT; i++) {
            JsonObject cell = cells.add<JsonObject>();
            int gi = g_layout.gaugeIndex(i);
            cell["gauge_index"] = gi;
            cell["name"] = g_gauges[gi]->getConfig().title;
        }
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    // ── Layout (POST) — set single cell gauge ───────────────────────
    _server.on("/api/layout", HTTP_POST, [](AsyncWebServerRequest* req) {
        req->send(400, "application/json", "{\"error\":\"no body\"}");
    }, nullptr, [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
            req->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }
        if (!doc["cell"].is<int>() || !doc["gauge_index"].is<int>()) {
            req->send(400, "application/json", "{\"error\":\"missing cell or gauge_index\"}");
            return;
        }
        int cell = doc["cell"];
        int gi = doc["gauge_index"];
        if (cell < 0 || cell >= CELL_COUNT || gi < 0 || gi >= GAUGE_COUNT) {
            req->send(400, "application/json", "{\"error\":\"out of range\"}");
            return;
        }
        WebCommand cmd;
        cmd.type = CMD_SET_GAUGE;
        cmd.cell = cell;
        cmd.gaugeIndex = gi;
        enqueueCommand(cmd);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── Views (GET) ─────────────────────────────────────────────────
    _server.on("/api/views", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (int v = 0; v < VIEW_COUNT; v++) {
            JsonObject view = arr.add<JsonObject>();
            view["index"] = v;
            view["name"] = g_views[v].name;
            JsonArray gauges = view["gauges"].to<JsonArray>();
            for (int i = 0; i < CELL_COUNT; i++) {
                gauges.add(g_views[v].gauges[i]);
            }
        }
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    // ── Views (POST) — switch view ──────────────────────────────────
    _server.on("/api/views", HTTP_POST, [](AsyncWebServerRequest* req) {
        req->send(400, "application/json", "{\"error\":\"no body\"}");
    }, nullptr, [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
            req->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }
        int vi = doc["view"] | -1;
        if (vi < 0 || vi >= VIEW_COUNT) {
            req->send(400, "application/json", "{\"error\":\"invalid view\"}");
            return;
        }
        WebCommand cmd;
        cmd.type = CMD_SWITCH_VIEW;
        cmd.viewIndex = vi;
        enqueueCommand(cmd);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── Controls Registry (GET) — all available controls ────────────
    _server.on("/api/controls/registry", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < CTRL_REGISTRY_COUNT; i++) {
            JsonObject ctrl = arr.add<JsonObject>();
            ctrl["index"] = i;
            ctrl["label"] = g_controlRegistry[i].label;
            ctrl["category"] = g_controlRegistry[i].category;
        }
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    // ── Controls Assign (POST) — change slot assignment ─────────────
    _server.on("/api/controls/assign", HTTP_POST, [](AsyncWebServerRequest* req) {
        req->send(400, "application/json", "{\"error\":\"no body\"}");
    }, nullptr, [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
            req->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }
        int slot = doc["slot"] | -1;
        int control = doc["control"] | -1;
        if (slot < 0 || slot >= CTRL_SLOT_COUNT || control < 0 || control >= CTRL_REGISTRY_COUNT) {
            req->send(400, "application/json", "{\"error\":\"out of range\"}");
            return;
        }
        WebCommand cmd;
        cmd.type = CMD_SET_CONTROL;
        cmd.cell = slot;
        cmd.controlIndex = control;
        enqueueCommand(cmd);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── Controls (GET) — list slot assignments and states ────────────
    _server.on("/api/controls", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < CTRL_SLOT_COUNT; i++) {
            JsonObject ctrl = arr.add<JsonObject>();
            ctrl["slot"] = i;
            ctrl["control"] = g_controlsPage.getAssignment(i);
            ctrl["label"] = g_controlsPage.getSlotLabel(i);
            ctrl["on"] = g_controlsPage.getState(i);
        }
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    // ── Controls (POST) — toggle a control slot ─────────────────────
    _server.on("/api/controls", HTTP_POST, [](AsyncWebServerRequest* req) {
        req->send(400, "application/json", "{\"error\":\"no body\"}");
    }, nullptr, [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
            req->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }
        int slot = doc["slot"] | -1;
        if (slot < 0 || slot >= CTRL_SLOT_COUNT) {
            req->send(400, "application/json", "{\"error\":\"invalid slot\"}");
            return;
        }
        WebCommand cmd;
        cmd.type = CMD_TOGGLE_CONTROL;
        cmd.controlIndex = slot;
        enqueueCommand(cmd);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── WiFi scan — register BEFORE /api/wifi to avoid prefix match ─

    // ── WiFi scan (GET) — request scan via command queue ───────────
    _server.on("/api/wifi/scan/results", HTTP_GET, [this](AsyncWebServerRequest* req) {
        if (!_scanDone) {
            req->send(200, "application/json", "{\"scanning\":true}");
            return;
        }
        portENTER_CRITICAL(&_scanMux);
        String json = _scanJson;
        portEXIT_CRITICAL(&_scanMux);
        req->send(200, "application/json", json);
    });

    _server.on("/api/wifi/scan", HTTP_GET, [this](AsyncWebServerRequest* req) {
        _scanDone = false;
        WebCommand cmd;
        cmd.type = CMD_WIFI_SCAN;
        enqueueCommand(cmd);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── WiFi (GET) — list saved networks ────────────────────────────
    _server.on("/api/wifi", HTTP_GET, [](AsyncWebServerRequest* req) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        for (int i = 0; i < g_credStore.count(); i++) {
            arr.add(g_credStore.network(i).ssid);
        }
        String json;
        serializeJson(doc, json);
        req->send(200, "application/json", json);
    });

    // ── WiFi (POST) — save network + restart ────────────────────────
    _server.on("/api/wifi", HTTP_POST, [](AsyncWebServerRequest* req) {
        req->send(400, "application/json", "{\"error\":\"no body\"}");
    }, nullptr, [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
            req->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }
        const char* ssid = doc["ssid"];
        const char* pass = doc["password"];
        if (!ssid || !pass) {
            req->send(400, "application/json", "{\"error\":\"missing ssid or password\"}");
            return;
        }
        if (!g_credStore.save(ssid, pass)) {
            req->send(400, "application/json", "{\"error\":\"max networks reached\"}");
            return;
        }
        req->send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
        // Delay restart so response is sent
        WebCommand cmd;
        cmd.type = CMD_RESTART;
        enqueueCommand(cmd);
    });

    // ── WiFi (DELETE) — remove network ──────────────────────────────
    _server.on("/api/wifi", HTTP_DELETE, [](AsyncWebServerRequest* req) {
        req->send(400, "application/json", "{\"error\":\"no body\"}");
    }, nullptr, [this](AsyncWebServerRequest* req, uint8_t* data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        if (deserializeJson(doc, data, len)) {
            req->send(400, "application/json", "{\"error\":\"invalid json\"}");
            return;
        }
        const char* ssid = doc["ssid"];
        if (!ssid) {
            req->send(400, "application/json", "{\"error\":\"missing ssid\"}");
            return;
        }
        g_credStore.remove(ssid);
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // ── OTA firmware upload ─────────────────────────────────────────
    _server.on("/ota", HTTP_POST,
        // Response handler (called after upload completes)
        [this](AsyncWebServerRequest* req) {
            if (Update.hasError()) {
                req->send(500, "application/json", "{\"error\":\"update failed\"}");
                _otaInProgress = false;
            } else {
                req->send(200, "application/json", "{\"ok\":true,\"restarting\":true}");
                _otaComplete = true;
            }
        },
        // Upload handler (called for each chunk)
        [this](AsyncWebServerRequest* req, const String& filename, size_t index, uint8_t* data, size_t len, bool final) {
            if (index == 0) {
                Serial.printf("[OTA] Begin: %s\n", filename.c_str());
                _otaInProgress = true;
                _otaProgress = 0;
                _otaTotal = req->contentLength();
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }
            if (Update.isRunning()) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
                if (_otaTotal > 0) {
                    _otaProgress = (int)(((index + len) * 100) / _otaTotal);
                }
            }
            if (final) {
                if (Update.end(true)) {
                    Serial.printf("[OTA] Success: %u bytes\n", index + len);
                    _otaProgress = 100;
                } else {
                    Update.printError(Serial);
                }
            }
        }
    );
}

// ── Captive portal routes ───────────────────────────────────────────

void WebServerManager::setupCaptivePortal() {
    // iOS
    _server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    });
    // Android
    _server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    });
    _server.on("/gen_204", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    });
    // Windows
    _server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    });
    _server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* req) {
        req->redirect("http://192.168.4.1/");
    });

    // Catch-all: redirect non-API requests to root
    _server.onNotFound([](AsyncWebServerRequest* req) {
        if (req->url().startsWith("/api/")) {
            req->send(404, "application/json", "{\"error\":\"not found\"}");
        } else {
            req->redirect("http://192.168.4.1/");
        }
    });
}
