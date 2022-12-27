#include <Arduino.h>
#include "rotaryEncoder.h"

static const unsigned char fullStepsTable[7][4] = {
    // 00           01              10              11
    {R_START,       FS_R_CW_BEGIN,  FS_R_CCW_BEGIN, R_START},           // R_START
    {FS_R_CW_NEXT,  R_START,        FS_R_CW_FINAL,  R_START | DIR_CW},  // R_CW_FINAL
    {FS_R_CW_NEXT,  FS_R_CW_BEGIN,  R_START,        R_START},           // R_CW_BEGIN
    {FS_R_CW_NEXT,  FS_R_CW_BEGIN,  FS_R_CW_FINAL,  R_START},           // R_CW_NEXT
    {FS_R_CCW_NEXT, R_START,        FS_R_CCW_BEGIN, R_START},           // R_CCW_BEGIN
    {FS_R_CCW_NEXT, FS_R_CCW_FINAL, R_START,        R_START | DIR_CCW}, // R_CCW_FINAL
    {FS_R_CCW_NEXT, FS_R_CCW_FINAL, FS_R_CCW_BEGIN, R_START}            // R_CCW_NEXT
};

static const unsigned char halfStepsTable[6][4] = {
    // 00                  01              10                       11
    {HS_R_START_M,           HS_R_CW_BEGIN,     HS_R_CCW_BEGIN,     R_START},           // R_START (00)
    {HS_R_START_M | DIR_CCW, R_START,           HS_R_CCW_BEGIN,     R_START},           // R_CCW_BEGIN
    {HS_R_START_M | DIR_CW,  HS_R_CW_BEGIN,     R_START,            R_START},           // R_CW_BEGIN
    {HS_R_START_M,           HS_R_CCW_BEGIN_M,  HS_R_CW_BEGIN_M,    R_START},           // R_START_M (11)
    {HS_R_START_M,           HS_R_START_M,      HS_R_CW_BEGIN_M,    R_START | DIR_CW},  // R_CW_BEGIN_M 
    {HS_R_START_M,           HS_R_CCW_BEGIN_M,  HS_R_START_M,       R_START | DIR_CCW}  // R_CCW_BEGIN_M
};

RotaryEncoder::RotaryEncoder(uint8_t outputAPin, uint8_t outputBPin, RotaryMode mode) : m_dat_pin(outputAPin), m_clk_pin(outputBPin), m_mode(mode) {
    m_input_last_state = 0;
    m_cw_button = 0;
    m_ccw_button = 0;
}

void RotaryEncoder::init() {
    pinMode(m_dat_pin, INPUT_PULLUP);
    pinMode(m_clk_pin, INPUT_PULLUP);
}

uint8_t RotaryEncoder::read() {
    uint8_t direction;

    if (m_mode == RotaryMode::HALF_STEP) {
        m_input_last_state = halfStepsTable[m_input_last_state & STEP_MASK][(digitalRead(m_dat_pin) << 1) | digitalRead(m_clk_pin)];
    } else {
        m_input_last_state = fullStepsTable[m_input_last_state & STEP_MASK][(digitalRead(m_dat_pin) << 1) | digitalRead(m_clk_pin)];
    }

    direction = (m_input_last_state & DIR_MASK);

    return direction;
}

void RotaryEncoder::set_joystick_id(uint8_t id) {
    m_cw_button = id;
    m_ccw_button = id + 1;
}