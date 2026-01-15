#include "lp5817.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Global I2C instance
static i2c_inst_t *lp5817_i2c;

void lp5817_init(i2c_inst_t *i2c) {
	lp5817_i2c = i2c;
}

uint8_t lp5817_enable(bool on) {
	printf("Setting enable to {}", on);
	uint8_t packet[2];
	packet[0] = LP5817_CHIP_EN;
	packet[1] = 0x01; //on ? 0x01 : 0x00;
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	return result != 2;
}

// ch = 0 to 2
uint8_t lp5817_set_dot_current(uint8_t ch, uint8_t value) {
	printf("Setting dot-current (DC) of channel %d to %d", ch, value);
	uint8_t packet[2];
	packet[0] = LP5817_OUT0_DC + ch;
	packet[1] = value;
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	return result != 2;
}
