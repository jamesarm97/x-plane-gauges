#pragma once

// ── Version ────────────────────────────────────────────────────────
#define FW_VERSION "2.0.1"

// ── X-Plane Configuration ───────────────────────────────────────────
#define XPLANE_PORT   49000             // X-Plane UDP port (default, overridden by beacon)
#define XPLANE_FREQ   10                // Dataref update frequency in Hz
#define XPLANE_DISCOVERY_TIMEOUT_MS 30000  // Give up auto-discovery after 30s

// ── Display Constants ───────────────────────────────────────────────
#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   480

// ── Dashboard Grid (3 columns x 2 rows) ────────────────────────────
#define GRID_COLS       3
#define GRID_ROWS       2
#define CELL_COUNT      (GRID_COLS * GRID_ROWS)
#define CELL_WIDTH      266     // 800/3 ≈ 266 (last column gets 268)
#define CELL_HEIGHT     240     // 480/2

// ── Gauge Geometry (per-cell, same as M5Dial) ───────────────────────
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

// ── Gauge Picker ────────────────────────────────────────────────────
#define PICKER_WIDTH        420     // Popup width in pixels
#define PICKER_HEIGHT       440     // Popup height in pixels
#define PICKER_TIMEOUT_MS   5000    // Auto-close after 5 seconds
#define PICKER_ROW_HEIGHT   50      // Height of each gauge entry

// ── Buzzer (LEDC PWM) ──────────────────────────────────────────────
#define BUZZER_GPIO           2     // GPIO pin for external passive buzzer
#define BUZZER_LEDC_CHANNEL   0     // LEDC channel
#define BUZZER_LEDC_TIMER     0     // LEDC timer
#define WARNING_BEEP_FREQ     880   // Hz (A5 note)
#define WARNING_BEEP_ON_MS    200   // Beep duration
#define WARNING_BEEP_OFF_MS   300   // Silence between beeps
#define MUTE_HOLD_MS          1000  // Long-press to toggle mute

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
#define COLOR_GRID_LINE   0x4208  // Medium gray for grid borders
#define COLOR_CELL_SEL    0x07FF  // Cyan highlight for selected cell
#define COLOR_PICKER_BG   0x10A2  // Dark panel background
#define COLOR_PICKER_FG   0xFFFF  // Picker text
#define COLOR_PICKER_HL   0x07FF  // Picker highlight
