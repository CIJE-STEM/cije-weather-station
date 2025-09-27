/*
 * BasicWeatherStation.ino
 * 
 * Basic example using the CijeWeatherStation library
 * This example shows how to set up a weather station with DHT22 and wind sensor
 */

#include <CijeWeatherStation.h>

// Create weather station instance
CijeWeatherStation station;

// Configuration - UPDATE THESE VALUES!
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const int STATION_ID = 1;
const char* PASSKEY = "20XV54";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Starting Basic Weather Station...");
  
  // Configure the weather station
  station.setWiFiCredentials(WIFI_SSID, WIFI_PASSWORD);
  station.setStationCredentials(STATION_ID, PASSKEY);
  station.setDHTPin(32, DHT22);           // DHT22 on GPIO 32
  station.setStatusLEDPin(2);             // Built-in LED
  station.setWindPin(36);                 // Wind sensor on GPIO 36
  station.setReadingInterval(3600000);    // 1 hour readings
  
  // Initialize the weather station
  if (!station.begin()) {
    Serial.println("Failed to initialize weather station!");
    Serial.println("Check your configuration and try again.");
    while (true) {
      delay(1000);
    }
  }
  
  Serial.println("Weather station initialized successfully!");
  Serial.println("Type 'help' for available commands.");
}

void loop() {
  // Run the weather station
  station.loop();
  
  // Handle serial commands
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "test") {
      Serial.println("Manual reading triggered...");
      station.forceReading();
    }
    else if (command == "info") {
      station.printSystemInfo();
    }
    else if (command == "wifi") {
      station.printWiFiStatus();
    }
    else if (command == "status") {
      Serial.println("Current Status: " + String(station.getStatusString()));
      Serial.println("Connected: " + String(station.isConnected() ? "Yes" : "No"));
      Serial.println("Failures: " + String(station.getConsecutiveFailures()));
      Serial.println("Uptime: " + CijeWeatherUtils::formatUptime(station.getUptime()));
    }
    else if (command == "reading") {
      station.printLastReading();
    }
    else if (command == "sensors") {
      station.testSensors();
    }
    else if (command == "restart") {
      station.restart();
    }
    else if (command == "help") {
      Serial.println("\n=== Available Commands ===");
      Serial.println("test     - Force a sensor reading");
      Serial.println("info     - Show system information");
      Serial.println("wifi     - Show WiFi status");
      Serial.println("status   - Show current status");
      Serial.println("reading  - Show last sensor reading");
      Serial.println("sensors  - Test sensor readings");
      Serial.println("restart  - Restart the ESP32");
      Serial.println("help     - Show this help");
      Serial.println("==========================\n");
    }
    else {
      Serial.println("Unknown command: '" + command + "'. Type 'help' for available commands.");
    }
  }
  
  delay(100);
}
