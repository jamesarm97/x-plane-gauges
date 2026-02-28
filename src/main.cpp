#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "display_driver.h"
#include "config.h"
#include "wifi_manager.h"
#include "wifi_credentials.h"
#include "wifi_credential_store.h"
#include "xplane_discovery.h"
#include "xplane_client.h"
#include "gauge_renderer.h"
#include "dashboard_layout.h"
#include "gauge_picker.h"
#include "buzzer_driver.h"
#include "view_manager.h"
#include "web_server.h"
#include "controls_page.h"
#include "gauges/gauge_registry.h"

// ── Application State ───────────────────────────────────────────────
enum AppState {
    STATE_WIFI_CONNECTING,
    STATE_DISCOVERING,
    STATE_RUNNING,
    STATE_AP_MODE,
    STATE_OTA_UPDATING,
};

static AppState          g_state = STATE_WIFI_CONNECTING;
static WiFiManager       g_wifi;
static XPlaneDiscovery   g_discovery;
static XPlaneClient      g_xplane;
static GaugeRenderer     g_renderer;
DashboardLayout          g_layout;       // non-static: accessed by web_server.cpp
static GaugePicker       g_picker;
static BuzzerDriver      g_buzzer;
ViewManager              g_viewManager;  // non-static: accessed by web_server.cpp
WiFiCredentialStore      g_credStore;    // non-static: accessed by web_server.cpp
static WebServerManager  g_webServer;
ControlsPage             g_controlsPage;  // non-static: accessed by web_server.cpp

// ── Discovered X-Plane address ──────────────────────────────────────
static char g_xplaneIP[20] = {0};
static uint16_t g_xplanePort = XPLANE_PORT;

// ── Timing ──────────────────────────────────────────────────────────
static unsigned long g_lastFrameTime = 0;
static constexpr unsigned long XPLANE_TIMEOUT_MS = 2000;

// ── Touch state ─────────────────────────────────────────────────────
static bool g_touchActive = false;
static unsigned long g_touchStartMs = 0;
static int g_touchStartCell = -1;
static bool g_longPressTriggered = false;

// ── Mute flash ──────────────────────────────────────────────────────
static unsigned long g_muteFlashEnd = 0;

// ── Connection tracking for backlight dimming ───────────────────────
static bool g_xplaneConnected = false;

// ── Two-finger gesture tracking ─────────────────────────────────────
static bool g_twoFingerActive = false;
static int g_twoFingerStartX0 = 0, g_twoFingerStartX1 = 0;
static unsigned long g_twoFingerStartMs = 0;
static bool g_twoFingerSwipeTriggered = false;
static bool g_suppressNextTap = false;       // Suppress single-tap after two-finger gesture
static unsigned long g_viewChangeDebounce = 0;
static constexpr int SWIPE_THRESHOLD = 50;         // Pixels for swipe detection
static constexpr unsigned long VIEW_DEBOUNCE_MS = 500;
static constexpr unsigned long TWO_FINGER_TAP_MS = 800;  // Max duration for tap (GT911 reports ~490ms)

// ── View picker popup ───────────────────────────────────────────────
static bool g_viewPickerOpen = false;
static unsigned long g_viewPickerOpenTime = 0;
static constexpr unsigned long VIEW_PICKER_TIMEOUT_MS = 5000;

// ── Forward declarations ────────────────────────────────────────────
void handleWiFiConnecting();
void handleDiscovering();
void handleRunning();
void handleAPMode();
void handleOTAUpdating();
void startRunning();
void startAPMode();
void subscribeAll();
void subscribeCell(int cellIndex);
void unsubscribeCell(int cellIndex);
void handleTouchInput();
void processWebCommands();
void switchView(int viewIndex);
void drawViewPicker(LGFX_Sprite& fb);
void drawViewOverlay(LGFX_Sprite& fb);

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("[Boot] X-Plane Dashboard v" FW_VERSION);

    // Initialize display + backlight via CH422G
    display_init();
    Serial.println("[Boot] Display initialized");

    // Splash screen
    LGFX_Sprite splash;
    splash.setPsram(true);
    splash.setColorDepth(16);
    splash.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
    splash.fillSprite(COLOR_BG);
    splash.setTextDatum(middle_center);

    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;

    splash.setTextColor(COLOR_TITLE);
    splash.setTextSize(4.0);
    splash.drawString("X-Plane Dashboard", cx, cy - 40);

    splash.setTextColor(COLOR_DIAL_RIM);
    splash.setTextSize(2.5);
    splash.drawString("v" FW_VERSION, cx, cy + 20);

    splash.setTextColor(COLOR_LABEL);
    splash.setTextSize(2.0);
    splash.drawString("Initializing...", cx, cy + 65);

    splash.pushSprite(&g_display, 0, 0);
    splash.deleteSprite();
    delay(500);

    // Initialize renderer (creates PSRAM framebuffer)
    g_renderer.begin(&g_display);

    // Load dashboard layout from NVS (so web UI works before X-Plane connects)
    g_layout.begin();

    // Load stored WiFi credentials from NVS
    g_credStore.load();
    Serial.printf("[Boot] Loaded %d WiFi networks from NVS\n", g_credStore.count());

    // Start WiFi with hardcoded + NVS credentials
    g_wifi.begin(WIFI_NETWORKS, WIFI_NETWORK_COUNT);
    for (int i = 0; i < g_credStore.count(); i++) {
        g_wifi.addNetwork(g_credStore.network(i).ssid, g_credStore.network(i).password);
    }
    g_state = STATE_WIFI_CONNECTING;
}

void loop() {
    switch (g_state) {
        case STATE_WIFI_CONNECTING:
            handleWiFiConnecting();
            break;
        case STATE_DISCOVERING:
            handleDiscovering();
            break;
        case STATE_RUNNING:
            handleRunning();
            break;
        case STATE_AP_MODE:
            handleAPMode();
            break;
        case STATE_OTA_UPDATING:
            handleOTAUpdating();
            break;
    }
}

void handleWiFiConnecting() {
    g_wifi.ensureConnected();

    LGFX_Sprite& fb = g_renderer.getFramebuffer();
    g_wifi.drawStatus(fb);

    // Show AP fallback countdown
    unsigned long elapsed = millis() - g_wifi.connectStartTime();
    if (elapsed < WIFI_CONNECT_TIMEOUT_MS) {
        int secsLeft = (WIFI_CONNECT_TIMEOUT_MS - elapsed) / 1000;
        fb.setTextDatum(bottom_center);
        fb.setTextColor(COLOR_DIAL_RIM);
        fb.setTextSize(1.8);
        char buf[40];
        snprintf(buf, sizeof(buf), "AP fallback in %ds", secsLeft);
        fb.drawString(buf, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 20);
    }

    fb.pushSprite(&g_display, 0, 0);

    if (g_wifi.isConnected()) {
        // Start web server immediately so it's accessible during discovery
        g_webServer.begin(false);
        Serial.printf("[WiFi] Connected. Web UI at http://%s/\n", WiFi.localIP().toString().c_str());
        delay(500);
        g_discovery.begin();
        g_state = STATE_DISCOVERING;
    } else if (elapsed >= WIFI_CONNECT_TIMEOUT_MS) {
        startAPMode();
    } else {
        delay(200);
    }
}

void handleDiscovering() {
    if (!g_wifi.isConnected()) {
        g_state = STATE_WIFI_CONNECTING;
        return;
    }

    // Process web commands during discovery (gauge config, WiFi, etc.)
    processWebCommands();


    if (g_discovery.listen()) {
        // Found X-Plane!
        IPAddress ip = g_discovery.foundIP();
        snprintf(g_xplaneIP, sizeof(g_xplaneIP), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        g_xplanePort = g_discovery.foundPort();

        // Show discovery result
        LGFX_Sprite& fb = g_renderer.getFramebuffer();
        int cx = SCREEN_WIDTH / 2;
        int cy = SCREEN_HEIGHT / 2;

        fb.fillSprite(COLOR_BG);
        fb.setTextDatum(middle_center);
        fb.setTextColor(COLOR_ARC_GREEN);
        fb.setTextSize(4.0);
        fb.drawString("X-Plane Found!", cx, cy - 50);
        fb.setTextColor(COLOR_VALUE);
        fb.setTextSize(3.0);
        fb.drawString(g_xplaneIP, cx, cy + 15);
        fb.setTextColor(COLOR_DIAL_RIM);
        fb.setTextSize(2.0);
        fb.drawString(g_discovery.computerName(), cx, cy + 65);
        fb.pushSprite(&g_display, 0, 0);
        delay(1500);

        startRunning();
    } else {
        // Show searching animation
        LGFX_Sprite& fb = g_renderer.getFramebuffer();
        g_discovery.drawStatus(fb);

        // Show device IP for web access
        fb.setTextDatum(bottom_center);
        fb.setTextColor(COLOR_DIAL_RIM);
        fb.setTextSize(1.8);
        char ipBuf[48];
        snprintf(ipBuf, sizeof(ipBuf), "Web UI: http://%s", WiFi.localIP().toString().c_str());
        fb.drawString(ipBuf, SCREEN_WIDTH / 2, SCREEN_HEIGHT - 20);

        fb.pushSprite(&g_display, 0, 0);
        delay(200);
    }
}

void startRunning() {
    g_xplane.begin(g_xplaneIP, g_xplanePort);
    g_layout.begin();
    g_picker.begin(&g_layout);
    g_buzzer.begin();
    g_controlsPage.begin(&g_xplane);

    subscribeAll();

    g_lastFrameTime = millis();
    g_state = STATE_RUNNING;
}

void subscribeAll() {
    for (int i = 0; i < CELL_COUNT; i++) {
        subscribeCell(i);
    }
}

void subscribeCell(int cellIndex) {
    const GaugeConfig& cfg = g_layout.gauge(cellIndex)->getConfig();

    // Slot 0: primary dataref at index = cellIndex (0-5)
    g_xplane.subscribe(cfg.dataref, XPLANE_FREQ, cellIndex);

    // Slots 1-7: extra datarefs at index = slot * 10 + cellIndex
    for (int s = 0; s < cfg.extraDatarefCount; s++) {
        int idx = (s + 1) * 10 + cellIndex;
        g_xplane.subscribe(cfg.extraDatarefs[s], XPLANE_FREQ, idx);
    }

    // Update valueCount on CellState
    g_layout.cell(cellIndex).valueCount = 1 + cfg.extraDatarefCount;
}

void unsubscribeCell(int cellIndex) {
    const GaugeConfig& cfg = g_layout.gauge(cellIndex)->getConfig();

    g_xplane.unsubscribe(cfg.dataref, cellIndex);

    for (int s = 0; s < cfg.extraDatarefCount; s++) {
        int idx = (s + 1) * 10 + cellIndex;
        g_xplane.unsubscribe(cfg.extraDatarefs[s], idx);
    }
}

void resubscribeAll() {
    for (int i = 0; i < CELL_COUNT; i++) {
        unsubscribeCell(i);
        subscribeCell(i);
    }
}

// ── AP Mode ─────────────────────────────────────────────────────────

void startAPMode() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    delay(100);

    Serial.printf("[AP] Started: %s @ %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

    g_webServer.begin(true);
    g_state = STATE_AP_MODE;
}

void handleAPMode() {
    g_webServer.processDNS();

    // Check for OTA
    if (g_webServer.otaInProgress()) {
        g_state = STATE_OTA_UPDATING;
        return;
    }

    // Process web commands (WiFi save triggers restart via CMD_RESTART)
    processWebCommands();


    // Render AP info screen
    unsigned long now = millis();
    static unsigned long lastAPFrame = 0;
    if (now - lastAPFrame < 100) return;  // 10fps for status screen
    lastAPFrame = now;

    LGFX_Sprite& fb = g_renderer.getFramebuffer();
    fb.fillSprite(COLOR_BG);
    int cx = SCREEN_WIDTH / 2;

    fb.setTextDatum(middle_center);
    fb.setTextColor(COLOR_TITLE);
    fb.setTextSize(3.5);
    fb.drawString("WiFi Setup Mode", cx, 60);

    fb.setTextColor(COLOR_LABEL);
    fb.setTextSize(2.5);
    fb.drawString("Connect to WiFi network:", cx, 140);

    fb.setTextColor(COLOR_ARC_GREEN);
    fb.setTextSize(3.0);
    fb.drawString(AP_SSID, cx, 190);

    fb.setTextColor(COLOR_LABEL);
    fb.setTextSize(2.0);
    fb.drawString("Password: " AP_PASSWORD, cx, 240);

    fb.setTextColor(COLOR_DIAL_RIM);
    fb.setTextSize(2.0);
    fb.drawString("Then open browser to:", cx, 310);

    fb.setTextColor(COLOR_VALUE);
    fb.setTextSize(3.0);
    fb.drawString("http://192.168.4.1", cx, 360);

    // Connected clients count
    fb.setTextColor(COLOR_DIAL_RIM);
    fb.setTextSize(1.8);
    char buf[40];
    snprintf(buf, sizeof(buf), "Clients: %d", WiFi.softAPgetStationNum());
    fb.drawString(buf, cx, 430);

    fb.pushSprite(&g_display, 0, 0);
}

// ── OTA Updating ────────────────────────────────────────────────────

void handleOTAUpdating() {
    // Render OTA progress
    unsigned long now = millis();
    static unsigned long lastOTAFrame = 0;
    if (now - lastOTAFrame < 50) return;  // 20fps for progress bar
    lastOTAFrame = now;

    LGFX_Sprite& fb = g_renderer.getFramebuffer();
    fb.fillSprite(COLOR_BG);
    int cx = SCREEN_WIDTH / 2;
    int cy = SCREEN_HEIGHT / 2;

    fb.setTextDatum(middle_center);
    fb.setTextColor(COLOR_ARC_YELLOW);
    fb.setTextSize(3.5);
    fb.drawString("Updating Firmware", cx, cy - 80);

    fb.setTextColor(COLOR_LABEL);
    fb.setTextSize(2.5);
    fb.drawString("Do not disconnect!", cx, cy - 30);

    // Progress bar
    int barW = 500;
    int barH = 40;
    int bx = (SCREEN_WIDTH - barW) / 2;
    int by = cy + 20;
    int progress = g_webServer.otaProgress();

    fb.drawRoundRect(bx, by, barW, barH, 6, COLOR_DIAL_RIM);
    int fillW = (barW - 4) * progress / 100;
    if (fillW > 0) {
        fb.fillRoundRect(bx + 2, by + 2, fillW, barH - 4, 4, COLOR_ARC_GREEN);
    }

    // Percentage text
    char buf[10];
    snprintf(buf, sizeof(buf), "%d%%", progress);
    fb.setTextColor(COLOR_LABEL);
    fb.setTextSize(2.5);
    fb.drawString(buf, cx, by + barH + 30);

    fb.pushSprite(&g_display, 0, 0);

    // Restart after OTA complete
    if (g_webServer.otaComplete()) {
        fb.fillSprite(COLOR_BG);
        fb.setTextDatum(middle_center);
        fb.setTextColor(COLOR_ARC_GREEN);
        fb.setTextSize(3.5);
        fb.drawString("Update Complete!", cx, cy - 20);
        fb.setTextColor(COLOR_LABEL);
        fb.setTextSize(2.5);
        fb.drawString("Restarting...", cx, cy + 30);
        fb.pushSprite(&g_display, 0, 0);
        delay(2000);
        ESP.restart();
    }
}

// ── Process web commands ────────────────────────────────────────────

void processWebCommands() {
    WebCommand cmd;
    while (g_webServer.dequeueCommand(cmd)) {
        switch (cmd.type) {
            case CMD_SET_GAUGE:
                g_layout.setGauge(cmd.cell, cmd.gaugeIndex);
                if (g_viewManager.currentView() != VIEW_CUSTOM) {
                    g_viewManager.setView(VIEW_CUSTOM);
                }
                if (g_state == STATE_RUNNING) {
                    resubscribeAll();
                }
                Serial.printf("[Web] Set cell %d to gauge %d\n", cmd.cell, cmd.gaugeIndex);
                break;
            case CMD_SWITCH_VIEW:
                switchView(cmd.viewIndex);
                g_viewManager.setView(cmd.viewIndex);
                Serial.printf("[Web] Switch to view %d\n", cmd.viewIndex);
                break;
            case CMD_RESTART:
                Serial.println("[Web] Restart requested");
                delay(500);
                ESP.restart();
                break;
            case CMD_TOGGLE_CONTROL:
                g_controlsPage.toggle(cmd.controlIndex);
                if (g_viewManager.currentView() != VIEW_CONTROLS) {
                    g_viewManager.setView(VIEW_CONTROLS);
                    switchView(VIEW_CONTROLS);
                }
                Serial.printf("[Web] Toggle control slot %d\n", cmd.controlIndex);
                break;
            case CMD_SET_CONTROL:
                g_controlsPage.setSlot(cmd.cell, cmd.controlIndex);
                Serial.printf("[Web] Set slot %d to control %d\n", cmd.cell, cmd.controlIndex);
                break;
            case CMD_WIFI_SCAN: {
                Serial.println("[Web] Running WiFi scan on Core 1...");
                int n = WiFi.scanNetworks(false, false, false, 300);
                Serial.printf("[Web] Scan found %d networks\n", n);
                JsonDocument doc;
                JsonArray arr = doc.to<JsonArray>();
                for (int i = 0; i < n; i++) {
                    String ssid = WiFi.SSID(i);
                    if (ssid.length() == 0) continue;
                    JsonObject net = arr.add<JsonObject>();
                    net["ssid"] = ssid;
                    net["rssi"] = WiFi.RSSI(i);
                    net["open"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
                }
                WiFi.scanDelete();
                String json;
                serializeJson(doc, json);
                Serial.printf("[Web] Scan results: %s\n", json.c_str());
                g_webServer.setScanResults(json);
                break;
            }
            default:
                break;
        }
    }

    // Check for OTA start
    if (g_webServer.otaInProgress() && g_state != STATE_OTA_UPDATING) {
        g_state = STATE_OTA_UPDATING;
    }
}

void handleTouchInput() {
    // Read up to 2 touch points for multi-finger gesture detection
    lgfx::touch_point_t tp[2];
    int touchCount = g_display.getTouch(tp, 2);
    bool pressed = (touchCount > 0);
    bool twoFingers = (touchCount >= 2);


    // Keep last valid touch position for use on release
    static int lastTouchX = 0, lastTouchY = 0;
    if (pressed) {
        lastTouchX = tp[0].x;
        lastTouchY = tp[0].y;
    } else {
        tp[0].x = lastTouchX;
        tp[0].y = lastTouchY;
    }

    unsigned long now = millis();

    // ── Two-finger gesture handling ─────────────────────────────────
    if (twoFingers && !g_twoFingerActive && !g_picker.isOpen() && !g_viewPickerOpen) {
        // Two-finger touch start
        g_twoFingerActive = true;
        g_twoFingerStartX0 = tp[0].x;
        g_twoFingerStartX1 = tp[1].x;
        g_twoFingerStartMs = now;
        g_twoFingerSwipeTriggered = false;
    }

    if (twoFingers && g_twoFingerActive && !g_twoFingerSwipeTriggered) {
        // Check for horizontal swipe — average of both fingers
        if (now - g_viewChangeDebounce > VIEW_DEBOUNCE_MS) {
            int dx0 = tp[0].x - g_twoFingerStartX0;
            int dx1 = tp[1].x - g_twoFingerStartX1;
            int avgDx = (dx0 + dx1) / 2;
            if (avgDx > SWIPE_THRESHOLD) {
                g_viewManager.nextView();
                switchView(g_viewManager.currentView());
                g_viewChangeDebounce = now;
                g_twoFingerSwipeTriggered = true;
            } else if (avgDx < -SWIPE_THRESHOLD) {
                g_viewManager.prevView();
                switchView(g_viewManager.currentView());
                g_viewChangeDebounce = now;
                g_twoFingerSwipeTriggered = true;
            }
        }
    }

    if (!twoFingers && g_twoFingerActive) {
        // Two-finger release (at least one finger lifted)
        unsigned long dur = now - g_twoFingerStartMs;
        if (!g_twoFingerSwipeTriggered && (dur < TWO_FINGER_TAP_MS)) {
            // Short two-finger tap → open view picker
            g_viewPickerOpen = true;
            g_viewPickerOpenTime = now;
        }
        g_twoFingerActive = false;
        // Suppress single-finger tap until all fingers are released
        g_suppressNextTap = true;
    }

    // Clear suppression when all fingers are off the screen
    if (!pressed && g_suppressNextTap) {
        g_suppressNextTap = false;
        g_touchActive = false;  // Reset single-finger state too
    }

    // ── View picker touch handling ──────────────────────────────────
    if (g_viewPickerOpen) {
        // Auto-close timeout
        if (now - g_viewPickerOpenTime > VIEW_PICKER_TIMEOUT_MS) {
            g_viewPickerOpen = false;
        }
        // Single-finger tap to select view (300ms grace after open to skip stray finger)
        if (pressed && !twoFingers && touchCount == 1 &&
            (now - g_viewPickerOpenTime > 300)) {
            int pickerW = 300;
            int rowH = 70;
            int headerH = 60;
            int pickerH = VIEW_COUNT * rowH + headerH;
            int px = (SCREEN_WIDTH - pickerW) / 2;
            int py = (SCREEN_HEIGHT - pickerH) / 2;

            if (tp[0].x >= px && tp[0].x < px + pickerW &&
                tp[0].y >= py + headerH && tp[0].y < py + pickerH) {
                int row = (tp[0].y - py - headerH) / rowH;
                if (row >= 0 && row < VIEW_COUNT) {
                    g_viewManager.setView(row);
                    switchView(row);
                    g_viewPickerOpen = false;
                }
            } else if (tp[0].x < px || tp[0].x >= px + pickerW ||
                       tp[0].y < py || tp[0].y >= py + pickerH) {
                // Tap outside — close
                g_viewPickerOpen = false;
            }
        }
        // Consume touch — don't pass to gauge picker or cell tap
        return;
    }

    // Skip single-finger processing if two fingers are active or suppressed
    if (g_twoFingerActive || g_suppressNextTap) return;

    // ── Controls page touch routing ─────────────────────────────────
    if (g_viewManager.currentView() == VIEW_CONTROLS) {
        bool justPressed = (pressed && !g_touchActive);
        g_controlsPage.handleTouch(tp[0].x, tp[0].y, pressed, justPressed);
        g_touchActive = pressed;
        return;
    }

    // ── Single-finger touch handling (existing logic) ───────────────
    if (pressed && !g_touchActive) {
        // Touch start
        g_touchActive = true;
        g_touchStartMs = now;
        g_touchStartCell = -1;
        g_longPressTriggered = false;

        if (!g_picker.isOpen()) {
            int col = tp[0].x / CELL_WIDTH;
            int row = tp[0].y / CELL_HEIGHT;
            if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
                g_touchStartCell = row * GRID_COLS + col;
            }
        }
    }

    // Long-press detection (mute toggle) — only when picker is closed
    if (pressed && g_touchActive && !g_longPressTriggered && !g_picker.isOpen()) {
        if (now - g_touchStartMs >= MUTE_HOLD_MS) {
            g_longPressTriggered = true;
            g_buzzer.toggleMute();
            g_muteFlashEnd = now + 1000;
        }
    }

    // Forward all touch events to picker
    if (g_picker.isOpen() && !g_longPressTriggered) {
        bool changed = g_picker.handleTouch(tp[0].x, tp[0].y, pressed);
        if (changed) {
            // When gauge picker changes a gauge in CUSTOM view, stay in custom
            if (g_viewManager.currentView() != VIEW_CUSTOM) {
                g_viewManager.setView(VIEW_CUSTOM);
            }
            resubscribeAll();
        }
    }

    if (!pressed && g_touchActive) {
        // Touch release — open picker on short tap when picker is closed
        if (!g_longPressTriggered && !g_picker.isOpen() && g_touchStartCell >= 0) {
            g_picker.open(g_touchStartCell);
        }
        g_touchActive = false;
    }
}

void switchView(int viewIndex) {
    if (viewIndex == VIEW_CONTROLS) {
        // Controls page — no gauge layout or datarefs needed
        return;
    }
    if (viewIndex == VIEW_CUSTOM) {
        // Reload user's saved layout from NVS
        g_layout.begin();
    } else {
        // Apply predefined view layout
        const ViewDef& v = g_views[viewIndex];
        for (int i = 0; i < CELL_COUNT; i++) {
            g_layout.cell(i).gaugeIndex = v.gauges[i];
            g_layout.cell(i).valueCount = 1 + g_gauges[v.gauges[i]]->getConfig().extraDatarefCount;
            for (int s = 0; s < MAX_VALUES; s++) {
                g_layout.cell(i).values[s] = 0.0f;
                g_layout.cell(i).smoothedValues[s] = 0.0f;
            }
            g_layout.cell(i).hasData = false;
        }
    }
    if (g_state == STATE_RUNNING) {
        resubscribeAll();
    }
}

void drawViewPicker(LGFX_Sprite& fb) {
    if (!g_viewPickerOpen) return;

    int pickerW = 300;
    int rowH = 70;
    int headerH = 60;
    int pickerH = VIEW_COUNT * rowH + headerH;
    int px = (SCREEN_WIDTH - pickerW) / 2;
    int py = (SCREEN_HEIGHT - pickerH) / 2;

    // Semi-transparent overlay effect (darken background)
    fb.fillRect(px - 4, py - 4, pickerW + 8, pickerH + 8, COLOR_BG);

    // Panel background
    fb.fillRoundRect(px, py, pickerW, pickerH, 8, COLOR_PICKER_BG);
    fb.drawRoundRect(px, py, pickerW, pickerH, 8, COLOR_PICKER_HL);

    // Header
    fb.setTextDatum(middle_center);
    fb.setTextColor(COLOR_PICKER_HL);
    fb.setTextSize(2.2);
    fb.drawString("VIEWS", px + pickerW / 2, py + headerH / 2);
    fb.drawFastHLine(px + 4, py + headerH, pickerW - 8, COLOR_PICKER_HL);

    // View rows
    int currentView = g_viewManager.currentView();
    for (int i = 0; i < VIEW_COUNT; i++) {
        int ry = py + headerH + i * rowH;
        bool selected = (i == currentView);

        if (selected) {
            fb.fillRect(px + 4, ry + 2, pickerW - 8, rowH - 4, 0x0014);
        }

        fb.setTextDatum(middle_center);
        fb.setTextColor(selected ? COLOR_PICKER_HL : COLOR_PICKER_FG);
        fb.setTextSize(2.0);
        fb.drawString(g_views[i].name, px + pickerW / 2, ry + rowH / 2);
    }
}

void drawViewOverlay(LGFX_Sprite& fb) {
    if (!g_viewManager.shouldShowName()) return;

    // Show current view name at bottom center
    fb.setTextDatum(bottom_center);
    fb.setTextColor(COLOR_PICKER_HL);
    fb.setTextSize(1.8);

    // Background bar
    int textW = 120;
    int textH = 28;
    int bx = SCREEN_WIDTH / 2 - textW / 2;
    int by = SCREEN_HEIGHT - textH - 4;
    fb.fillRoundRect(bx, by, textW, textH, 4, COLOR_PICKER_BG);
    fb.drawRoundRect(bx, by, textW, textH, 4, COLOR_PICKER_HL);
    fb.drawString(g_viewManager.currentViewName(), SCREEN_WIDTH / 2, SCREEN_HEIGHT - 6);
}

void handleRunning() {
    if (!g_wifi.isConnected()) {
        g_state = STATE_WIFI_CONNECTING;
        return;
    }

    // Process web commands before touch/render
    processWebCommands();


    // Process touch input
    handleTouchInput();
    g_picker.update();

    // Receive data from X-Plane — route by slot*10+cell index scheme
    int idx;
    float val;
    while (g_xplane.receive(idx, val)) {
        int slot = idx / 10;
        int cellIdx = idx % 10;

        // Slot 0 uses direct indices 0-5; slots 1-7 use 10-75
        if (slot == 0 && cellIdx >= 0 && cellIdx < CELL_COUNT) {
            CellState& cs = g_layout.cell(cellIdx);
            cs.values[0] = val;
            cs.lastReceiveTime = millis();
            if (!cs.hasData) {
                cs.smoothedValues[0] = val;
                cs.hasData = true;
            }
        }
        else if (slot >= 1 && slot <= 7 && cellIdx >= 0 && cellIdx < CELL_COUNT) {
            CellState& cs = g_layout.cell(cellIdx);
            cs.values[slot] = val;
            cs.lastReceiveTime = millis();
            if (!cs.hasData) {
                cs.smoothedValues[slot] = val;
            }
        }
    }

    // Frame rate limit
    unsigned long now = millis();
    if (now - g_lastFrameTime < FRAME_TIME_MS) return;
    g_lastFrameTime = now;

    // Update smoothing and connection status for all cells
    for (int i = 0; i < CELL_COUNT; i++) {
        CellState& cs = g_layout.cell(i);

        bool connected = cs.hasData && (now - cs.lastReceiveTime < XPLANE_TIMEOUT_MS);
        if (!connected && cs.hasData) {
            for (int s = 0; s < cs.valueCount; s++) {
                cs.values[s] = 0.0f;
                cs.smoothedValues[s] = 0.0f;
            }
            cs.hasData = false;
        }

        if (cs.hasData) {
            for (int s = 0; s < cs.valueCount; s++) {
                cs.smoothedValues[s] += SMOOTH_ALPHA * (cs.values[s] - cs.smoothedValues[s]);
            }
        }
    }

    // Track connection status and dim/brighten backlight
    bool anyConnected = false;
    for (int i = 0; i < CELL_COUNT; i++) {
        if (g_layout.cell(i).hasData) { anyConnected = true; break; }
    }
    if (anyConnected != g_xplaneConnected) {
        g_xplaneConnected = anyConnected;
        display_set_backlight(anyConnected);
    }

    // Update buzzer
    g_buzzer.updateAll(g_layout);

    // Get framebuffer reference
    LGFX_Sprite& fb = g_renderer.getFramebuffer();

    if (g_viewManager.currentView() == VIEW_CONTROLS) {
        // Controls page — full-screen button grid
        fb.fillSprite(COLOR_BG);
        g_controlsPage.draw(fb);
    } else {
        // Render all 6 cells
        for (int i = 0; i < CELL_COUNT; i++) {
            CellState& cs = g_layout.cell(i);
            bool connected = cs.hasData && (now - cs.lastReceiveTime < XPLANE_TIMEOUT_MS);
            g_renderer.renderCell(i, g_layout.gauge(i), cs.values, cs.smoothedValues, cs.valueCount, connected);
        }

        // Draw grid lines
        g_renderer.drawGrid();

        // Highlight selected cell if picker is open
        if (g_picker.isOpen()) {
            g_renderer.drawCellHighlight(g_picker.selectedCell(), COLOR_CELL_SEL);
        }

        // Draw picker overlay
        g_picker.draw(fb);
    }
    if (g_buzzer.isMuted()) {
        if (now < g_muteFlashEnd) {
            fb.setTextDatum(middle_center);
            fb.setTextSize(3.0);
            fb.setTextColor(COLOR_ARC_YELLOW);
            fb.drawString("MUTED", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
        } else {
            fb.setTextDatum(top_center);
            fb.setTextSize(1.0);
            fb.setTextColor(COLOR_ARC_YELLOW);
            fb.drawString("MUTE", SCREEN_WIDTH / 2, 4);
        }
    } else if (now < g_muteFlashEnd) {
        fb.setTextDatum(middle_center);
        fb.setTextSize(3.0);
        fb.setTextColor(COLOR_ARC_GREEN);
        fb.drawString("UNMUTED", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
    }

    // Draw view name overlay (shown briefly after switching)
    drawViewOverlay(fb);

    // Draw view picker popup (two-finger tap)
    drawViewPicker(fb);

    // Push entire framebuffer to display
    g_renderer.push();
}
