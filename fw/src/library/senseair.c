#include "senseair.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <stdio.h>

uint16_t get_co2_reading() {
	uint8_t cmd[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};
	uint8_t response[7];
	int bytes_read = 0;

	// Clear any stale data in the RX FIFO before sending command
	while (uart_is_readable(UART_ID)) {
		uart_getc(UART_ID);
	}

	// Send the Modbus request
	uart_write_blocking(UART_ID, cmd, sizeof(cmd));
	//uart_default_tx_wait_blocking();
	sleep_ms(10);

	uint8_t count_wait = 0;
	while (bytes_read < 7) {
		if (uart_is_readable(UART_ID)) {
			response[bytes_read++] = uart_getc(UART_ID);
		}

		if (count_wait++ > 100) {
			printf("[senseair] UART Timeout - Sensor not responding.\n");
			return bytes_read;
		}
		sleep_ms(1);
	}

	printf("[senseair] got response:[%02x:%02x:%02x:%02x:%02x:%02x:%02x]\n", response[0], response[1], response[2], response[3], response[4], response[5], response[6]);
	// Basic Modbus validation: Check function code and byte count
	if (response[1] == 0x04 && response[2] == 0x02) {
		uint16_t co2 = (uint16_t)response[3] << 8 | response[4];
		printf("[senseair] CO2: %u ppm\n", co2);
		return co2;
	}
	printf("[senseair] Error: Invalid Modbus response received.\n");
	return 0;
}
