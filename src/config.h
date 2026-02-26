#pragma once

// ── X-Plane Configuration ───────────────────────────────────────────
#define XPLANE_PORT   49000             // X-Plane UDP port (default, overridden by beacon)
#define XPLANE_FREQ   10                // Dataref update frequency in Hz
#define XPLANE_DISCOVERY_TIMEOUT_MS 30000  // Give up auto-discovery after 30s

// ── Display Constants ───────────────────────────────────────────────
#define SCREEN_WIDTH    240
#define SCREEN_HEIGHT   240
#define CENTER_X        120
#define CENTER_Y        120
#define DIAL_RADIUS     110
#define TICK_OUTER_R    105
#define TICK_MAJOR_R    88
#define TICK_MINOR_R    95
#define LABEL_RADIUS    68
#define ARC_RADIUS      100
#define ARC_THICKNESS   6
#define NEEDLE_LENGTH   80
#define NEEDLE_WIDTH    4
#define HUB_RADIUS      8

// ── Rendering ───────────────────────────────────────────────────────
#define TARGET_FPS      30
#define FRAME_TIME_MS   (1000 / TARGET_FPS)
#define SMOOTH_ALPHA    0.15f   // Exponential smoothing factor for needle

// ── Gauge Selector ──────────────────────────────────────────────────
#define SELECTOR_TIMEOUT_MS  3000   // Auto-confirm after 3 seconds

// ── Colors (RGB565) ─────────────────────────────────────────────────
#define COLOR_BG          0x0000  // Black
#define COLOR_DIAL_BG     0x18E3  // Dark gray
#define COLOR_DIAL_RIM    0xC618  // Light gray
#define COLOR_TICK        0xFFFF  // White
#define COLOR_LABEL       0xFFFF  // White
#define COLOR_NEEDLE      0xFFFF  // White
#define COLOR_HUB         0xC618  // Light gray
#define COLOR_TITLE       0x07FF  // Cyan
#define COLOR_VALUE       0x07E0  // Green
#define COLOR_ARC_GREEN   0x07E0  // Green
#define COLOR_ARC_YELLOW  0xFFE0  // Yellow
#define COLOR_ARC_RED     0xF800  // Red
#define COLOR_ARC_WHITE   0xFFFF  // White
#define COLOR_SELECTOR_BG 0x0000  // Black overlay
#define COLOR_SELECTOR_FG 0x07FF  // Cyan text
