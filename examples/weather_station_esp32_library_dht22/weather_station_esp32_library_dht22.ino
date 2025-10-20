/*
 * BasicWeatherStation.ino
 * 
 * Basic example using the CijeWeatherStation library
 * This example shows how to set up a weather station with DHT22 and wind sensor
 */

#include <CIJEWeatherStation.h>
#include "esp_sleep.h"

// Create weather station instance
CIJEWeatherStation station;

// Configuration - UPDATE THESE VALUES!
const char* WIFI_SSID = "WIFI_SSID";
const char* WIFI_PASSWORD = "WIFI_PASSWORD";
const int STATION_ID = #;
const char* PASSKEY = "PASSKEY";

// Sleep duration (in microseconds)
// 1 minute  = 60 * 1e6 = 60000000 µs
// 5 minutes = 300000000 µs
// 15 minutes = 900000000 µs
// 30 minutes = 1800000000 µs
// 1 hour    = 3600000000 µs
const uint64_t SLEEP_DURATION = 60000000ULL;  // currently set to 1 minute

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting Weather Station...");

  station.setWiFiCredentials(WIFI_SSID, WIFI_PASSWORD);
  station.setStationCredentials(STATION_ID, PASSKEY);
  station.setDHTPin(32, DHT22);
  station.setStatusLEDPin(2);
  station.setWindPin(34);

  if (!station.begin()) {
    Serial.println("Init failed. Going to sleep for retry...");
    esp_sleep_enable_timer_wakeup(60000000);  // sleep 1 min on fail
    esp_deep_sleep_start();
  }

  Serial.println("Taking and uploading reading...");
  station.forceReading();   // perform sensor read + upload

  Serial.println("Upload done. Going to sleep...");
  esp_sleep_enable_timer_wakeup(SLEEP_DURATION);
  esp_deep_sleep_start();
}

void loop() {
  // Nothing needed here — device sleeps after setup.
}