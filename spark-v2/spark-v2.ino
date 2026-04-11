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
 *
 * SPARK v2 - Unified OASIS Armor Firmware
 *
 * Based on spark-generic-v1 (ESP-NOW) with spark-v1 repulsor logic.
 * Device type selected via #define below.
 */

//#define DEBUG
#define SUPPRESS_NOISE

/* =========== DEVICE TYPE — uncomment exactly one =========== */
#define SPARK_DEVICE_HAND_REPULSOR
//#define SPARK_DEVICE_SHOULDER

/* =========== DEVICE INSTANCE =========== */
#ifdef SPARK_DEVICE_HAND_REPULSOR
  const char topic[] = "repulsor/right";   /* "repulsor/left" for left hand */
#elif defined(SPARK_DEVICE_SHOULDER)
  const char topic[] = "shoulder/left";
#endif

/* =========== COMMON =========== */
#include <WiFi.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"
#include "espnow_client.h"

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2)
  #define VOLT_PIN A2
#else
  #define VOLT_PIN A0
#endif

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

unsigned long previousMillis = 0;
unsigned long previousSendMillis = 0;
const long sendInterval = 5000;

/* =========== REPULSOR-SPECIFIC =========== */
#ifdef SPARK_DEVICE_HAND_REPULSOR

#include <Adafruit_LSM6DSO32.h>
#include <SPI.h>
#include <FastLED.h>

Adafruit_LSM6DSO32 lsm6ds;
Adafruit_Sensor *lsm_temp, *lsm_accel;

/* LED defines — identical to spark-v1 */
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS    19
#define NUM_INSIDE  7
#define NUM_OUTSIDE (NUM_LEDS - NUM_INSIDE)
#define BRIGHTNESS  80
#define FIRE_BRIGHTNESS 190
#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2)
  #define LED_PIN 35
#else
  #define LED_PIN 10
#endif

#define BLAST_DURATION 1057

enum states { POWER_OFF, POWERING_UP, POWERING_DOWN, POWER_ON };

uint8_t power_switch_state;
uint8_t fire_switch_state;
int firing;
uint8_t flash_delay;
uint8_t state;
uint8_t last_state;
CRGB leds[NUM_LEDS];
uint8_t light_level;

#define HAND_UP_TIME   1.064
#define HAND_STEP_SIZE 15
double hand_delay = (1000 * HAND_UP_TIME) / (255 / HAND_STEP_SIZE);

#define HAND_UP_SOUND    "hand_rep_on.ogg"
#define HAND_BLAST_SOUND "hand_rep_fire2.ogg"
#define HAND_DOWN_SOUND  "hand_rep_off.ogg"

/* LED arrays — kept exactly as spark-v1 */
CRGB outside_leds[NUM_LEDS];
CRGB inside_leds[NUM_LEDS];
CRGB all_on[NUM_LEDS];
CRGB all_off[NUM_LEDS];
CLEDController *repulsor;

/* Helper: send audio command via ESP-NOW (replaces MQTT publish in spark-v1) */
void sendAudioCmd(const char *command, const char *filename, const char *position) {
   JsonDocument doc;
   char json[128];
   doc["device"] = "audio";
   doc["command"] = command;
   doc["arg1"] = filename;
   if (position) {
      doc["arg2"] = position;
   }
   serializeJson(doc, json, sizeof(json));
   if (isESPNowConnected()) {
      sendESPNowData(json);
   }
   Serial.println(json);
}

void sendAudioPlay(const char *filename) { sendAudioCmd("play", filename, NULL); }
void sendAudioPlayAt(const char *filename, const char *pos) { sendAudioCmd("play", filename, pos); }
void sendAudioStop(const char *filename) { sendAudioCmd("stop", filename, NULL); }

#endif /* SPARK_DEVICE_HAND_REPULSOR */

/* =========== SHOULDER-SPECIFIC =========== */
#ifdef SPARK_DEVICE_SHOULDER
#include <Adafruit_AHTX0.h>
Adafruit_AHTX0 aht;
#endif

/* =========================================================================
 * SETUP
 * ========================================================================= */
void setup() {
   int retries = 0;

   Serial.begin(115200);
   delay(2000);

   Serial.println("=============================");
   Serial.print("SPARK v2: ");
   Serial.println(topic);
   Serial.println("=============================");

/* --- Repulsor hardware init (copied from spark-v1 setup) --- */
#ifdef SPARK_DEVICE_HAND_REPULSOR
   Serial.println("LSM6DS setup...");

#if defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32S2)
   Wire1.setPins(SDA1, SCL1);
   if (!lsm6ds.begin_I2C(LSM6DS_I2CADDR_DEFAULT, &Wire1, 0)) {
#else
   if (!lsm6ds.begin_I2C(LSM6DS_I2CADDR_DEFAULT, &Wire, 0)) {
#endif
      Serial.println("Failed to find LSM6DS chip");
      while (1) { delay(10); }
   }
   Serial.println("LSM6DS Found!");

   lsm6ds.setAccelRange(LSM6DSO32_ACCEL_RANGE_16_G);
   lsm6ds.setGyroRange(LSM6DS_GYRO_RANGE_1000_DPS);
   lsm6ds.setAccelDataRate(LSM6DS_RATE_12_5_HZ);
   lsm6ds.setGyroDataRate(LSM6DS_RATE_12_5_HZ);

   lsm_temp = lsm6ds.getTemperatureSensor();
   lsm_accel = lsm6ds.getAccelerometerSensor();

#if defined(NEOPIXEL_POWER)
   pinMode(NEOPIXEL_POWER, OUTPUT);
   digitalWrite(NEOPIXEL_POWER, HIGH);
#endif

   repulsor = &FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
                  .setCorrection(CRGB(225, 255, 255));
   memmove(&leds, &all_off, NUM_LEDS * sizeof(CRGB));
   leds[12] = CRGB::Red;
   repulsor->showLeds(20);

   /* Build LED template arrays — identical to spark-v1 */
   for (int i = 0; i < NUM_OUTSIDE; i++) {
      outside_leds[i] = CRGB::White;
   }
   for (int i = NUM_OUTSIDE; i < NUM_LEDS; i++) {
      inside_leds[i] = CRGB::White;
   }
   fill_solid(all_off, NUM_LEDS, CRGB::Black);
   fill_solid(all_on, NUM_LEDS, CRGB::White);

   power_switch_state = 0;
   fire_switch_state = 0;
   firing = 0;
   flash_delay = 0;
   state = POWER_OFF;
   last_state = POWER_OFF;
   memmove(&leds, &all_off, NUM_LEDS * sizeof(CRGB));
   light_level = 0;
#endif /* SPARK_DEVICE_HAND_REPULSOR */

/* --- Shoulder hardware init --- */
#ifdef SPARK_DEVICE_SHOULDER
   if (!aht.begin()) {
      Serial.println("Could not find AHT sensor");
      while (1) { delay(10); }
   }
   Serial.println("AHT sensor found");
#endif

   /* --- ESP-NOW init (from spark-generic-v1) --- */
   setupESPNowClient(topic);

   /* --- Repulsor: blink red during registration, green on connect --- */
#ifdef SPARK_DEVICE_HAND_REPULSOR
   {
      unsigned long reg_start = millis();
      unsigned long last_blink = 0;
      bool led_on = false;
      while (!isESPNowConnected() && (millis() - reg_start < 10000)) {
         loopESPNowClient();
         if (millis() - last_blink >= 500) {
            last_blink = millis();
            led_on = !led_on;
            leds[12] = led_on ? CRGB::Red : CRGB::Black;
            repulsor->showLeds(20);
         }
         delay(50);
      }
      if (isESPNowConnected()) {
         Serial.println("AURA connected");
         leds[12] = CRGB::Green;
         repulsor->showLeds(20);
         delay(500);
      } else {
         Serial.println("Standalone mode (no AURA)");
         leds[12] = CRGB(255, 165, 0); /* Orange */
         repulsor->showLeds(20);
         delay(500);
      }
      /* Clear for normal operation */
      memmove(&leds, &all_off, NUM_LEDS * sizeof(CRGB));
      repulsor->showLeds(BRIGHTNESS);
   }
#endif

   Serial.println("Setup complete");
}

/* =========================================================================
 * LOOP
 * ========================================================================= */
void loop() {
   static JsonDocument send_doc;
   static char output_json[256];

   /* ESP-NOW housekeeping */
   loopESPNowClient();

   unsigned long currentMillis = millis();

   /* --- Telemetry (every sendInterval) --- */
   if (isESPNowConnected() && currentMillis - previousSendMillis >= sendInterval) {
      previousSendMillis = currentMillis;

      uint32_t voltageMilliVolts = analogReadMilliVolts(VOLT_PIN);
      float voltage = 2 * voltageMilliVolts / 1000.0;

      send_doc["device"] = topic;
      send_doc["voltage"] = roundf(voltage * 100) / 100.0;

#ifdef SPARK_DEVICE_HAND_REPULSOR
      sensors_event_t temp;
      lsm_temp->getEvent(&temp);
      send_doc["temp"] = roundf(temp.temperature * 100) / 100.0;
#elif defined(SPARK_DEVICE_SHOULDER)
      sensors_event_t humidity, temp;
      aht.getEvent(&humidity, &temp);
      send_doc["temp"] = roundf(temp.temperature * 100) / 100.0;
      send_doc["humidity"] = roundf(humidity.relative_humidity * 100) / 100.0;
#endif

      serializeJson(send_doc, output_json, 256);
      send_doc.clear();
      sendESPNowData(output_json);
   }

/* =========== REPULSOR STATE MACHINE (copied from spark-v1 loop) =========== */
#ifdef SPARK_DEVICE_HAND_REPULSOR
   if (currentMillis - previousMillis >= hand_delay) {
      sensors_event_t accel;
      int power_switch = 0;
      int fire_switch = 0;
      double stop_pos;
      char str_stop_pos[8];

      /* Get sensor data */
      lsm_accel->getEvent(&accel);

      if (accel.acceleration.x > 7.0) {
         power_switch = 1;
      }
      if (accel.acceleration.x > 9.2) {
         fire_switch = 1;
      }

      /* Fire detection */
      if ((firing <= 0) && (fire_switch != fire_switch_state)) {
         fire_switch_state = fire_switch;
         if ((state == POWER_ON) && (fire_switch_state == 1)) {
            sendAudioPlay(HAND_BLAST_SOUND);

            if (firing <= 0) {
               repulsor->showLeds(FIRE_BRIGHTNESS);
            } else {
               repulsor->showLeds(BRIGHTNESS);
               flash_delay = 2;
            }
            firing = BLAST_DURATION;
         }
      }

      if (firing >= 0) {
         if (flash_delay) {
            flash_delay--;
            if (flash_delay == 0) {
               repulsor->showLeds(FIRE_BRIGHTNESS);
            }
         }
         firing -= hand_delay;
         if (firing <= 0) {
            repulsor->showLeds(BRIGHTNESS);
         }
      }

      /* Power switch state change */
      if (power_switch != power_switch_state) {
         power_switch_state = power_switch;

         switch (state) {
            case POWER_OFF:
               sendAudioPlay(HAND_UP_SOUND);
               state = POWERING_UP;
               last_state = POWER_OFF;
               break;

            case POWERING_UP:
               sendAudioStop(HAND_UP_SOUND);
               stop_pos = 1 - (double)(light_level) / 255;
               dtostrf(stop_pos, 4, 2, str_stop_pos);
               sendAudioPlayAt(HAND_DOWN_SOUND, str_stop_pos);
               state = POWERING_DOWN;
               last_state = POWERING_UP;
               break;

            case POWERING_DOWN:
               sendAudioStop(HAND_DOWN_SOUND);
               stop_pos = 1 - (double)(light_level) / 255;
               dtostrf(stop_pos, 4, 2, str_stop_pos);
               sendAudioPlayAt(HAND_UP_SOUND, str_stop_pos);
               state = POWERING_UP;
               last_state = POWERING_DOWN;
               break;

            case POWER_ON:
               sendAudioPlay(HAND_DOWN_SOUND);
               state = POWERING_DOWN;
               last_state = POWER_ON;
               break;
         }
      }

      /* LED animation — identical to spark-v1 */
      switch (state) {
         case POWER_OFF:
            if (last_state != POWER_OFF) {
               memmove(&leds, &all_off, NUM_LEDS * sizeof(CRGB));
               repulsor->showLeds(BRIGHTNESS);
               last_state = POWER_OFF;
            }
            break;

         case POWERING_UP:
            light_level = light_level + HAND_STEP_SIZE;
            memmove(&leds, &inside_leds, NUM_LEDS * sizeof(CRGB));
            for (int i = NUM_OUTSIDE - 1; i < NUM_LEDS; i++) {
               leds[i].nscale8(light_level);
            }
            repulsor->showLeds(BRIGHTNESS);
            if (light_level == 255) {
               state = POWER_ON;
            }
            last_state = POWERING_UP;
            break;

         case POWERING_DOWN:
            light_level -= HAND_STEP_SIZE;
            memmove(&leds, &inside_leds, NUM_LEDS * sizeof(CRGB));
            for (int i = NUM_OUTSIDE - 1; i < NUM_LEDS; i++) {
               leds[i].nscale8(light_level);
            }
            repulsor->showLeds(BRIGHTNESS);
            if (light_level == 0) {
               state = POWER_OFF;
            }
            last_state = POWERING_DOWN;
            break;

         case POWER_ON:
            if (last_state != POWER_ON) {
               memmove(&leds, &all_on, NUM_LEDS * sizeof(CRGB));
               repulsor->showLeds(BRIGHTNESS);
               last_state = POWER_ON;
            }
            break;
      }

      previousMillis = currentMillis;
   }
#endif /* SPARK_DEVICE_HAND_REPULSOR */
}
