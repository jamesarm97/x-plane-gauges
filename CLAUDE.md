# X-Plane M5Dial Flight Gauge Display

## Project Overview
ESP32-S3 (M5Dial) device that displays real-time X-Plane flight simulator instrument gauges on a 240x240 circular display. Uses UDP to auto-discover X-Plane and subscribe to datarefs at 10Hz.

## Build & Deploy
```bash
# Build
pio run

# Upload to device (USB-C)
pio run -t upload

# Monitor serial output
pio device monitor -b 115200

# Build + Upload + Monitor
pio run -t upload && pio device monitor -b 115200
```

## Hardware
- **Board:** M5Dial (ESP32-S3, 8MB flash, PSRAM)
- **Display:** 240x240 circular, GC9A01 driver
- **Input:** Rotary encoder + button
- **Speaker:** Built-in via `M5Dial.Speaker` (M5Unified Speaker_Class)
- **Platform:** PlatformIO with espressif32 v6.5.0

## Architecture

### State Machine (main.cpp)
```
STATE_WIFI_CONNECTING → STATE_DISCOVERING → STATE_RUNNING
```

### Gauge System
- **gauge_base.h** - Abstract base with GaugeConfig (title, dataref, arcs, ticks, ranges)
- **gauge_registry.h** - All 21 gauges registered in categories
- **gauge_renderer.h/cpp** - Renders gauges to M5Canvas sprite, then pushes to display
- **gauge_selector.h/cpp** - Encoder-based gauge selection with 3s auto-confirm, saved to NVS

### Key Structures
- `ArcSegment` - Colored zone (startValue, endValue, color)
- `TickConfig` - Scale marks (majorInterval, minorInterval, labelDivisor)
- `GaugeConfig` - Full gauge definition including custom renderer flag

### Custom Renderers (bypass standard arc/needle drawing)
- **Heading** - Rotating compass card
- **Bank** - Artificial horizon (sky/ground split)
- **Position/Lat/Lon** - Text-based coordinate displays

### Data Flow
XPlane UDP → xplane_client → main.cpp (smoothing) → gauge_renderer → M5Canvas → display

## File Layout
```
src/
├── config.h              # Display dims, colors, timing constants
├── main.cpp              # App entry, state machine, render loop
├── gauge_renderer.h/cpp  # Arc/needle/tick rendering engine
├── gauge_selector.h/cpp  # Rotary encoder gauge switching
├── wifi_manager.h/cpp    # WiFi connection with visual feedback
├── wifi_credentials.h    # EXCLUDED from git (secrets)
├── xplane_client.h/cpp   # UDP subscribe/receive for datarefs
├── xplane_discovery.h/cpp # X-Plane BECN beacon auto-discovery
└── gauges/
    ├── gauge_base.h      # Abstract base class
    ├── gauge_registry.h  # Central registry of all gauges
    └── *.h               # Individual gauge configs (21 gauges)
```

## Gauges with RED/Danger Zones (candidates for audio alerts)
| Gauge | Danger Condition | Dataref |
|-------|-----------------|---------|
| Airspeed | >130 KTS (Vne) or <40 KTS (stall) | sim/flightmodel/position/indicated_airspeed |
| Oil Pressure | <25 PSI or >95 PSI | sim/flightmodel/engine/ENGN_oil_press_[0] |
| Oil Temp | >110 C | sim/flightmodel/engine/ENGN_oil_temp_[0] |
| CHT | >260 C | sim/flightmodel/engine/ENGN_cht_[0] |
| EGT | >900 C | sim/flightmodel/engine/ENGN_egt_[0] |
| Fuel | <8 GAL | sim/flightmodel/weight/m_fuel[0] |
| Volts | <11V or >15V | sim/flightmodel/engine/ENGN_bat_volt[0] |
| RPM | >2500 RPM | sim/flightmodel/engine/ENGN_tacrad[0] |
| Manifold | >35 inHg | sim/flightmodel/engine/ENGN_MPR[0] |
| G-Force | <0G or >3G | sim/flightmodel/position/g_nrml |
| Pitch | >20 or <-10 deg | sim/flightmodel/position/theta |

## Conventions
- All gauge configs are header-only (.h files) with static constexpr data
- Colors use RGB565 format; standard set defined in config.h
- Needle smoothing uses exponential filter (alpha=0.15)
- WiFi credentials go in wifi_credentials.h (gitignored); see .example.h for template
- GitHub remote: git@github.com:jamesarm97/x-plane-gauges.git
