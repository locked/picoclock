#include "lp5817.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Global I2C instance
static i2c_inst_t *lp5817_i2c;

void lp5817_init(i2c_inst_t *i2c) {
	lp5817_i2c = i2c;
}

uint8_t lp5817_enable(bool on) {
	printf("[lp5817] Setting enable to %d...\r\n", on);
	uint8_t packet[2];
	packet[0] = LP5817_CHIP_EN;
	packet[1] = on ? 0x01 : 0x00;
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	printf("[lp5817] Enable set to %d\r\n", on);
	return result != 2;
}

// ch = 0 to 2
uint8_t lp5817_set_dot_current(uint8_t ch, uint8_t value) {
	printf("[lp5817] Setting dot-current (DC) of channel %d to %d\r\n", ch, value);
	uint8_t packet[2];
	if (ch == 0) {
		packet[0] = LP5817_OUT0_DC;
	} else if (ch == 1) {
		packet[0] = LP5817_OUT1_DC;
	} else if (ch == 2) {
		packet[0] = LP5817_OUT2_DC;
	}
	packet[1] = value;
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	return result != 2;
}

// ch = 0 to 2
uint8_t lp5817_set_pwm(uint8_t ch, uint8_t value) {
	uint8_t packet[2];
	if (ch == 0) {
		packet[0] = LP5817_OUT0_MANUAL_PWM;
	} else if (ch == 1) {
		packet[0] = LP5817_OUT1_MANUAL_PWM;
	} else if (ch == 2) {
		packet[0] = LP5817_OUT2_MANUAL_PWM;
	}
	packet[1] = value;
	printf("[lp5817] Setting dot-current (DC) of channel %d to %d packet:[%02d:%02d]\r\n", ch, value, packet[0], packet[1]);
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	return result != 2;
}

uint8_t lp5817_set_max_current_code() {
	printf("[lp5817] Set max current\r\n");
	uint8_t packet[2];
	packet[0] = LP5817_DEV_CONFIG0;
	packet[1] = 0x01;
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	return result != 2;
}

uint8_t lp5817_set_output_enable_all() {
	printf("[lp5817] Enable output ON\r\n");
	uint8_t packet[2];
	packet[0] = LP5817_DEV_CONFIG1;
	packet[1] = 0x07;
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	return result != 2;
}

// ch = 0 to 2
uint8_t lp5817_set_output_enable(uint8_t ch) {
	printf("[lp5817] Enable output channel %d\r\n", ch);
	uint8_t packet[2];
	packet[0] = LP5817_DEV_CONFIG1;
	packet[1] = 1 << ch;
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	return result != 2;
}

uint8_t lp5817_update() {
	printf("[lp5817] Update\r\n");
	uint8_t packet[2];
	packet[0] = LP5817_UPDATE_CMD;
	packet[1] = 0x55;
	int result = i2c_write_blocking(lp5817_i2c, LP5817_DEFAULT_ADDRESS, packet, 2, false);
	return result != 2;
}

// Helpers
uint8_t lp5817_turn_on(uint8_t c0, uint8_t c1, uint8_t c2) {
	lp5817_set_pwm(0, c0);
	lp5817_set_pwm(1, c1);
	lp5817_set_pwm(2, c2);
}
uint8_t lp5817_turn_off() {
	lp5817_set_pwm(0, 0);
	lp5817_set_pwm(1, 0);
	lp5817_set_pwm(2, 0);
}
