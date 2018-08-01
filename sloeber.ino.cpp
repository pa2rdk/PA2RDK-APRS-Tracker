#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2018-08-01 17:26:59

#include "Arduino.h"
#include <RDKAPRS.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <TimerOne.h>
#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal_SSD1306.h>

void loop() ;
void printRemote(float flat, float flon, bool isTX);
void showDisplay(float flat, float flon, bool isTX) ;
void locationUpdate(float flat, float flon) ;
char* deg_to_nmea(float fdeg, boolean is_lat) ;
void invertLCD(boolean i) ;
void txing() ;
void sinus_irq()     ;
void saveConfig() ;
void loadConfig() ;
void printConfig() ;
void setSettings(bool doSet) ;
void getStringValue(int length, bool fixedLen) ;
byte getCharValue() ;
byte getNumericValue() ;
void serialFlush() ;
static void smartDelay(unsigned long ms) ;
static void printFloat(float val, bool valid, int len, int prec, bool toLCD) ;
static void printInt(unsigned long val, bool valid, int len, bool toLCD) ;
static void printDateTime(TinyGPSDate &d, TinyGPSTime &t, bool toLCD) ;
static void printStr(const char *str, int len) ;
void setDra(byte rxFreq, byte txFreq, byte rxTone, byte txTone) ;
void setBigSize(boolean i) ;
void setLCDReverse(boolean i) ;
void beginLCD() ;
void setup() ;

#include "RDKTracker.ino"


#endif
