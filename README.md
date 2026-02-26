# X-Plane Flight Gauge Display for M5Dial

A real-time flight instrument gauge display for the [M5Stack M5Dial](https://shop.m5stack.com/products/m5stack-dial-esp32-s3-smart-rotary-knob-w-1-28-round-touch-screen) (ESP32-S3) that connects to X-Plane 12 over WiFi using the native RREF UDP protocol.

Rotate the dial to browse through 21 different flight instruments rendered on the 240x240 round display with smooth needle animation and colored arc segments.

## Features

- **Automatic X-Plane discovery** — finds the simulator on your local network via BECN beacon packets (no hardcoded IP required)
- **21 instrument gauges** selectable via the rotary encoder:
  - **Flight**: Airspeed, Altitude, VSI, Heading (compass card), Bank (attitude indicator), Pitch, G-Force
  - **Controls**: Throttle, Flaps
  - **Engine**: RPM, Manifold Pressure, EGT, CHT, Oil Temp, Oil Pressure
  - **Fuel & Electrical**: Fuel Quantity, Fuel Flow, Bus Voltage
  - **Navigation**: Combined Lat/Lon Position, Latitude, Longitude
- **Smooth needle animation** with exponential smoothing at 30 FPS
- **Connection status indicator** — green/red dot shows live X-Plane link status
- **Double-buffered rendering** — flicker-free sprite-based display updates
- **Multi-SSID support** — configure multiple WiFi networks; device auto-connects to the strongest available
- **Audio warnings** — speaker beeps when gauge value enters a red danger zone (overspeed, over-temp, low fuel, etc.)
- **Mute control** — press and hold the dial button for 1 second to toggle audio mute
- **Auto-reconnect** on WiFi or X-Plane connection loss

## Hardware

- [M5Stack M5Dial](https://shop.m5stack.com/products/m5stack-dial-esp32-s3-smart-rotary-knob-w-1-28-round-touch-screen) (ESP32-S3, 1.28" round GC9A01 display, rotary encoder)
- USB-C cable for programming and power

## Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or VS Code extension)
- X-Plane 12 running on the same local network
- WiFi network accessible by both the M5Dial and X-Plane host

## Setup

1. Clone this repository:
   ```bash
   git clone https://github.com/yourusername/xplane-m5dial-gauge.git
   cd xplane-m5dial-gauge
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
   The device uses WiFiMulti to automatically connect to the strongest available network from the list.

3. Build and upload:
   ```bash
   pio run -t upload
   ```

4. Start X-Plane 12 — the device will automatically discover the simulator on your network.

## Usage

- **Power on** — the device connects to WiFi, then listens for X-Plane's beacon broadcast
- **Rotate the dial** — browse through available gauges (every detent click changes the gauge)
- **Gauge auto-confirms** after 3 seconds of inactivity on the selector
- **Press and hold the button** (1 second) — toggle audio warning mute; a "MUTE" indicator appears at the top of the display when muted

## How It Works

The M5Dial listens for X-Plane's BECN beacon packets broadcast on UDP port 49707 to automatically discover the simulator's IP address and port. Once found, it subscribes to datarefs using the RREF protocol on the discovered port, receiving real-time flight data at 10 Hz. Each gauge is defined declaratively with tick marks, colored arcs, value ranges, and labels — some gauges (heading compass, bank attitude, lat/lon coordinates) use fully custom renderers.

## Project Structure

```
src/
├── main.cpp                 # Application state machine and main loop
├── config.h                 # Display and rendering constants
├── wifi_credentials.h       # Your WiFi networks (gitignored)
├── wifi_credentials.example.h  # Template for wifi_credentials.h
├── wifi_manager.h/cpp       # WiFiMulti connection management
├── xplane_discovery.h/cpp   # BECN beacon auto-discovery
├── xplane_client.h/cpp      # RREF UDP protocol (subscribe/receive datarefs)
├── gauge_renderer.h/cpp     # Sprite-based gauge rendering engine
├── gauge_selector.h/cpp     # Rotary encoder gauge selection UI
├── warning_beeper.h         # Audio warning system for red zone alerts
└── gauges/
    ├── gauge_base.h         # GaugeConfig struct and GaugeBase class
    ├── gauge_registry.h     # All gauge instances and registry array
    ├── airspeed_gauge.h     # Airspeed indicator (kts)
    ├── altitude_gauge.h     # Altimeter (ft)
    ├── vsi_gauge.h          # Vertical speed indicator (fpm)
    ├── heading_gauge.h      # Compass card (custom renderer)
    ├── bank_gauge.h         # Bank/attitude indicator (custom renderer)
    ├── pitch_gauge.h        # Pitch angle (degrees)
    ├── gforce_gauge.h       # G-force meter
    ├── throttle_gauge.h     # Throttle position (%)
    ├── flap_gauge.h         # Flap position (%)
    ├── rpm_gauge.h          # Engine RPM
    ├── manifold_gauge.h     # Manifold pressure (inHg)
    ├── egt_gauge.h          # Exhaust gas temperature (C)
    ├── cht_gauge.h          # Cylinder head temperature (C)
    ├── oil_temp_gauge.h     # Oil temperature (C)
    ├── oil_press_gauge.h    # Oil pressure (PSI)
    ├── fuel_gauge.h         # Fuel quantity (gal)
    ├── fuel_flow_gauge.h    # Fuel flow (GPH)
    ├── volts_gauge.h        # Bus voltage (V)
    ├── position_gauge.h     # Combined lat/lon (custom renderer)
    ├── lat_gauge.h          # Latitude (custom renderer)
    └── lon_gauge.h          # Longitude (custom renderer)
platformio.ini               # PlatformIO build configuration
```

## Configuration

WiFi credentials are in `src/wifi_credentials.h` (see Setup). Other settings in `src/config.h`:

| Setting | Default | Description |
|---------|---------|-------------|
| `XPLANE_FREQ` | 10 | Dataref update frequency (Hz) |
| `TARGET_FPS` | 30 | Display refresh rate |
| `SMOOTH_ALPHA` | 0.15 | Needle smoothing factor (0–1) |
| `SELECTOR_TIMEOUT_MS` | 3000 | Auto-confirm gauge selection (ms) |
| `WARNING_BEEP_FREQ` | 880 | Warning tone frequency in Hz (A5 note) |
| `WARNING_BEEP_ON_MS` | 200 | Warning beep duration (ms) |
| `WARNING_BEEP_OFF_MS` | 300 | Silence between beeps (ms) |
| `WARNING_VOLUME` | 80 | Speaker volume (0–255) |
| `MUTE_HOLD_MS` | 1000 | Button hold time to toggle mute (ms) |

## Dependencies

Managed automatically by PlatformIO:

- [M5Dial](https://github.com/m5stack/M5Dial) ^1.0.2
- [M5Unified](https://github.com/m5stack/M5Unified) ^0.2.2
- [M5GFX](https://github.com/m5stack/M5GFX) ^0.2.3
- Arduino WiFi (ESP32 built-in)

## License

MIT
