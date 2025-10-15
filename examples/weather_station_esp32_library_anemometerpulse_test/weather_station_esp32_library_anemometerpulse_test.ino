// 
// Anemometer for wind speed via pulse detection (mph)

// https://thecije.org/weather-hub/


// ——— CONFIG ———

#define PULSE_PIN            34    // GPIO for optional anemometer pulses

const float VOLTAGE_DIVIDER_RATIO = 2.0;  // adjust per your voltage divider
const unsigned long SLEEP_INTERVAL_MS = 60000;  // 1 minute between readings
const float PULSE_FACTOR   = 2.25;            // mph per pulse/sec (calibrate)


// ——— GLOBALS ———
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

  float freq = count / dt;               // pulses/sec
  return freq * PULSE_FACTOR;
}


void setup() {
  Serial.begin(115200);
  delay(500);

  // Interrupt for anemometer
  pinMode(PULSE_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(PULSE_PIN), onPulse, RISING);

}

void loop() {
  float windMph     = readWindSpeed();
  Serial.printf("W: %.2f mph\n", windMph);
}
