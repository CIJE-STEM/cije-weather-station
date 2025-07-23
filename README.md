# CIJE Weather Station Library

Arduino library for ESP32-based weather stations that collect and submit data to the CIJE Weather Hub.

## Features

- **Easy Configuration**: Simple API for setting up WiFi, sensors, and station credentials
- **Multiple Sensors**: Support for DHT22 (temperature/humidity) and analog wind sensors
- **Automatic Submission**: Configurable intervals for data collection and API submission
- **Status Monitoring**: LED status indicators and comprehensive system information
- **Error Handling**: Automatic retry logic and failure recovery
- **Serial Commands**: Interactive debugging and testing commands

## Hardware Requirements

- ESP32 development board
- DHT22 temperature and humidity sensor
- Analog wind speed sensor (optional)
- Status LED (built-in LED works)

## Wiring Diagram

\`\`\`
ESP32 Pin | Component
----------|----------
GPIO 32   | DHT22 Data Pin
GPIO 36   | Wind Sensor Analog Output
GPIO 2    | Status LED (built-in)
3.3V      | DHT22 VCC, Wind Sensor VCC
GND       | DHT22 GND, Wind Sensor GND
\`\`\`

## Installation

1. Download the library files
2. Place in your Arduino libraries folder: `~/Arduino/libraries/cije-weather-station/`
3. Install required dependencies:
   - DHT sensor library by Adafruit
   - Adafruit Unified Sensor

## Quick Start

\`\`\`cpp
#include <CijeWeatherStation.h>

CijeWeatherStation station;

void setup() {
  Serial.begin(115200);
  
  // Configure the station
  station.setWiFiCredentials("YOUR_WIFI", "YOUR_PASSWORD");
  station.setStationCredentials(1, "YOUR_PASSKEY");
  station.setDHTPin(32, DHT22);
  
  // Initialize
  if (!station.begin()) {
    Serial.println("Failed to initialize!");
    while(true) delay(1000);
  }
}

void loop() {
  station.loop();
  delay(100);
}
\`\`\`

## Configuration Methods

### Network Configuration
\`\`\`cpp
station.setWiFiCredentials("SSID", "PASSWORD");
station.setAPIURL("https://your-api-url.com/api/weather/submit");
\`\`\`

### Station Credentials
\`\`\`cpp
station.setStationCredentials(stationID, "passkey");
\`\`\`

### Hardware Configuration
\`\`\`cpp
station.setDHTPin(32, DHT22);        // DHT sensor on GPIO 32
station.setStatusLEDPin(2);          // Status LED on GPIO 2
station.setWindPin(36);              // Wind sensor on GPIO 36
\`\`\`

### Timing Configuration
\`\`\`cpp
station.setReadingInterval(3600000); // 1 hour in milliseconds
station.setTimeouts(30000, 15000);   // WiFi and HTTP timeouts
\`\`\`

### Wind Sensor Calibration
\`\`\`cpp
station.setWindCalibration(3.3, 32.4); // 3.3V max = 32.4 MPH max
\`\`\`

## Serial Commands

When connected to Serial Monitor, you can use these commands:

- `test` - Force an immediate sensor reading
- `info` - Display system information
- `wifi` - Show WiFi connection status
- `status` - Show current operational status
- `reading` - Show last sensor reading
- `sensors` - Test sensor readings
- `restart` - Restart the ESP32
- `help` - Show available commands

## Status LED Indicators

- **Slow blink (2s)**: Normal operation, connected
- **Fast blink (200ms)**: Connecting to WiFi
- **Very fast blink (100ms)**: Error condition
- **Triple flash**: Successful data submission

## API Integration

The library automatically formats and submits data to the CIJE Weather Hub API:

\`\`\`
POST /api/weather/submit
Content-Type: application/x-www-form-urlencoded

station_id=1&passkey=YOUR_PASSKEY&temperature=72.5&humidity=45.2&wind_speed=5.3
\`\`\`

## Error Handling

- Automatic WiFi reconnection
- Sensor reading validation
- HTTP retry logic
- Consecutive failure tracking
- Automatic restart after 5 consecutive failures

## Examples

### BasicWeatherStation
Full featured weather station with all sensors and interactive commands.

### SimpleTest
Quick testing version with 30-second intervals for rapid development.

## Troubleshooting

### Common Issues

1. **DHT sensor not responding**
   - Check wiring: VCC to 3.3V, GND to GND, DATA to GPIO 32
   - Ensure 2-second stabilization delay
   - Try different GPIO pin if needed

2. **WiFi connection fails**
   - Verify SSID and password are correct
   - Check signal strength
   - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)

3. **API submission fails**
   - Verify station ID and passkey are correct
   - Check internet connectivity
   - Confirm API URL is accessible

4. **Wind sensor readings incorrect**
   - Calibrate using `setWindCalibration()`
   - Check analog pin connection (GPIO 36)
   - Verify sensor voltage range (0-3.3V)

### Debug Information

Use `station.printSystemInfo()` to get comprehensive debug information including:
- Library version
- Hardware configuration
- Network status
- Memory usage
- Sensor readings
- Error counts

## Library Structure

\`\`\`
cije-weather-station/
├── src/
│   ├── CijeWeatherStation.h
│   └── CijeWeatherStation.cpp
├── examples/
│   ├── BasicWeatherStation/
│   │   └── BasicWeatherStation.ino
│   └── SimpleTest/
│       └── SimpleTest.ino
├── library.properties
├── keywords.txt
└── README.md
\`\`\`

## Version History

- **v1.0.0** - Initial release with DHT22 and wind sensor support

## License

MIT License - see LICENSE file for details.

## Support

For support and questions:
- GitHub Issues: https://github.com/cije/cije-weather-station/issues
- Email: support@cije.org
