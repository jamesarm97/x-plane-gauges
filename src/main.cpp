#include <Arduino.h>
#include "display_driver.h"
#include "config.h"
#include "wifi_manager.h"
#include "wifi_credentials.h"
#include "xplane_discovery.h"
#include "xplane_client.h"
#include "gauge_renderer.h"
#include "dashboard_layout.h"
#include "gauge_picker.h"
#include "buzzer_driver.h"
#include "gauges/gauge_registry.h"
#include "gauges/position_gauge.h"

// ── Application State ───────────────────────────────────────────────
enum AppState {
    STATE_WIFI_CONNECTING,
    STATE_DISCOVERING,
    STATE_RUNNING,
};

static AppState          g_state = STATE_WIFI_CONNECTING;
static WiFiManager       g_wifi;
static XPlaneDiscovery   g_discovery;
static XPlaneClient      g_xplane;
static GaugeRenderer     g_renderer;
static DashboardLayout   g_layout;
static GaugePicker       g_picker;
static BuzzerDriver      g_buzzer;

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

// ── Forward declarations ────────────────────────────────────────────
void handleWiFiConnecting();
void handleDiscovering();
void handleRunning();
void startRunning();
void subscribeAll();
void subscribeCell(int cellIndex);
void unsubscribeCell(int cellIndex);
void handleTouchInput();

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

    g_wifi.begin(WIFI_NETWORKS, WIFI_NETWORK_COUNT);
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
    }
}

void handleWiFiConnecting() {
    g_wifi.ensureConnected();

    LGFX_Sprite& fb = g_renderer.getFramebuffer();
    g_wifi.drawStatus(fb);
    fb.pushSprite(&g_display, 0, 0);

    if (g_wifi.isConnected()) {
        delay(500);
        g_discovery.begin();
        g_state = STATE_DISCOVERING;
    } else {
        delay(200);
    }
}

void handleDiscovering() {
    if (!g_wifi.isConnected()) {
        g_state = STATE_WIFI_CONNECTING;
        return;
    }

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
        fb.pushSprite(&g_display, 0, 0);
        delay(200);
    }
}

void startRunning() {
    g_xplane.begin(g_xplaneIP, g_xplanePort);
    g_layout.begin();
    g_picker.begin(&g_layout);
    g_buzzer.begin();

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
    g_xplane.subscribe(cfg.dataref, XPLANE_FREQ, cellIndex);

    // Subscribe secondary dataref for position gauge
    if (g_layout.needsSecondary(cellIndex)) {
        g_xplane.subscribe(PositionGauge::secondaryDataref(), XPLANE_FREQ,
                          PositionGauge::SECONDARY_INDEX_BASE + cellIndex);
    }
}

void unsubscribeCell(int cellIndex) {
    const GaugeConfig& cfg = g_layout.gauge(cellIndex)->getConfig();
    g_xplane.unsubscribe(cfg.dataref, cellIndex);

    if (g_layout.needsSecondary(cellIndex)) {
        g_xplane.unsubscribe(PositionGauge::secondaryDataref(),
                            PositionGauge::SECONDARY_INDEX_BASE + cellIndex);
    }
}

void resubscribeAll() {
    for (int i = 0; i < CELL_COUNT; i++) {
        unsubscribeCell(i);
        subscribeCell(i);
    }
}

void handleTouchInput() {
    lgfx::touch_point_t tp;
    int touchCount = g_display.getTouch(&tp, 1);
    bool pressed = (touchCount > 0);

    if (pressed && !g_touchActive) {
        // Touch start
        g_touchActive = true;
        g_touchStartMs = millis();
        g_touchStartCell = -1;
        g_longPressTriggered = false;

        if (!g_picker.isOpen()) {
            int col = tp.x / CELL_WIDTH;
            int row = tp.y / CELL_HEIGHT;
            if (col >= 0 && col < GRID_COLS && row >= 0 && row < GRID_ROWS) {
                g_touchStartCell = row * GRID_COLS + col;
            }
        }
    }

    // Long-press detection (mute toggle) — only when picker is closed
    if (pressed && g_touchActive && !g_longPressTriggered && !g_picker.isOpen()) {
        if (millis() - g_touchStartMs >= MUTE_HOLD_MS) {
            g_longPressTriggered = true;
            g_buzzer.toggleMute();
            g_muteFlashEnd = millis() + 1000;
        }
    }

    // Forward all touch events to picker
    if (g_picker.isOpen() && !g_longPressTriggered) {
        bool changed = g_picker.handleTouch(tp.x, tp.y, pressed);
        if (changed) {
            resubscribeAll();
        }
    }

    if (!pressed && g_touchActive) {
        // Touch release — open picker on short tap when picker is closed
        if (!g_longPressTriggered && !g_picker.isOpen() && g_touchStartCell >= 0) {
            // Simulate press+release to open picker for this cell
            int ox, oy, w, h;
            GaugeRenderer::cellOrigin(g_touchStartCell, ox, oy);
            GaugeRenderer::cellSize(g_touchStartCell, w, h);
            g_picker.handleTouch(ox + w / 2, oy + h / 2, true);
            g_picker.handleTouch(ox + w / 2, oy + h / 2, false);
        }
        g_touchActive = false;
    }
}

void handleRunning() {
    if (!g_wifi.isConnected()) {
        g_state = STATE_WIFI_CONNECTING;
        return;
    }

    // Process touch input
    handleTouchInput();
    g_picker.update();

    // Receive data from X-Plane
    int idx;
    float val;
    while (g_xplane.receive(idx, val)) {
        // Direct cell index (0-5)
        if (idx >= 0 && idx < CELL_COUNT) {
            CellState& cs = g_layout.cell(idx);
            cs.rawValue = val;
            cs.lastReceiveTime = millis();
            if (!cs.hasData) {
                cs.smoothedValue = val;
                cs.hasData = true;
            }
        }
        // Secondary dataref for position gauge (100-105)
        else if (idx >= PositionGauge::SECONDARY_INDEX_BASE &&
                 idx < PositionGauge::SECONDARY_INDEX_BASE + CELL_COUNT) {
            int cellIdx = idx - PositionGauge::SECONDARY_INDEX_BASE;
            g_layout.cell(cellIdx).secondaryValue = val;
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
            cs.rawValue = 0.0f;
            cs.smoothedValue = 0.0f;
            cs.hasData = false;
        }

        if (cs.hasData) {
            cs.smoothedValue += SMOOTH_ALPHA * (cs.rawValue - cs.smoothedValue);
        }

        // Update position gauge secondary value
        if (g_layout.needsSecondary(i)) {
            PositionGauge* pg = static_cast<PositionGauge*>(g_layout.gauge(i));
            pg->secondaryValue = cs.secondaryValue;
        }
    }

    // Update buzzer
    g_buzzer.updateAll(g_layout);

    // Render all 6 cells
    for (int i = 0; i < CELL_COUNT; i++) {
        CellState& cs = g_layout.cell(i);
        bool connected = cs.hasData && (now - cs.lastReceiveTime < XPLANE_TIMEOUT_MS);
        g_renderer.renderCell(i, g_layout.gauge(i), cs.rawValue, cs.smoothedValue, connected);
    }

    // Draw grid lines
    g_renderer.drawGrid();

    // Highlight selected cell if picker is open
    if (g_picker.isOpen()) {
        g_renderer.drawCellHighlight(g_picker.selectedCell(), COLOR_CELL_SEL);
    }

    // Draw picker overlay
    g_picker.draw(g_renderer.getFramebuffer());

    // Draw mute indicator
    LGFX_Sprite& fb = g_renderer.getFramebuffer();
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

    // Push entire framebuffer to display
    g_renderer.push();
}
