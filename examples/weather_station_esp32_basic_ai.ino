/*
 * CIJE Weather Hub - ESP32 Weather Station
 * 
 * Hardware:
 * - ESP32-WROOM-32 Development Board
 * - BME280 Temperature , Humidity and Pressure Sensor
 * - Analog Wind Speed Sensor (e.g., anemometer with analog output)
 * 
 * Connections:
 * - BME280 VCC -> 3.3V or 5V (BME280 supports both)
 * - BME280 GND -> GND  
 * - BME280 SCL -> GPIO 
 * - BME280 SDA -> GPIO 
 * - Wind Speed Sensor Analog Out -> ADC1_CHANNEL_0 (GPIO 32) - or another suitable ADC pin
 * - Wind Speed Sensor VCC -> 3.3V or 5V
 * - Wind Speed Sensor GND -> GND
 * 
 * Libraries Required:
 * - WiFi (ESP32 Core)
 * - HTTPClient (ESP32 Core)
 * - Adafruit BME280 Temp/Humd/Press Sensor
 *
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_BME280.h>
#include <time.h> // For timestamp (though not used in simplified post data)

// ==================== HARDWARE CONFIGURATION ====================

// Pin Assignments
//BME 280 Temperature, Humidity and Pressure
#define BME280_I2C_ADDR 0x76
Adafruit_BME280 bme;

#define ADC_PIN_WIND 32                              // Analog pin for wind speed sensor (e.g., GPIO 36 is ADC1_CHANNEL_0)

// Hardware Specifications
const String BOARD_MODEL = "ESP32-WROOM-32";
const String SENSOR_MODEL_BME280 = "BME280";
const String SENSOR_MODEL_WIND = "Analog Anemometer";
const String PROJECT_NAME = "CIJE Weather Hub - ESP32 Station";
const String PROJECT_VERSION = "1.0.0";
const String STATION_NAME = "BJDelta";

// Sensor Specifications
const float BM280_TEMP_MIN = -40.0;                 // BM280 minimum temperature (°C)
const float BM280_TEMP_MAX = 85.0;                  // BM280 maximum temperature (°C)
const float BM280_HUM_MIN = 0.0;                    // BM280 minimum humidity (%)
const float BM280_HUM_MAX = 100.0;                  // BM280 maximum humidity (%)
const float BM280_PRESS_MIN = 300.0;                // BM280 minimum pressure (hPa)
const float BM280_PRESS_MAX = 1100.0;               // BM280 maximum pressure (hPa)

// Wind Speed Calibration (Adjust these values based on your sensor's datasheet and calibration)
// Example: If 0-3.3V maps to 0-32.4 MPH, then 32.4 / 3.3 = 9.818
const float WIND_VOLTAGE_MAX = 3.3;                 // Max voltage output of your wind sensor
const float WIND_SPEED_MAX_MPH = 32.4;              // Max wind speed in MPH corresponding to max voltage
const float WIND_CONVERSION = WIND_SPEED_MAX_MPH / WIND_VOLTAGE_MAX; // Conversion factor V -> MPH

// ==================== NETWORK CONFIGURATION ====================

// WiFi Configuration
const char* WIFI_SSID = "OSxDesign_Rem";           // Replace with your WiFi network name
const char* WIFI_PASSWORD = "ixnaywifi";   // Replace with your WiFi password
const int WIFI_TIMEOUT = 30000;                     // 30 seconds WiFi connection timeout
const int HTTP_TIMEOUT = 15000;                     // 15 seconds HTTP request timeout

// Weather Station Credentials
const int STATION_ID = 5;                           // Replace with your registered station ID
const String PASSKEY = "EEGYLM";                   // Weather station passkey

// API Configuration
const String API_URL = "https://v0-cije-weather-hub.vercel.app/api/weather/submit";
const String USER_AGENT = "ESP32-WeatherStation-Test/1.0";
const String CONTENT_TYPE = "application/x-www-form-urlencoded";

// ==================== TIMING CONFIGURATION ====================

// Sampling Configuration
//const unsigned long SAMPLING_INTERVAL = 60000;       // 1 hour = 3,600,000 milliseconds
const unsigned long SAMPLING_INTERVAL = 3600000;   // 1 hour = 3,600,000 milliseconds
const int BME280_STABILIZATION_DELAY = 2000;         // 2 seconds for sensor stabilization

// LED Feedback
const int STATUS_LED = 4;
const int LED_BLINK_INTERVAL = 2000;                // Status LED blink interval (ms)
const int SUCCESS_BLINK_DURATION = 100;             // Success blink duration (ms)
const int ERROR_BLINK_DURATION = 200;               // Error blink duration (ms)

// ==================== ERROR HANDLING CONFIGURATION ====================

const int MAX_CONSECUTIVE_FAILURES = 5;             // Max failures before restart
const int RESTART_DELAY = 5000;                     // Delay before restart (ms)
const int LOOP_DELAY = 1000;                        // Main loop delay (ms)

// ==================== GLOBAL VARIABLES ====================
unsigned long lastSensorReading = 0;
int consecutiveFailures = 0;
float wind_speed = 0.0;
// ==================== SETUP ====================

void setup() 
{
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println(PROJECT_NAME);
  Serial.println("=================================");
  Serial.println("BME280 Data Pin: I2C GPIO ");
  Serial.println("Wind Speed Analog Pin: GPIO " + String(ADC_PIN_WIND));
  Serial.println("Sampling Interval: 1 hour");
  Serial.println("=================================");
  
  // Initialize hardware
  pinMode(STATUS_LED, OUTPUT);
  digitalWrite(STATUS_LED, LOW);
  float temperature, humidity, pressure;

    
  // Initialize sensors
  if (!bme.begin(BME280_I2C_ADDR)) 
  {
    Serial.println("❌ ERROR: Could not find a valid BME280 sensor");
    Serial.println("Check wiring: VCC, GND, I2C SCL, SDA");
  } 
  else 
  {
    // Get Temp, Humidity and Pressure from BME280 Sensor
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0F;  // Convert Pa to hPa

    Serial.println("✅ BME280 sensor initialized successfully");
    Serial.println("Initial temperature: " + String(temperature) + "°C");
    Serial.println("Initial humidity: " + String(humidity) + "%");
    Serial.println("Initial pressure: " + String(pressure) + "hPa");
    blinkSuccess();
  }

  // Initialize ADC for wind speed
  analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
  analogSetAttenuation(ADC_11db); // Set attenuation to 11dB for 0-3.3V range
  Serial.println("✅ ADC for wind speed initialized on GPIO " + String(ADC_PIN_WIND));
  
  // Connect to WiFi
  connectToWiFi();
  
  Serial.println("Setup complete! Starting weather monitoring...");
  Serial.println("=================================");
}

// ==================== MAIN LOOP ====================

void loop() 
{
  unsigned long currentTime = millis();
  
  // Check for serial commands
  checkSerialCommands();
  
  // Check if it's time for a sensor reading
  if (currentTime - lastSensorReading >= SAMPLING_INTERVAL) {
    
    // Ensure WiFi is connected
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected. Reconnecting...");
      connectToWiFi();
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      readAndSendWeatherData();
      lastSensorReading = currentTime;
    } else {
      Serial.println("❌ Cannot send data - WiFi not connected");
      consecutiveFailures++;
    }
  }
  
  // Handle consecutive failures
  if (consecutiveFailures >= MAX_CONSECUTIVE_FAILURES) 
  {
    Serial.println("⚠️  Too many consecutive failures. Restarting ESP32...");
    delay(RESTART_DELAY);
    ESP.restart();
  }
  
  // Blink status LED
  blinkStatusLED();
  delay(LOOP_DELAY);
}

// ==================== WIFI FUNCTIONS ====================

void connectToWiFi() 
{
  Serial.println("Connecting to WiFi: " + String(WIFI_SSID));
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  
  while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT) {
    delay(500);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi connected successfully!");
    Serial.println("IP Address: " + WiFi.localIP().toString());
    Serial.println("Signal Strength: " + String(WiFi.RSSI()) + " dBm");
    digitalWrite(STATUS_LED, HIGH);
    consecutiveFailures = 0;
  } else {
    Serial.println();
    Serial.println("❌ WiFi connection failed!");
    Serial.println("Check SSID and password");
    consecutiveFailures++;
  }
}

// ==================== SENSOR FUNCTIONS ====================

float readWindSpeed() 
{
  int adcValue = analogRead(ADC_PIN_WIND);
  // Convert ADC value (0-4095) to voltage (0-3.3V)
  float voltage = (float)adcValue / 4095.0 * 3.3; 
  // Convert voltage to MPH using calibration factor
  wind_speed = voltage * WIND_CONVERSION;
  return wind_speed;
}

void readAndSendWeatherData() 
{
  Serial.println("\n--- Reading Weather Data ---");
  
  float temperature, humidity, pressure;
    
  // Initialize sensors
  if (!bme.begin(BME280_I2C_ADDR)) {
    Serial.println("❌ ERROR: Could not find a valid BME280 sensor");
    Serial.println("Check wiring: VCC, GND, I2C SCL, SDA");
  } 
  else 
  {
    // Get Temp, Humidity and Pressure from BME280 Sensor
    temperature = bme.readTemperature();
    humidity = bme.readHumidity();
    pressure = bme.readPressure() / 100.0F;  // Convert Pa to hPa
  
  Serial.println("Temperature: " + String(temperature, 2) + "°C");
  Serial.println("Humidity: " + String(humidity, 2) + "%");
  Serial.println("Wind Speed: " + String(wind_speed, 2) + " mph");
  Serial.println("Pressure: " + String(pressure, 2) + " hPa");
  blinkSuccess();
  }

  // Send data to server
  bool success = sendWeatherData(temperature, humidity, wind_speed);
  
  if (success) 
  {
    Serial.println("✅ Data sent successfully");
    consecutiveFailures = 0;
    blinkSuccess();
  } else {
    Serial.println("❌ Failed to send data");
    consecutiveFailures++;
  }
  
  Serial.println("--- End Reading ---\n");
}

// ==================== HTTP FUNCTIONS ====================

bool sendWeatherData(float temperature, float humidity, float wind_speed) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected");
    return false;
  }
  
  HTTPClient http;
  http.begin(API_URL);
  http.setTimeout(HTTP_TIMEOUT);
  http.addHeader("Content-Type", CONTENT_TYPE);
  http.addHeader("User-Agent", USER_AGENT);
  
  // Create form data payload
  String postData = "station_id=" + String(STATION_ID) +
                   "&passkey=" + PASSKEY +
                   "&temperature=" + String(temperature, 2) +
                   "&humidity=" + String(humidity, 2) +
                   "&wind_speed=" + String(wind_speed, 2);
  
  Serial.println("Sending to: " + API_URL);
  Serial.println("Payload: " + postData);
  
  int httpResponseCode = http.POST(postData);
  
  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("HTTP Response Code: " + String(httpResponseCode));
    Serial.println("Response: " + response);
    
    if (httpResponseCode == 200) {
      Serial.println("✅ HTTP 200 OK - Data accepted by server");
    } else if (httpResponseCode == 400) {
      Serial.println("❌ HTTP 400 Bad Request - Check data format");
    } else if (httpResponseCode == 401) {
      Serial.println("❌ HTTP 401 Unauthorized - Check station ID and passkey");
    } else if (httpResponseCode == 429) {
      Serial.println("⏰ HTTP 429 Too Many Requests - Submitting too frequently");
      Serial.println("Server message: " + response);
      http.end();
      return true; // Return true to avoid restart cycle for expected 429
    } else if (httpResponseCode == 500) {
      Serial.println("❌ HTTP 500 Server Error - Server-side issue");
    } else {
      Serial.println("⚠️  HTTP " + String(httpResponseCode) + " - Unexpected response");
    }
    
    http.end();
    return (httpResponseCode == 200 || httpResponseCode == 429);
  } else {
    Serial.println("❌ HTTP Error: " + String(httpResponseCode));
    Serial.println("Error: " + http.errorToString(httpResponseCode));
    Serial.println("Check internet connection and server availability");
    http.end();
    return false;
  }
}

// ==================== UTILITY FUNCTIONS ====================

void blinkStatusLED() {
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  
  if (millis() - lastBlink > LED_BLINK_INTERVAL) {
    ledState = !ledState;
    digitalWrite(STATUS_LED, ledState);
    lastBlink = millis();
  }
}

void blinkSuccess() {
  for (int i = 0; i < 3; i++) {
    digitalWrite(STATUS_LED, HIGH);
    delay(SUCCESS_BLINK_DURATION);
    digitalWrite(STATUS_LED, LOW);
    delay(SUCCESS_BLINK_DURATION);
  }
}

void blinkError() {
  Serial.println("Entering error mode - continuous LED blinking");
  Serial.println("Reset ESP32 to retry");
  while (true) {
    digitalWrite(STATUS_LED, HIGH);
    delay(ERROR_BLINK_DURATION);
    digitalWrite(STATUS_LED, LOW);
    delay(ERROR_BLINK_DURATION);
  }
}

// ==================== SERIAL COMMANDS ====================

void checkSerialCommands() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    command.toLowerCase();
    
    if (command == "test") {
      Serial.println("Running sensor test...");
      readAndSendWeatherData();
    } else if (command == "restart") {
      Serial.println("Restarting ESP32...");
      delay(1000);
      ESP.restart();
    } else if (command == "info") {
      Serial.println("\n=== System Information ===");
      Serial.println("Station ID: " + String(STATION_ID));
      Serial.println("Passkey: " + PASSKEY);
      Serial.println("API URL: " + API_URL);
      Serial.println("Sampling Interval: " + String(SAMPLING_INTERVAL / 1000) + " seconds");
      Serial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
      Serial.println("Uptime: " + String(millis() / 1000) + " seconds");
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi: Connected (" + String(WiFi.RSSI()) + " dBm)");
        Serial.println("IP: " + WiFi.localIP().toString());
      } else {
        Serial.println("WiFi: Disconnected");
      }
      Serial.println("========================\n");
    } else if (command == "help") {
      Serial.println("\n=== Available Commands ===");
      Serial.println("test     - Force sensor reading and data transmission");
      Serial.println("restart  - Restart the ESP32");
      Serial.println("info     - Show system information");
      Serial.println("help     - Show this help message");
      Serial.println("==========================\n");
    } else {
      Serial.println("Unknown command: '" + command + "'. Type 'help' for available commands.");
    }
  }
}
