#include <M5Dial.h>
#include "config.h"
#include "wifi_manager.h"
#include "wifi_credentials.h"
#include "xplane_discovery.h"
#include "xplane_client.h"
#include "gauge_renderer.h"
#include "gauge_selector.h"
#include "warning_beeper.h"
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
static GaugeSelector     g_selector;
static WarningBeeper     g_beeper;

// ── Discovered X-Plane address (stored as string for XPlaneClient) ──
static char g_xplaneIP[20] = {0};
static uint16_t g_xplanePort = XPLANE_PORT;

// ── Needle smoothing state ──────────────────────────────────────────
static float g_rawValue      = 0.0f;
static float g_smoothedValue = 0.0f;
static bool  g_hasData       = false;
static unsigned long g_lastFrameTime   = 0;
static unsigned long g_lastReceiveTime = 0;
static constexpr unsigned long XPLANE_TIMEOUT_MS = 2000;

// ── Secondary dataref for position gauge ────────────────────────────
float g_secondaryValue = 0.0f;
static bool g_secondaryActive = false;

// ── Discovery timing ────────────────────────────────────────────────
static unsigned long g_discoveryStart = 0;

// ── Mute long-press tracking ───────────────────────────────────────
static bool g_muteHoldTriggered = false;
static unsigned long g_muteFlashEnd = 0;

// ── Forward declarations ────────────────────────────────────────────
void handleWiFiConnecting();
void handleDiscovering();
void handleRunning();
void startRunning();
void updateSecondarySubscription();

void setup() {
    auto cfg = M5.config();
    M5Dial.begin(cfg, true, false);

    M5Dial.Display.setRotation(0);
    M5Dial.Display.fillScreen(COLOR_BG);
    M5Dial.Display.setTextDatum(middle_center);
    M5Dial.Display.setTextColor(COLOR_TITLE);
    M5Dial.Display.setTextSize(1.5);
    M5Dial.Display.drawString("X-Plane Gauge", CENTER_X, CENTER_Y - 15);
    M5Dial.Display.setTextSize(1.0);
    M5Dial.Display.setTextColor(COLOR_DIAL_RIM);
    M5Dial.Display.drawString("v" FW_VERSION, CENTER_X, CENTER_Y + 10);
    M5Dial.Display.setTextColor(COLOR_LABEL);
    M5Dial.Display.drawString("Initializing...", CENTER_X, CENTER_Y + 30);
    delay(500);

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
    g_wifi.drawStatus(M5Dial.Display);

    if (g_wifi.isConnected()) {
        delay(500);

        // Start beacon discovery
        g_discovery.begin();
        g_discoveryStart = millis();
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
        M5Dial.Display.fillScreen(COLOR_BG);
        M5Dial.Display.setTextDatum(middle_center);
        M5Dial.Display.setTextColor(COLOR_ARC_GREEN);
        M5Dial.Display.setTextSize(1.5);
        M5Dial.Display.drawString("X-Plane Found!", CENTER_X, CENTER_Y - 35);
        M5Dial.Display.setTextColor(COLOR_VALUE);
        M5Dial.Display.setTextSize(1.2);
        M5Dial.Display.drawString(g_xplaneIP, CENTER_X, CENTER_Y + 5);
        M5Dial.Display.setTextColor(COLOR_DIAL_RIM);
        M5Dial.Display.setTextSize(1.0);
        M5Dial.Display.drawString(g_discovery.computerName(), CENTER_X, CENTER_Y + 35);
        delay(1500);

        startRunning();
    } else {
        // Show searching animation
        g_discovery.drawStatus(M5Dial.Display);
        delay(200);
    }
}

void startRunning() {
    g_xplane.begin(g_xplaneIP, g_xplanePort);
    g_renderer.begin();
    g_selector.begin(&g_xplane);
    g_beeper.begin();

    const GaugeConfig& cfg = g_selector.currentGauge()->getConfig();
    g_xplane.subscribe(cfg.dataref, XPLANE_FREQ, g_selector.currentIndex());

    g_rawValue = 0.0f;
    g_smoothedValue = 0.0f;
    g_hasData = false;
    g_lastFrameTime = millis();

    g_state = STATE_RUNNING;
}

void updateSecondarySubscription() {
    bool needsSecondary = (g_selector.currentGauge() == &g_positionGauge);

    if (needsSecondary && !g_secondaryActive) {
        g_xplane.subscribe(PositionGauge::secondaryDataref(), XPLANE_FREQ, PositionGauge::SECONDARY_INDEX);
        g_secondaryActive = true;
        g_secondaryValue = 0.0f;
    } else if (!needsSecondary && g_secondaryActive) {
        g_xplane.unsubscribe(PositionGauge::secondaryDataref(), PositionGauge::SECONDARY_INDEX);
        g_secondaryActive = false;
        g_secondaryValue = 0.0f;
    }
}

void handleRunning() {
    if (!g_wifi.isConnected()) {
        g_state = STATE_WIFI_CONNECTING;
        return;
    }

    g_selector.update();

    // Long-press button to toggle mute (only when not selecting a gauge)
    if (!g_selector.isSelecting() && M5Dial.BtnA.pressedFor(MUTE_HOLD_MS)) {
        if (!g_muteHoldTriggered) {
            g_muteHoldTriggered = true;
            g_beeper.toggleMute();
            g_muteFlashEnd = millis() + 1000;
        }
    }
    if (!M5Dial.BtnA.isPressed()) {
        g_muteHoldTriggered = false;
    }

    if (g_selector.selectionChanged()) {
        g_rawValue = 0.0f;
        g_smoothedValue = 0.0f;
        g_hasData = false;
        g_beeper.stop();
        updateSecondarySubscription();
    }

    int idx;
    float val;
    while (g_xplane.receive(idx, val)) {
        g_lastReceiveTime = millis();
        if (idx == g_selector.currentIndex()) {
            g_rawValue = val;
            if (!g_hasData) {
                g_smoothedValue = val;
                g_hasData = true;
            }
        } else if (idx == PositionGauge::SECONDARY_INDEX) {
            g_secondaryValue = val;
        }
    }

    unsigned long now = millis();
    if (now - g_lastFrameTime < FRAME_TIME_MS) return;
    g_lastFrameTime = now;

    bool xplaneConnected = g_hasData && (now - g_lastReceiveTime < XPLANE_TIMEOUT_MS);
    if (!xplaneConnected && g_hasData) {
        g_rawValue = 0.0f;
        g_smoothedValue = 0.0f;
        g_hasData = false;
        g_beeper.stop();
    }

    if (g_hasData) {
        g_smoothedValue += SMOOTH_ALPHA * (g_rawValue - g_smoothedValue);
    }

    g_beeper.update(g_selector.currentGauge()->getConfig(), g_smoothedValue, g_hasData);

    g_renderer.render(g_selector.currentGauge(), g_rawValue, g_smoothedValue, xplaneConnected);
    g_selector.draw(g_renderer.getCanvas());

    // Show mute indicator
    if (g_beeper.isMuted()) {
        M5Canvas& canvas = g_renderer.getCanvas();
        unsigned long now = millis();
        if (now < g_muteFlashEnd) {
            // Brief large flash on toggle
            canvas.setTextDatum(middle_center);
            canvas.setTextSize(1.5);
            canvas.setTextColor(COLOR_ARC_YELLOW);
            canvas.drawString("MUTED", CENTER_X, CENTER_Y);
        } else {
            // Small persistent indicator
            canvas.setTextDatum(top_center);
            canvas.setTextSize(0.8);
            canvas.setTextColor(COLOR_ARC_YELLOW);
            canvas.drawString("MUTE", CENTER_X, 4);
        }
    } else if (millis() < g_muteFlashEnd) {
        // Brief flash when unmuting
        M5Canvas& canvas = g_renderer.getCanvas();
        canvas.setTextDatum(middle_center);
        canvas.setTextSize(1.5);
        canvas.setTextColor(COLOR_ARC_GREEN);
        canvas.drawString("UNMUTED", CENTER_X, CENTER_Y);
    }

    g_renderer.push(M5Dial.Display);
}
