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

/*
 * Small arduino program that allows to handle my requirements for my chickens:
 *
 * - Detects with an RTC when the sunrise and sunset happen. For this there are
 *   two variables LATITUDE and LONGITUDE that must be set to your coordinates.
 *
 * - Turns on the light right after sunrise so the chickens can have some light
 *   to eat if they wish since at that moment there is still not enough light
 *   for them to properly watch where they sleep. For this the RANGE variable
 *   is used for the amount of time that will be turned on after sunrise.
 *
 * - Turns on the light when the sunset is going to happen. Also RANGE is used
 *   to decide when it is going to happen. But in this case also STEP_DELAY,
 *   since we want to wait for an amount of time after sunrise to turn on the light.
 *   This is due to the fact that after sunrise normally there is still a bit of light
 *   and the chickens are outside till the last second. Currently this is done as
 *   (SUNSET + STEP_DELAY, RANGE).
 *
 * - The pin to handle the relay for the light is set in LIGHT_RELAY.
 *
 * - A ON-OFF-ON switch button is used to override the RTC logic so the light is turned
 *   ON or OFF. LIGHT_ON_SWITCH and LIGHT_OFF_SIWTCH are the pins to handle the light.
 *
 * - To control the door a L298N driver is used. And the pins L298N_IN1 and L298N_IN2
 *   are used to handle it.
 *
 * - An ON-OFF-ON switch is used to override the RTC and to manually handle the door.
 *   This is configured in the pins OPEN_DOOR_SWITCH and CLOSE_DOOR_SWITCH. If none
 *   of the pins are ON then the RTC will be used to automatically handle the door.
 */


#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

/* pin that turns on the light */
const int LIGHT_RELAY = 10;
const int LIGHT_ON_SWITCH = 2;
const int LIGHT_OFF_SWITCH = 3;

/* pins to handle the door */
const int OPEN_DOOR_SWITCH = 4;
const int CLOSE_DOOR_SWITCH = 5;

/* pins for door limit switches */
const int DOOR_OPEN_LIMIT_SWITCH = 6;
const int DOOR_CLOSE_LIMIT_SWITCH = 7;

/* L298N for the door */
const int L298N_IN1 = 8;
const int L298N_IN2 = 9;

void setup(void)
{
  lcd.init();
  lcd.backlight();

  tardis.TimeZone(TIMEZONE);
  tardis.Position(LATITUDE, LONGITUDE);

  pinMode(LIGHT_RELAY, OUTPUT);
  pinMode(LIGHT_ON_SWITCH, INPUT);
  pinMode(LIGHT_OFF_SWITCH, INPUT);
  pinMode(OPEN_DOOR_SWITCH, INPUT);
  pinMode(CLOSE_DOOR_SWITCH, INPUT);
  pinMode(L298N_IN1, OUTPUT);
  pinMode(L298N_IN2, OUTPUT);
  pinMode(DOOR_OPEN_LIMIT_SWITCH, INPUT);
  pinMode(DOOR_CLOSE_LIMIT_SWITCH, INPUT);
}

static void print_to_lcd(bool light_on,
                         bool light_off,
                         bool open_door,
                         bool door_opened,
                         bool close_door,
                         bool door_closed)
{
  /* Is it worth doing some caching to avoid printing all the time to the
   * lcd device?
   */
  if (light_on || light_off || open_door || close_door) {
    /* FIXME: kind of hacky adding all the spaces but oh well... */
    if (light_on) {
      if (open_door) {
        lcd.print("L ON/D Open     ");
      } else if (close_door) {
        lcd.print("L ON/D Close     ");
      } else {
        lcd.print("Light ON        ");
      }
    } else if (light_off) {
      if (open_door) {
        lcd.print("L OFF/D Open     ");
      } else if (close_door) {
        lcd.print("L OFF/D Close     ");
      } else {
        lcd.print("Light OFF       ");
      }
    } else if (open_door) {
      lcd.print("Door Open");
      if (door_opened) {
        lcd.print("/opened");
      } else {
        lcd.print("       ");
      }
    } else {
      lcd.print("Door Close");
      if (door_closed) {
        lcd.print("/closed");
      } else {
        lcd.print("      ");
      }
    }
  }
}

static void change_light_state(bool light_on,
                               bool light_off)
{
  bool on = light_on ||
            ((now >= (sunset + step_delay)) && (now < (sunset + step_delay + range))) ||
            ((now >= sunrise) && (now < (sunrise + range)));

  on &= !light_off;

  digitalWrite(LIGHT_RELAY, on ? LOW : HIGH);
}

/* Handles the state of the door and opens the door or closes it depending
 * of the different inputs.
 * Note that when opening/closing the door we wait for a RANGE and the STEP_DELAY
 * since it can be still slightly dark or slighly lightly and the chickens might
 * be still outside or insecure to leave the coop.
 */
static void handle_door(bool open_door,
                        bool close_door,
                        bool door_opened,
                        bool door_closed)
{
  enum {
    DOOR_STATE_NONE,
    DOOR_STATE_OPENING,
    DOOR_STATE_CLOSING
  } door_state;

  door_state = DOOR_STATE_NONE;

  if (open_door) {
    if (!door_opened) {
      door_state = DOOR_STATE_OPENING;
    }
  } else if (close_door) {
    if (!door_closed) {
      door_state = DOOR_STATE_CLOSING;
    }
  } else if (now >= (sunrise + step_delay) && now < (sunset + step_delay + range) && !door_opened) {
    door_state = DOOR_STATE_OPENING;
  } else if (now >= (sunset + step_delay + range) && !door_closed) {
    door_state = DOOR_STATE_CLOSING;
  }

  if (door_state == DOOR_STATE_OPENING) {
    digitalWrite(L298N_IN1, HIGH);
    digitalWrite(L298N_IN2, LOW);
  } else if (door_state == DOOR_STATE_CLOSING) {
    digitalWrite(L298N_IN1, LOW);
    digitalWrite(L298N_IN2, HIGH);
  } else {
    digitalWrite(L298N_IN1, LOW);
    digitalWrite(L298N_IN2, LOW);
  }
}

void loop(void)
{
  bool light_on = digitalRead(LIGHT_ON_SWITCH) == HIGH;
  bool light_off = digitalRead(LIGHT_OFF_SWITCH) == HIGH;
  bool open_door = digitalRead(OPEN_DOOR_SWITCH) == HIGH;
  bool close_door = digitalRead(CLOSE_DOOR_SWITCH) == HIGH;
  bool door_opened = digitalRead(DOOR_OPEN_LIMIT_SWITCH) == HIGH;
  bool door_closed = digitalRead(DOOR_CLOSE_LIMIT_SWITCH) == HIGH;

  print_to_lcd(light_on, light_off, open_door, door_opened, close_door, door_closed);
  change_light_state(light_on, light_off);
  handle_door(open_door, close_door, door_opened, door_closed);

  delay(1000);
}
