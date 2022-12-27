#pragma once

#include "stdint.h"

#define DIR_MASK        0x30
#define STEP_MASK       0x0F

/* Direction flags */
static constexpr uint8_t R_START    = 0x00;
static constexpr uint8_t DIR_NONE   = 0x00;
static constexpr uint8_t DIR_CW     = 0x10;
static constexpr uint8_t DIR_CCW    = 0x20;

/* Full step step flags */
#define FS_R_CW_FINAL   0x01
#define FS_R_CW_BEGIN   0x02
#define FS_R_CW_NEXT    0x03
#define FS_R_CCW_BEGIN  0x04
#define FS_R_CCW_FINAL  0x05
#define FS_R_CCW_NEXT   0x06

/* Half step step flags */
#define HS_R_CCW_BEGIN   0x1
#define HS_R_CW_BEGIN    0x2
#define HS_R_START_M     0x3
#define HS_R_CW_BEGIN_M  0x4
#define HS_R_CCW_BEGIN_M 0x5

enum class RotaryMode {
    HALF_STEP = 0,
    FULL_STEP = 1
};

class RotaryEncoder {
public:
    RotaryEncoder(uint8_t outputAPin, uint8_t outputBPin, RotaryMode mode);
    void init(void);
    uint8_t read(void);
    void set_joystick_id(uint8_t id);
private:
    uint8_t m_dat_pin;
    uint8_t m_clk_pin;
    uint8_t m_cw_button;
    uint8_t m_ccw_button;
    uint8_t m_input_last_state;
    RotaryMode m_mode;

    uint32_t m_last_changed_time;
    bool m_change_expired;
};