#ifndef SENSIRION_H
#define SENSIRION_H

#include "hardware/i2c.h"

typedef enum { SENSIRION_BYTE = 1, SHORT = 2, INTEGER = 4, LONG_INTEGER = 8 } INT_TYPE;

#define NO_ERROR 0
#define CRC_ERROR 1
#define BYTE_NUM_ERROR 4
#define I2C_READ_FAILED -1
#define CRC8_POLYNOMIAL 0x31
#define CRC8_INIT 0xFF
#define CRC8_LEN 1
#define SENSIRION_WORD_SIZE 2

#define SENSIRION_I2C_TIMEOUT 200000

#define SCD43_ADDR 0x62
#define STCC4_ADDR 0x64

#define STCC4_PRODUCT_ID 151060874


void sensirion_init(i2c_inst_t *i2c);
bool sensirion_scd43_init();
bool sensirion_stcc4_init();
int16_t sensirion_scd43_read();
int16_t sensirion_stcc4_read();

#endif
