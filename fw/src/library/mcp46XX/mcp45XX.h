#ifndef MCP4551_H
#define MCP4551_H

#include "hardware/i2c.h"
#include <stdint.h>

// I2C Address of device (7-bit address)
// Binary: 0101 + 11 + A0
#define MCP4551_DEFAULT_ADDRESS 0x2E

// Memory Address Bits (Command Byte High 4 bits)
#define MCP4551_RA_WIPER0       0x00 // Volatile Wiper 0
#define MCP4551_RA_TCON         0x04 // Terminal Control Register
#define MCP4551_RA_STATUS       0x05 // Status Register

// Command Bits (Command Byte bits 3:2)
#define MCP4551_CMD_WRITE       0x00 // (xxxx00xx)
#define MCP4551_CMD_INC         0x04 // (xxxx01xx)
#define MCP4551_CMD_DEC         0x08 // (xxxx10xx)
#define MCP4551_CMD_READ        0x0C // (xxxx11xx)

// TCON Register bit definitions (for Wiper 0)
#define MCP4551_TCON_GCEN       0x100 // General Call Enable
#define MCP4551_TCON_R0HW       0x008 // Hardware Configuration Control
#define MCP4551_TCON_R0A        0x004 // Terminal A Connection
#define MCP4551_TCON_R0W        0x002 // Wiper Connection
#define MCP4551_TCON_R0B        0x001 // Terminal B Connection
#define MCP4551_TCON_ALL_EN     0x10F

// Common WIPER values (8-bit resolution: 0-255)
#define MCP4551_WIPER_MID       0x80
#define MCP4551_WIPER_MAX       0xFF
#define MCP4551_WIPER_MIN       0x00

// Function prototypes
void mcp4551_init(i2c_inst_t *i2c);
void mcp4551_set_wiper(uint8_t value);
uint8_t mcp4551_read_wiper(void);

#endif
