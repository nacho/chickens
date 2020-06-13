#include <Wire.h>
#include <RTClib.h>
#include <TimeLord.h>

RTC_DS3231 rtc;

float const LATITUDE = 42.42;
float const LONGITUDE = -8.76;
TimeLord tardis;
int TIMEZONE = 2 * 60; /* gmt + 2 */

/* pin that turns on the light */
int relay = 12;

int RANGE = 30; /* in minutes */

void setup(void)
{
  if (!rtc.begin()) {
    /* FIXME: print an error in the serial? */
    while (1);
  }

  tardis.TimeZone(TIMEZONE);
  tardis.Position(LATITUDE, LONGITUDE);

  pinMode(relay, OUTPUT);
}

static DateTime datetime_from_tardis(bool sunrise)
{
  DateTime now = rtc.now();
  byte today[6] = {0,};

  today[tl_year] = now.year();
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

  return ((now >= (sunset - range)) && (now < (sunset + range)));
}

/* leave them a range for them to eat */
static bool time_to_wakeup(void)
{
  DateTime now = rtc.now();
  DateTime sunrise = datetime_from_tardis(true);
  TimeSpan range(RANGE * 60);

  return ((now >= sunrise) && (now < (sunrise + range)));
}

void loop(void)
{
  if (time_to_sleep() || time_to_wakeup()) {
    digitalWrite(relay, HIGH);
  } else {
    digitalWrite(relay, LOW);
  }

  delay(1000);
}
