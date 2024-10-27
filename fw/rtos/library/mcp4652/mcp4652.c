#include "mcp4652.h"

#include "pico/stdlib.h"

// Global
i2c_inst_t *mcp4652_i2c;

void mcp4652_set_i2c(i2c_inst_t *i2c) {
	mcp4652_i2c = i2c;
}

void mcp4652_set_wiper(uint16_t value) {
	// Work (Wiper0 == Left)
	uint8_t temp[2];
	temp[0] = ((value >> 8 & 0x01) | MCP4652_CMD_WRITE);
	temp[1] = (value & 0xFF);
    int result = i2c_write_blocking(mcp4652_i2c, MCP4652_DEFAULT_ADDRESS, temp, 2, false);
    if (result != 2) {
        printf("mcp4652: failed to write for wiper0\n");
        return;
    }

	// Wiper1 (== Right)
	temp[0] = ((value >> 8 & 0x01) | MCP4652_CMD_WRITE | MCP4652_CMD_WIPER1);
	temp[1] = (value & 0xFF);
    result = i2c_write_blocking(mcp4652_i2c, MCP4652_DEFAULT_ADDRESS, temp, 2, false);
    if (result != 2) {
        printf("mcp4652: failed to write for wiper1\n");
        return;
    }
}
