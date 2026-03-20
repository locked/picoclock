#ifndef BTCTRL_H
#define BTCTRL_H

#include "hardware/uart.h"

void btctrl_init(uart_inst_t *uart, uint8_t pin_tx, uint8_t pin_rx);
void btctrl_start();
void btctrl_end();
void btctrl_status(char *res);

#endif
