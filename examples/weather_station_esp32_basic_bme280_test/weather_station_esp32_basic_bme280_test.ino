/*
 * CIJE Weather Hub - ESP32 Weather Station
 *  
 * This is a test program for testing the BME280 Sensor
 * It uses the same pin configuration as the comple code
 * 
 * Hardware:
 * - ESP32-WROOM-32 Development Board
 * - BME280 Temperature , Humidity and Pressure Sensor
 * 
 * Connections:
 * - BME280 VCC -> 3.3V or 5V (BME280 supports both)
 * - BME280 GND -> GND  
 * - BME280 SCL -> GPIO SCL
 * - BME280 SDA -> GPIO SDA
 *  
 * Libraries Required:
 * - WiFi (ESP32 Core)
 * - HTTPClient (ESP32 Core)
 * - Adafruit BME280 Temp/Humd/Press Sensor
 *
 */
#include <Adafruit_BME280.h>

// ==================== HARDWARE CONFIGURATION ====================

// Pin Assignments
//BME 280 Temperature, Humidity and Pressure
#define BME280_I2C_ADDR 0x76
Adafruit_BME280 bme;


// ==================== SETUP ====================

void setup() 
{
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println("BME280 Data Pin: I2C GPIO ");
  Serial.println("=================================");
  
  // Initialize hardware
  float temperature, humidity, pressure;

    
  // Initialize sensors
  if (!bme.begin(BME280_I2C_ADDR)) 
  {
    Serial.println("❌ ERROR: Could not find a valid BME280 sensor");
    Serial.println("Check wiring: VCC, GND, I2C SCL, SDA");
  } 
  else 
  {
    Serial.println("✅ BME280 sensor initialized successfully");
  }

  Serial.println("Setup complete! Starting weather monitoring...");
  Serial.println("=================================");
  
  //Read and Display Sensor Data
  readAndSendWeatherData();
}

// ==================== MAIN LOOP ====================
void loop() 
{
  readAndSendWeatherData();
  delay(500);
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
    pressure = bme.readPressure() / 3386.38;  // Convert Pa to inHg
    //pressure = bme.readPressure() / 100.0F;  // Convert Pa 
    Serial.println("Temperature: " + String(temperature, 2) + "°C");
    Serial.println("Humidity: " + String(humidity, 2) + "%");
    Serial.println("Pressure: " + String(pressure, 2) + " inHg");

  }
}

  
