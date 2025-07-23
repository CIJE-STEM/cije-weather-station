/*
 * CijeWeatherStation.h
 * CIJE Weather Hub - Arduino Library for ESP32 Weather Stations
 * 
 * This library provides a complete interface for ESP32-based weather stations
 * to collect sensor data and submit to the CIJE Weather Hub API.
 * 
 * Author: CIJE Weather Hub Team
 * Version: 1.0.0
 * License: MIT
 */

#ifndef CIJE_WEATHER_STATION_H
#define CIJE_WEATHER_STATION_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// Library version
#define CIJE_WEATHER_STATION_VERSION "1.0.0"

// Default configuration constants
#define DEFAULT_DHT_PIN 32
#define DEFAULT_STATUS_LED_PIN 2
#define DEFAULT_WIND_PIN 36
#define DEFAULT_DHT_TYPE DHT22
#define DEFAULT_READING_INTERVAL 3600000  // 1 hour in milliseconds
#define DEFAULT_WIFI_TIMEOUT 30000        // 30 seconds
#define DEFAULT_HTTP_TIMEOUT 15000        // 15 seconds
#define DEFAULT_API_URL "https://v0-cije-weather-hub.vercel.app/api/weather/submit"

// Sensor reading structure
struct WeatherReading {
  float temperature;    // Temperature in Fahrenheit
  float humidity;       // Humidity percentage
  float windSpeed;      // Wind speed in MPH
  bool isValid;         // Whether the reading is valid
  unsigned long timestamp; // When the reading was taken
};

// Configuration structure
struct WeatherStationConfig {
  // Network settings
  const char* wifiSSID;
  const char* wifiPassword;
  const char* apiURL;
  
  // Station credentials
  int stationID;
  const char* passkey;
  
  // Hardware pins
  int dhtPin;
  int statusLedPin;
  int windPin;
  int dhtType;
  
  // Timing settings
  unsigned long readingInterval;
  unsigned long wifiTimeout;
  unsigned long httpTimeout;
  
  // Wind sensor calibration
  float windVoltageMax;
  float windSpeedMaxMPH;
};

// Status enumeration
enum WeatherStationStatus {
  WS_STATUS_INITIALIZING,
  WS_STATUS_WIFI_CONNECTING,
  WS_STATUS_WIFI_CONNECTED,
  WS_STATUS_WIFI_FAILED,
  WS_STATUS_SENSOR_ERROR,
  WS_STATUS_READING_SENSORS,
  WS_STATUS_SUBMITTING_DATA,
  WS_STATUS_SUBMIT_SUCCESS,
  WS_STATUS_SUBMIT_FAILED,
  WS_STATUS_IDLE
};

class CijeWeatherStation {
private:
  WeatherStationConfig config;
  DHT* dht;
  WeatherStationStatus currentStatus;
  unsigned long lastReading;
  unsigned long lastStatusUpdate;
  int consecutiveFailures;
  bool initialized;
  WeatherReading lastValidReading;
  
  // Private methods
  bool initializeHardware();
  bool connectToWiFi();
  WeatherReading readSensors();
  float readWindSpeed();
  bool submitReading(const WeatherReading& reading);
  void updateStatusLED();
  void blinkLED(int times, int duration);
  
public:
  // Constructor
  CijeWeatherStation();
  
  // Destructor
  ~CijeWeatherStation();
  
  // Configuration methods
  void setWiFiCredentials(const char* ssid, const char* password);
  void setStationCredentials(int stationID, const char* passkey);
  void setAPIURL(const char* url);
  void setDHTPin(int pin, int type = DHT22);
  void setStatusLEDPin(int pin);
  void setWindPin(int pin);
  void setReadingInterval(unsigned long interval);
  void setWindCalibration(float voltageMax, float speedMaxMPH);
  void setTimeouts(unsigned long wifiTimeout, unsigned long httpTimeout);
  
  // Core methods
  bool begin();
  void loop();
  bool forceReading();
  
  // Status methods
  WeatherStationStatus getStatus();
  const char* getStatusString();
  bool isConnected();
  WeatherReading getLastReading();
  int getConsecutiveFailures();
  unsigned long getUptime();
  
  // Utility methods
  void printSystemInfo();
  void printWiFiStatus();
  void printLastReading();
  bool testSensors();
  void restart();
  
  // Configuration getters
  WeatherStationConfig getConfig();
};

// Utility functions (can be used independently)
namespace CijeWeatherUtils {
  float celsiusToFahrenheit(float celsius);
  float fahrenheitToCelsius(float fahrenheit);
  String formatUptime(unsigned long milliseconds);
  String formatTimestamp(unsigned long timestamp);
  bool isValidTemperature(float temp);
  bool isValidHumidity(float humidity);
  bool isValidWindSpeed(float windSpeed);
}

#endif // CIJE_WEATHER_STATION_H
