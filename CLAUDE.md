# CLAUDE.md - LLM Integration Guide

## Project Overview

**SPARK** (Sensor-based Positioning and Actuation Repulsor Kinetics) is the embedded hand/gauntlet firmware for the O.A.S.I.S. wearable computing platform. It handles gesture recognition, sensor data collection, and wireless communication with the helmet system.

---

## Architecture

### Hardware Targets

| Platform | Microcontroller | Use Case |
|----------|-----------------|----------|
| XIAO ESP32-C3 | ESP32-C3 | Primary development target |
| XIAO ESP32-S3 | ESP32-S3 | Alternative with more GPIO |
| XIAO ESP32-S2 | ESP32-S2 | Alternative target |

### Communication Modes

SPARK supports two wireless communication modes:

| Mode | Protocol | Use Case |
|------|----------|----------|
| `COMM_MODE_WIFI` | MQTT over WiFi | Standard networking, debugging |
| `COMM_MODE_ESPNOW` | ESP-NOW | Low-latency peer-to-peer |

### Key Components

| Component | Purpose |
|-----------|---------|
| `spark-v1/` | Original SPARK firmware |
| `spark-generic-v1/` | Generic/portable version with ESP-NOW |
| `espnow_client.*` | ESP-NOW communication layer |
| `arduino_secrets.h` | WiFi/MQTT credentials (template) |

---

## Build System

### Prerequisites

- Arduino IDE 2.x or PlatformIO
- ESP32 board support package
- Required libraries:
  - `ArduinoMqttClient`
  - `ArduinoJson`
  - `Adafruit AHTX0` (temperature/humidity sensor)
  - `WiFi` (ESP32 built-in)

### Build Steps

```bash
# Arduino IDE
1. Open spark-generic-v1/spark-generic-v1.ino
2. Select board: XIAO ESP32-C3 (or appropriate target)
3. Copy arduino_secrets.h.template to arduino_secrets.h
4. Configure WiFi/MQTT credentials
5. Upload to device
```

### Configuration

Key compile-time options in the sketch:

```cpp
#define DEBUG                    // Enable serial debug output
#define SUPPRESS_NOISE           // Filter sensor noise
#define COMMUNICATION_MODE       // COMM_MODE_WIFI or COMM_MODE_ESPNOW
#define CONN_RETRY_ATTEMPTS 5    // WiFi connection retries
```

---

## MQTT Topics

| Topic | Direction | Purpose |
|-------|-----------|---------|
| `shoulder/left` | Publish | Left hand sensor data |
| `shoulder/right` | Publish | Right hand sensor data |

Message format is JSON via ArduinoJson.

---

## Working with This Codebase

### When Modifying Firmware

1. **Test on hardware** - Embedded code requires physical testing
2. **Preserve power efficiency** - Battery-powered device
3. **Maintain latency requirements** - Real-time gesture response
4. **Document pin assignments** - Hardware connections matter
5. **Keep credentials separate** - Never commit `arduino_secrets.h`

### Common Tasks

| Task | Approach |
|------|----------|
| Add new sensor | Implement in setup/loop, publish via MQTT |
| Change communication mode | Modify `COMMUNICATION_MODE` define |
| Debug communication | Enable `DEBUG` define, check serial output |
| Add new gesture | Implement detection logic, publish event |

### Code Style

- Arduino/C++ conventions
- Use descriptive variable names
- Comment hardware-specific code
- Keep ISRs minimal
- Prefer non-blocking patterns

---

## Integration Points

### O.A.S.I.S. Ecosystem

| Component | Relationship |
|-----------|--------------|
| **AURA** | Receives sensor data, coordinates with helmet |
| **DAWN** | Processes gesture commands via MQTT |
| **MIRAGE** | Displays feedback for hand interactions |

### S.C.O.P.E. Coordination

- **Meta-repo**: [malcolmhoward/the-oasis-project-meta-repo](https://github.com/malcolmhoward/the-oasis-project-meta-repo)
- **Tracking issue**: See S.C.O.P.E. for cross-component coordination
- **Documentation**: Aggregated in S.C.O.P.E. coordination docs

---

## Commands

```bash
# Verify Arduino CLI (if using CLI)
arduino-cli board list

# Compile (Arduino CLI)
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 spark-generic-v1/

# Upload (Arduino CLI)
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:XIAO_ESP32C3 spark-generic-v1/
```

---

## License

SPARK is licensed under **GPLv3**. See LICENSE for details.
