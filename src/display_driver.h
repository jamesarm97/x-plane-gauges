#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <driver/i2c.h>
#include <Wire.h>

// ── CH422G IO Expander (I2C address 0x24) ─────────────────────────────
// Controls backlight (EXIO2), touch reset (EXIO1), SD CS (EXIO4)
#define CH422G_ADDR       0x24
#define CH422G_SET_IO_CMD 0x48   // Set output mode command
#define CH422G_WRITE_CMD  0x70   // Write output pins command

// Expander pin bit masks
#define CH422G_EXIO1  (1 << 1)   // Touch reset
#define CH422G_EXIO2  (1 << 2)   // LCD backlight
#define CH422G_EXIO4  (1 << 4)   // SD card CS

// ── LovyanGFX Display Class ──────────────────────────────────────────
class LGFX : public lgfx::LGFX_Device {
public:
    lgfx::Bus_RGB      _bus_instance;
    lgfx::Panel_RGB    _panel_instance;
    lgfx::Touch_GT911  _touch_instance;

    LGFX() {
        // ── RGB Bus configuration ────────────────────────────────────
        {
            auto cfg = _bus_instance.config();
            cfg.panel = &_panel_instance;

            // Data pins (16-bit: 5 blue + 6 green + 5 red)
            cfg.pin_d0  = GPIO_NUM_8;   // B0
            cfg.pin_d1  = GPIO_NUM_3;   // B1
            cfg.pin_d2  = GPIO_NUM_46;  // B2
            cfg.pin_d3  = GPIO_NUM_9;   // B3
            cfg.pin_d4  = GPIO_NUM_1;   // B4
            cfg.pin_d5  = GPIO_NUM_5;   // G0
            cfg.pin_d6  = GPIO_NUM_6;   // G1
            cfg.pin_d7  = GPIO_NUM_7;   // G2
            cfg.pin_d8  = GPIO_NUM_15;  // G3
            cfg.pin_d9  = GPIO_NUM_16;  // G4
            cfg.pin_d10 = GPIO_NUM_4;   // G5
            cfg.pin_d11 = GPIO_NUM_45;  // R0
            cfg.pin_d12 = GPIO_NUM_48;  // R1
            cfg.pin_d13 = GPIO_NUM_47;  // R2
            cfg.pin_d14 = GPIO_NUM_21;  // R3
            cfg.pin_d15 = GPIO_NUM_14;  // R4

            // Control signals
            cfg.pin_henable = GPIO_NUM_40;
            cfg.pin_vsync   = GPIO_NUM_41;
            cfg.pin_hsync   = GPIO_NUM_39;
            cfg.pin_pclk    = GPIO_NUM_42;

            // Timing parameters for ST7262 / 800x480
            cfg.freq_write = 16000000;

            cfg.hsync_polarity    = 0;
            cfg.hsync_front_porch = 8;
            cfg.hsync_pulse_width = 4;
            cfg.hsync_back_porch  = 8;

            cfg.vsync_polarity    = 0;
            cfg.vsync_front_porch = 8;
            cfg.vsync_pulse_width = 4;
            cfg.vsync_back_porch  = 8;

            cfg.pclk_active_neg = 1;
            cfg.de_idle_high    = 0;
            cfg.pclk_idle_high  = 0;

            _bus_instance.config(cfg);
        }

        // ── Panel configuration ──────────────────────────────────────
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

        // ── Touch (GT911) ────────────────────────────────────────────
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

// ── Global display instance ──────────────────────────────────────────
static LGFX g_display;

// ── CH422G helper: write output pins ─────────────────────────────────
static void ch422g_write(uint8_t value) {
    Wire.beginTransmission(CH422G_ADDR >> 1);   // 7-bit address
    Wire.write(value);
    Wire.endTransmission();
}

static void ch422g_set_io_mode() {
    // Set CH422G to push-pull output mode
    Wire.beginTransmission(CH422G_SET_IO_CMD >> 1);
    Wire.write(0x01);   // Enable output mode
    Wire.endTransmission();
}

// ── Display initialization ───────────────────────────────────────────
static void display_init() {
    // Init I2C for CH422G and touch
    Wire.begin(GPIO_NUM_8, GPIO_NUM_9, 400000);
    delay(10);

    // Configure CH422G IO expander
    ch422g_set_io_mode();
    delay(10);

    // Assert touch reset (EXIO1 low), backlight off
    ch422g_write(0x00);
    delay(50);

    // Release touch reset (EXIO1 high), backlight still off
    ch422g_write(CH422G_EXIO1);
    delay(50);

    // Initialize LovyanGFX
    g_display.init();
    g_display.setRotation(0);

    // Turn on backlight (EXIO2 high) + keep touch reset released (EXIO1 high)
    ch422g_write(CH422G_EXIO1 | CH422G_EXIO2);
    delay(50);
}
