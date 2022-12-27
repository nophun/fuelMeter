#pragma once

#define M5PICO

#if defined(SEEDUINO_XIAO)
#define BUTTON          6
#define ROT1_CLK        2
#define ROT1_DAT        3
#elif defined(M5PICO)
#define I2C_SDA         22
#define I2C_SCL         21
#define BUTTON          26
#define ROT1_CLK        18
#define ROT1_DAT        36
#define LED_PIN         32
#define NUM_PIXELS      4
enum LEDs {
  LEDAbs,
  LEDTc,
  LEDRightFlag,
  LEDLeftFlag,
};
#endif
#define OLED_ADDRESS    0x3C

static constexpr uint8_t cMIN_LAPTIME {80U};
static constexpr uint8_t cMAX_LAPTIME {150U};
static constexpr uint8_t cMIN_RACE_REMAINING {0U};
static constexpr uint8_t cMAX_RACE_REMAINING {120U};
static constexpr uint8_t cMIN_FUEL_CUNSUMPTION {20U};
static constexpr uint8_t cMAX_FUEL_CUNSUMPTION {45U};
static constexpr float cWarmUpLapMultiplier {1.2F};

static constexpr uint8_t cSEC_IN_MIN {60U};

enum class displayMode {
  None                  = 0,
  SHFuelTime            = 1,
  SHFuelLaps            = 2,
  SHFuelConsumption     = 3,
  InputWarmup           = 4,
  InputRaceLength       = 5,
  InputLaptime          = 6,
  InputFuelConsumption  = 7,
  CalcFuelNeeded        = 8,
  CalcLaps              = 9,

  LastMode
};

enum class buttonPressMode {
  Short,
  Long,
  None
};
