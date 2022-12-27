#include <Wire.h>
#include "oled.h"
#include "display.h"
#include "rotaryEncoder.h"
#include "FuelMeter.h"
#if defined(M5PICO)
#include "BluetoothSerial.h"
#include "Adafruit_NeoPixel.h"
#endif

static constexpr size_t BUFLEN {128U};
static constexpr displayMode cFirstDisplayMode = displayMode::SHFuelTime;
char buf[BUFLEN] {""};
uint8_t bufpos {0};
volatile displayMode mode {displayMode::None};
volatile uint8_t dir {DIR_NONE};
uint32_t timestamp {0};
uint32_t button_time {0};
#if defined(M5PICO)
BluetoothSerial SerialBT;
Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

/* Calculator */
static constexpr uint8_t cNUM_OF_RACE_LENGTH_OPTIONS {8U};
uint8_t race_length_options[cNUM_OF_RACE_LENGTH_OPTIONS] {15U, 20U, 25U, 30U, 45U, 60U, 90U, 120U};
uint8_t warmup {static_cast<uint8_t>(false)};
uint8_t race_length_index {1U};
uint8_t race_length {race_length_options[race_length_index]};
bool custom_race_length {false};
uint8_t laptime {100U};
uint8_t fuel_consumption {30U};
char display_str[6] {""};
volatile bool oled_updated {true};
volatile bool fuel_updated {true};

/*  */
bool abs_active {false};
bool tc_active {false};
bool yellow_flag_active {false};
bool blue_flag_active {false};

OLED oled = OLED(OLED_ADDRESS, 5*60);
RotaryEncoder encoder_a(ROT1_CLK, ROT1_DAT, RotaryMode::FULL_STEP);

void update_header(void) {
  switch(mode) {
    case displayMode::SHFuelTime:
      oled.set_header("FUEL REMAINING TIME");
      oled.set_value(" ---- ");
      oled.set_unit(UNIT_none);
      break;
    case displayMode::SHFuelLaps:
      oled.set_header("FUEL REMAINING LAPS");
      oled.set_value(" ---- ");
      oled.set_unit(UNIT_none);
      break;
    case displayMode::SHFuelConsumption:
      oled.set_header("FUEL CONSUMPTION");
      oled.set_value(" ---- ");
      oled.set_unit(UNIT_none);
      break;
    case displayMode::InputWarmup:
      oled.set_header("FORMATION LAP?");
      oled.set_unit(UNIT_none);
      break;
    case displayMode::InputRaceLength:
      if (custom_race_length) {
        oled.set_header("RACE REMAINING?");
      } else {
        oled.set_header("RACE LENGTH?");
      }
      oled.set_unit(UNIT_min);
      break;
    case displayMode::InputLaptime:
      oled.set_header("LAPTIME?");
      oled.set_unit(UNIT_none);
      break;
    case displayMode::InputFuelConsumption:
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
  oled_updated = true;
}

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
      mode = cFirstDisplayMode;
    } else {
      modeint = static_cast<int>(mode) + 1;
      if (modeint == static_cast<int>(displayMode::LastMode)) {
        mode = cFirstDisplayMode;
      } else {
        mode = static_cast<displayMode>(modeint);
      }
    }
    update_header();
  } else if (press_mode == buttonPressMode::Long) {
    switch(mode) {
      case displayMode::InputRaceLength:
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
  dir = encoder_a.read();
}

void setup() {
  Serial.begin(115200U);
#if defined(M5PICO)
  pinMode(I2C_SDA, INPUT_PULLUP);
  pinMode(I2C_SCL, INPUT_PULLUP);
  SerialBT.begin("ESP32");
#endif

#if defined(M5PICO)
  Wire.begin(I2C_SDA, I2C_SCL);
#else
  Wire.begin();
#endif
  oled.start();

  encoder_a.init();
  attachInterrupt(digitalPinToInterrupt(ROT1_CLK), encoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROT1_DAT), encoder, CHANGE);

  pinMode(BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON), button, CHANGE);

#if defined(M5PICO)
  pinMode(LED_PIN, OUTPUT);
  pixels.begin();
#endif

  update_header();
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
  Serial.println();
  return laps;
}

bool read_until(char *buffer, size_t maxbuflen) {
  char tmp;
  bufpos = 0;
  tmp = SerialBT.read();
  while (tmp != ';') {
    if (tmp == 0xFF) {
      return false;
    }
    buffer[bufpos++] = tmp;
    tmp = SerialBT.read();
    if (bufpos == maxbuflen) {
      bufpos = maxbuflen-1;
      break;
    }
  }
  buffer[bufpos] = '\0';
  return true;
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

void get_value_from_SH() {
  char cmd;
  if (SerialBT.available() > 0) {
    timestamp = millis();
    cmd = SerialBT.read();
    if (read_until(buf, BUFLEN)) {
      switch (cmd) {
        case 'T':
          if (mode == displayMode::SHFuelTime) {
            oled_updated = true;
            oled.set_value(buf);
            Serial.println(buf);
          }
          break;
        case 'L':
          if (mode == displayMode::SHFuelLaps) {
            oled_updated = true;
            oled.set_value(buf);
            Serial.println(buf);
          }
          break;
        case 'C':
          if (mode == displayMode::SHFuelConsumption) {
            oled_updated = true;
            oled.set_value(buf);
            Serial.println(buf);
          }
          break;
        case 'A':
          if (buf[0] == 'A') {
            abs_active = (buf[1] == '1');
          } else if (buf[0] == 'T') {
            tc_active = (buf[1] == '1');
          }
          break;
        case 'F':
          if (buf[0] == 'Y') {
            yellow_flag_active = (buf[1] == '1');
          } else if (buf[0] == 'B') {
            blue_flag_active = (buf[1] == '1');
          }
          break;
        default:
          break;
      }
    }
  }
}

void update_leds(void) {
  static const uint32_t colorNONE = pixels.Color(0, 0, 0);
  static const uint32_t colorRED = pixels.Color(255, 0, 0);
  static const uint32_t colorBLUE = pixels.Color(0, 0, 255);
  static const uint32_t colorYELLOW = pixels.Color(255, 255, 0);

  if (abs_active) {
    pixels.setPixelColor(LEDAbs, colorRED);
  } else {
    pixels.setPixelColor(LEDAbs, colorNONE);
  }

  if (tc_active) {
    pixels.setPixelColor(LEDTc, colorRED);
  } else {
    pixels.setPixelColor(LEDTc, colorNONE);
  }

  pixels.setPixelColor(LEDRightFlag, colorNONE);
  pixels.setPixelColor(LEDLeftFlag, colorNONE);
  if (yellow_flag_active) {
    pixels.setPixelColor(LEDRightFlag, colorYELLOW);
    pixels.setPixelColor(LEDLeftFlag, colorYELLOW);
  }
  if (blue_flag_active) {
    pixels.setPixelColor(LEDRightFlag, colorBLUE);
    pixels.setPixelColor(LEDLeftFlag, colorBLUE);
  }
}

void loop() {
  uint8_t current_dir = dir;
  if (current_dir != DIR_NONE) {
    oled_updated = true;
    fuel_updated = true;
  }

  get_value_from_SH();
  switch (mode) {
    case displayMode::SHFuelTime:
    case displayMode::SHFuelLaps:
    case displayMode::SHFuelConsumption:
      SerialBT.flush();
      break;
    case displayMode::InputWarmup:
      if (oled_updated) {
        adjust_parameter(current_dir, &warmup, false, true);
        sprintf(display_str, "%s", warmup!=0U?"YES":"NO");
        oled.set_value(display_str);
      }
      break;
    case displayMode::InputRaceLength:
      if (oled_updated) {
        adjust_parameter(current_dir, &laptime, cMIN_LAPTIME, cMAX_LAPTIME);
        sprintf(display_str, "%d:%02d", laptime / cSEC_IN_MIN, laptime % cSEC_IN_MIN);
        oled.set_value(display_str);
      }
      break;
    case displayMode::InputLaptime:
      if (oled_updated) {
        if (custom_race_length) {
          adjust_parameter(current_dir, &race_length, cMIN_RACE_REMAINING, cMAX_RACE_REMAINING);
          race_length_index = find_race_length_index(race_length);
        } else {
          adjust_parameter(current_dir, &race_length_index, 0, cNUM_OF_RACE_LENGTH_OPTIONS - 1);
          race_length = race_length_options[race_length_index];
        }
        oled.set_value(race_length, 0);
      }
      break;
    case displayMode::InputFuelConsumption:
      if (oled_updated) {
        adjust_parameter(current_dir, &fuel_consumption, cMIN_FUEL_CUNSUMPTION, cMAX_FUEL_CUNSUMPTION);
        oled.set_value(fuel_consumption, 1);
      }
      break;
    case displayMode::CalcFuelNeeded:
      if (fuel_updated) {
        int fuel_needed = static_cast<int>(calculate_fuel_needed(warmup, race_length, laptime, fuel_consumption));
        oled.set_value(fuel_needed, 1);
        oled_updated = true;
        fuel_updated = false;
      }
      break;
    case displayMode::CalcLaps:
      if (fuel_updated) {
        float laps = calculate_laps(warmup, race_length, laptime)*100;
        oled.set_value(laps, 2);
        oled_updated = true;
        fuel_updated = false;
      }
      break;
    default:
      break;
  }

  if (oled_updated) {
    dir = DIR_NONE;
    oled.refresh();
    oled_updated = false;
  } else {
    delay(1);
  }

  update_leds();

  pixels.show();
}
