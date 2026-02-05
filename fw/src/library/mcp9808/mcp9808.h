#ifndef _PICOSENSOR_MCP9808_H
#define _PICOSENSOR_MCP9808_H

#include "hardware/i2c.h"
#include "pico/stdlib.h"

#define MCP9808_I2C_ADDR 0x18

float mcp9808_get_temperature();

#endif
