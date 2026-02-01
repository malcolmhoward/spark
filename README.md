# SPARK

**Sensor-based Positioning and Actuation Repulsor Kinetics** - Embedded hand/gauntlet firmware for the O.A.S.I.S. wearable computing platform.

## Overview

SPARK provides gesture recognition, sensor data collection, and wireless communication capabilities for O.A.S.I.S. hand/gauntlet modules. It runs on ESP32-based microcontrollers and communicates with the helmet system via MQTT or ESP-NOW.

## Features

- **Gesture Recognition** - Detect hand movements and positions
- **Sensor Integration** - Temperature, humidity, and motion sensing
- **Dual Communication** - WiFi/MQTT or low-latency ESP-NOW
- **Multiple Targets** - Support for ESP32-C3, S2, and S3 variants

## Hardware

| Component | Recommended |
|-----------|-------------|
| Microcontroller | Seeed XIAO ESP32-C3 |
| Sensors | Adafruit AHT20 (temp/humidity) |
| Power | 3.7V LiPo battery |

## Quick Start

### Prerequisites

- Arduino IDE 2.x or PlatformIO
- ESP32 board support package
- Required libraries:
  - ArduinoMqttClient
  - ArduinoJson
  - Adafruit AHTX0

### Installation

1. Clone this repository
2. Copy `arduino_secrets.h.template` to `arduino_secrets.h`
3. Configure your WiFi and MQTT credentials
4. Open `spark-generic-v1/spark-generic-v1.ino` in Arduino IDE
5. Select your board (XIAO ESP32-C3)
6. Upload to device

### Configuration

Edit compile-time options in the sketch:

```cpp
#define COMMUNICATION_MODE COMM_MODE_ESPNOW  // or COMM_MODE_WIFI
#define DEBUG                                 // Enable serial debug
```

## Project Structure

```
spark/
├── spark-v1/              # Original firmware
│   ├── spark-v1.ino
│   └── arduino_secrets.h
├── spark-generic-v1/      # Generic/portable version
│   ├── spark-generic-v1.ino
│   ├── espnow_client.cpp
│   ├── espnow_client.h
│   └── arduino_secrets.h
├── CLAUDE.md              # LLM integration guide
├── CONTRIBUTING.md        # Contribution guidelines
└── LICENSE                # GPLv3
```

## Communication

### MQTT Topics

| Topic | Purpose |
|-------|---------|
| `shoulder/left` | Left hand sensor data |
| `shoulder/right` | Right hand sensor data |

### ESP-NOW

Direct peer-to-peer communication with paired devices for low-latency operation.

## Related Projects

SPARK is part of the [O.A.S.I.S. Project](https://github.com/The-OASIS-Project):

| Component | Purpose |
|-----------|---------|
| [MIRAGE](https://github.com/The-OASIS-Project/mirage) | HUD display system |
| [DAWN](https://github.com/The-OASIS-Project/dawn) | AI voice assistant |
| [AURA](https://github.com/The-OASIS-Project/aura) | Helmet sensor firmware |
| [BEACON](https://github.com/The-OASIS-Project/beacon) | CAD models |
| [GENESIS](https://github.com/The-OASIS-Project/genesis) | Python utilities |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the **GNU General Public License v3.0** - see [LICENSE](LICENSE) for details.
