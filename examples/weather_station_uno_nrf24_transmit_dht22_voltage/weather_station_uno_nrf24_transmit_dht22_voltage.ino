#include "SPI.h"
#include "NRFLite.h"
#include "DHT.h"

//Sesnor Setup
#define DHTPIN 2 

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);


// NRF2401 Radio Setup
const static uint8_t RADIO_ID = 1;             // Our radio's id.
const static uint8_t DESTINATION_RADIO_ID = 0; // Id of the radio we will transmit to.
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

struct RadioPacket // Any packet up to 32 bytes can be sent.
{
    uint8_t FromRadioId;
    uint32_t OnTimeMillis;
    uint32_t FailedTxCount;
    float t;
    float h;
    float battery;
};

NRFLite _radio;
RadioPacket _radioData;

// Battery voltage
// The battery voltage is reduce through a 100K/33K network 
// 
int battVoltagePin = A0;  // Battery Voltage Analog Measurement Pin
int battDigital = 0;      // A/D Count from Battery Voltage Divider
float battery = 0.0;  // 0.0 could mean no battery data collected

void setup() 
{
  Serial.begin(115200);
  // Start DHT22 Sensor 
  dht.begin();

  if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
  {
      Serial.println("Cannot communicate with radio");
      while (1); // Wait here forever.
  }
    
  _radioData.FromRadioId = RADIO_ID;
}

void loop() {
  // Wait a few seconds between measurements.
  
  // Read Battery Voltage
  battDigital = analogRead(battVoltagePin);
  // Calculate battery voltage
  // Voltage calculated on A/D 1/2023 of 5V X the divider ratio 4.03 + a 
  // reverse polarity protection diode voltage of 0.45v
  battery = (4.03 * ((5.0 / 1023.0) * (battDigital))) + 0.45;  //  

  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
    // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));
  Serial.print("Battery: ");
  Serial.print(battery);
  Serial.print(" V");

  Serial.println();

  _radioData.OnTimeMillis = millis();
  _radioData.t = t;
  _radioData.h = h;
  _radioData.battery = battery;
  
    Serial.print("Sending ");
    Serial.print(_radioData.OnTimeMillis);
    Serial.print(" ms");
    
    
    Serial.print(_radioData.t);
    Serial.print(_radioData.h);
    Serial.print(_radioData.battery);

    // By default, 'send' transmits data and waits for an acknowledgement.  If no acknowledgement is received,
    // it will try again up to 16 times.  This retry logic is built into the radio hardware itself, so it is very fast.
    // You can also perform a NO_ACK send that does not request an acknowledgement.  In this situation, the data packet
    // will only be transmitted a single time and there is no verification of delivery.  So NO_ACK sends are suited for
    // situations where performance is more important than reliability.
    //   _radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData), NRFLite::REQUIRE_ACK) // THE DEFAULT
    //   _radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData), NRFLite::NO_ACK)
    
    if (_radio.send(DESTINATION_RADIO_ID, &_radioData, sizeof(_radioData))) // Note how '&' must be placed in front of the variable name.
    {
        Serial.println("...Success");
    }
    else
    {
        Serial.println("...Failed");
        _radioData.FailedTxCount++;
    }

  delay(2000);  // Replace with deep sleep interval (approximately 1 Minute)
}
