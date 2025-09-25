/*
 * CIJE Weather Hub - ESP32 Weather Station
 *  
 * This is a test program for testing the DHT22 Sensor
 * It uses the same pin configuration as the comple code
 *
 * Hardware:
 * - ESP32-WROOM-32 Development Board
 * - BME280 Temperature , Humidity and Pressure Sensor
 * - Analog Wind Speed Sensor (e.g., anemometer with analog output)
 * 
 * Connections:
 * - DHT22 VCC -> 3.3V or 5V (DHT22 supports both)
 * - DHT22 GND -> GND  
 * - DHT Data -> GPIO Pin 2 
 * - BME280 SDA -> GPIO 
 * - Wind Speed Sensor Analog Out -> ADC1_CHANNEL_0 (GPIO 32) - or another suitable ADC pin
 * - Wind Speed Sensor VCC -> 3.3V or 5V
 * - Wind Speed Sensor GND -> GND
 *   Battery Voltage -> GPIO32 ADC4
 * 
 * Libraries Required:
 * - WiFi (ESP32 Core)
 * - HTTPClient (ESP32 Core)
 * - DHT22  Temp/Humd  Adafruit DHT sensor Library ver 1.46
 *
 */

#include <DHT.h>

// ==================== HARDWARE CONFIGURATION ====================

// Pin Assignments
//DHT22 Temperature, Humidity

#define DHTPIN 5
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);

// ==================== GLOBAL VARIABLES ====================
float humidity = 0.0;
float temperature = 0.0;

// ==================== SETUP ====================
void setup() 
{
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println("DHT22 Data Pin: 2 GPIO ");
  Serial.println("=================================");
  
  // Initialize hardware
  float temperature, humidity;

    
  // Initialize sensors
  dht.begin();

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
      
  // Initialize sensors
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
    // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, humidity);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(temperature, humidity, false);

    Serial.println("Temperature: " + String(temperature, 2) + "°C");
    Serial.println("Humidity: " + String(humidity, 2) + "%");
}
    
