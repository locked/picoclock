#ifndef LP5817_H
#define LP5817_H

#include "hardware/i2c.h"
#include <stdint.h>

// From https://github.com/esp-cpp/espp/blob/69721957/components/lp5817/include/lp5817.hpp
#define LP5817_DEFAULT_ADDRESS 0x2D
#define LP5817_BROADCAST_ADDRESS 0x34

#define LP5817_CURRENT_MA_25_5 0
#define LP5817_CURRENT_MA_51 1

#define LP5817_CHIP_EN 0x00      // bit0 CHIP_EN
#define LP5817_DEV_CONFIG0 0x01  // bit0 MAX_CURRENT
#define LP5817_DEV_CONFIG1 0x02  // bits[2:0] OUTx_EN
#define LP5817_DEV_CONFIG2 0x03  // bits[2:0] OUTx_FADE_EN  bits[7:4] LED_FADE_TIME
#define LP5817_DEV_CONFIG3 0x04  // bits[6:4] OUTx_EXP_EN
// 0x05..0x0C reserved
#define LP5817_SHUTDOWN_CMD 0x0D
#define LP5817_RESET_CMD 0x0E
#define LP5817_UPDATE_CMD 0x0F
// 0x10..0x12 reserved
#define LP5817_FLAG_CLR 0x13  // bit0 POR_CLR  bit1 TSD_CLR
#define LP5817_OUT0_DC 0x14
#define LP5817_OUT1_DC 0x15
#define LP5817_OUT2_DC 0x16
// 0x17 reserved
#define LP5817_OUT0_MANUAL_PWM 0x18
#define LP5817_OUT1_MANUAL_PWM 0x19
#define LP5817_OUT2_MANUAL_PWM 0x1A
// 0x1B..0x3F reserved
#define LP5817_FLAG 0x40 // bit0 POR  bit1 TSD

#define LP5817_CHIP_EN_MASK 0x01               // CHIP_EN bit
#define LP5817_MAX_CURRENT_MASK 0x01           // DEV_CONFIG0 bit0
#define LP5817_DEV_CONFIG2_FADE_TIME_MASK 0xF0 // DEV_CONFIG2 bits[7:4]
#define LP5817_FLAG_CLR_POR_MASK 0x01          // bit0
#define LP5817_FLAG_CLR_TSD_MASK 0x02          // bit1

#define LP5817_FADE_TIME_0 0x00      ///< No fade (instant)
#define LP5817_FADE_TIME_50MS 0x01   ///< 50ms
#define LP5817_FADE_TIME_100MS 0x02  ///< 100ms
#define LP5817_FADE_TIME_150MS 0x03  ///< 150ms
#define LP5817_FADE_TIME_200MS 0x04  ///< 200ms
#define LP5817_FADE_TIME_250MS 0x05  ///< 250ms
#define LP5817_FADE_TIME_300MS 0x06  ///< 300ms
#define LP5817_FADE_TIME_350MS 0x07  ///< 350ms
#define LP5817_FADE_TIME_400MS 0x08  ///< 400ms
#define LP5817_FADE_TIME_450MS 0x09  ///< 450ms
#define LP5817_FADE_TIME_500MS 0x0A  ///< 500ms
#define LP5817_FADE_TIME_1S 0x0B     ///< 1 second
#define LP5817_FADE_TIME_2S 0x0C     ///< 2 seconds
#define LP5817_FADE_TIME_4S 0x0D     ///< 4 seconds
#define LP5817_FADE_TIME_6S 0x0E     ///< 6 seconds
#define LP5817_FADE_TIME_8S 0x0F     ///< 8 seconds


void lp5817_init(i2c_inst_t *i2c);
uint8_t lp5817_enable(bool on);
uint8_t lp5817_set_dot_current(uint8_t ch, uint8_t value);
uint8_t lp5817_set_pwm(uint8_t ch, uint8_t value);
uint8_t lp5817_set_output_enable_all();
uint8_t lp5817_set_output_enable(uint8_t ch);
uint8_t lp5817_update();
uint8_t lp5817_set_max_current_code();
uint8_t lp5817_turn_on(uint8_t c0, uint8_t c1, uint8_t c2);
uint8_t lp5817_turn_off();

#endif
