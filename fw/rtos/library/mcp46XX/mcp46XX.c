#include "mcp46XX.h"

#include "pico/stdlib.h"

// Global
i2c_inst_t *mcp46XX_i2c;

void mcp46XX_set_i2c(i2c_inst_t *i2c) {
	mcp46XX_i2c = i2c;
}

void mcp46XX_set_wiper(uint8_t address_i2c, uint16_t value) {
	// Work (Wiper0 == Left)
	uint8_t temp[2];
	temp[0] = ((value >> 8 & 0x01) | MCP4652_CMD_WRITE);
	temp[1] = (value & 0xFF);
    int result = i2c_write_blocking(mcp46XX_i2c, address_i2c, temp, 2, false);
    if (result != 2) {
        printf("mcp46XX[%02x]: failed to write for wiper0\n", address_i2c);
        return;
    }

	// Wiper1 (== Right)
	temp[0] = ((value >> 8 & 0x01) | MCP4652_CMD_WRITE | MCP4652_CMD_WIPER1);
	temp[1] = (value & 0xFF);
    result = i2c_write_blocking(mcp46XX_i2c, address_i2c, temp, 2, false);
    if (result != 2) {
        printf("mcp46XX[%02x]: failed to write for wiper1\n", address_i2c);
        return;
    }
}

void mcp4652_set_wiper(uint16_t value) {
	mcp46XX_set_wiper(MCP4652_DEFAULT_ADDRESS, value);
}

void mcp4651_set_wiper(uint16_t value) {
	mcp46XX_set_wiper(MCP4651_DEFAULT_ADDRESS, value);
}
