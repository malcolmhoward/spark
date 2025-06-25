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

#include "espnow_client.h"

// Global state variables
uint8_t espnow_sequence = 0;
uint8_t espnow_hub_mac[6] = {0};
char espnow_device_topic[32] = {0};
unsigned long last_message_time = 0;
bool topic_rejected = false;
unsigned long last_reg_time = 0;
unsigned long last_data_time = 0;

// ESP-Now shared key (must match on all devices)
static const uint8_t PMK[16] = {
  0x54, 0x68, 0x69, 0x73, 0x49, 0x73, 0x41, 0x53, 
  0x68, 0x61, 0x72, 0x65, 0x64, 0x4B, 0x65, 0x79  /* "ThisIsASharedKey" in hex */
};

// ESP-Now broadcast address
static const uint8_t broadcast_addr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ESP-Now callback for received data - updated for new API
void onESPNowDataRecv(const esp_now_recv_info* info, const uint8_t* data, int len) {
  // IMMEDIATELY log that we received something
  Serial.println("******************************************");
  Serial.println("ESP-Now: RECV CALLBACK TRIGGERED");
  Serial.print("ESP-Now: Raw data received, length: ");
  Serial.println(len);
  
  // Extract the sender's MAC address from the info structure
  const uint8_t* mac = info->src_addr;
  
  // Format MAC for logging
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  Serial.print("ESP-Now: Received from MAC: ");
  Serial.println(macStr);
  
  // Dump first few bytes for debugging
  Serial.print("ESP-Now: Data hex dump: ");
  for (int i = 0; i < min(len, 16); i++) {
    Serial.print(data[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  // Update last message time
  last_message_time = millis();
  
  if (len < sizeof(espnow_message_t)) {
    Serial.println("ESP-Now: Received malformed data (too short)");
    Serial.println("******************************************");
    return;
  }

  // Copy the message
  espnow_message_t msg;
  memcpy(&msg, data, sizeof(espnow_message_t));
  
  // Log message type
  Serial.print("ESP-Now: Message type: ");
  Serial.println(msg.type);

  // Process message based on type
  switch (msg.type) {
    case ESPNOW_MSG_REG_ACK: {
      Serial.println("ESP-Now: Registration acknowledged by AURA hub");

      // Reset rejection flag if we were previously rejected
      if (topic_rejected) {
        Serial.println("ESP-Now: AURA accepted our registration after previous rejection!");
        topic_rejected = false;
      }

      // Store the hub's MAC address
      memcpy(espnow_hub_mac, mac, 6);
      
      // Always add AURA as a peer, regardless of encryption
      esp_now_peer_info_t peer_info;
      memset(&peer_info, 0, sizeof(peer_info));
      memcpy(peer_info.peer_addr, mac, 6);
      peer_info.channel = ESPNOW_CHANNEL;
      
      // Try with and without encryption
      esp_now_del_peer(mac);
      peer_info.encrypt = false;  // Try without encryption first
      
      if (esp_now_add_peer(&peer_info) != ESP_OK) {
        Serial.println("ESP-Now: Failed to add AURA hub as unencrypted peer");
      } else {
        Serial.println("ESP-Now: AURA hub added as unencrypted peer successfully");
      }
      
      Serial.println("ESP-Now: Now registered and ready to send data");
      break;
    }
      
    case ESPNOW_MSG_REG_REJECT: {
      Serial.println("ESP-Now: Registration rejected - topic already in use");
      topic_rejected = true;
      break;
    }
      
    case ESPNOW_MSG_DATA_ACK: {
      // Just acknowledge we got the ACK
      Serial.println("ESP-Now: Received data acknowledgment from AURA");
      break;
    }
      
    case ESPNOW_MSG_PING: {
      Serial.println("ESP-Now: Received ping from AURA hub, sending pong");
      
      // Send pong response
      espnow_message_t pong;
      memset(&pong, 0, sizeof(pong));
      pong.type = ESPNOW_MSG_PONG;
      strncpy(pong.topic, espnow_device_topic, sizeof(pong.topic));
      pong.timestamp = millis();
      
      // Get our own MAC
      uint8_t self_mac[6];
      WiFi.macAddress(self_mac);
      memcpy(pong.mac, self_mac, 6);
      
      // Send the pong back to the sender
      esp_err_t result = esp_now_send(mac, (uint8_t*)&pong, sizeof(pong));
      
      if (result == ESP_OK) {
        Serial.println("ESP-Now: Pong sent to AURA hub");
      } else {
        Serial.println("ESP-Now: Failed to send pong");
      }
      break;
    }
      
    default: {
      // Just log other message types
      Serial.print("ESP-Now: Received message type: ");
      Serial.println(msg.type);
      break;
    }
  }

  Serial.println("******************************************");
}

// ESP-Now callback for send status
void onESPNowDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
          mac_addr[0], mac_addr[1], mac_addr[2],
          mac_addr[3], mac_addr[4], mac_addr[5]);
          
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.print("ESP-Now: Message sent successfully to ");
    Serial.println(macStr);
  } else {
    Serial.print("ESP-Now: Failed to send message to ");
    Serial.println(macStr);
  }
}

// Initialize ESP-Now client
void setupESPNowClient(const char* deviceTopic) {
  Serial.println("Setting up ESP-Now client...");
  
  // Save device topic
  strncpy(espnow_device_topic, deviceTopic, sizeof(espnow_device_topic));
  
  // Set WiFi mode (ensure clean state)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100); // Give WiFi time to enter clean state
  
  // Set the WiFi channel explicitly - IMPORTANT!
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  
  // Print MAC address and channel for debugging
  uint8_t self_mac[6];
  WiFi.macAddress(self_mac);
  
  uint8_t channel;
  esp_wifi_get_channel(&channel, NULL);
  
  Serial.print("ESP-Now: My MAC address is ");
  for (int i = 0; i < 6; i++) {
    Serial.print(self_mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.print(" on channel ");
  Serial.println(channel);
  
  // Initialize ESP-Now
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-Now initialization failed");
    return;
  }
  delay(100); // Give ESP-Now time to initialize
  
  // Set Primary Master Key (PMK) for encryption
  if (esp_now_set_pmk((uint8_t *)PMK) != ESP_OK) {
    Serial.println("Failed to set PMK for encryption");
  } else {
    Serial.println("Successfully set shared encryption key");
  }
  
  // Register callbacks
  if (esp_now_register_recv_cb(onESPNowDataRecv) != ESP_OK) {
    Serial.println("Failed to register receive callback!");
  } else {
    Serial.println("Successfully registered receive callback");
  }
  
  if (esp_now_register_send_cb(onESPNowDataSent) != ESP_OK) {
    Serial.println("Failed to register send callback!");
  } else {
    Serial.println("Successfully registered send callback");
  }
  
  // Add broadcast peer for discovery
  esp_now_peer_info_t peer_info;
  memset(&peer_info, 0, sizeof(peer_info));
  memcpy(peer_info.peer_addr, broadcast_addr, 6);
  peer_info.channel = ESPNOW_CHANNEL;
  peer_info.encrypt = false;  // Broadcast cannot be encrypted
  
  // Delete if exists
  esp_now_del_peer(broadcast_addr);
  delay(10);
  
  if (esp_now_add_peer(&peer_info) != ESP_OK) {
    Serial.println("Failed to add broadcast peer");
    return;
  }
  
  // Reset state
  espnow_sequence = 0;
  memset(espnow_hub_mac, 0, 6);
  last_message_time = 0;
  last_reg_time = 0;
  last_data_time = 0;
  topic_rejected = false;
  
  Serial.println("ESP-Now client initialized successfully");
  
  // Send initial registration
  sendESPNowRegistration();
}

// Send registration message
void sendESPNowRegistration() {
  if (topic_rejected) {
    Serial.println("ESP-Now: Topic was rejected, not sending registration");
    return;
  }
  
  espnow_message_t msg;
  memset(&msg, 0, sizeof(msg));
  
  // Set message fields
  msg.type = ESPNOW_MSG_REGISTRATION;
  strncpy(msg.topic, espnow_device_topic, sizeof(msg.topic));
  msg.timestamp = millis();
  
  // Get our own MAC
  uint8_t self_mac[6];
  WiFi.macAddress(self_mac);
  memcpy(msg.mac, self_mac, 6);
  
  // Remember time of registration
  last_reg_time = millis();
  
  // Send to broadcast address
  Serial.println("ESP-Now: Sending registration request");
  esp_err_t result = esp_now_send(broadcast_addr, (uint8_t*)&msg, sizeof(msg));
  
  if (result != ESP_OK) {
    Serial.print("ESP-Now: Failed to send registration, error: ");
    Serial.println(result);
  }
}

// Check if we're connected to AURA hub
bool isESPNowConnected() {
  // We're connected if we have AURA's MAC and haven't been rejected
  return (memcmp(espnow_hub_mac, "\0\0\0\0\0\0", 6) != 0) && !topic_rejected;
}

// Send JSON data via ESP-Now
bool sendESPNowData(const char* jsonString) {
  // Check if we're connected
  if (!isESPNowConnected()) {
    Serial.println("ESP-Now: Not connected, cannot send data");
    return false;
  }
  
  // Create message
  espnow_message_t msg;
  memset(&msg, 0, sizeof(msg));
  
  // Set message fields
  msg.type = ESPNOW_MSG_DATA;
  msg.sequence = espnow_sequence++;
  strncpy(msg.topic, espnow_device_topic, sizeof(msg.topic));
  msg.timestamp = millis();
  
  // Get our own MAC
  uint8_t self_mac[6];
  WiFi.macAddress(self_mac);
  memcpy(msg.mac, self_mac, 6);
  
  // Copy JSON data
  size_t json_len = strlen(jsonString);
  if (json_len > sizeof(msg.data)) {
    json_len = sizeof(msg.data);
  }
  memcpy(msg.data, jsonString, json_len);
  msg.data_len = json_len;
  
  // Update last data send time
  last_data_time = millis();
  
  // Send to AURA hub
  esp_err_t result = esp_now_send(espnow_hub_mac, (uint8_t*)&msg, sizeof(msg));
  
  if (result != ESP_OK) {
    Serial.print("ESP-Now: Failed to send data, error: ");
    Serial.println(result);
    return false;
  }
  
  return true;
}

// Helper function to handle time differences safely
unsigned long timeSince(unsigned long start) {
  unsigned long now = millis();
  // Handle potential inversion or rollover
  return (now >= start) ? (now - start) : 0;
}

// Main loop function for ESP-Now client
void loopESPNowClient() {
  // Registration logic
  if (!isESPNowConnected() && !topic_rejected && timeSince(last_reg_time) > 5000) {
    sendESPNowRegistration();
  }
  
  // Rejection logic
  static bool rejection_message_printed = false;
  if (topic_rejected && !rejection_message_printed) {
    Serial.println("ESP-Now: Topic was rejected by AURA. Please use a different topic name.");
    Serial.println("ESP-Now: Communication disabled until device is reset with a new topic.");
    rejection_message_printed = true;
  }
  
  // Reconnection logic with safe time calculation
  if (isESPNowConnected() && 
      !topic_rejected && 
      timeSince(last_message_time) > 30000) {
    
    Serial.println("ESP-Now: No communication with AURA, reconnecting");
    memset(espnow_hub_mac, 0, 6);
    last_reg_time = millis() - 6000;
  }
}