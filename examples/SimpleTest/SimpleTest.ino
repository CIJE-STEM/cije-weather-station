/*
 * SimpleTest.ino
 * 
 * Simple test example for quick DHT22 testing
 * Reads temperature and humidity every 30 seconds
 */

#include <CijeWeatherStation.h>

CijeWeatherStation station;

// Configuration - UPDATE THESE VALUES!
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const int STATION_ID = 1;
const char* PASSKEY = "20XV54";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== Simple Weather Station Test ===");
  
  // Configure for testing (30 second intervals)
  station.setWiFiCredentials(WIFI_SSID, WIFI_PASSWORD);
  station.setStationCredentials(STATION_ID, PASSKEY);
  station.setDHTPin(32, DHT22);
  station.setStatusLEDPin(2);
  station.setReadingInterval(30000);  // 30 seconds for testing
  
  // Initialize
  if (!station.begin()) {
    Serial.println("Initialization failed!");
    while (true) delay(1000);
  }
  
  Serial.println("Test started - readings every 30 seconds");
  Serial.println("Commands: 'test' for manual reading, 'info' for system info");
}

void loop() {
  station.loop();
  
  // Simple command handling
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    cmd.toLowerCase();
    
    if (cmd == "test") {
      station.forceReading();
    } else if (cmd == "info") {
      station.printSystemInfo();
    } else if (cmd == "reading") {
      station.printLastReading();
    }
  }
  
  delay(100);
}
