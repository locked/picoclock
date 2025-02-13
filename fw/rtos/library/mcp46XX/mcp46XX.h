#ifndef MCP46XX_H
#define MCP46XX_H

#include "hardware/i2c.h"
#include <stdint.h>
#include <stdio.h>

// I2C Address of device
#define MCP4652_DEFAULT_ADDRESS	0x2C	// ‘0101 1’b + A1:A0,  A0:A1 are connected to GND
#define MCP4651_DEFAULT_ADDRESS	0x28	// ‘0101’b + A2:A1:A0  A0:A1:A2 are connected to GND

// Command definitions (sent to WIPER register)
#define MCP4652_CMD_WRITE	0x00 // (xxxx00xx)
#define MCP4652_CMD_INC		0x04 // (xxxx01xx)
#define MCP4652_CMD_DEC		0x08 // (xxxx10xx)
#define MCP4652_CMD_READ	0x0C // (xxxx11xx)

#define MCP4652_CMD_WIPER0	0x00 // (0000xxxx)
#define MCP4652_CMD_WIPER1	0x10 // (0001xxxx)

// Control bit definitions (sent to TCON register)
#define MCP4652_TCON_GC_EN	0x100
#define MCP4652_TCON_R0_EN	0x008
#define MCP4652_TCON_A_EN	0x004
#define MCP4652_TCON_W_EN	0x002
#define MCP4652_TCON_B_EN	0x001
#define MCP4652_TCON_ALL_EN	0x1FF

// Register addresses
#define MCP4652_RA_WIPER	0x00
#define MCP4652_RA_TCON		0x40


// Common WIPER values
#define MCP4652_WIPER_MID	0x080
#define MCP4652_WIPER_A		0x100
#define MCP4652_WIPER_B		0x000

void mcp46XX_set_i2c(i2c_inst_t *i2c);
void mcp4652_set_wiper(uint16_t value);
void mcp4651_set_wiper(uint16_t value);

#endif
