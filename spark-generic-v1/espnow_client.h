/*
 * This file is part of the OASIS Project.
 * https://github.com/orgs/The-OASIS-Project/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * By contributing to this project, you agree to license your contributions
 * under the GPLv3 (or any later version) or any future licenses chosen by
 * the project author(s). Contributions include any modifications,
 * enhancements, or additions to the project. These contributions become
 * part of the project and are adopted by the project author(s).
 */

#ifndef ESPNOW_CLIENT_H
#define ESPNOW_CLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>

// Enable ESP-Now mode
#define USE_ESPNOW

// ESP-Now channel (1-14)
#define ESPNOW_CHANNEL 1

// ESP-Now message types
#define ESPNOW_MSG_REGISTRATION    1  // Registration request from SPARK
#define ESPNOW_MSG_REG_ACK         2  // Registration acknowledgment
#define ESPNOW_MSG_REG_REJECT      3  // Registration rejection (topic already exists)
#define ESPNOW_MSG_DATA            4  // Data message from SPARK
#define ESPNOW_MSG_DATA_ACK        5  // Data acknowledgment
#define ESPNOW_MSG_PING            6  // Ping to check connectivity
#define ESPNOW_MSG_PONG            7  // Pong response

// ESP-Now message structure (must match AURA implementation)
typedef struct {
  uint8_t type;                      // Message type
  uint8_t sequence;                  // Sequence number
  char topic[32];                    // Device topic/identifier
  uint8_t mac[6];                    // Sender MAC address
  uint32_t timestamp;                // Message timestamp (millis)
  uint8_t data_len;                  // Length of data payload
  uint8_t data[200];                 // Data payload (JSON or other)
} espnow_message_t;

// Function prototypes
void setupESPNowClient(const char* deviceTopic);
void loopESPNowClient();
bool sendESPNowData(const char* jsonString);
bool isESPNowConnected();
void sendESPNowRegistration();

// Global state variables
extern uint8_t espnow_sequence;       // Message sequence counter
extern uint8_t espnow_hub_mac[6];     // AURA hub MAC address
extern char espnow_device_topic[32];  // This device's topic
extern unsigned long last_message_time; // Last time we received any message
extern bool topic_rejected;           // Whether our topic was rejected
extern unsigned long last_reg_time;   // Last time we sent registration
extern unsigned long last_data_time;  // Last time we sent data

#endif // ESPNOW_CLIENT_H