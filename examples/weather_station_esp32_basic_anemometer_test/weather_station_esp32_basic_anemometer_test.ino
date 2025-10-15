/*
 * CIJE Weather Hub - ESP32 Weather Station
 * 
 * This is a test program for testing the Wind Sensor
 * It uses the same pin configuration as the complete code
 *  
 * Hardware:
 * - ESP32-WROOM-32 Development Board
 * - Analog Wind Speed Sensor (e.g., anemometer with analog output)
 *   Wiring: https://github.com/CIJE-STEM/cije-weather-station/blob/main/docs/weather_station_cije_anemometer.pdf
 * 
 * Connections:
 * - Wind Speed Sensor Analog Out -> ADC1_CHANNEL_0 (GPIO 32) - or another suitable ADC pin
 * - Wind Speed Sensor VCC -> 3.3V or 5V
 * - Wind Speed Sensor GND -> GND
 *
 */


// ==================== HARDWARE CONFIGURATION ====================

// Pin Assignments

#define ADC_PIN_WIND 34                              // Analog pin for wind speed sensor (e.g., GPIO 36 is ADC1_CHANNEL_0)


// Wind Speed Calibration (Adjust these values based on your sensor's datasheet and calibration)
// Example: If 0-3.3V maps to 0-32.4 MPH, then 32.4 / 3.3 = 9.818
const float WIND_VOLTAGE_MAX = 3.3;                 // Max voltage output of your wind sensor
const float WIND_SPEED_MAX_MPH = 32.4;              // Max wind speed in MPH corresponding to max voltage
const float WIND_CONVERSION = WIND_SPEED_MAX_MPH / WIND_VOLTAGE_MAX; // Conversion factor V -> MPH


float wind_speed = 0.0;
// ==================== SETUP ====================

void setup() 
{
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n=================================");
  Serial.println("Wind Speed Analog Pin: GPIO " + String(ADC_PIN_WIND));
  Serial.println("=================================");
  
  // Initialize ADC for wind speed
  analogReadResolution(12); // Set ADC resolution to 12 bits (0-4095)
  analogSetAttenuation(ADC_11db); // Set attenuation to 11dB for 0-3.3V range
  Serial.println("✅ ADC for wind speed initialized on GPIO " + String(ADC_PIN_WIND));
    
  readAndSendWeatherData();
}

// ==================== MAIN LOOP ====================
void loop() 
{
  readAndSendWeatherData();
  delay(500);
}

// ==================== SENSOR FUNCTIONS ====================

float readWindSpeed() 
{
  int adcValue = analogRead(ADC_PIN_WIND);
  // Convert ADC value (0-4095) to voltage (0-3.3V)
  float voltage = (((float)adcValue / 4095.0) * 3.3) - 0.100; //Include ADC offset voltage 150mV
  // Convert voltage to MPH using calibration factor
  wind_speed = abs(voltage * WIND_CONVERSION);

  return wind_speed;
}

void readAndSendWeatherData() 
{
  Serial.println("\n--- Reading Weather Data ---");

    readWindSpeed();

    Serial.println("Wind Speed: " + String(wind_speed, 2) + " mph");
  

  Serial.println("--- End Reading ---\n");
}


