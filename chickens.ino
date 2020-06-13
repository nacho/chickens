/*
  LiquidCrystal Library - Blink
 
 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the 
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.
 
 This sketch prints "Hello World!" to the LCD and makes the 
 cursor block blink.
 
 The circuit:
 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 5
 * LCD D5 pin to digital pin 4
 * LCD D6 pin to digital pin 3
 * LCD D7 pin to digital pin 2
 * LCD R/W pin to ground
 * 10K resistor:
   * ends to +5V and ground
   * wiper to LCD VO pin (pin 3)
 
 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe 
 modified 22 Nov 2010
 by Tom Igoe
 
 This example code is in the public domain.

 http://arduino.cc/en/Tutorial/LiquidCrystalBlink
 
 */

// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>
#include <RTClib.h>
#include <TimeLord.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
float const LATITUDE = 42.42;
float const LONGITUDE = -8.76;
TimeLord tardis;
byte today[] = {  0, 0, 12, 27, 10, 2020 };
int TIMEZONE = 2 * 60; // gmt + 2


void setup() {
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  
  if (! rtc.begin()) {
    lcd.print("Couldn't find RTC");
    while (1);
  }
  
  tardis.TimeZone(TIMEZONE);
  tardis.Position(LATITUDE, LONGITUDE);
}

void loop() {
  DateTime now = rtc.now();
  byte today[6] = {0,};
  
  today[tl_year] = now.year();
  today[tl_month] = now.month();
  today[tl_day] = now.day();
  today[tl_hour] = now.hour();
  
  if (tardis.SunSet(today)) {
    lcd.setCursor(0, 0);
    lcd.print((int)today[tl_hour]);
    lcd.print(':');
    lcd.print((int)today[tl_minute]);
  }
 
  if (tardis.SunRise(today)) {
    lcd.setCursor(0, 1);
    lcd.print((int)today[tl_hour]);
    lcd.print(':');
    lcd.print((int)today[tl_minute]);
  } else {
    lcd.setCursor(0, 0);
    lcd.print(now.year());
    lcd.print('/');
    lcd.print(now.month());
    lcd.print('/');
    lcd.print(now.day());
    
    lcd.setCursor(0, 1);
    lcd.print(now.hour());
    lcd.print(':');
    lcd.print(now.minute());
    lcd.print(':');
    lcd.print(now.second());
  }
  delay(1000);
}


