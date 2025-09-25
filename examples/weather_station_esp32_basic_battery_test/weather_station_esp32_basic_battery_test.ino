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



// ==================== HARDWARE CONFIGURATION ====================

// Pin Assignments

#define ADC_PIN_WIND 32                              // Analog pin for wind speed sensor (e.g., GPIO 36 is ADC1_CHANNEL_0)


void setup() 
{
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
 
  // Initialize ADC for wind speed
  analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
  analogSetAttenuation(ADC_11db); // Set attenuation to 11dB for 0-3.3V range
  Serial.println("✅ ADC for wind speed initialized on GPIO " + String(ADC_PIN_WIND));

  Serial.println("Setup complete! Starting weather monitoring...");
  Serial.println("=================================");
  
  //Read and Display Sensor Data
  readAndSendWeatherData();
}

// ==================== MAIN LOOP ====================
void loop() 
{

}

// ==================== SENSOR FUNCTIONS ====================
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
    //pressure = bme.readPressure() / 3386.38;  // Convert Pa to Hg
    pressure = bme.readPressure() / 100.0F;  // Convert Pa to Hg
    
    readWindSpeed();

    Serial.println("Temperature: " + String(temperature, 2) + "°C");
    Serial.println("Humidity: " + String(humidity, 2) + "%");
    Serial.println("Pressure: " + String(pressure, 2) + " hPa");
    Serial.println("Wind Speed: " + String(wind_speed, 2) + " mph");
    
    blinkSuccess();
  }

  // Send data to server
  bool success = sendWeatherData(temperature, humidity, pressure, wind_speed);
  
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

bool sendWeatherData(float temperature, float humidity, float pressure, float wind_speed) {
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
                    "&pressure=" + String(pressure, 2) +
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
