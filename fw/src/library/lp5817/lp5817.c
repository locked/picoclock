#include "lp5817.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Global I2C instance
static i2c_inst_t *lp5817_i2c;

void lp5817_init(i2c_inst_t *i2c) {
    lp5817_i2c = i2c;
}
