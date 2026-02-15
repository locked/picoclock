#include "mcp45XX.h"
#include "pico/stdlib.h"
#include <stdio.h>

// Global I2C instance
static i2c_inst_t *mcp4551_i2c;

/**
 * @brief Initialize the I2C instance for the MCP4551
 */
void mcp4551_init(i2c_inst_t *i2c) {
    mcp4551_i2c = i2c;
}

/**
 * @brief Sets the wiper value (0-255)
 * @param value 8-bit wiper position
 */
void mcp4551_set_wiper(uint8_t value) {
	uint8_t packet[2];

	printf("[mcp4551] [%02x]: set wiper to:[%d]\n", MCP4551_DEFAULT_ADDRESS, value);

	// Byte 0: [Memory Address (4 bits) | Command (2 bits) | Data MSB (2 bits)]
	// Address 0x00 for Wiper 0, Command 0x00 for Write.
	// Since MCP4551 is 8-bit, Data MSBs are always 0.
	packet[0] = (MCP4551_RA_WIPER0 << 4) | MCP4551_CMD_WRITE;
	packet[1] = value;

	int result = i2c_write_blocking(mcp4551_i2c, MCP4551_DEFAULT_ADDRESS, packet, 2, false);

	if (result != 2) {
		printf("[mcp4551] [%02x]: failed to write wiper\n", MCP4551_DEFAULT_ADDRESS);
	}
}

/**
 * @brief Reads the current wiper value from the volatile register
 * @return 8-bit wiper position
 */
uint8_t mcp4551_read_wiper(void) {
    uint8_t reg = (MCP4551_RA_WIPER0 << 4) | MCP4551_CMD_READ;
    uint8_t buffer[2] = {0};

    // Send read command
    i2c_write_blocking(mcp4551_i2c, MCP4551_DEFAULT_ADDRESS, &reg, 1, true);
    // Read 2 bytes (Command/Status + Data)
    i2c_read_blocking(mcp4551_i2c, MCP4551_DEFAULT_ADDRESS, buffer, 2, false);

    return buffer[1]; 
}
