#ifndef OST_HAL_H
#define OST_HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include "qor.h"

#define OST_ID_SPI_FOR_SDCARD 0

#define GPIO_PWM 16

// normal screens
#define SCREEN_MAIN 0
#define SCREEN_WEATHER 1
#define SCREEN_LIST_ALARMS 2
#define SCREEN_DEBUG 3
// unreachable screens
#define SCREEN_ALARM 10

#define MAX_SCREEN_ID 3

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0

//#define PCBV1 1

#ifdef PCBV1
#define BTN_1 8
#define BTN_2 9
#define BTN_3 10
#define BTN_4 19
#define BTN_5 20
#define BTN_6 21
#define FRONT_PANEL_LED_PIN 11
#else
// PCBv2 OK
#define BTN_1 8
#define BTN_2 9
#define BTN_3 10
#define BTN_4 20
#define BTN_5 21
#define BTN_6 28
#define FRONT_PANEL_LED_PIN 5	// on gpio extender
#endif

#ifdef PCBV1
#define I2C_SDA 12
#define I2C_SCL 13
#define I2C_CHANNEL i2c0
#else
// PCBv2 OK
#define I2C_SDA 18
#define I2C_SCL 19
#define I2C_CHANNEL i2c1
#endif
#define I2C_BAUD_RATE 400000

// PCBv2 OK
#define I2S_DATA_PIN 22
#define I2S_CLOCK_PIN_BASE 26

// PCBv2 OK
#define SDCARD_SCK 14
#define SDCARD_MOSI 15
#define SDCARD_MISO 12
#define SD_CARD_CS 13
#define SD_CARD_PRESENCE 2		// on gpio extender


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

	// Wakeup alarm global structure
	typedef struct {
	  int isset;
	  int weekdays;	// This is a bitmask
	  int hour;
	  int min;
      int in_mins;
      char chime[20];
	} wakeup_alarm_struct;

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
	} config_struct;

    // ----------------------------------------------------------------------------
    // SHARED TYPES
    // ----------------------------------------------------------------------------
    typedef struct
    {
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
    } rect_t;

    typedef struct
    {
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } color_t;

    typedef enum
    {
        OST_GPIO_DEBUG_LED,
        OST_GPIO_DEBUG_PIN
    } ost_hal_gpio_t;

    typedef enum
    {
        OST_BUTTON_OK = 0x01,
        OST_BUTTON_HOME = 0x02,
        OST_BUTTON_PAUSE = 0x04,
        OST_BUTTON_VOLUME_UP = 0x08,
        OST_BUTTON_VOLUME_DOWN = 0x10,
        OST_BUTTON_LEFT = 0x20,
        OST_BUTTON_RIGHT = 0x40
    } ost_button_t;

    // ----------------------------------------------------------------------------
    // HIGH LEVEL API
    // ----------------------------------------------------------------------------
    // System API
    void system_initialize();
    void system_putc(char ch);
    void ost_system_delay_ms(uint32_t delay);
    void ost_system_stopwatch_start();
    uint32_t ost_system_stopwatch_stop();

	uint8_t get_uniq_id();

    // Audio API
    void ost_audio_play(const char *filename);
    void ost_audio_stop();
    int ost_audio_process();
    typedef void (*ost_audio_callback_t)(void);
    void ost_audio_register_callback(ost_audio_callback_t cb);

    // Buttons APU
    typedef void (*ost_button_callback_t)(uint32_t flags);
    void ost_button_register_callback(ost_button_callback_t cb);

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

	void getWeekdayStr(int weekday, char *weekdays_str);
	void getWeekdaysStr(int weekdays, char *weekdays_str);

    // ----------------------------------------------------------------------------
    // AUDIO HAL
    // ----------------------------------------------------------------------------
    void ost_hal_audio_new_frame(const void *buffer, int size);

#ifdef __cplusplus
}
#endif

#endif // OST_HAL_H
