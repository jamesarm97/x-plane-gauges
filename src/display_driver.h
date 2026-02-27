#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <driver/i2c.h>
#include <Wire.h>

// ── CH422G IO Expander ──────────────────────────────────────────────
// Uses different I2C addresses for different operations:
//   0x48 (7-bit 0x24) = set system parameters / IO mode
//   0x70 (7-bit 0x38) = write EXIO output pins
#define CH422G_SET_IO_CMD 0x48
#define CH422G_WRITE_CMD  0x70

// Expander pin bit masks
#define CH422G_EXIO1  (1 << 1)   // Touch reset
#define CH422G_EXIO2  (1 << 2)   // LCD backlight
#define CH422G_EXIO3  (1 << 3)   // LCD reset

// ── LovyanGFX Display Class ────────────────────────────────────────
// Pin mapping from Waveshare official board config:
// BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_4_3.h
class LGFX : public lgfx::LGFX_Device {
public:
    lgfx::Bus_RGB      _bus_instance;
    lgfx::Panel_RGB    _panel_instance;
    lgfx::Touch_GT911  _touch_instance;

    LGFX() {
        // ── RGB Bus configuration ───────────────────────────────────
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;

            // 16-bit RGB565 data pins (5 blue + 6 green + 5 red)
            cfg.pin_d0  = GPIO_NUM_14;  // B0
            cfg.pin_d1  = GPIO_NUM_38;  // B1
            cfg.pin_d2  = GPIO_NUM_18;  // B2
            cfg.pin_d3  = GPIO_NUM_17;  // B3
            cfg.pin_d4  = GPIO_NUM_10;  // B4
            cfg.pin_d5  = GPIO_NUM_39;  // G0
            cfg.pin_d6  = GPIO_NUM_0;   // G1
            cfg.pin_d7  = GPIO_NUM_45;  // G2
            cfg.pin_d8  = GPIO_NUM_48;  // G3
            cfg.pin_d9  = GPIO_NUM_47;  // G4
            cfg.pin_d10 = GPIO_NUM_21;  // G5
            cfg.pin_d11 = GPIO_NUM_1;   // R0
            cfg.pin_d12 = GPIO_NUM_2;   // R1
            cfg.pin_d13 = GPIO_NUM_42;  // R2
            cfg.pin_d14 = GPIO_NUM_41;  // R3
            cfg.pin_d15 = GPIO_NUM_40;  // R4

            // Control signals
            cfg.pin_henable = GPIO_NUM_5;   // DE (Data Enable)
            cfg.pin_vsync   = GPIO_NUM_3;
            cfg.pin_hsync   = GPIO_NUM_46;
            cfg.pin_pclk    = GPIO_NUM_7;

            // Timing parameters for ST7262 / 800x480
            // Lower PCLK reduces PSRAM contention that causes screen drift
            cfg.freq_write = 12000000;

            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 4;
            cfg.hsync_back_porch  = 16;

            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 4;
            cfg.vsync_back_porch  = 8;

            cfg.pclk_active_neg = 1;
            cfg.de_idle_high    = 0;
            cfg.pclk_idle_high  = 1;

            _bus_instance.config(cfg);
        }

        // ── Panel configuration ─────────────────────────────────────
        {
            auto cfg = _panel_instance.config();
            cfg.memory_width  = 800;
            cfg.memory_height = 480;
            cfg.panel_width   = 800;
            cfg.panel_height  = 480;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;

            _panel_instance.config(cfg);
        }

        _panel_instance.setBus(&_bus_instance);

        // ── Touch (GT911) ───────────────────────────────────────────
        // GPIO8 (SDA) and GPIO9 (SCL) are dedicated I2C — NOT shared
        // with RGB data bus on the Waveshare 4.3" board.
        {
            auto cfg = _touch_instance.config();
            cfg.x_min = 0;
            cfg.x_max = 799;
            cfg.y_min = 0;
            cfg.y_max = 479;
            cfg.bus_shared = false;
            cfg.offset_rotation = 0;

            cfg.i2c_port = I2C_NUM_0;
            cfg.i2c_addr = 0x14;
            cfg.pin_sda  = GPIO_NUM_8;
            cfg.pin_scl  = GPIO_NUM_9;
            cfg.pin_int  = GPIO_NUM_4;
            cfg.freq     = 400000;

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance);
        }

        setPanel(&_panel_instance);
    }
};

// ── Global display instance ─────────────────────────────────────────
static LGFX g_display;

// ── CH422G helpers ──────────────────────────────────────────────────
static void ch422g_write(uint8_t value) {
    Wire.beginTransmission(CH422G_WRITE_CMD >> 1);  // 0x70 >> 1 = 0x38
    Wire.write(value);
    Wire.endTransmission();
}

static void ch422g_set_io_mode() {
    Wire.beginTransmission(CH422G_SET_IO_CMD >> 1);  // 0x48 >> 1 = 0x24
    Wire.write(0x01);   // Enable push-pull output mode
    Wire.endTransmission();
}

// ── Display initialization ──────────────────────────────────────────
// Follows Waveshare official sequence: IO expander → LCD reset →
// touch reset (with INT held low for GT911 address) → backlight on.
static void display_init() {
    // Init I2C for CH422G
    Wire.begin(GPIO_NUM_8, GPIO_NUM_9, 400000);
    delay(10);

    // Configure CH422G IO expander — enable output mode
    ch422g_set_io_mode();
    delay(10);

    // Start with all outputs low (LCD reset asserted, touch reset asserted)
    ch422g_write(0x00);
    delay(10);

    // LCD reset sequence: release EXIO3 high, wait 100ms
    ch422g_write(CH422G_EXIO3);
    delay(100);

    // Touch reset sequence (selects GT911 I2C address via INT pin):
    // 1. Hold INT low → selects address 0x14
    pinMode(GPIO_NUM_4, OUTPUT);
    digitalWrite(GPIO_NUM_4, LOW);
    delay(10);
    // 2. Assert touch reset (EXIO1 low) — already low, keep LCD reset high
    ch422g_write(CH422G_EXIO3);
    delay(100);
    // 3. Release touch reset (EXIO1 high)
    ch422g_write(CH422G_EXIO1 | CH422G_EXIO3);
    delay(200);
    // 4. Release INT pin
    pinMode(GPIO_NUM_4, INPUT);

    // Turn on backlight
    ch422g_write(CH422G_EXIO1 | CH422G_EXIO2 | CH422G_EXIO3);
    delay(50);

    // Release Wire so LovyanGFX can manage I2C for touch
    Wire.end();

    // Initialize LovyanGFX (sets up RGB bus + touch I2C)
    g_display.init();
    g_display.setRotation(0);
}
