/*
Radio    Arduino
CE    -> 9
CSN   -> 10 (Hardware SPI SS)
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
IRQ   -> No connection
VCC   -> No more than 3.6 volts
GND   -> GND

SD Card Reader/Writer
CS    -> 4
MOSI  -> 11 (Hardware SPI MOSI)
MISO  -> 12 (Hardware SPI MISO)
SCK   -> 13 (Hardware SPI SCK)
VCC   -> 3.3-5.0 volts
GND   -> GND

*/
#include <Wire.h> 
#include "SPI.h"
#include "NRFLite.h"
#include <Servo.h>
#include <Bonezegei_DS3231.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>

Bonezegei_DS3231 rtc(0x68);
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const int chipSelect = 4;

// Assign local variables
Servo temp;
Servo humd;
float temperature = 0.0;
float humidity = 0.0;
float battVolt = 0.0;
int tempPos = 0;
int humdPos = 0;
int rtcArray[6] = {0};
unsigned long filePos = 0;

int tHour = 0;

String dataString = "";

const static uint8_t RADIO_ID = 0;       // Our radio's id.  The transmitter will send to this id.
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

// Setlocal sturct to match remote structure for data transfer
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

void setup()
{
    Serial.begin(115200);
    temp.attach(2);
    humd.attach(3);
    rtc.begin();
    lcd.init();
    lcd.backlight();

    Serial.println("Initializing SD card...");

    if (!SD.begin(chipSelect)) {
        Serial.println("initialization failed. Things to check:");
        Serial.println("1. is a card inserted?");
        Serial.println("2. is your wiring correct?");
        Serial.println("3. did you change the chipSelect pin to match your shield or module?");
        Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");
    }

    Serial.println("SD Init Complete");
  
    // rtc.setFormat(24);        //Set 12 Hours Format
    // rtc.setAMPM(0);           //Set AM or PM    0 = AM  1 =PM
    // rtc.setTime("17:49:00");  //Set Time    Hour:Minute:Seconds
    // rtc.setDate("5/26/25");   //Set Date    Month/Date/Year
    

    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN))
    {
        Serial.println("Cannot communicate with radio");
    }
   
    delay(500);
}

void loop()
{
    //Get current time from Real Time Clock
    if(rtc.getTime())  
    {
        //Get current Time, date Values and place in Array for Data Logging
        rtcArray[0] = rtc.getMonth();
        rtcArray[1] = rtc.getDay();
        rtcArray[2] = rtc.getYear();
        rtcArray[3] = rtc.getHour();
        rtcArray[4] = rtc.getMinute();
        rtcArray[5] = rtc.getSeconds();
    }
    
    //Verify that Radio is connecting and sending data from remote
    while (_radio.hasData())
    {
        _radio.readData(&_radioData); // Note how '&' must be placed in front of the variable name.
        
        // Assign temperature, humidity to local variables
        temperature = _radioData.t;
        humidity = _radioData.h;
        battVolt = _radioData.battery;
 
        // Scale and set Servo positions for Temperature and Humdidty
        tempPos = map(temperature,-40,50,180,0);
        humdPos = map(humidity,0,100,0,180);
        
        temp.write(tempPos);
        humd.write(humdPos);
    }
    
    //Sned data to LCD display
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(temperature);
    lcd.print("C");
    lcd.setCursor(10,0);
    lcd.print(humidity);
    lcd.print("%");
    lcd.setCursor(0,1);
    lcd.print(rtcArray[3]);  //hour
    lcd.print(":");
    lcd.print(rtcArray[4]);  //minutes
    lcd.print(":");
    lcd.print(rtcArray[5]);  //seconds
    lcd.setCursor(10,1);
    lcd.print(battVolt);
    lcd.print("V");
    
    //Using the current hour if different than last check send data ot logger file
    if(rtcArray[3] != tHour)
    {
        //Dignosis data to check that hour is updating
        // Serial.print(rtcArray[4]);// print the current hour
        // Serial.print("  ");
        // Serial.print(tHour);
        // Serial.println();
   
        //Clear data string for proper formating
        dataString = "";
        
        //Assemble data string with date, time and remote data
        for( int ptr = 0 ; ptr < 6 ; ptr++)
        {
           
            dataString = dataString + rtcArray[ptr];
            if(ptr < 2)
            {
              dataString = dataString + "/";
            }
            else if(ptr < 3)
            {
                dataString = dataString + ",";
            }
            else if(ptr < 5)
            {
                dataString = dataString + ":";
            }
        }
        //Add remote temperature, humdidty, and battery voltage
        dataString = dataString + "," + temperature + "," + humidity + "," + battVolt;

        //Open the datalogging file to append data
        File dataFile = SD.open("datalog.csv", FILE_WRITE);
        
        //Verify data file exists and is ready for appedned data
        if(dataFile)            
        {
            dataFile.println(dataString);
            dataFile.close();               //Standard practive is to close file when done
            dataFile.flush();               //Added safety check to ensure file has been written
            //Serial.println(dataString);   //Diagnostic data show what was written
        }
        else
        {
            //Serial.println("Error opening datalog.csv"); //Show that file was in error
            dataFile.close();
        }
        tHour = rtcArray[3];  //Update hour for next check
    }
    
    delay(750);  //Remote posts data every second so no need to over due read cycles

}
