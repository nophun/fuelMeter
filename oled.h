#pragma once

#include "display.h"

#define OLED_CMD 0x00
#define OLED_DATA 0x40
//#define OLED_ROTATE
#define COLUMNS   128
#define ROWS      4
#define ROWPIXELS 32
/*
 * Class OLED
 */
class OLED {
private:
  uint32_t display_buf[COLUMNS*ROWS/4];
  uint8_t addr;
  uint32_t update_time {0};
  uint32_t interval;
  uint8_t new_content {0xFF};
  void command_ssd1306(uint8_t addr, uint8_t cmd);
  void command_ssd1306(uint8_t addr, uint8_t cmd, uint8_t conf);
  void command_ssd1306(uint8_t addr, uint8_t cmd, uint8_t conf, uint8_t param);
  void command_ssd1306(uint8_t addr, uint8_t cmd, uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f);
  void init_ssd1306(uint8_t addr);
  void init_ssd1306_32(uint8_t addr);
  void init_ssd1306_64_toimii(uint8_t addr);
  void write_data_ssd1306(uint8_t addr, uint8_t* data, uint32_t len);
  void update_ssd1306(uint8_t addr, uint32_t* data);

public:
  OLED(uint8_t addr, uint32_t interval);
  void start(void);
  void refresh(void);
  void set_value(const char *buf);
  void set_value(int32_t value, uint8_t decimals);
  void set_header(const char *buf, Alignment alignment = Alignment::Left);
  void set_unit(enum UNITS unit);

};