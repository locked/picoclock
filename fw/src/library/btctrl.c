#include "btctrl.h"

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/unique_id.h"

static uart_inst_t *btctrl_uart;


void btctrl_read(char *res) {
	sprintf(res, "");
	while (uart_is_readable(btctrl_uart) && strlen(res) < 50) {
		char c = uart_getc(btctrl_uart);
		if (c != '\n') {
			sprintf(res, "%s%c", res, c);
		}
	}
	printf("received:[%s]", res);
}


void btctrl_init(uart_inst_t *uart, uint8_t pin_tx, uint8_t pin_rx) {
	btctrl_uart = uart;

	uart_init(btctrl_uart, 9600);
	gpio_set_function(pin_tx, GPIO_FUNC_UART);
	gpio_set_function(pin_rx, GPIO_FUNC_UART);

	btctrl_end();
}

void btctrl_start() {
	char board_id[20];
	char cmd[30];

	pico_get_unique_board_id_string(board_id, 20);
	sprintf(cmd, "START:PicoClk%.4s\n", board_id);
	uart_puts(btctrl_uart, cmd);
	sleep_ms(1);
	char res[50];
	btctrl_read(res);
}

void btctrl_end() {
	uart_puts(btctrl_uart, "END\n");
	sleep_ms(1);
	char res[50];
	btctrl_read(res);
}

void btctrl_status(char *res) {
	uart_puts(btctrl_uart, "STATUS\n");
	sleep_ms(1);
	btctrl_read(res);
}
