////////////////////////////////////////////////////////////
// Updated 20.01.2015 Romik
// Compiled by Arduino - 1.0.6
////////////////////////////////////////////////////////////
// ---------------------------------------------------------
// 24LC512 512K EEPROM   == 0x50
// RTC DS1307            == 0x68
// 23016 IP Expander     == 0x20
// BM085                 == 0x77
////////////////////////////////////////////////////////////
// GPS EM406 Serial 2 - 4800 если 0/0/2000 - ошибка -- Serial(2)
// DEBUG Serial 1 - 4800
// MMC SD 4 Gig
// SM5100B  GSM GPRS Module Serial(3)
// Serial(1) Debug
//
// SM5100B Commands
// at+cops=?
// +COPS: (1,"MegaFon ",,"25002"),(3,"MTS",,"25001"),(3,"BEE LINE",,"25099"),,(0-4),(0,2)

// ------------------- Working Project 27.04.2011 -----------------------

// SPI port SD/MMC

// uint8_t const SS_PIN    = 53;
// uint8_t const MOSI_PIN  = 51;
// uint8_t const MISO_PIN  = 50;
// uint8_t const SCK_PIN   = 52;

// 4/30  * LCD RS pin to digital pin 12
// 6/32  * LCD Enable pin to digital pin 11
// 11/34 * LCD D4 pin to digital pin 5
// 12/36 * LCD D5 pin to digital pin 4
// 13/38 * LCD D6 pin to digital pin 3
// 14/40 * LCD D7 pin to digital pin 2
//
// 18 (interrupt 5), 19 (interrupt 4)
/////////////////////////////////////////////////////////////////////////////////////////


#define UTC 3

#include <glcd.h>
#include <fonts/allFonts.h>
#include "RTClib.h"
#include <BMP085.h>
#include <IOexpander.h>
#include <Wire.h>
#include <inttypes.h>
#include <Sunrise.h>
#include <SD.h>
#include <TinyGPS++.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <EEPROMAnything.h>
#include <I2C_eeprom.h>
#include <Average.h>

File root;

IOexpander IOexp;

BMP085 dps = BMP085();

long Temperature = 0, Pressure = 0, Altitude = 0;

RTC_DS1307 rtc;

void setup() {
  
  Wire.begin();
  
  rtc.begin();
  dps.init(MODE_ULTRA_HIGHRES, 25000, true);  // Разрешение BMP180
  
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
   
  pinMode(30,OUTPUT);
  digitalWrite(30,HIGH);
  
  Serial2.begin(4800);  // GPS EM406 Pin 17
  Serial3.begin(9600);  // GSM Modem
  
  GLCD.Init();
  GLCD.SelectFont(System5x7);
  GLCD.println("hello, world!");
  
  if(IOexp.init(0x20, MCP23016))
   GLCD.println("IO expander works!");
  else
   GLCD.println("No IOexpander!!");  

  if (!SD.begin()) {
    GLCD.println("initialization failed!");
  }
  
   root = SD.open("/");
   printDirectory(root, 0);

}

void loop() {
  
/*
  dps.getTemperature(&Temperature); 
  dps.getPressure(&Pressure);
  dps.getAltitude(&Altitude);
  
  GLCD.print("C");
  GLCD.print(Temperature*0.1);
  GLCD.print(" A");
  GLCD.print(Altitude/100.0);
  GLCD.print(" P");
  GLCD.println(Pressure/133.3);
  
  delay(1000);
  */
  
  i2c_scanner();
  
  // GLCD.CursorTo(0, 1);
  // GLCD.println(millis()/1000);
  
  // if(Serial3.available()) {
  //   char nmea = Serial3.read();
  //   GLCD.print(nmea);
  //  }
 
}

void sms(void) {
  Serial3.print("AT+CMGF=1\r\n");                
  delay(500);
  Serial3.print("AT+CMGS=\"+79636455132\"\r\n"); 
  delay(500);
  Serial3.print("SMS Message");
  Serial3.print(char(26));
}

void print_time() {
  
    DateTime now = rtc.now();
    
    GLCD.print(now.year(), DEC);
    GLCD.print('/');
    GLCD.print(now.month(), DEC);
    GLCD.print('/');
    GLCD.print(now.day(), DEC);
    GLCD.print(' ');
    GLCD.print(now.hour(), DEC);
    GLCD.print(':');
    GLCD.print(now.minute(), DEC);
    GLCD.print(':');
    GLCD.print(now.second(), DEC);
    GLCD.println();
    
}

void i2c_scanner() {
  
  byte error, address;
  int nDevices;
 
  nDevices = 0;

  for(address = 1; address < 127; address++ )
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
 
    if (error == 0)
    {
      GLCD.print("I2C device: 0x");
      if (address<16) GLCD.print("0");
      GLCD.print(address,HEX);
      GLCD.println("  !");
      nDevices++;
    }
  }  
  delay(5000);           // wait 5 seconds for next scan
}

void printDirectory(File dir, int numTabs) {
  
   while(true) {
     
     File entry =  dir.openNextFile();
  
     if (! entry) {
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       GLCD.print('\t');
     }
     GLCD.print(entry.name());
     if (entry.isDirectory()) {
       GLCD.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       GLCD.print("\t\t");
       GLCD.println(entry.size(), DEC);
     }
     entry.close();
   }
}

