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

#include "arduino_secrets.h"
#include "espnow_client.h"

// Global state variables
uint8_t espnow_sequence = 0;
uint8_t espnow_hub_mac[6] = {0};
char espnow_device_topic[32] = {0};
unsigned long last_message_time = 0;
bool topic_rejected = false;
unsigned long last_reg_time = 0;
unsigned long last_data_time = 0;
unsigned long last_rejection_time = 0;
const unsigned long REJECTION_RETRY_DELAY = 35000;  // 35 seconds (longer than AURA's 30s timeout)

/*
 * ESP-Now shared key (must match on all devices)
 * NOTE: If you're using this code, change this key.
 *       1) It would be insecure to keep this key since it's public.
 *       2) If you don't change it and there's someone else using this code,
 *          your devices will connect to other systems.
 *
 * SECRET_PMK is located in the arduino_secrets.h.
 */
static const uint8_t PMK[16] = SECRET_PMK;

// ESP-Now broadcast address
static const uint8_t broadcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ESP-Now callback for received data
void onESPNowDataRecv(const esp_now_recv_info* info, const uint8_t* data, int len) {
  const uint8_t* mac = info->src_addr;

  // Update last message time
  last_message_time = millis();

  if (len < sizeof(espnow_message_t)) {
    Serial.println("ESP-Now: Malformed message (too short)");
    return;
  }

  espnow_message_t msg;
  memcpy(&msg, data, sizeof(espnow_message_t));

  switch (msg.type) {
    case ESPNOW_MSG_REG_ACK: {
      Serial.println("ESP-Now: Registration acknowledged by AURA hub");

      if (topic_rejected) {
        Serial.println("ESP-Now: Accepted after previous rejection");
        topic_rejected = false;
      }

      memcpy(espnow_hub_mac, mac, 6);

      esp_now_peer_info_t peer_info;
      memset(&peer_info, 0, sizeof(peer_info));
      memcpy(peer_info.peer_addr, mac, 6);
      peer_info.channel = ESPNOW_CHANNEL;
      peer_info.encrypt = true;
      memcpy(peer_info.lmk, PMK, 16);

      esp_err_t result = esp_now_add_peer(&peer_info);
      if (result != ESP_OK) {
        Serial.print("ESP-Now: Failed to add peer: ");
        Serial.println(esp_err_to_name(result));
        return;
      }

      Serial.println("ESP-Now: Registered and ready");
      break;
    }

    case ESPNOW_MSG_REG_REJECT: {
      Serial.println("ESP-Now: Registration rejected - topic in use");
      topic_rejected = true;
      last_rejection_time = millis();
      break;
    }

    case ESPNOW_MSG_DATA_ACK: {
      // Silently acknowledge — no logging to avoid UART blocking during rapid fire
      break;
    }

    case ESPNOW_MSG_PING: {
      // Send pong response
      espnow_message_t pong;
      memset(&pong, 0, sizeof(pong));
      pong.type = ESPNOW_MSG_PONG;
      strncpy(pong.topic, espnow_device_topic, sizeof(pong.topic));
      pong.timestamp = millis();

      uint8_t self_mac[6];
      WiFi.macAddress(self_mac);
      memcpy(pong.mac, self_mac, 6);

      esp_now_send(mac, (uint8_t*)&pong, sizeof(pong));
#ifdef DEBUG
      Serial.println("ESP-Now: Pong sent");
#endif
      break;
    }

    default: {
#ifdef DEBUG
      Serial.print("ESP-Now: Unknown message type: ");
      Serial.println(msg.type);
#endif
      break;
    }
  }
}

// ESP-Now callback for send status
void onESPNowDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) {
    Serial.println("ESP-Now: Send failed");
  }
}

// Initialize ESP-Now client
void setupESPNowClient(const char* deviceTopic) {
  Serial.println("ESP-Now: Initializing...");

  // Save device topic
  strncpy(espnow_device_topic, deviceTopic, sizeof(espnow_device_topic));

  // Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Set channel
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  // Print MAC address
  uint8_t self_mac[6];
  WiFi.macAddress(self_mac);
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
         self_mac[0], self_mac[1], self_mac[2],
         self_mac[3], self_mac[4], self_mac[5]);
  Serial.print("ESP-Now: MAC ");
  Serial.print(macStr);
  Serial.print(" channel ");

  uint8_t channel;
  esp_wifi_get_channel(&channel, NULL);
  Serial.println(channel);

  // Initialize ESP-Now
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-Now: Init failed!");
    return;
  }
  delay(100);

  // Set PMK for encryption
  if (esp_now_set_pmk((uint8_t *)PMK) != ESP_OK) {
    Serial.println("ESP-Now: Failed to set PMK");
  }

  // Register callbacks
  esp_now_register_recv_cb(onESPNowDataRecv);
  esp_now_register_send_cb(onESPNowDataSent);

  // Add broadcast peer for discovery
  esp_now_peer_info_t peer_info;
  memset(&peer_info, 0, sizeof(peer_info));
  memcpy(peer_info.peer_addr, broadcast_addr, 6);
  peer_info.channel = ESPNOW_CHANNEL;
  peer_info.encrypt = false;

  esp_now_del_peer(broadcast_addr);
  delay(10);

  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    Serial.println("ESP-Now: Failed to add broadcast peer");
    return;
  }

  // Reset state
  espnow_sequence = 0;
  memset(espnow_hub_mac, 0, 6);
  last_message_time = 0;
  last_reg_time = 0;
  last_data_time = 0;
  topic_rejected = false;

  Serial.println("ESP-Now: Client initialized");

  // Send initial registration
  sendESPNowRegistration();
}

// Send registration message
void sendESPNowRegistration() {
  if (topic_rejected) {
    return;
  }

  espnow_message_t msg;
  memset(&msg, 0, sizeof(msg));

  msg.type = ESPNOW_MSG_REGISTRATION;
  strncpy(msg.topic, espnow_device_topic, sizeof(msg.topic));
  msg.timestamp = millis();

  uint8_t self_mac[6];
  WiFi.macAddress(self_mac);
  memcpy(msg.mac, self_mac, 6);

  last_reg_time = millis();

  Serial.println("ESP-Now: Sending registration");
  esp_now_send(broadcast_addr, (uint8_t*)&msg, sizeof(msg));
}

// Check if we're connected to AURA hub
bool isESPNowConnected() {
  return (memcmp(espnow_hub_mac, "\0\0\0\0\0\0", 6) != 0) && !topic_rejected;
}

// Send JSON data via ESP-Now
bool sendESPNowData(const char* jsonString) {
  if (!isESPNowConnected()) {
    return false;
  }

  espnow_message_t msg;
  memset(&msg, 0, sizeof(msg));

  msg.type = ESPNOW_MSG_DATA;
  msg.sequence = espnow_sequence++;
  strncpy(msg.topic, espnow_device_topic, sizeof(msg.topic));
  msg.timestamp = millis();

  uint8_t self_mac[6];
  WiFi.macAddress(self_mac);
  memcpy(msg.mac, self_mac, 6);

  size_t json_len = strlen(jsonString);
  if (json_len > sizeof(msg.data)) {
    json_len = sizeof(msg.data);
  }
  memcpy(msg.data, jsonString, json_len);
  msg.data_len = json_len;

  last_data_time = millis();

  esp_err_t result = esp_now_send(espnow_hub_mac, (uint8_t*)&msg, sizeof(msg));

  if (result != ESP_OK) {
    Serial.println("ESP-Now: Send failed");
    return false;
  }

  return true;
}

// Helper function to handle time differences safely
unsigned long timeSince(unsigned long start) {
  unsigned long now = millis();
  return (now >= start) ? (now - start) : 0;
}

// Main loop function for ESP-Now client
void loopESPNowClient() {
   // Registration logic
   if (!isESPNowConnected()) {
      // If we have a cached hub MAC and weren't rejected, try direct connection first
      if (memcmp(espnow_hub_mac, "\0\0\0\0\0\0", 6) != 0 && !topic_rejected) {
         static unsigned long last_direct_attempt = 0;
         if (timeSince(last_direct_attempt) > 5000) {
            Serial.println("ESP-Now: Direct reconnection attempt");

            espnow_message_t msg;
            memset(&msg, 0, sizeof(msg));
            msg.type = ESPNOW_MSG_REGISTRATION;
            strncpy(msg.topic, espnow_device_topic, sizeof(msg.topic));
            msg.timestamp = millis();
            WiFi.macAddress(msg.mac);

            esp_err_t result = esp_now_send(espnow_hub_mac, (uint8_t*)&msg, sizeof(msg));
            if (result == ESP_OK) {
               last_reg_time = millis();
               last_direct_attempt = millis();
               return;
            }
            memset(espnow_hub_mac, 0, 6);
         }
      }

      // Broadcast registration for discovery
      if (!topic_rejected && timeSince(last_reg_time) > 5000) {
         sendESPNowRegistration();
      }
   }

   // Show rejection status periodically
   static unsigned long last_rejection_msg = 0;
   if (topic_rejected && timeSince(last_rejection_msg) > 10000) {
      unsigned long remaining = REJECTION_RETRY_DELAY - timeSince(last_rejection_time);
      Serial.print("ESP-Now: Rejected, retry in ");
      Serial.print(remaining / 1000);
      Serial.println("s");
      last_rejection_msg = millis();
   }

   // Connection timeout
   if (isESPNowConnected() && timeSince(last_message_time) > 30000) {
      Serial.println("ESP-Now: Timeout, reconnecting");

      esp_now_del_peer(espnow_hub_mac);

      memset(espnow_hub_mac, 0, 6);
      last_reg_time = millis() - 6000;
      topic_rejected = false;
   }
}
