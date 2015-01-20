////////////////////////////////////////////////////////////
// Updated 20.01.2015 Romik
// Compiled by Arduino - 1.0.6
////////////////////////////////////////////////////////////
// ---------------------------------------------------------
// Mega 512K EEPROM
// Mega Ds1307 Real Time
// 23016 IP Expander
// GPS EM406 Serial 2 - 4800 если 0/0/2000 - ошибка -- Serial(2)
// DEBUG Serial 1 - 4800
// MMC SD 4 Gig
// SM5100B  GSM GPRS Module Serial(3)
// Serial(1) Debug
// BM085 
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


#include <glcd.h>
#include <fonts/allFonts.h>

void setup() {
  
  pinMode(30,OUTPUT);
  digitalWrite(30,HIGH);
  
  Serial2.begin(4800);  // GPS EM406 Pin 17
  Serial3.begin(9600);  // GSM Modem
  
  GLCD.Init();
  GLCD.SelectFont(System5x7);
  GLCD.print("hello, world!");
 
  // sms();
   
}

void loop() {
  
  // GLCD.CursorTo(0, 1);
  // GLCD.println(millis()/1000);
  
 if(Serial3.available()) {
   char nmea = Serial3.read();
   GLCD.print(nmea);
  }
 
}

void sms(void) {
  Serial3.print("AT+CMGF=1\r\n");                
  delay(500);
  Serial3.print("AT+CMGS=\"+79636455132\"\r\n"); 
  delay(500);
  Serial3.print("SMS Message");
  Serial3.print(char(26));
}
