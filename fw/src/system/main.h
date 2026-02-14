#ifndef OST_HAL_H
#define OST_HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "pcf8563/pcf8563.h"

#define OST_ID_SPI_FOR_SDCARD 0

#define GPIO_PWM 16

// normal screens
#define SCREEN_MAIN 0
#define SCREEN_WEATHER 1
#define SCREEN_LIST_ALARMS 2
#define SCREEN_DEBUG 3
#define SCREEN_METRIC 4
// unreachable screens
#define SCREEN_ALARM 10

#define MAX_SCREEN_ID 4

#define UART_ID uart0
//#define BAUD_RATE 115200
#define BAUD_RATE 9600
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

typedef struct
{
	uint8_t ev;
	uint8_t song;
} ost_net_event_t;

typedef enum
{
	OST_SYS_NO_EVENT,
	OST_SYS_BUTTON,
	OST_SYS_REFRESH_SCREEN,
	OST_SYS_ALARM,
	OST_SYS_UPDATE_TIME,
	OST_SYS_PLAY_SOUND,
	OST_SYS_DISP_MSG
} ost_system_state_t;

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
	uint16_t co2;
	uint8_t ens160_status;
	float temp;
} metrics_t;


// ----------------------------------------------------------------------------
// HIGH LEVEL API
// ----------------------------------------------------------------------------
// System API
void system_initialize();
void system_putc(char ch);
void ost_system_delay_ms(uint32_t delay);

// Audio API
void main_audio_play(const char *filename);
void main_audio_stop();
int main_audio_process();
void main_audio_new_frame(const void *buffer, int size);

// ----------------------------------------------------------------------------
// SDCARD HAL
// ----------------------------------------------------------------------------

void ost_hal_sdcard_set_slow_clock(void);
void ost_hal_sdcard_set_fast_clock(void);

/**
 * @brief Deselect the SD-Card by driving the Chip Select to high level (eg: 3.3V)
 *
 */
void ost_hal_sdcard_cs_high();

/**
 * @brief Deselect the SD-Card by driving the Chip Select to low level (eg: 0V)
 *
 */
void ost_hal_sdcard_cs_low();

// We have a separated API here to allow specific optimizations such as the use of DMA

void ost_hal_sdcard_spi_exchange(const uint8_t *buffer, uint8_t *out, uint32_t size);

void ost_hal_sdcard_spi_write(const uint8_t *buffer, uint32_t size);

void ost_hal_sdcard_spi_read(uint8_t *out, uint32_t size);

/**
 * @brief Return 1 if the SD card is physically inserted, otherwise 0
 *
 * @return uint8_t SD card is present or not
 */
uint8_t ost_hal_sdcard_get_presence();

#ifdef __cplusplus
}
#endif

#endif // OST_HAL_H
