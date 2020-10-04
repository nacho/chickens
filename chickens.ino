/*
 * Copyright (C) 2020 - Ignacio Casal Quinteiro
 *
 * Ths program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Ths program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Ths program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <Wire.h>
#include <RTClib.h>
#include <TimeLord.h>
#include <LiquidCrystal_I2C.h>

RTC_DS3231 rtc;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

float const LATITUDE = 42.42;
float const LONGITUDE = -8.76;
TimeLord tardis;
int TIMEZONE = 2 * 60; /* gmt + 2 */

/* pin that turns on the light */
int LIGHT_RELAY = 12;
int LIGHT_OVERRIDE_SWITCH = 2;

int RANGE = 30; /* in minutes */

void setup(void)
{
  if (!rtc.begin()) {
    /* FIXME: print an error in the serial? */
    while (1);
  }

  lcd.init();
  lcd.backlight();

  tardis.TimeZone(TIMEZONE);
  tardis.Position(LATITUDE, LONGITUDE);

  pinMode(LIGHT_RELAY, OUTPUT);
  pinMode(LIGHT_OVERRIDE_SWITCH, INPUT);
}

static DateTime datetime_from_tardis(bool sunrise)
{
  DateTime now = rtc.now();
  byte today[6] = {0,};

  /* it is bytes so we cannot use the direct year which is over uint8 */
  today[tl_year] = (now.year() - 2000);
  today[tl_month] = now.month();
  today[tl_day] = now.day();
  today[tl_hour] = now.hour();

  if (sunrise) {
    tardis.SunRise(today);
  } else {
    tardis.SunSet(today);
  }

  return DateTime(today[tl_year], today[tl_month], today[tl_day], today[tl_hour], today[tl_minute], today[tl_second]);
}

/* gets whether we are in range to turn on the light for the chickens to sleep */
static bool time_to_sleep(void)
{
  DateTime now = rtc.now();
  DateTime sunset = datetime_from_tardis(false);
  TimeSpan range(RANGE * 60);

  return ((now >= sunset) && (now < (sunset + range)));
}

/* leave them a range for them to eat */
static bool time_to_wakeup(void)
{
  DateTime now = rtc.now();
  DateTime sunrise = datetime_from_tardis(true);
  TimeSpan range(RANGE * 60);

  return ((now >= sunrise) && (now < (sunrise + range)));
}

static void print_to_lcd(bool light_override)
{
  DateTime now = rtc.now();
  DateTime sunrise = datetime_from_tardis(true);
  DateTime sunset = datetime_from_tardis(false);

  lcd.setCursor(0, 0);
  lcd.print(now.day());
  lcd.print("/");
  lcd.print(now.month());
  lcd.print("/");
  lcd.print(now.year() - 2000);
  lcd.print(" ");
  lcd.print(now.hour());
  lcd.print(":");
  lcd.print(now.minute() < 10 ? "0" + String(now.minute()) : now.minute());
  lcd.print(":");
  lcd.print(now.second() < 10 ? "0" + String(now.second()) : now.second());

  lcd.setCursor(0, 1);

  /* Is it worth doing some caching to avoid printing all the time to the
   * lcd device?
   */
  if (light_override) {
    /* FIXME: kind of hacky adding all the spaces but oh well... */
    lcd.print("Light override  ");
  } else {
    lcd.print("R/");
    lcd.print(sunrise.hour());
    lcd.print(":");
    lcd.print(sunrise.minute() < 10 ? "0" + String(sunrise.minute()) : sunrise.minute());

    lcd.print(" ");
    lcd.print("S/");
    lcd.print(sunset.hour());
    lcd.print(":");
    lcd.print(sunset.minute() < 10 ? "0" + String(sunset.minute()) : sunset.minute());
  }
}

static void change_light_state(bool on)
{
  digitalWrite(LIGHT_RELAY, on ? LOW : HIGH);
}

void loop(void)
{
  bool light_override = digitalRead(LIGHT_OVERRIDE_SWITCH) == HIGH;

  print_to_lcd(light_override);
  change_light_state(light_override || time_to_sleep() || time_to_wakeup());

  delay(1000);
}
