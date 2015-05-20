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

#define DEBUG 0
#define GSM_DEBUG 1

#include <glcd.h>

#include "fonts/fixednums7x15.h"          
#include "fonts/fixednums15x31.h"
#include "fonts/SystemFont5x7.h"

// ----- Мои Фонты ------

#include "TimeFont.h"    
#include "newbasic3x5.h"

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

#define FIVE_MINUT 300000
#define TWO_DAYS   172800

// ----- EEPROM ----------------------------

unsigned long BAR_EEPROM_POS = 0;

struct press_t // Данные о давлении и температуре
{    
    long Press,Temp;
    unsigned long unix_time; 
    
}  bmp085_data;

struct press_t_out // Данные о давлении и температуре
{    
    long Press,Temp;
    unsigned long unix_time; 
    
}  bmp085_data_out;

#define EEPROM_ADDRESS_512      (0x50) // EEPROM

#define EE24LC512MAXBYTES             524288/8
#define EE24LC32MAXBYTES               32768/8
#define EE24LC256MAXBYTES             262144/8

I2C_eeprom eeprom512(EEPROM_ADDRESS_512,EE24LC256MAXBYTES);

File root;

IOexpander IOexp;

BMP085 dps = BMP085();

long Temperature = 0, Pressure = 0, Altitude = 0;

RTC_DS1307 rtc;

unsigned long currentMillis;
unsigned long previousMillis;

unsigned long GPRSpreviousMillis;      // GPRS/GSM Test
unsigned long BarSavePreviousInterval; // Для барометра сохранять
unsigned long barPreviousInterval;     // Выводим давление

#define GSM Serial3

#define ds oneWire
#define ONE_WIRE_BUS 6
#define TEMPERATURE_PRECISION 12

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensor(&oneWire);
DeviceAddress ThermometerAddr = {  0x28, 0x13, 0xA9, 0x19, 0x03, 0x00, 0x00, 0x93 }; // D18B20+

boolean GPRS_Stat = false;

void setup() {
  
  Wire.begin();  // I2C Attach
  delay(500); 
  
  Serial.begin(9600);
  
  sensor.begin();
  sensor.setResolution(ThermometerAddr, TEMPERATURE_PRECISION);
  
  rtc.begin();
  dps.init(MODE_ULTRA_HIGHRES, 25000, true);  // Разрешение BMP180
  
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
   
  pinMode(30,OUTPUT);    // Включаем подсветку экрана
  digitalWrite(30,HIGH);
  
  Serial2.begin(4800);  // GPS EM406 Pin 17
  Serial3.begin(9600);  // GSM Modem 
  
  GLCD.Init();
  GLCD.SelectFont(System5x7);
  
  if (IOexp.init(0x20, MCP23016)) {
    GLCD.println("MCP23016 is OK.");
    IOexp.pinModePort(0,OUTPUT); 
    IOexp.pinModePort(1,INPUT); 
  } else
   GLCD.println("MCP23016 Error.");

  if (!SD.begin()) 
   GLCD.println("SD-CARD Error.");
  else 
   GLCD.println("SD-CARD is OK.");
   
  i2c_scanner();
   
  delay(3000);  
   
//   root = SD.open("/");
//   printDirectory(root, 0);
  
//   if (GPRS_Status()) {
//    sms();
//   }

  GLCD.ClearScreen(WHITE);
  
  // erase_eeprom_bmp085();
  
  DrawGrid();
  ShowBarData(true);  // Start 
  
}

// -------------------------- Рисуем график давления ----------------------------------

void DrawBar() {

 for(int x=119;x>23;x--) {
   int r = random(700, 800);
   int m = map(r,700,800,55,27);
   GLCD.DrawLine( x, 55, x, 27, WHITE); 
   GLCD.DrawLine( x, 55, x, m, BLACK); 
 }
 
}

// ---------------------- Выводим на экран сетку для графиков -----------------------

void DrawGrid() {

  GLCD.DrawLine( 0, 55, 126, 55, BLACK); // Горизонт
  GLCD.DrawLine( 15, 27, 15, 63, BLACK); // Вертикаль
  
  GLCD.DrawLine( 15+1, 27, 15+2, 27, BLACK); // Разметка по верикали
  
  for(int i=2;i<27;i+=2) {
   GLCD.DrawLine( 15+1, 27+i, 15+2, 27+i, BLACK); // Разметка по вертикали
  }
  
  GLCD.SelectFont(newbasic3x5);
  
  // --------------------------- Выводим линейки времени внизу графика ------
  
  DateTime now = rtc.now();

  byte h_now = now.hour();
  
  byte s = 0;
  
  byte array[4] = { 0,6,12,18 };

  if (h_now > 0  && h_now < 6 ) s = 0;
  if (h_now > 6  && h_now < 12) s = 1; 
  if (h_now > 12 && h_now < 18) s = 2;
  if (h_now > 18              ) s = 3;
  
  for(int h=0;h<16;h+=2) {
   GLCD.GotoXY(114-(h*6),56);
   GLCD.print("  ");
   GLCD.GotoXY(114-(h*6),56);
   GLCD.print(h_now);
   h_now = array[s];
   if (s == 0) s = 3; else s--;   
  }
 
  // --------------------- Вывели линейку времени под графиком ---------------------
  
  GLCD.DrawLine( 15, 27, 15, 55, BLACK);

}

// ------------------------- Записываем данные в EEPROM -------------------

void Save_Bar_Data() {

    // 48 часов * 60 минут = 2880 Минут
    // 2880 минут / 30 минут = 96 Ячеек
    // (UnixTime / 1800) % 96 = номер ячейки

  dps.getPressure(&Pressure);        // Давление
  dps.getTemperature(&Temperature);  // Температура
   
  DateTime now = rtc.now();
       
  bmp085_data.Press = Pressure/133.3; 
  bmp085_data.Temp  = Temperature*0.1;
  
   bmp085_data.unix_time = now.unixtime();  // - (60 * 60 * UTC);
   
   BAR_EEPROM_POS = ( (bmp085_data.unix_time/1800)%96 ) * sizeof(bmp085_data); // Номер ячейки памяти.
   
   const byte* p = (const byte*)(const void*)&bmp085_data;
   for (unsigned int i = 0; i < sizeof(bmp085_data); i++) 
    eeprom512.writeByte(BAR_EEPROM_POS++,*p++);
   
}
  
// -------------------------------- Основной Цикл -------------------------------------------------------------------

void loop() {

  currentMillis = millis();
  
  if(currentMillis - previousMillis > 1000) {
   previousMillis = currentMillis; 
   g_print_time();
   g_temp();
   
   dps.getPressure(&Pressure);        // Давление
   dps.getTemperature(&Temperature);  // Температура
  
  }
  
  if(currentMillis - BarSavePreviousInterval > (FIVE_MINUT*4)) { // Каждые 20 минут Save BAR Parameters 
   BarSavePreviousInterval = currentMillis;
   Save_Bar_Data();  
  }
    
    
 if (GSM_DEBUG) { 
   if (GSM.available()) {
     char in = GSM.read();
     Serial.print(in);
   }
   
   if (Serial.available()) {
     char in = Serial.read();
     GSM.print(in);
   }
 }
   

  ShowBarData(false);
  
  
  //if(currentMillis - GPRSpreviousMillis > 10000) {
  //GPRSpreviousMillis = currentMillis; 
  // if (Check_AT_Command()) {
    //GPRS_Stat = GPRS_Status();
  // }
   // if (GPRS_Stat) sms();
  //} 
   
  
      /*  if(currentMillis - GPRSpreviousMillis > 2000) {          
        GPRSpreviousMillis = currentMillis; 
         if (GPRS_Status()) {
          sms();
          GLCD.println("GSM is OK");
         }
         GLCD.println("No GSM");
        }
        
        */

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
  
 //  i2c_scanner();
  
  // GLCD.CursorTo(0, 1);
  // GLCD.println(millis()/1000);
  
}

// ---------------- Barometer Graphics ------------------------

void ShowBarData(boolean s) {
 
 if (s == true) dps.getPressure(&Pressure);  // Первоначальный запуск
 
 if (( currentMillis - barPreviousInterval > FIVE_MINUT/2 ) || s == true ) {  
      barPreviousInterval = currentMillis;      
  
    DrawGrid();  // Обновляем сетку
   
   if (DEBUG) Read_Data_BMP_EEPROM();  // Выводим в Serial все данные.

   DateTime now = rtc.now();    
  
   Average<long> bar_data(96);  // Вычисление максимального и минимального значения
   
   long barArray[96];   
   
   BAR_EEPROM_POS = 0;
    
   for(byte j = 0;j < 96; j++) {           
  
     byte* pp = (byte*)(void*)&bmp085_data_out;  
     for (unsigned int i = 0; i < sizeof(bmp085_data_out); i++)
     *pp++ = eeprom512.readByte(BAR_EEPROM_POS++); 
     
    if ((now.unixtime() - bmp085_data_out.unix_time) < TWO_DAYS)  {
     
      barArray[j] = bmp085_data_out.Press;   
      bar_data.push(bmp085_data_out.Press);
      
      if (DEBUG) Serial.println( barArray[j]);
      
    } else barArray[j] = 0.0;
    
   } // Попытались считать все 96 значений давления

  //  Выводим - Максимум и Минимум
  
  GLCD.GotoXY(1,25);
  GLCD.print(bar_data.maximum());

  GLCD.GotoXY(1,47);
  GLCD.print(bar_data.minimum(),DEC);
  
   // ----- Выводим текущие давление ----
   
   GLCD.GotoXY(1,56);
   GLCD.print((int)(Pressure/133.3),DEC);
  
   BAR_EEPROM_POS = 0;

   // Давление     

   int m;             // Высота линии
   int x = 119;    // Стартовая позиция
   
   byte current_position = (now.unixtime()/1800)%96; 
        
   for(byte j=0;j<96;j++) {
     
    if (j != 0) { 
      m = map(barArray[current_position],bar_data.minimum(),bar_data.maximum(),54,27);  
    } else {
      if (Pressure/133.3 < bar_data.maximum() && Pressure/133.3 > bar_data.minimum()) {
       m = map(Pressure/133.3,bar_data.minimum(),bar_data.maximum(),54,27);         // Текущие значение
      } else { 
       m = 54;  // текущие значение может не попасть в предел
      }
    }

    GLCD.DrawLine( x, 54, x, 27, WHITE);  // Стереть линию
     
    if (barArray[current_position] != 0.0) {     
      GLCD.DrawLine( x, 54, x, m, BLACK); 
       if (DEBUG) Serial.println(m);
    }
    
    if (DEBUG) Serial.println(current_position);
    
    if (current_position == 0) current_position = 96;
    
    current_position--; 
    
    x--;

   } 
  
  }  
  
}


void sms(void) {
  Serial3.print("AT+CMGF=1\r\n");                
  delay(500);
  Serial3.print("AT+CMGS=\"+79636455132\"\r\n"); 
  delay(500);
  Serial3.print("Mega 2560 GPS GSM is OK.");
  Serial3.print(char(26));
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


// ------------ Выводим время ------------------------------------

void g_print_time( void ) {

 GLCD.CursorToXY(1,1);
 
 DateTime now = rtc.now();

  byte s = now.second();
  byte m = now.minute();
  byte h = now.hour();
   
  GLCD.SelectFont(TimeFont);

  // GLCD.SelectFont(newbasic3x5);

  if (h < 10) GLCD.print("0"); GLCD.print(h,DEC); 
   GLCD.print(":");
  if (m < 10) GLCD.print("0"); GLCD.print(m,DEC); 
   GLCD.print(":");
  if (s < 10) GLCD.print("0"); GLCD.print(s,DEC); 
  
}

// ---------------------- Выводим температуру и давление ----------------------

void g_temp() {
    
   char output[8];
   
   sensor.requestTemperatures();
  
   float tempC = sensor.getTempC(ThermometerAddr);
  
   GLCD.SelectFont(System5x7);
    
   GLCD.CursorToXY(95,2);
   dps.getPressure(&Pressure);  
   unsigned int p = Pressure/133.3;
   sprintf(output,"P:%d",p);
   GLCD.print(output);
   
   int t = tempC;
   sprintf(output,"t:%d",t);
   GLCD.CursorToXY(95,12);   
   GLCD.print(output);
   
}

// --------------- I2C Проверка ---------------------------------

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
      GLCD.print("I2C: 0x");
      if (address<16) GLCD.print("0");
      GLCD.print(address,HEX);
      if (address == 0x50) GLCD.println(" EEPROM");
      if (address == 0x68) GLCD.println(" DS1307");
      if (address == 0x20) GLCD.println(" MCP23016");
      if (address == 0x77) GLCD.println(" BMP085");
      nDevices++;
    }
  }  
}


boolean Check_AT_Command(void) {

  byte incount = 0; 
  char xdata[100];
  char *found = 0;

  Serial3.println("AT");
  delay(1000);

  if (Serial3.available()) {
    while (Serial3.available()) {
      xdata[incount++] = Serial3.read();
      if (incount == (sizeof(xdata)-2)) { 
        break; 
      }
    }

    xdata[incount] = '\0';

    found = strstr(xdata,"\r\nOK\r\n");

    if (found) { 
      return(true); 
    } 
    else { 
      asm volatile ("  jmp 0"); // Reboot
      return(false); 
    }

  }

  return(false);
}

// ----------------------------------- GPRS Status --------------------------------

boolean GPRS_Status(void) {

  byte incount = 0; 
  char xdata[100];
  char *found = 0;

  Serial3.println("AT+CGATT?");
  delay(1000);

  if (Serial3.available()) {
    while (Serial3.available()) {
      xdata[incount++] = Serial3.read();
      if (incount == (sizeof(xdata)-2)) { 
        break; 
      }
    }

    xdata[incount] = '\0';

    found = strstr(xdata,"\r\n+CGATT: 1\r\n");

    if (found) { 
      return(true); 
    } 

  }

  return(false);
}


// --------------------------- Output BMP_085 DATA from EEPROM -----------------------------------

void Read_Data_BMP_EEPROM( void ) {
  
   Average<long> bar_data(96); // Вычисление максимального и минимального значения
   Average<long> tem_data(96); // Вычисление максимального и минимального значения
   
   BAR_EEPROM_POS = 0;

   Serial.println("--------------- START -----------------------");
    
   DateTime now = rtc.now();
    
   for(byte j=0;j<96;j++) {
           
    byte* pp = (byte*)(void*)&bmp085_data_out; 
    for (unsigned int i = 0; i < sizeof(bmp085_data_out); i++)
     *pp++ = eeprom512.readByte(BAR_EEPROM_POS++);
    
    if (bmp085_data_out.Press != 0.0) bar_data.push(bmp085_data_out.Press);
    
     tem_data.push(bmp085_data_out.Temp);         
    
    Serial.print(bmp085_data_out.Press);      Serial.print(";");
    Serial.print(bmp085_data_out.Temp);       Serial.print(";");
    Serial.print(bmp085_data_out.unix_time);  Serial.print(";");
    Serial.print(now.unixtime() - bmp085_data_out.unix_time); Serial.print(";");

    DateTime eeTime (bmp085_data_out.unix_time);

    Serial.print(eeTime.year()); Serial.print("-");
    Serial.print(eeTime.month());Serial.print("-");
    Serial.print(eeTime.day());

    Serial.print("  ");

    Serial.print(eeTime.hour());Serial.print(":");
    Serial.print(eeTime.minute());Serial.print(":");
    Serial.print(eeTime.second());
    
    
    Serial.print(" Min/Max: ");
    Serial.print(map(bmp085_data_out.Press,bar_data.minimum(),bar_data.maximum(),106,1));
    Serial.print(" ");
    Serial.print(map(bmp085_data_out.Temp ,tem_data.minimum(),tem_data.maximum(),106,1));
    
    
    Serial.println();
          
   }
   
   Serial.println("-----------MAX and MIN-------------");
   Serial.println(bar_data.maximum());
   Serial.println(bar_data.minimum());
   Serial.println("");
   Serial.println(tem_data.maximum());
   Serial.println(tem_data.minimum());
   
   Serial.println("--------------- STOP -----------------------");
   
   BAR_EEPROM_POS = 0;
    
}

void erase_eeprom_bmp085( void ) {
  
  BAR_EEPROM_POS = 0;
   
  bmp085_data.Press         = 0.0;
  bmp085_data.Temp         = 0.0;
  bmp085_data.unix_time  = 0;
  
  while(BAR_EEPROM_POS < (EE24LC512MAXBYTES - (sizeof(bmp085_data) + 1))) {
   
   const byte* p = (const byte*)(const void*)&bmp085_data;
   for (unsigned int i = 0; i < sizeof(bmp085_data); i++)
    eeprom512.writeByte(BAR_EEPROM_POS++,*p++);
  }
    
  
}
