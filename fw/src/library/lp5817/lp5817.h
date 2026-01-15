#ifndef LP5817_H
#define LP5817_H

#include "hardware/i2c.h"
#include <stdint.h>

// From https://github.com/esp-cpp/espp/blob/69721957/components/lp5817/include/lp5817.hpp

#define LP5817_CHIP_EN = 0x00,     // bit0 CHIP_EN
#define LP5817_DEV_CONFIG0 = 0x01, // bit0 MAX_CURRENT
#define LP5817_DEV_CONFIG1 = 0x02, // bits[2:0] OUTx_EN
#define LP5817_DEV_CONFIG2 = 0x03, // bits[2:0] OUTx_FADE_EN, bits[7:4] LED_FADE_TIME
#define LP5817_DEV_CONFIG3 = 0x04, // bits[6:4] OUTx_EXP_EN
// 0x05..0x0C reserved
#define LP5817_SHUTDOWN_CMD = 0x0D,
#define LP5817_RESET_CMD = 0x0E,
#define LP5817_UPDATE_CMD = 0x0F,
// 0x10..0x12 reserved
#define LP5817_FLAG_CLR = 0x13, // bit0 POR_CLR, bit1 TSD_CLR
#define LP5817_OUT0_DC = 0x14,
#define LP5817_OUT1_DC = 0x15,
#define LP5817_OUT2_DC = 0x16,
// 0x17 reserved
#define LP5817_OUT0_MANUAL_PWM = 0x18,
#define LP5817_OUT1_MANUAL_PWM = 0x19,
#define LP5817_OUT2_MANUAL_PWM = 0x1A,
// 0x1B..0x3F reserved
#define LP5817_FLAG = 0x40 // bit0 POR, bit1 TSD

void lp5817_init(i2c_inst_t *i2c);

#endif
