#include "sensirion.h"
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <inttypes.h>

// Global I2C instance
static i2c_inst_t *sensirion_i2c;

void sensirion_init(i2c_inst_t *i2c) {
	sensirion_i2c = i2c;
}


void sensirion_common_to_integer(const uint8_t* source, uint8_t* destination,
                                 INT_TYPE int_type, uint8_t data_length) {
    if (data_length > int_type) {
        data_length = 0;  // we do not read at all if data_length is bigger than
                          // the provided integer!
    }
    // pad missing bytes
    uint8_t offset = int_type - data_length;
    for (uint8_t i = 0; i < offset; i++) {
        destination[int_type - i - 1] = 0;
    }
    for (uint8_t i = 1; i <= data_length; i++) {
        destination[int_type - offset - i] = source[i - 1];
    }
}
uint32_t sensirion_common_bytes_to_uint32_t(const uint8_t* bytes) {
    return (uint32_t)bytes[0] << 24 | (uint32_t)bytes[1] << 16 |
           (uint32_t)bytes[2] << 8 | (uint32_t)bytes[3];
}
uint16_t sensirion_common_bytes_to_uint16_t(const uint8_t* bytes) {
    return (uint16_t)bytes[0] << 8 | (uint16_t)bytes[1];
}
float stcc4_signal_temperature(uint16_t raw_temperature) {
    float temperature = 0.0;
    temperature = -45.0 + ((175.0 * raw_temperature) / 65535.0);
    return temperature;
}
float stcc4_signal_relative_humidity(uint16_t raw_relative_humidity) {
    float relative_humidity = 0.0;
    relative_humidity = -6.0 + ((125.0 * raw_relative_humidity) / 65535.0);
    return relative_humidity;
}
uint8_t sensirion_i2c_generate_crc(const uint8_t* data, uint16_t count) {
    uint16_t current_byte;
    uint8_t crc = CRC8_INIT;
    uint8_t crc_bit;

    /* calculates 8-Bit checksum with given polynomial */
    for (current_byte = 0; current_byte < count; ++current_byte) {
        crc ^= (data[current_byte]);
        for (crc_bit = 8; crc_bit > 0; --crc_bit) {
            if (crc & 0x80)
                crc = (crc << 1) ^ CRC8_POLYNOMIAL;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}
int8_t sensirion_i2c_check_crc(const uint8_t* data, uint16_t count,
                               uint8_t checksum) {
    if (sensirion_i2c_generate_crc(data, count) != checksum)
        return CRC_ERROR;
    return NO_ERROR;
}
int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint8_t count) {
    if (i2c_read_blocking(sensirion_i2c, address, data, count, false) != count) {
        return I2C_READ_FAILED;
    }
    return 0;
}
int16_t sensirion_i2c_read_data_inplace(uint8_t address, uint8_t* buffer,
                                        uint16_t expected_data_length) {
    int16_t error;
    uint16_t i, j;
    uint16_t size = (expected_data_length / SENSIRION_WORD_SIZE) *
                    (SENSIRION_WORD_SIZE + CRC8_LEN);

    if (expected_data_length % SENSIRION_WORD_SIZE != 0) {
        return BYTE_NUM_ERROR;
    }

    error = sensirion_i2c_hal_read(address, buffer, size);
    if (error) {
        return error;
    }

    for (i = 0, j = 0; i < size; i += SENSIRION_WORD_SIZE + CRC8_LEN) {

        error = sensirion_i2c_check_crc(&buffer[i], SENSIRION_WORD_SIZE,
                                        buffer[i + SENSIRION_WORD_SIZE]);
        if (error) {
            return error;
        }
        buffer[j++] = buffer[i];
        buffer[j++] = buffer[i + 1];
    }

    return NO_ERROR;
}


// STCC4
void sensirion_stcc4_init() {
	uint32_t product_id = 0;
	uint64_t serial_number = 0;
	uint8_t packet[2];
	int8_t result;
	uint8_t rxdata[12];

	// stop continuous measurement
	packet[0] = 0x3f;
	packet[1] = 0x86;
	result = i2c_write_blocking(sensirion_i2c, 0x64, packet, 2, false);
	sleep_ms(1200);

	// get product id & serial
	packet[0] = 0x36;
	packet[1] = 0x5b;
	result = i2c_write_blocking(sensirion_i2c, 0x64, packet, 2, false);
	sleep_ms(1);
	result = sensirion_i2c_read_data_inplace(0x64, rxdata, 12);
	product_id = sensirion_common_bytes_to_uint32_t(&rxdata[0]);
	sensirion_common_to_integer(&rxdata[4], (uint8_t*)&serial_number, LONG_INTEGER, 8);
	printf("[picoclock] STCC4 product_id:[%d] rx:[%02x%02x%02x%02x] [%02x%02x%02x%02x] [%02x%02x%02x%02x] serial:[%" PRIx64 "]\r\n",
		product_id,
		rxdata[0],
		rxdata[1],
		rxdata[2],
		rxdata[3],
		rxdata[4],
		rxdata[5],
		rxdata[6],
		rxdata[7],
		rxdata[8],
		rxdata[9],
		rxdata[10],
		rxdata[11],
		serial_number
	);

	// start continuous measurement
	packet[0] = 0x21;
	packet[1] = 0x8b;
	result = i2c_write_blocking(sensirion_i2c, 0x64, packet, 2, false);
	sleep_ms(100);
}

int16_t sensirion_stcc4_read() {
	uint8_t packet[2];
	int8_t result;
	uint8_t rxdata[12];
	uint16_t retries = 0;
	uint16_t max_retries = 100;
	int16_t co2;
	do {
		// read measurement
		packet[0] = 0xec;
		packet[1] = 0x05;
		result = i2c_write_blocking(sensirion_i2c, 0x64, packet, 2, true);
		sleep_ms(1);
		result = sensirion_i2c_read_data_inplace(0x64, rxdata, 8);
		co2 = sensirion_common_bytes_to_uint16_t(&rxdata[0]);
		float temp = stcc4_signal_temperature(sensirion_common_bytes_to_uint16_t(&rxdata[2]));
		float rh = stcc4_signal_relative_humidity(sensirion_common_bytes_to_uint16_t(&rxdata[4]));
		uint16_t status = sensirion_common_bytes_to_uint16_t(&rxdata[6]);
		printf("[picoclock] STCC4 result:[%d] co2:[%i] [%.1f] [%.1f] [%u]\r\n", result, co2, temp, rh, status);
		sleep_ms(100);
		retries++;
		watchdog_update();
	} while (result != NO_ERROR && retries < max_retries);
	return co2;
}


// SCD43
void sensirion_scd43_init() {
    uint64_t serial_number = 0;
	uint8_t packet[2];
	int8_t result;
	uint8_t rxdata[12];

	// wake up
	packet[0] = 0x36;
	packet[1] = 0xf6;
	result = i2c_write_blocking(sensirion_i2c, 0x62, packet, 2, false);
	sleep_ms(30);

	// stop periodic measurement
	packet[0] = 0x3f;
	packet[1] = 0x86;
	result = i2c_write_blocking(sensirion_i2c, 0x62, packet, 2, false);
	sleep_ms(500);

	// reinit
	packet[0] = 0x36;
	packet[1] = 0x46;
	result = i2c_write_blocking(sensirion_i2c, 0x62, packet, 2, false);
	sleep_ms(30);

	// get serial
	packet[0] = 0x36;
	packet[1] = 0x82;
	result = i2c_write_blocking(sensirion_i2c, 0x62, packet, 2, false);
	sleep_ms(1);
	result = sensirion_i2c_read_data_inplace(0x62, rxdata, 6);
	sensirion_common_to_integer(&rxdata[0], (uint8_t*)&serial_number, LONG_INTEGER, 6);
	printf("[picoclock] SCD4X rx:[%02x%02x%02x%02x%02x%02x] [%" PRIx64 "] [%" PRIu64 "] [%lld]\r\n",
		rxdata[0],
		rxdata[1],
		rxdata[2],
		rxdata[3],
		rxdata[4],
		rxdata[5],
		serial_number,
		serial_number,
		serial_number
	);

	// start periodic measurement
	packet[0] = 0x21;
	packet[1] = 0xb1;
	result = i2c_write_blocking(sensirion_i2c, 0x62, packet, 2, false);
	sleep_ms(200);
}

int16_t sensirion_scd43_read() {
	uint8_t packet[2];
	int8_t result;
	uint8_t rxdata[12];

	// get data ready status
	bool ready = false;
	uint16_t retries = 0;
	uint16_t max_retries = 100;
	do {
		packet[0] = 0xe4;
		packet[1] = 0xb8;
		result = i2c_write_blocking(sensirion_i2c, 0x62, packet, 2, false);
		sleep_ms(1);
		result = sensirion_i2c_read_data_inplace(0x62, rxdata, 2);
		uint16_t data_ready_status = sensirion_common_bytes_to_uint16_t(&rxdata[0]);
		ready = (data_ready_status & 2047) != 0;
		printf("[picoclock] SCD43 raw status:[%i] [%d]\r\n", data_ready_status, ready);
		sleep_ms(100);
		retries++;
		watchdog_update();
	} while (!ready && retries < max_retries);

	// read measurement
	packet[0] = 0xec;
	packet[1] = 0x05;
	result = i2c_write_blocking(sensirion_i2c, 0x62, packet, 2, false);
	sleep_ms(3);
	sensirion_i2c_read_data_inplace(0x62, rxdata, 6);
	int16_t co2 = sensirion_common_bytes_to_uint16_t(&rxdata[0]);
	float temp = stcc4_signal_temperature(sensirion_common_bytes_to_uint16_t(&rxdata[2]));
	float rh = stcc4_signal_relative_humidity(sensirion_common_bytes_to_uint16_t(&rxdata[4]));
	printf("[picoclock] SCD43 co2:[%i] [%.1f] [%.1f]\r\n", co2, temp, rh);
	return co2;
}
