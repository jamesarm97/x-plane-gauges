# X-Plane Flight Gauge Dashboard

A real-time multi-instrument flight dashboard for the [Waveshare ESP32-S3-LCD-4.3](https://www.waveshare.com/esp32-s3-lcd-4.3.htm) touchscreen that connects to X-Plane 12 over WiFi using the native RREF UDP protocol.

Displays 6 gauges simultaneously on a 800x480 touchscreen in a 3x2 grid layout, with 29 instruments to choose from and 4 switchable view presets.

## Features

- **Automatic X-Plane discovery** — finds the simulator on your local network via BECN beacon packets (no hardcoded IP required)
- **6-gauge dashboard** — 3x2 grid showing 6 instruments simultaneously
- **29 instrument gauges** organized in 6 categories:
  - **Flight**: Airspeed, Altitude, VSI, Heading (compass card), Bank (attitude), Pitch, G-Force
  - **Controls**: Throttle, Flaps
  - **Engine**: RPM, Manifold Pressure, EGT, CHT, Oil Temp, Oil Pressure
  - **Fuel & Electrical**: Fuel Quantity, Fuel Flow, Bus Voltage
  - **Navigation**: Combined Lat/Lon Position, Latitude, Longitude
  - **Advanced**: Attitude Indicator, HSI, Annunciator Panel, Engine Cluster, Wind Indicator, G-Meter, Fuel Endurance, Trim Indicators
- **4 preset views** — quickly switch between predefined instrument layouts:
  - **FLIGHT**: Airspeed, Attitude, Altitude, HSI, Heading, VSI
  - **ENGINE**: RPM, Manifold, Oil Temp, Oil Pressure, EGT, Fuel Flow
  - **SYSTEMS**: Annunciator, Trim, G-Meter, Fuel Endurance, Wind, Volts
  - **CUSTOM**: User-configured layout saved to NVS flash
- **Touch interaction**:
  - Tap a cell to open the gauge picker popup
  - Two-finger swipe left/right to cycle between views
  - Two-finger tap to open the view selection popup
  - Long-press to toggle audio mute
- **Multi-dataref gauges** — advanced instruments subscribe to up to 8 X-Plane datarefs each
- **Smooth needle animation** with exponential smoothing at 30 FPS
- **Double-buffered rendering** — flicker-free sprite-based display updates
- **Multi-SSID support** — configure multiple WiFi networks; device auto-connects to the strongest available
- **Audio warnings** — buzzer beeps when gauge values enter red danger zones
- **Auto-reconnect** on WiFi or X-Plane connection loss with backlight dimming when disconnected

## Hardware

- [Waveshare ESP32-S3-LCD-4.3](https://www.waveshare.com/esp32-s3-lcd-4.3.htm) (ESP32-S3, 4.3" 800x480 RGB LCD, GT911 capacitive touch)
- USB-C cable for programming and power
- Optional: passive buzzer on GPIO 2 for audio warnings

## Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- X-Plane 12 running on the same local network
- WiFi network accessible by both the display and X-Plane host

## Setup

1. Clone this repository:
   ```bash
   git clone https://github.com/jamesarm97/x-plane-gauges.git
   cd x-plane-gauges
   git checkout waveshare-4.3
   ```

2. Set up WiFi credentials:
   ```bash
   cp src/wifi_credentials.example.h src/wifi_credentials.h
   ```
   Edit `src/wifi_credentials.h` with your WiFi network(s):
   ```cpp
   static const WiFiCredential WIFI_NETWORKS[] = {
       {"YourHomeSSID", "YourHomePassword"},
       {"YourOfficeSSID", "YourOfficePassword"},
   };
   ```

3. Build and upload:
   ```bash
   pio run -e waveshare43 -t upload
   ```

4. Start X-Plane 12 — the device will automatically discover the simulator on your network.

## Usage

- **Power on** — the device connects to WiFi, then discovers X-Plane via beacon broadcast
- **Tap a gauge cell** — opens a popup picker to choose from 29 instruments (scroll to browse categories)
- **Two-finger swipe** left/right — switch between FLIGHT, ENGINE, SYSTEMS, and CUSTOM views
- **Two-finger tap** — opens the view selection popup for direct view switching
- **Long-press any cell** (1 second) — toggle audio warning mute
- **CUSTOM view** — your gauge assignments are automatically saved to flash and restored on reboot

## How It Works

The ESP32-S3 listens for X-Plane's BECN beacon packets broadcast on UDP port 49707 to automatically discover the simulator's IP address and port. Once found, it subscribes to datarefs using the RREF protocol, receiving real-time flight data at 10 Hz. Advanced gauges use a multi-dataref subscription system where each gauge can request up to 8 datarefs routed via slot-based indexing. Each gauge is defined declaratively with tick marks, colored arcs, value ranges, and labels — advanced gauges use fully custom renderers for complex displays like the attitude indicator, HSI, and engine cluster.

## Project Structure

```
src/
├── main.cpp                    # Application state machine, touch handling, view system
├── config.h                    # Display, rendering, and color constants
├── display_driver.h            # Waveshare ST7262 RGB display + CH422G backlight
├── wifi_credentials.h          # Your WiFi networks (gitignored)
├── wifi_credentials.example.h  # Template for wifi_credentials.h
├── wifi_manager.h/cpp          # WiFiMulti connection management
├── xplane_discovery.h/cpp      # BECN beacon auto-discovery
├── xplane_client.h/cpp         # RREF UDP protocol (subscribe/receive datarefs)
├── gauge_renderer.h/cpp        # Sprite-based gauge rendering engine
├── gauge_picker.h/cpp          # Touch-based gauge selection popup
├── dashboard_layout.h          # 6-cell grid state management with NVS persistence
├── view_manager.h              # Predefined view layouts (FLIGHT/ENGINE/SYSTEMS/CUSTOM)
├── buzzer_driver.h             # Audio warning system for red zone alerts
└── gauges/
    ├── gauge_base.h            # GaugeConfig struct and GaugeBase class
    ├── gauge_registry.h        # All 29 gauge instances and registry array
    ├── airspeed_gauge.h        # Airspeed indicator (kts)
    ├── altitude_gauge.h        # Altimeter (ft)
    ├── vsi_gauge.h             # Vertical speed indicator (fpm)
    ├── heading_gauge.h         # Compass card (custom renderer)
    ├── bank_gauge.h            # Bank/attitude indicator (custom renderer)
    ├── pitch_gauge.h           # Pitch angle (degrees)
    ├── gforce_gauge.h          # G-force meter
    ├── throttle_gauge.h        # Throttle position (%)
    ├── flap_gauge.h            # Flap position (%)
    ├── rpm_gauge.h             # Engine RPM
    ├── manifold_gauge.h        # Manifold pressure (inHg)
    ├── egt_gauge.h             # Exhaust gas temperature (C)
    ├── cht_gauge.h             # Cylinder head temperature (C)
    ├── oil_temp_gauge.h        # Oil temperature (C)
    ├── oil_press_gauge.h       # Oil pressure (PSI)
    ├── fuel_gauge.h            # Fuel quantity (gal)
    ├── fuel_flow_gauge.h       # Fuel flow (GPH)
    ├── volts_gauge.h           # Bus voltage (V)
    ├── position_gauge.h        # Combined lat/lon (custom renderer)
    ├── lat_gauge.h             # Latitude (custom renderer)
    ├── lon_gauge.h             # Longitude (custom renderer)
    ├── attitude_gauge.h        # Attitude indicator with pitch ladder
    ├── hsi_gauge.h             # HSI with CDI and glideslope
    ├── annunciator_gauge.h     # Gear/Flaps/Stall/Overspeed panel
    ├── engine_cluster_gauge.h  # 6-bar engine instrument cluster
    ├── wind_gauge.h            # Wind speed and direction indicator
    ├── gmeter_gauge.h          # G-meter with peak tracking
    ├── fuel_endurance_gauge.h  # Fuel endurance timer (HH:MM)
    └── trim_gauge.h            # Elevator/aileron/rudder trim bars
platformio.ini                  # PlatformIO build configuration
```

## Configuration

WiFi credentials are in `src/wifi_credentials.h` (see Setup). Other settings in `src/config.h`:

| Setting | Default | Description |
|---------|---------|-------------|
| `XPLANE_FREQ` | 10 | Dataref update frequency (Hz) |
| `TARGET_FPS` | 30 | Display refresh rate |
| `SMOOTH_ALPHA` | 0.15 | Needle smoothing factor (0–1) |
| `PICKER_TIMEOUT_MS` | 5000 | Auto-close gauge picker (ms) |
| `WARNING_BEEP_FREQ` | 880 | Warning tone frequency in Hz (A5 note) |
| `MUTE_HOLD_MS` | 1000 | Long-press to toggle mute (ms) |

## Dependencies

Managed automatically by PlatformIO:

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) ^1.1.16
- Arduino WiFi, Wire, Preferences (ESP32 built-in)

## M5Dial Version

The `main` branch contains the original single-gauge version for the [M5Stack M5Dial](https://shop.m5stack.com/products/m5stack-dial-esp32-s3-smart-rotary-knob-w-1-28-round-touch-screen) (240x240 round display, rotary encoder input, 21 gauges).

## License

MIT
