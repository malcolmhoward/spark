# S.P.A.R.K. - Sensor-based Positioning and Actuation Repulsor Kinetics (Embedded Hand Firmware)

<img src="https://www.oasisproject.net/assets/SPARK_Logo_bg.png" alt="SPARK Logo" width="350" align="right">

This code is a healthy mix of example code from various libraries with a special thanks to Adafruit for their awesome hardware and software.

## Overview

S.P.A.R.K. (Sensor-based Positioning and Actuation Repulsor Kinetics) is the embedded hand/gauntlet firmware for the O.A.S.I.S. ecosystem. It provides gesture recognition, environmental sensing, and visual feedback for wearable gauntlet devices.

SPARK runs on ESP32-based microcontrollers and supports two communication modes: WiFi/MQTT for network integration and ESP-NOW for low-latency peer-to-peer links with the AURA helmet hub. The firmware includes two variants: a full gauntlet implementation with IMU gesture recognition and NeoPixel effects (`spark-v1`), and a generic sensor node for auxiliary mounting points (`spark-generic-v1`).

Key capabilities:
- IMU-based gesture recognition (power-up and fire gestures) with configurable thresholds
- NeoPixel LED effects (19 LEDs: 7 jewel + 12 ring) for visual feedback
- Environmental sensing (temperature, humidity via AHT20)
- Battery voltage monitoring
- Dual communication: WiFi/MQTT or ESP-NOW encrypted peer-to-peer
- Audio command dispatch for gesture-triggered sound effects

**Note:** I will have a circuit diagram in the future. This is a pretty simple one though. All of the devices are I2C devices that can be daisy-chained and it's powered by USB.

## Hardware

### spark-v1 (Right Hand Gauntlet)

* [Adafruit QT Py ESP32-S3](https://www.adafruit.com/product/5426)
* [Adafruit LSM6DSO32](https://www.adafruit.com/product/4692) - 6-DOF IMU (accelerometer + gyroscope)
* [NeoPixel Jewel - 7 x 5050 RGB LED](https://www.adafruit.com/product/2226)
* [NeoPixel Ring - 12 x 5050 RGB LED](https://www.adafruit.com/product/1643)

### spark-generic-v1 (Generic Sensor Node)

| Component | Description | Link |
|-----------|-------------|------|
| Seeed XIAO ESP32-C3 | Primary microcontroller (also supports ESP32-S3/S2) | [Seeed Studio](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html) |
| Adafruit AHT20 | Temperature and humidity sensor (I2C) | [Adafruit](https://www.adafruit.com/product/4566) |

## Libraries

- [ArduinoMqttClient](https://github.com/arduino-libraries/ArduinoMqttClient)
- [WiFi](https://github.com/arduino-libraries/WiFi)
- [Adafruit LSM6DSO32](https://github.com/adafruit/Adafruit_LSM6DS)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [FastLED](https://github.com/FastLED/FastLED)

## Building

This firmware is currently being built in the Arduino IDE using the libraries and target hardware mentioned above.

1. Open `spark-v1/spark-v1.ino` or `spark-generic-v1/spark-generic-v1.ino` in Arduino IDE 2.x
2. Select board: Seeed XIAO ESP32-C3 (generic) or Adafruit QT Py ESP32-S3 (v1)
3. Configure `arduino_secrets.h` with WiFi credentials and ESP-NOW PMK
4. Upload via USB

Alternatively, using Arduino CLI:
```bash
arduino-cli compile --fqbn esp32:esp32:XIAO_ESP32C3 spark-generic-v1/
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp32:esp32:XIAO_ESP32C3 spark-generic-v1/
```

## Configuration

### Communication Mode

Set in the `.ino` file (compile-time):
- `#define COMMUNICATION_MODE COMM_MODE_WIFI` — WiFi with MQTT
- `#define COMMUNICATION_MODE COMM_MODE_ESPNOW` — ESP-NOW peer-to-peer

### Network Settings

- WiFi credentials and ESP-NOW PMK: `arduino_secrets.h`
- MQTT broker: Auto-detected from WiFi gateway IP, port 1883
- ESP-NOW channel: 1 (configurable via `ESPNOW_CHANNEL`)
- Connection retry attempts: 5 (configurable via `CONN_RETRY_ATTEMPTS`)

### Pin Assignments

| Pin | ESP32-C3 | ESP32-S3/S2 | Purpose |
|-----|----------|-------------|---------|
| Voltage ADC | A0 | A2 | Battery voltage monitoring |
| LED Data | GPIO 10 | GPIO 35 | NeoPixel data line (spark-v1) |

### Gesture Thresholds (spark-v1)

| Gesture | Axis | Threshold | Description |
|---------|------|-----------|-------------|
| Power activation | accel.x | > 7.0 m/s² | Arm raise to power up |
| Fire activation | accel.x | > 9.2 m/s² | Quick thrust to fire |

IMU settings: Accelerometer range 16G, Gyroscope range 1000 DPS, Data rate 12.5 Hz.

### Data Transmission Intervals

- spark-generic-v1: Every 5 seconds
- spark-v1: Every 1 second (sensor data), ~64ms (gesture loop)

## Usage

1. Flash the firmware (see Building section)
2. Monitor via serial at 115200 baud for debug output
3. The device automatically:
   - Connects to WiFi/MQTT broker or discovers AURA via ESP-NOW broadcast
   - Begins sensor polling and data transmission
   - (spark-v1) Monitors IMU for gestures and drives NeoPixel effects

### MQTT Topics

- spark-v1 publishes to: `repulsor/right`
- spark-generic-v1 publishes to: `shoulder/left`

## Communication

SPARK supports two communication modes, selectable at compile time.

### WiFi/MQTT Mode

Connects to the MQTT broker (auto-detected from WiFi gateway IP, port 1883).

| Direction | Topic | Payload Example |
|-----------|-------|-----------------|
| Publish | `repulsor/right` (v1) or `shoulder/left` (generic) | `{"device": "repulsor/right", "voltage": 4.20, "temp": 24.5}` |
| Publish | `dawn` | `{"device": "audio", "command": "play", "arg1": "hand_rep_fire2.ogg"}` (gesture events, v1 only) |
| Subscribe | Device-specific topic | `{"device": "<name>", "value": <integer>}` |

### ESP-NOW Mode

Direct peer-to-peer communication with AURA as the hub. Uses encrypted ESP-NOW with shared PMK.

Registration flow:
1. SPARK broadcasts registration message with its topic identifier
2. AURA responds with ACK (accepted) or REJECT (duplicate topic)
3. After ACK, encrypted data exchange begins
4. Inactive connections timeout after 30 seconds; rejected peers retry after 35 seconds

ESP-NOW message types: REGISTRATION, REG_ACK, REG_REJECT, DATA, DATA_ACK, PING, PONG.

### Data Transmitted

| Data | Source | Variant |
|------|--------|---------|
| Battery voltage | ADC pin | Both |
| Temperature | AHT20 (generic) or LSM6DSO32 internal (v1) | Both |
| Humidity | AHT20 | Generic only |
| Gesture events (audio commands) | LSM6DSO32 IMU | v1 only |

## Troubleshooting

| Issue | Possible Cause | Solution |
|-------|---------------|----------|
| WiFi connection fails | Wrong credentials | Check `arduino_secrets.h` SSID and password |
| ESP-NOW registration rejected | Duplicate topic on hub | Use a unique topic identifier per device |
| ESP-NOW no hub found | Channel mismatch or AURA offline | Verify ESP-NOW channel matches AURA; ensure AURA is powered |
| No sensor data | I2C wiring issue | Check I2C connections (SDA/SCL); verify sensor power |
| Battery voltage reads 0 | Wrong ADC pin for board | Use A0 for ESP32-C3, A2 for ESP32-S3/S2 |
| NeoPixels not lighting | Wrong LED pin or power | Verify LED_PIN matches board; check 5V power supply |
| IMU gestures not triggering | Thresholds too high | Adjust acceleration thresholds in gesture detection code |
| Build fails | Missing board support | Install ESP32 board support package in Arduino IDE |
| Serial output garbled | Wrong baud rate | Set serial monitor to 115200 |

## Related Components

- [A.U.R.A.](https://www.oasisproject.net/components/aura/) - SPARK registers as an ESP-NOW peer with AURA; sends sensor data for relay to the HUD
- [D.A.W.N.](https://www.oasisproject.net/components/dawn/) - Receives gesture-triggered audio commands via MQTT (`dawn` topic)
- [M.I.R.A.G.E.](https://www.oasisproject.net/components/mirage/) - Displays hand/gauntlet sensor feedback on the HUD
- [B.E.A.C.O.N.](https://www.oasisproject.net/components/beacon/) - CAD models for the gauntlet enclosure that houses SPARK hardware
