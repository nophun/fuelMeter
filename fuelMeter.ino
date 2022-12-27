#include <Wire.h>
#include "oled.h"
#include "display.h"
#include "rotaryEncoder.h"
#include "FuelMeter.h"

#define BUFLEN 128
char buf[BUFLEN] = "";
uint8_t bufpos = 0;
volatile displayMode mode = displayMode::None;
uint32_t timestamp;
uint32_t button_time;
bool long_press;

/* Calculator */
static constexpr uint8_t cNUM_OF_RACE_LENGTH_OPTIONS = 8;
uint8_t race_length_options[cNUM_OF_RACE_LENGTH_OPTIONS] = {15, 20, 25, 30, 45, 60, 90, 120};
uint8_t warmup;
uint8_t race_length;
uint8_t race_length_index;
bool custom_race_length;
uint8_t laptime;
uint8_t fuel_consumption;
char display_str[6] = "";
volatile bool oled_updated;
volatile bool fuel_updated;

OLED oled = OLED(OLED_ADDRESS, 5*60);
RotaryEncoder encoder_a(ROT1_CLK, ROT1_DAT, RotaryMode::HALF_STEP);

void button(void) {
  buttonPressMode press_mode = buttonPressMode::None;
  int modeint;
  if (digitalRead(BUTTON) == 0) {
    button_time = millis();
  } else {
    if (millis() - button_time > 1000) {
      press_mode = buttonPressMode::Long;
    } else {
      press_mode = buttonPressMode::Short;
    }
  }

  if (press_mode == buttonPressMode::Short) {
    if (mode == displayMode::None) {
      mode = displayMode::CalcWarmup;
    } else {
      modeint = static_cast<int>(mode) + 1;
      if (modeint == static_cast<int>(displayMode::LastMode)) {
        mode = displayMode::CalcWarmup;
      } else {
        mode = static_cast<displayMode>(modeint);
      }
    }
    oled_updated = true;
    switch(mode) {
      case displayMode::FuelTime:
        oled.set_header("FUEL USED THIS LAP");
        oled.set_unit(UNIT_none);
        break;
      case displayMode::FuelUsedLap:
        oled.set_header("LITERS PER LAP");
        oled.set_unit(UNIT_none);
        break;
      case displayMode::FuelConsumption:
        oled.set_header("FUEL REMAINING LAPS");
        oled.set_unit(UNIT_none);
        break;
      case displayMode::FuelLaps:
        oled.set_header("FUEL REMAINING TIME");
        oled.set_unit(UNIT_none);
        break;
      case displayMode::CalcWarmup:
        oled.set_header("FORMATION LAP?");
        oled.set_unit(UNIT_none);
        break;
      case displayMode::CalcRaceLength:
        if (custom_race_length) {
          oled.set_header("RACE REMAINING?");
        } else {
          oled.set_header("RACE LENGTH?");
        }
        oled.set_unit(UNIT_min);
        break;
      case displayMode::CalcLaptime:
        oled.set_header("LAPTIME?");
        oled.set_unit(UNIT_none);
        break;
      case displayMode::CalcFuelConsumption:
        oled.set_header("FUEL CONSUMPTION?");
        oled.set_unit(UNIT_lL);
        break;
      case displayMode::CalcFuelNeeded:
        oled.set_header("-> FUEL NEEDED", Alignment::Right);
        oled.set_unit(UNIT_l);
        fuel_updated = true;
        break;
      case displayMode::CalcLaps:
        oled.set_header("-> LAPS", Alignment::Right);
        oled.set_unit(UNIT_none);
        fuel_updated = true;
        break;
      default:
        break;
    }
  } else if (press_mode == buttonPressMode::Long) {
    switch(mode) {
      case displayMode::CalcRaceLength:
        oled_updated = true;
        custom_race_length = !custom_race_length;
        if (custom_race_length) {
          oled.set_header("RACE REMAINING");
        } else {
          oled.set_header("RACE LENGTH");
        }
        break;
      default:
        break;
    }
  }
}

void encoder(void) {
  encoder_a.read();
}

void setup() {
#if defined(M5PICO)
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
#endif
  warmup = static_cast<uint8_t>(false);
  custom_race_length = false;
  race_length_index = 1U;
  race_length = race_length_options[race_length_index];
  laptime = 100U;
  fuel_consumption = 30U;
  oled_updated = true;
  fuel_updated = true;
  Serial.begin(115200U);
#if defined(M5PICO)
  Wire.begin(I2C_SDA, I2C_SCL);
#else
  Wire.begin();
#endif
  oled.start();

  encoder_a.init();

  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), button, CHANGE);

  oled.set_value("123456");
  oled.refresh();
}
// abs(20-39) - abs(15-39)
// 19-24
// -5
// abs(30-39) - abs(20-39)
// 9-19
// -10
// abs(40-39) - abs(30-39)
// 1-9
// -8
// abs(45-39) - abs(40-39)
// 6-1
// 5
uint8_t find_race_length_index(uint8_t race_length) {
  uint8_t closest_option = 0U;
  int8_t diff;
  for (int i = 1; i < cNUM_OF_RACE_LENGTH_OPTIONS; ++i) {
    diff = abs(race_length_options[i] - race_length) - abs(race_length_options[closest_option] - race_length);
    Serial.println(diff);
    if (diff > 0) {
      return closest_option;
    } else {
      closest_option = i;
    }
  }
  return closest_option;
}

float calculate_fuel_needed(uint8_t warmup, uint8_t race_length, uint8_t laptime, uint8_t fuel_consumption) {
  float fuel_needed = {};
  int laps = 0;
  laps = ceil(calculate_laps(warmup, race_length, laptime));
  Serial.print("Laps ");
  Serial.print(laps);
  fuel_needed = laps * fuel_consumption;
  Serial.print(", Fuel needed ");
  Serial.println(fuel_needed / 10.0F);
  return fuel_needed;
}

float calculate_laps(uint8_t warmup, uint8_t race_length, uint8_t laptime) {
  float laps = {};
  laps = race_length * 60 / static_cast<float>(laptime);
  Serial.print("Laps ");
  Serial.print(laps);
  if (warmup != 0U) {
    laps += cWarmUpLapMultiplier;
    Serial.print(" + 1");
  }
  return laps;
}

void read_until() {
  char tmp;
  bufpos = 0;
  tmp = Serial.read();
  while (tmp != ';') {
    Serial.print(tmp);
    buf[bufpos++] = tmp;
    tmp = Serial.read();
    if (bufpos == BUFLEN) {
      bufpos = BUFLEN-1;
      break;
    }
  }
  buf[bufpos] = '\0';
}

void adjust_parameter(uint8_t dir, uint8_t *param, uint8_t min, uint8_t max) {
  if (dir == DIR_CCW) {
    if (*param > min) {
      (*param)--;
    }
  } else if (dir == DIR_CW) {
    if (*param < max) {
      (*param)++;
    }
  }
}

void loop() {
  // char tmp;
  // if (Serial.available() > 0) {
  //   timestamp = millis();
  //   tmp = Serial.read();
  //   read_until();
  //   switch (tmp) {
  //     case 'T':
  //       if (mode == displayMode::FuelTime) {
  //         // oled.set_header("FUEL REMAINING TIME");
  //         oled.set_value(buf);
  //       }
  //       break;
  //     case 'U':
  //       if (mode == displayMode::FuelUsedLap) {
  //         // oled.set_header("FUEL USED THIS LAP");
  //         oled.set_value(buf);
  //       }
  //       break;
  //     case 'C':
  //       if (mode == displayMode::FuelConsumption) {
  //         // oled.set_header("LITERS PER LAP");
  //         oled.set_value(buf);
  //       }
  //       break;
  //     case 'L':
  //       if (mode == displayMode::FuelLaps) {
  //         // oled.set_header("FUEL REMAINING LAPS");
  //         oled.set_value(buf);
  //       }
  //       break;
  //     default:
  //       break;
  //   }
  // }

  // if (millis() - timestamp > 5000) {
  //   oled.set_header("SIMHUB OFFLINE", Alignment::Center);
  //   oled.set_value(" ---- ");
  // }

  uint8_t dir = encoder_a.read();

  if (mode == displayMode::CalcFuelNeeded && fuel_updated) {
    int fuel_needed = static_cast<int>(calculate_fuel_needed(warmup, race_length, laptime, fuel_consumption));
    oled.set_value(fuel_needed, 1);
    oled_updated = true;
    fuel_updated = false;
  } else if (mode == displayMode::CalcLaps && fuel_updated) {
    float laps = calculate_laps(warmup, race_length, laptime)*100;
    oled.set_value(laps, 2);
    oled_updated = true;
    fuel_updated = false;
  } else if (dir != DIR_NONE) {
    oled_updated = true;
    fuel_updated = true;
  }

  if (oled_updated) {
    oled_updated = false;
    switch (mode) {
      case displayMode::CalcWarmup:
        adjust_parameter(dir, &warmup, false, true);
        sprintf(display_str, "%s", warmup!=0U?"YES":"NO");
        oled.set_value(display_str);
        break;
      case displayMode::CalcLaptime:
        adjust_parameter(dir, &laptime, cMIN_LAPTIME, cMAX_LAPTIME);
        sprintf(display_str, "%d:%02d", laptime / cSEC_IN_MIN, laptime % cSEC_IN_MIN);
        oled.set_value(display_str);
        break;
      case displayMode::CalcRaceLength:
        if (custom_race_length) {
          adjust_parameter(dir, &race_length, cMIN_RACE_REMAINING, cMAX_RACE_REMAINING);
          race_length_index = find_race_length_index(race_length);
        } else {
          adjust_parameter(dir, &race_length_index, 0, cNUM_OF_RACE_LENGTH_OPTIONS - 1);
          race_length = race_length_options[race_length_index];
        }
        oled.set_value(race_length, 0);
        break;
      case displayMode::CalcFuelConsumption:
        adjust_parameter(dir, &fuel_consumption, cMIN_FUEL_CUNSUMPTION, cMAX_FUEL_CUNSUMPTION);
        oled.set_value(fuel_consumption, 1);
        break;
      default:
        break;
    }

    oled.refresh();
  } else {
    delay(5);
  }
}
