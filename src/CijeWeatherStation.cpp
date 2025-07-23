/*
 * CijeWeatherStation.cpp
 * CIJE Weather Hub - Arduino Library Implementation
 */

#include "CijeWeatherStation.h"

// Constructor
CijeWeatherStation::CijeWeatherStation() {
  // Initialize with default configuration
  config.wifiSSID = nullptr;
  config.wifiPassword = nullptr;
  config.apiURL = DEFAULT_API_URL;
  config.stationID = 0;
  config.passkey = nullptr;
  config.dhtPin = DEFAULT_DHT_PIN;
  config.statusLedPin = DEFAULT_STATUS_LED_PIN;
  config.windPin = DEFAULT_WIND_PIN;
  config.dhtType = DEFAULT_DHT_TYPE;
  config.readingInterval = DEFAULT_READING_INTERVAL;
  config.wifiTimeout = DEFAULT_WIFI_TIMEOUT;
  config.httpTimeout = DEFAULT_HTTP_TIMEOUT;
  config.windVoltageMax = 3.3;
  config.windSpeedMaxMPH = 32.4;
  
  dht = nullptr;
  currentStatus = WS_STATUS_INITIALIZING;
  lastReading = 0;
  lastStatusUpdate = 0;
  consecutiveFailures = 0;
  initialized = false;
  
  // Initialize last reading
  lastValidReading.temperature = 0;
  lastValidReading.humidity = 0;
  lastValidReading.windSpeed = 0;
  lastValidReading.isValid = false;
  lastValidReading.timestamp = 0;
}

// Destructor
CijeWeatherStation::~CijeWeatherStation() {
  if (dht != nullptr) {
    delete dht;
  }
}

// Configuration methods
void CijeWeatherStation::setWiFiCredentials(const char* ssid, const char* password) {
  config.wifiSSID = ssid;
  config.wifiPassword = password;
}

void CijeWeatherStation::setStationCredentials(int stationID, const char* passkey) {
  config.stationID = stationID;
  config.passkey = passkey;
}

void CijeWeatherStation::setAPIURL(const char* url) {
  config.apiURL = url;
}

void CijeWeatherStation::setDHTPin(int pin, int type) {
  config.dhtPin = pin;
  config.dhtType = type;
}

void CijeWeatherStation::setStatusLEDPin(int pin) {
  config.statusLedPin = pin;
}

void CijeWeatherStation::setWindPin(int pin) {
  config.windPin = pin;
}

void CijeWeatherStation::setReadingInterval(unsigned long interval) {
  config.readingInterval = interval;
}

void CijeWeatherStation::setWindCalibration(float voltageMax, float speedMaxMPH) {
  config.windVoltageMax = voltageMax;
  config.windSpeedMaxMPH = speedMaxMPH;
}

void CijeWeatherStation::setTimeouts(unsigned long wifiTimeout, unsigned long httpTimeout) {
  config.wifiTimeout = wifiTimeout;
  config.httpTimeout = httpTimeout;
}

// Core methods
bool CijeWeatherStation::begin() {
  Serial.println("=== CIJE Weather Station Library v" CIJE_WEATHER_STATION_VERSION " ===");
  
  // Validate configuration
  if (config.wifiSSID == nullptr || config.wifiPassword == nullptr) {
    Serial.println("❌ ERROR: WiFi credentials not set!");
    return false;
  }
  
  if (config.stationID == 0 || config.passkey == nullptr) {
    Serial.println("❌ ERROR: Station credentials not set!");
    return false;
  }
  
  // Initialize hardware
  if (!initializeHardware()) {
    Serial.println("❌ ERROR: Hardware initialization failed!");
    return false;
  }
  
  // Connect to WiFi
  currentStatus = WS_STATUS_WIFI_CONNECTING;
  if (!connectToWiFi()) {
    Serial.println("❌ ERROR: WiFi connection failed!");
    currentStatus = WS_STATUS_WIFI_FAILED;
    return false;
  }
  
  currentStatus = WS_STATUS_IDLE;
  initialized = true;
  
  Serial.println("✅ Weather Station initialized successfully!");
  printSystemInfo();
  
  return true;
}

void CijeWeatherStation::loop() {
  if (!initialized) {
    return;
  }
  
  unsigned long currentTime = millis();
  
  // Check if it's time for a reading
  if (currentTime - lastReading >= config.readingInterval) {
    forceReading();
    lastReading = currentTime;
  }
  
  // Update status LED
  updateStatusLED();
  
  // Handle consecutive failures
  if (consecutiveFailures >= 5) {
    Serial.println("⚠️ Too many consecutive failures. Restarting...");
    restart();
  }
}

bool CijeWeatherStation::forceReading() {
  if (!initialized) {
    Serial.println("❌ Weather station not initialized!");
    return false;
  }
  
  Serial.println("\n--- Taking Weather Reading ---");
  currentStatus = WS_STATUS_READING_SENSORS;
  
  // Ensure WiFi is connected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected. Reconnecting...");
    currentStatus = WS_STATUS_WIFI_CONNECTING;
    if (!connectToWiFi()) {
      currentStatus = WS_STATUS_WIFI_FAILED;
      consecutiveFailures++;
      return false;
    }
  }
  
  // Read sensors
  WeatherReading reading = readSensors();
  
  if (!reading.isValid) {
    Serial.println("❌ Invalid sensor reading!");
    currentStatus = WS_STATUS_SENSOR_ERROR;
    consecutiveFailures++;
    return false;
  }
  
  // Store last valid reading
  lastValidReading = reading;
  
  // Submit data
  currentStatus = WS_STATUS_SUBMITTING_DATA;
  bool success = submitReading(reading);
  
  if (success) {
    Serial.println("✅ Data submitted successfully!");
    currentStatus = WS_STATUS_SUBMIT_SUCCESS;
    consecutiveFailures = 0;
    blinkLED(3, 100); // Success blink
  } else {
    Serial.println("❌ Data submission failed!");
    currentStatus = WS_STATUS_SUBMIT_FAILED;
    consecutiveFailures++;
  }
  
  currentStatus = WS_STATUS_IDLE;
  Serial.println("--- End Reading ---\n");
  
  return success;
}

// Private methods
bool CijeWeatherStation::initializeHardware() {
  // Initialize status LED
  pinMode(config.statusLedPin, OUTPUT);
  digitalWrite(config.statusLedPin, LOW);
  
  // Initialize DHT sensor
  if (dht != nullptr) {
    delete dht;
  }
  dht = new DHT(config.dhtPin, config.dhtType);
  dht->begin();
  
  // Wait for sensor stabilization
  delay(2000);
  
  // Test DHT sensor
  float testTemp = dht->readTemperature();
  if (isnan(testTemp)) {
    Serial.println("❌ DHT sensor not responding!");
    return false;
  }
  
  // Initialize ADC for wind sensor
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  Serial.println("✅ Hardware initialized successfully");
  return true;
}

bool CijeWeatherStation::connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(config.wifiSSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSSID, config.wifiPassword);
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < config.wifiTimeout) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    currentStatus = WS_STATUS_WIFI_CONNECTED;
    return true;
  } else {
    Serial.println();
    Serial.println("❌ WiFi connection failed!");
    currentStatus = WS_STATUS_WIFI_FAILED;
    return false;
  }
}

WeatherReading CijeWeatherStation::readSensors() {
  WeatherReading reading;
  reading.timestamp = millis();
  reading.isValid = false;
  
  // Read DHT22
  float tempC = dht->readTemperature();
  float humidity = dht->readHumidity();
  
  if (isnan(tempC) || isnan(humidity)) {
    Serial.println("❌ DHT sensor reading failed!");
    return reading;
  }
  
  // Read wind speed
  float windSpeed = readWindSpeed();
  
  // Convert temperature to Fahrenheit
  reading.temperature = CijeWeatherUtils::celsiusToFahrenheit(tempC);
  reading.humidity = humidity;
  reading.windSpeed = windSpeed;
  
  // Validate readings
  if (CijeWeatherUtils::isValidTemperature(reading.temperature) &&
      CijeWeatherUtils::isValidHumidity(reading.humidity) &&
      CijeWeatherUtils::isValidWindSpeed(reading.windSpeed)) {
    reading.isValid = true;
  }
  
  Serial.println("📊 Sensor readings:");
  Serial.println("   Temperature: " + String(tempC, 1) + "°C (" + String(reading.temperature, 1) + "°F)");
  Serial.println("   Humidity: " + String(reading.humidity, 1) + "%");
  Serial.println("   Wind Speed: " + String(reading.windSpeed, 1) + " mph");
  
  return reading;
}

float CijeWeatherStation::readWindSpeed() {
  int adcValue = analogRead(config.windPin);
  float voltage = (float)adcValue / 4095.0 * config.windVoltageMax;
  float windSpeed = voltage * (config.windSpeedMaxMPH / config.windVoltageMax);
  return windSpeed;
}

bool CijeWeatherStation::submitReading(const WeatherReading& reading) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
    return false;
  }
  
  HTTPClient http;
  http.begin(config.apiURL);
  http.setTimeout(config.httpTimeout);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.addHeader("User-Agent", "CijeWeatherStation/" CIJE_WEATHER_STATION_VERSION);
  
  // Create form data
  String postData = "station_id=" + String(config.stationID) +
                   "&passkey=" + String(config.passkey) +
                   "&temperature=" + String(reading.temperature, 2) +
                   "&humidity=" + String(reading.humidity, 2) +
                   "&wind_speed=" + String(reading.windSpeed, 2);
  
  Serial.println("🌐 Submitting to: " + String(config.apiURL));
  Serial.println("📤 Data: " + postData);
  
  int httpResponseCode = http.POST(postData);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("📥 HTTP " + String(httpResponseCode) + ": " + response);
    
    http.end();
    return (httpResponseCode == 200 || httpResponseCode == 429);
  } else {
    Serial.println("❌ HTTP Error: " + String(httpResponseCode));
    http.end();
    return false;
  }
}

void CijeWeatherStation::updateStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  unsigned long blinkInterval;
  
  switch (currentStatus) {
    case WS_STATUS_WIFI_CONNECTING:
      blinkInterval = 200; // Fast blink
      break;
    case WS_STATUS_WIFI_CONNECTED:
    case WS_STATUS_IDLE:
      blinkInterval = 2000; // Slow blink
      break;
    case WS_STATUS_WIFI_FAILED:
    case WS_STATUS_SENSOR_ERROR:
    case WS_STATUS_SUBMIT_FAILED:
      blinkInterval = 100; // Very fast blink (error)
      break;
    default:
      blinkInterval = 1000; // Normal blink
      break;
  }
  
  if (millis() - lastBlink > blinkInterval) {
    ledState = !ledState;
    digitalWrite(config.statusLedPin, ledState);
    lastBlink = millis();
  }
}

void CijeWeatherStation::blinkLED(int times, int duration) {
  for (int i = 0; i < times; i++) {
    digitalWrite(config.statusLedPin, HIGH);
    delay(duration);
    digitalWrite(config.statusLedPin, LOW);
    delay(duration);
  }
}

// Status methods
WeatherStationStatus CijeWeatherStation::getStatus() {
  return currentStatus;
}

const char* CijeWeatherStation::getStatusString() {
  switch (currentStatus) {
    case WS_STATUS_INITIALIZING: return "Initializing";
    case WS_STATUS_WIFI_CONNECTING: return "WiFi Connecting";
    case WS_STATUS_WIFI_CONNECTED: return "WiFi Connected";
    case WS_STATUS_WIFI_FAILED: return "WiFi Failed";
    case WS_STATUS_SENSOR_ERROR: return "Sensor Error";
    case WS_STATUS_READING_SENSORS: return "Reading Sensors";
    case WS_STATUS_SUBMITTING_DATA: return "Submitting Data";
    case WS_STATUS_SUBMIT_SUCCESS: return "Submit Success";
    case WS_STATUS_SUBMIT_FAILED: return "Submit Failed";
    case WS_STATUS_IDLE: return "Idle";
    default: return "Unknown";
  }
}

bool CijeWeatherStation::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

WeatherReading CijeWeatherStation::getLastReading() {
  return lastValidReading;
}

int CijeWeatherStation::getConsecutiveFailures() {
  return consecutiveFailures;
}

unsigned long CijeWeatherStation::getUptime() {
  return millis();
}

WeatherStationConfig CijeWeatherStation::getConfig() {
  return config;
}

// Utility methods
void CijeWeatherStation::printSystemInfo() {
  Serial.println("\n=== System Information ===");
  Serial.println("Library Version: " CIJE_WEATHER_STATION_VERSION);
  Serial.println("Station ID: " + String(config.stationID));
  Serial.println("Passkey: " + String(config.passkey));
  Serial.println("API URL: " + String(config.apiURL));
  Serial.println("Reading Interval: " + String(config.readingInterval / 1000) + " seconds");
  Serial.println("DHT Pin: " + String(config.dhtPin));
  Serial.println("Wind Pin: " + String(config.windPin));
  Serial.println("Status LED Pin: " + String(config.statusLedPin));
  Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  Serial.println("Uptime: " + CijeWeatherUtils::formatUptime(millis()));
  Serial.println("Status: " + String(getStatusString()));
  Serial.println("Consecutive Failures: " + String(consecutiveFailures));
  printWiFiStatus();
  Serial.println("==========================\n");
}

void CijeWeatherStation::printWiFiStatus() {
  Serial.println("--- WiFi Status ---");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Status: Connected ✅");
    Serial.println("SSID: " + WiFi.SSID());
    Serial.println("IP: " + WiFi.localIP().toString());
    Serial.println("Signal: " + String(WiFi.RSSI()) + " dBm");
    Serial.println("MAC: " + WiFi.macAddress());
  } else {
    Serial.println("Status: Disconnected ❌");
  }
  Serial.println("-------------------");
}

void CijeWeatherStation::printLastReading() {
  Serial.println("--- Last Reading ---");
  if (lastValidReading.isValid) {
    Serial.println("Temperature: " + String(lastValidReading.temperature, 1) + "°F");
    Serial.println("Humidity: " + String(lastValidReading.humidity, 1) + "%");
    Serial.println("Wind Speed: " + String(lastValidReading.windSpeed, 1) + " mph");
    Serial.println("Timestamp: " + String(lastValidReading.timestamp));
  } else {
    Serial.println("No valid reading available");
  }
  Serial.println("-------------------");
}

bool CijeWeatherStation::testSensors() {
  Serial.println("Testing sensors...");
  WeatherReading reading = readSensors();
  return reading.isValid;
}

void CijeWeatherStation::restart() {
  Serial.println("Restarting ESP32...");
  delay(1000);
  ESP.restart();
}

// Utility namespace implementation
namespace CijeWeatherUtils {
  float celsiusToFahrenheit(float celsius) {
    return (celsius * 9.0 / 5.0) + 32.0;
  }
  
  float fahrenheitToCelsius(float fahrenheit) {
    return (fahrenheit - 32.0) * 5.0 / 9.0;
  }
  
  String formatUptime(unsigned long milliseconds) {
    unsigned long seconds = milliseconds / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    
    seconds %= 60;
    minutes %= 60;
    hours %= 24;
    
    String uptime = "";
    if (days > 0) uptime += String(days) + "d ";
    if (hours > 0) uptime += String(hours) + "h ";
    if (minutes > 0) uptime += String(minutes) + "m ";
    uptime += String(seconds) + "s";
    
    return uptime;
  }
  
  String formatTimestamp(unsigned long timestamp) {
    return String(timestamp);
  }
  
  bool isValidTemperature(float temp) {
    return (temp >= -40.0 && temp <= 140.0); // Reasonable range in Fahrenheit
  }
  
  bool isValidHumidity(float humidity) {
    return (humidity >= 0.0 && humidity <= 100.0);
  }
  
  bool isValidWindSpeed(float windSpeed) {
    return (windSpeed >= 0.0 && windSpeed <= 200.0); // Reasonable max wind speed
  }
}
