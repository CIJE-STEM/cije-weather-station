// Extended Weather Station Test
// - DHT22 for temperature (°F) & humidity (%)
// - Optional anemometer for wind speed via pulse detection (mph)
// - BME280 for pressure (inHg)
// - Placeholder precipitation sensor input (in)
// - Battery voltage monitoring via ADC (voltage divider)
// - Wind direction via analog input mapped to N/E/S/W
// - UV index placeholder via analog input (0-11)
//
// To register, submit, and view your station data, visit:
// https://v0-cije-weather-hub.vercel.app/

#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "esp_sleep.h"

// ——— CONFIG ———
#define DHTPIN               32    // GPIO for DHT22 data
#define DHTTYPE              DHT22 // DHT sensor model
#define PULSE_PIN            34    // GPIO for optional anemometer pulses

// Placeholder pins
const int PRECIP_PIN       = 35;   // digital or analog input for precipitation
const int BATTERY_PIN      = 36;   // ADC pin via voltage divider for battery voltage
const int WIND_DIR_PIN     = 39;   // ADC pin for wind direction sensor
const int UV_PIN           = 33;   // ADC pin for UV sensor (placeholder)

const float VOLTAGE_DIVIDER_RATIO = 2.0;  // adjust per your voltage divider
const unsigned long SLEEP_INTERVAL_MS = 60000;  // 1 minute between readings
const float PULSE_FACTOR   = 2.25;            // mph per pulse/sec (calibrate)

// WiFi credentials
const char* WIFI_SSID      = "PutnamChalet";
const char* WIFI_PASS      = "67355976";

// CIJE Weather Hub API
const String API_URL       = "https://v0-cije-weather-hub.vercel.app/api/weather/submit";
const int STATION_ID       = 1;
const String PASSKEY       = "20XV54";

// ——— GLOBALS ———
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BME280 bme;             // I2C BME280 instance
volatile unsigned long pulseCount = 0; // increments on ISR

// ——— ISR ———
void IRAM_ATTR onPulse() {
  pulseCount++;
}

// ——— READ WIND SPEED ———
float readWindSpeed() {
  noInterrupts();
  unsigned long count = pulseCount;
  pulseCount = 0;
  interrupts();

  float dt = SLEEP_INTERVAL_MS / 1000.0; // seconds
  float freq = count / dt;               // pulses/sec
  return freq * PULSE_FACTOR;
}

// ——— READ WIND DIRECTION ———
String readWindDirection() {
  int raw = analogRead(WIND_DIR_PIN);
  // Simple 4-sector mapping (0-4095)
  if (raw < 1024)      return "N";
  else if (raw < 2048) return "E";
  else if (raw < 3072) return "S";
  else                 return "W";
}

// ——— SEND DATA ———
void readAndSendWeatherData() {
  // DHT22 readings
  float humidity    = dht.readHumidity();
  float tempF       = dht.readTemperature(true);

  // Wind speed
  float windMph     = readWindSpeed();

  // BME280 pressure in inHg
  float pres_Pa     = bme.readPressure();
  float pressure_inHg = pres_Pa * 0.0002953;

  // Precipitation placeholder
  int precipRaw     = digitalRead(PRECIP_PIN);
  float precipitation = precipRaw; // placeholder — use proper sensor logic

  // Battery voltage
  int battRaw       = analogRead(BATTERY_PIN);
  float batteryVoltage = battRaw / 4095.0 * 3.3 * VOLTAGE_DIVIDER_RATIO;

  // Wind direction
  String windDir    = readWindDirection();

  // UV index placeholder
  int uvRaw         = analogRead(UV_PIN);
  float uvIndex     = uvRaw * (15.0 / 4095.0); // placeholder conversion

  // Log to serial
  Serial.printf("T: %.2f°F  H: %.2f%%  W: %.2f mph\n", tempF, humidity, windMph);
  Serial.printf("P: %.2f inHg  Precip: %.0f  Batt: %.2f V\n", pressure_inHg, precipitation, batteryVoltage);
  Serial.printf("Dir: %s  UV idx: %.2f\n", windDir.c_str(), uvIndex);

  // Build POST payload
  String post = "station_id=" + String(STATION_ID) +
                "&passkey="   + PASSKEY +
                "&temperature="+ String(tempF,2) +
                "&humidity="  + String(humidity,2) +
                "&wind_speed="+ String(windMph,2) +
                "&pressure="  + String(pressure_inHg,2) +
                "&precipitation=" + String(precipitation,2) +
                "&battery="   + String(batteryVoltage,2) +
                "&wind_direction=" + windDir +
                "&uv_index=" + String(uvIndex,2);

  HTTPClient http;
  http.begin(API_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST(post);
  if (httpCode > 0) {
    Serial.printf("HTTP Response: %d\n", httpCode);
    if (httpCode >= 200 && httpCode < 300) {
      Serial.println("✅ Data sent successfully");
    } else {
      Serial.println("❌ Server error—check payload");
    }
  } else {
    Serial.printf("POST failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

// ——— WIFI ———
void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500); Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi: " + WiFi.localIP().toString());
  } else {
    Serial.println("WiFi failed");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Init sensors
  dht.begin();
  Wire.begin();
  if (!bme.begin(0x76)) Serial.println("BME280 init failed");

  // Interrupt for anemometer
  pinMode(PULSE_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), onPulse, RISING);

  // First measurement + send
  connectToWiFi();
  readAndSendWeatherData();

  // Deep sleep until next interval
  esp_sleep_enable_timer_wakeup(SLEEP_INTERVAL_MS * 1000ULL);
  Serial.printf("Sleeping for %lu ms\n", SLEEP_INTERVAL_MS);
  esp_deep_sleep_start();
}

void loop() {
  // never called — wakes from sleep
}
