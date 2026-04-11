#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "pcf8563/pcf8563.h"

#define VERSION "0.53"

#define OST_ID_SPI_FOR_SDCARD 0

#define GPIO_PWM 16

#define CPU_CLOCK_IDLE 100000
#define CPU_CLOCK_MAX 200000

// normal screens
#define SCREEN_MAIN 0
#define SCREEN_WEATHER 1
#define SCREEN_LIST_ALARMS 2
#define SCREEN_DEBUG 3
// unreachable screens
#define SCREEN_ALARM 10
#define SCREEN_MUSIC 11
#define SCREEN_MUSIC_BT 12

#define MAX_SCREEN_ID 3

#define UART_ID uart0
#define UART_BAUD_RATE 115200
//#define UART_BAUD_RATE 9600
#define UART_TX_PIN 0
#define UART_RX_PIN 1

// UART for ESP32 communication
#define UART_ESP32_UART_ID uart1
#define UART_ESP32_TX_PIN 36
#define UART_ESP32_RX_PIN 37

// Can be:
// 1, 2, mini
#define PCB_VERSION_1 1
#define PCB_VERSION_2 2
#define PCB_VERSION_MINI mini
#define PCB_VERSION PCB_VERSION_MINI

#if PCB_VERSION == PCB_VERSION_1
#define BTN_1 8
#define BTN_2 9
#define BTN_3 10
#define BTN_4 19
#define BTN_5 20
#define BTN_6 21
#define FRONT_PANEL_LED_PIN 11
#endif
#if PCB_VERSION == PCB_VERSION_2
// PCBv2 OK
#define BTN_1 8
#define BTN_2 9
#define BTN_3 10
#define BTN_4 20
#define BTN_5 21
#define BTN_6 28
#define FRONT_PANEL_LED_PIN 7	// on gpio extender
#endif
#if PCB_VERSION == PCB_VERSION_MINI
#define BTN_1 30
#define BTN_2 31
#define BTN_3 32
#define BTN_4 20
#define BTN_5 21
#define BTN_6 22
#define FRONT_PANEL_LED_PIN 8
#endif

#if PCB_VERSION == PCB_VERSION_1
#define I2C_SDA 12
#define I2C_SCL 13
#define I2C_CHANNEL i2c0
#endif
#if PCB_VERSION == PCB_VERSION_2
#define I2C_SDA 18
#define I2C_SCL 19
#define I2C_CHANNEL i2c1
#endif
#if PCB_VERSION == PCB_VERSION_MINI
#define I2C_SDA 18
#define I2C_SCL 19
#define I2C_CHANNEL i2c1
#endif
#define I2C_BAUD_RATE 100000
//#define I2C_BAUD_RATE 400000


#if PCB_VERSION == PCB_VERSION_MINI
#define I2S_DATA_PIN 45
#define I2S_CLOCK_PIN_BASE 46
//#define I2S_DATA_PIN 20        // OK
//#define I2S_CLOCK_PIN_BASE 21  // OK
#define AUDIO_MUTE_PIN 39
#define I2S_SELECT_PIN 38
#else
#define I2S_DATA_PIN 22
#define I2S_CLOCK_PIN_BASE 26
#define AUDIO_MUTE_PIN 0 // on gpio extender
#endif


#if PCB_VERSION == PCB_VERSION_MINI
#define SDCARD_SCK 42
#define SDCARD_MOSI 43
#define SDCARD_MISO 44
#define SD_CARD_CS 41
#define SD_CARD_PRESENCE 28
#else
#define SDCARD_SCK 14
#define SDCARD_MOSI 15
#define SDCARD_MISO 12
#define SD_CARD_CS 13
#define SD_CARD_PRESENCE 2	// on gpio extender
#endif

#define TRIGGER_SYNC_EVERY_SEC 1800

typedef void (*wifi_callback_t)(char *buffer, uint16_t buffer_length);

typedef int (*ota_segment_consumer_t)(char* buf);

typedef struct {
	char code_desc[100];
	char temperature_2m[20];
	char wind_speed_10m[20];
	char precipitation[20];
	int day_0_precip_sum;
	int day_0_temp_min;
	int day_0_temp_max;
	char day_0_sunrise[6];
	char day_0_sunset[6];
	int day_1_precip_sum;
	int day_1_temp_min;
	int day_1_temp_max;
	char day_1_sunrise[6];
	char day_1_sunset[6];
	int day_2_precip_sum;
	int day_2_temp_min;
	int day_2_temp_max;
	char day_2_sunrise[6];
	char day_2_sunset[6];
} weather_struct;

typedef struct {
	char wifi_ssid[50];
	char wifi_key[50];
	char remote_host[50];
	char screen[2];
} config_struct;

typedef struct {
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint16_t tvoc;
	uint16_t eco2;
	uint16_t s88_co2;
	int16_t scd43_co2;
	int16_t stcc4_co2;
	uint8_t ens160_status;
	uint32_t mem_free;
	float temp;
	bool sent;
	bool tosend;
} metrics_t;

typedef struct {
	int has_mcp9808;
	int has_ens160;
	int has_s88;
	int has_stcc4;
	int has_scd43;
} features_t;

// System API
void system_initialize();
void system_putc(char ch);

// Audio API
void main_audio_play(const char *filename);
void main_audio_stop();
int main_audio_process();
void main_audio_new_frame(const void *buffer, int size);

// SDCARD HAL
void hal_sdcard_set_slow_clock(void);
void hal_sdcard_set_fast_clock(void);

// Deselect the SD-Card by driving the Chip Select to high level (eg: 3.3V)
void hal_sdcard_cs_high();
// Deselect the SD-Card by driving the Chip Select to low level (eg: 0V)
void hal_sdcard_cs_low();

// We have a separated API here to allow specific optimizations such as the use of DMA
void hal_sdcard_spi_exchange(const uint8_t *buffer, uint8_t *out, uint32_t size);
void hal_sdcard_spi_write(const uint8_t *buffer, uint32_t size);
void hal_sdcard_spi_read(uint8_t *out, uint32_t size);

// Return 1 if the SD card is physically inserted, otherwise 0
uint8_t hal_sdcard_get_presence();

uint32_t get_fattime (void);

#ifdef __cplusplus
}
#endif

#endif // MAIN_H
