#include "mcp23009.h"

#include "pico/stdlib.h"
#include <stdio.h>

// Global
i2c_inst_t *mcp23009_i2c;

void mcp23009_set_i2c(i2c_inst_t *i2c) {
	mcp23009_i2c = i2c;
}

uint8_t mcp23009_read_register(uint8_t reg) {
    uint8_t buffer[2] = { reg, 0 };

    int rc = 0;
    int retry = 5;
    do {
        rc = 0;
        rc += i2c_write_timeout_us(mcp23009_i2c, MCP23009_DEFAULT_ADDRESS, buffer, 1, false, 50000);
        rc += i2c_read_timeout_us(mcp23009_i2c, MCP23009_DEFAULT_ADDRESS, buffer + 1, 1, false, 50000);
        if (rc != 2) {
            busy_wait_ms(1);
        }
    } while (rc != 2 && retry > 0);
    if (retry == 0) {
        panic("MCP23009: failed to read register %d", reg);
    }

    return buffer[1];
}

bool mcp23009_write_register(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = { reg, value };
    int rc = i2c_write_timeout_us(mcp23009_i2c, MCP23009_DEFAULT_ADDRESS, buffer, sizeof(buffer), false, 50000);
    if (rc == sizeof(buffer)) {
        return true;
    } else {
        panic("MCP23009: failed to write register %d", reg);
        return false;
    }
}

bool mcp23009_is_connected() {
    uint8_t buffer;
    int rc = i2c_read_timeout_us(mcp23009_i2c, MCP23009_DEFAULT_ADDRESS, &buffer, sizeof(buffer), false, 50000);
    return (rc == sizeof(buffer));
}

void mcp23009_set_direction(uint8_t mask) {
    mcp23009_write_register(MCP23009_IODIR, mask);
}

void mcp23009_set_polarity(uint8_t mask) {
    mcp23009_write_register(MCP23009_IPOL, mask);
}

void mcp23009_set_interrupt_source(uint8_t mask) {
    mcp23009_write_register(MCP23009_GPINTEN, mask);
}

void mcp23009_set_reference_state(uint8_t mask) {
    mcp23009_write_register(MCP23009_DEFVAL, mask);
}

void mcp23009_set_interrupt_event(uint8_t mask) {
    mcp23009_write_register(MCP23009_INTCON, mask);
}

void mcp23009_config(bool seqOp, bool irqOpenDrain, bool irqPolarity, bool irqClear) {
    uint8_t mask = 0x00;
    mask |= (seqOp << 5);
    mask |= (irqOpenDrain << 2);
    mask |= (irqPolarity << 1);
    mask |= irqClear;
    mcp23009_write_register(MCP23009_IOCON, mask);
}

void mcp23009_set_pull_ups(uint8_t mask)
{
    mcp23009_write_register(MCP23009_GPPU, mask);
}

uint8_t mcp23009_get_interrupt_flag() {
    return mcp23009_read_register(MCP23009_INTF);
}

uint8_t mcp23009_get_interrupt_capture() {
    return mcp23009_read_register(MCP23009_INTCAP);
}

uint8_t mcp23009_get_all() {
    return mcp23009_read_register(MCP23009_GPIO);
}

bool mcp23009_get(uint8_t pin) {
    uint8_t mask = mcp23009_get_all();
    return (mask & (1 << pin)) > 0;
}

void mcp23009_set_all(uint8_t mask) {
    mcp23009_write_register(MCP23009_GPIO, mask);
}

void mcp23009_set(uint8_t pin, bool value) {
    uint8_t mask = mcp23009_get_all();
    if (value) {
        mask |= (1 << pin);
    } else {
        mask &= ~(1 << pin);
    }
    mcp23009_set_all(mask);
}
