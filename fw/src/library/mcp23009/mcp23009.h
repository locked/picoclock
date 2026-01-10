#ifndef MCP23009_H
#define MCP23009_H

#include "hardware/i2c.h"

// I2C Address of device
#define MCP23009_DEFAULT_ADDRESS 0x20

#define MCP23009_IODIR 0x00   // pin direction
#define MCP23009_IPOL 0x01    // input polarity
#define MCP23009_GPINTEN 0x02 // interrupt source config
#define MCP23009_DEFVAL 0x03  // interrupt trigger event config (default value)
#define MCP23009_INTCON 0x04  // interrupt trigger event config (default or change)
#define MCP23009_IOCON 0x05   // chip config
#define MCP23009_GPPU 0x06    // pin pull-up
#define MCP23009_INTF 0x07    // irq trigger
#define MCP23009_INTCAP 0x08  // gpio status @ irq
#define MCP23009_GPIO 0x09    // gpio status
#define MCP23009_OLAT 0x0A    // output latches

void mcp23009_set_i2c(i2c_inst_t *i2c);
bool mcp23009_is_connected();
void mcp23009_config(bool seqOp, bool irqOpenDrain, bool irqPolarity, bool irqClear);
void mcp23009_set(uint8_t pin, bool value);
void mcp23009_set_direction(uint8_t mask);
void mcp23009_set_polarity(uint8_t mask);
void mcp23009_set_reference_state(uint8_t mask);
void mcp23009_set_pull_ups(uint8_t mask);

#endif
