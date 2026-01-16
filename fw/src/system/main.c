// C library
#include "main.h"
#include "debug.h"
#include "filesystem.h"
#include "sdcard.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "system.h"
#include "ui_task.h"
#include "net_task.h"
#include "alarms.h"
#include "fs_task.h"

// Raspberry Pico SDK
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/util/datetime.h"
#include "pico/aon_timer.h"
#include "hardware/i2c.h"
#include "hardware/watchdog.h"
#include "pico/unique_id.h"
#include "pico.h"
#include "pico/multicore.h"

// Screen
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"
#include "EPD_2in13b_V4.h"

#include "lp5817/lp5817.h"
#include "mcp46XX/mcp45XX.h"
#include "pcf8563/pcf8563.h"

// Audio (PIO)
#include "audio_player.h"
#include "pico_i2s.h"
static __attribute__((aligned(8))) pio_i2s i2s;
static volatile uint32_t msTicks = 0;
audio_ctx_t audio_ctx;
static audio_i2s_config_t config = {
	.freq = 22050,
	.bps = 32,
	.data_pin = I2S_DATA_PIN,
	.clock_pin_base = I2S_CLOCK_PIN_BASE
};


//
// Globals
//
weather_struct weather = {"", "", "", ""};
int current_screen = 0;
config_struct global_config;
int reboot_requested = 0;
char last_sync_str[9] = "";
bool refresh_screen = false;
bool refresh_screen_clear = false;
int ts_reset_alarm_screen = 0;


// ===========================================================================================================
// CONSTANTS / DEFINES
// ===========================================================================================================

const uint8_t BUTTONS[] = {BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6};
volatile uint8_t last_btn_values[6] = {1, 1, 1, 1, 1, 1};
static struct repeating_timer timer;
volatile int request_btn_push = -1;
volatile int request_audio_read = -1;

// ===========================================================================================================
// PROTOTYPES
// ===========================================================================================================
extern void init_spi(void);
void dma_init();
void __isr __time_critical_func(audio_i2s_dma_irq_handler)();


void ost_system_delay_ms(uint32_t delay) {
	busy_wait_ms(delay);
}

void check_buttons() {
	for (uint8_t btn=0; btn<6; btn++) {
		uint8_t btn_value = gpio_get(BUTTONS[btn]);
		if (btn_value == 1 && btn_value != last_btn_values[btn]) {
			request_btn_push = btn;
		}
		last_btn_values[btn] = btn_value;
	}
}

bool timer_callback(struct repeating_timer *t) {
	check_buttons();
	return true; // Keep the timer running
}


// ----------------------------------------------------------------------------
// AUDIO HAL
// ----------------------------------------------------------------------------
static void audio_callback(void) {
	main_audio_process();
}

void main_audio_play(const char *filename) {
	printf("audio_play... [%s]\r\n", filename);
	audio_play(&audio_ctx, filename);
	config.freq = audio_ctx.audio_info.sample_rate;
	config.channels = audio_ctx.audio_info.channels;
	printf("pico_i2s_set_frequency...\r\n");
	pico_i2s_set_frequency(&i2s, &config);

	i2s.buffer_index = 0;

	// On appelle une première fois le process pour récupérer et initialiser le premier buffer...
	printf("audio_process 1st buffer...\r\n");
	audio_process(&audio_ctx);

	// Puis le deuxième ... (pour avoir un buffer d'avance)
	audio_process(&audio_ctx);

	gpio_put(AUDIO_MUTE_PIN, 1); // Unmute
	// On lance les DMA
	printf("i2s_start...\r\n");
	i2s_start(&i2s);
}

void main_audio_stop() {
	gpio_put(AUDIO_MUTE_PIN, 0); // Mute

	//debug_printf("audio stop\r\n");
	memset(i2s.out_ctrl_blocks[0], 0, STEREO_BUFFER_SIZE * sizeof(uint32_t));
	memset(i2s.out_ctrl_blocks[1], 0, STEREO_BUFFER_SIZE * sizeof(uint32_t));
	audio_stop(&audio_ctx);
	i2s_stop(&i2s);
}

int main_audio_process() {
	return audio_process(&audio_ctx);
}

void main_audio_new_frame(const void *buff, int size) {
	if (size > STEREO_BUFFER_SIZE) {
		// Problème
		debug_printf("[picoclock] WARNING: audio_new_frame size:[%d] > STEREO_BUFFER_SIZE\n", size);
		return;
	}
	memcpy(i2s.out_ctrl_blocks[i2s.buffer_index], buff, size * sizeof(uint32_t));
	i2s.buffer_index = 1 - i2s.buffer_index;
}

void __isr __time_critical_func(audio_i2s_dma_irq_handler)() {
	//dma_hw->ints0 = 1u << i2s.dma_ch_out_data; // clear the IRQ
	dma_channel_acknowledge_irq0(i2s.dma_ch_out_data);

	// Warn the application layer that we have done on that channel
	request_audio_read = 1;
}


void init_i2c() {
	i2c_init(I2C_CHANNEL, I2C_BAUD_RATE);

	//printf("[picoclock] sensors: setting I2C pins: SDA=%d SCL=%d\n", SENSORS_SDA_PIN, SENSORS_SCL_PIN);
	gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
	gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

	//printf("[picoclock] sensors: pulling I2C pins up\n");
	gpio_pull_up(I2C_SDA);
	gpio_pull_up(I2C_SCL);
}

void init_gpio() {
	pio_set_gpio_base(pio0, 16);

	gpio_init(FRONT_PANEL_LED_PIN);
	gpio_set_dir(FRONT_PANEL_LED_PIN, GPIO_OUT);

	gpio_init(AUDIO_MUTE_PIN);
	gpio_set_dir(AUDIO_MUTE_PIN, GPIO_OUT);

	gpio_init(I2S_SELECT_PIN);
	gpio_set_dir(I2S_SELECT_PIN, GPIO_OUT);
	gpio_put(I2S_SELECT_PIN, 0);	// 0 select I2S from pico, 1 from ESP32

	// Buttons
	for (uint8_t btn=0; btn<6; btn++) {
		gpio_init(BUTTONS[btn]);
		gpio_set_dir(BUTTONS[btn], GPIO_IN);
		gpio_pull_up(BUTTONS[btn]);
	}
}

// I2C reserves some addresses for special purposes. We exclude these from the scan.
// These are any addresses of the form 000 0xxx or 111 1xxx
bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}
void i2c_bus_scan() {
	printf("\nI2C Bus Scan\n");
	printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

	for (int addr = 0; addr < (1 << 7); ++addr) {
		if (addr % 16 == 0) {
			printf("%02x ", addr);
		}

		// Perform a 1-byte dummy read from the probe address. If a slave
		// acknowledges this address, the function returns the number of bytes
		// transferred. If the address byte is ignored, the function returns
		// -1.

		// Skip over any reserved addresses.
		int ret;
		uint8_t rxdata;
		if (reserved_addr(addr))
			ret = PICO_ERROR_GENERIC;
		else
			ret = i2c_read_blocking(I2C_CHANNEL, addr, &rxdata, 1, false);

		printf(ret < 0 ? "." : "@");
		printf(addr % 16 == 15 ? "\n" : "  ");
	}
	printf("Done.\n");
}

void system_initialize() {
	stdio_init_all();

	set_sys_clock_khz(150000, true);

	// Init UART
	uart_init(UART_ID, BAUD_RATE);
	gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
	printf("[picoclock] START PICO_RP2350A=%d\r\n", PICO_RP2350A);

	// Init I2C
	init_i2c();
	printf("[picoclock] init_i2c OK\r\n");
	//i2c_bus_scan();

	pcf8563_set_i2c(I2C_CHANNEL);
	printf("[picoclock] pcf8563_set_i2c OK\r\n");

	init_gpio();

	// Init LED controller
	sleep_ms(2);
	lp5817_init(I2C_CHANNEL);
	lp5817_enable(true);
	lp5817_set_max_current_code();
	lp5817_set_dot_current(0, 0xff);	// Blue
	lp5817_set_dot_current(1, 0xff);	// Red - Not working
	lp5817_set_dot_current(2, 0xff);	// Green
	lp5817_set_output_enable_all();
	lp5817_update();

	// Init Sound
	mcp4551_init(I2C_CHANNEL);
	mcp4551_set_wiper(MCP4551_WIPER_MID); // 0 => high vol, 0xff => low vol

	i2s_program_setup(pio0, audio_i2s_dma_irq_handler, &i2s, &config);
	audio_init(&audio_ctx);
	printf("[picoclock] init sound OK\r\n");

	// Negative delay means "run every X ms from the start of the last run"
	add_repeating_timer_ms(-10, timer_callback, NULL, &timer);

	// Init SDCARD
	gpio_init(SD_CARD_CS);
	gpio_put(SD_CARD_CS, 1);
	gpio_set_dir(SD_CARD_CS, GPIO_OUT);
	//gpio_init(SD_CARD_PRESENCE);
	//gpio_set_dir(SD_CARD_PRESENCE, GPIO_IN);

	spi_init(spi1, 1000 * 1000); // slow clock

	spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

	gpio_set_function(SDCARD_SCK, GPIO_FUNC_SPI);
	gpio_set_function(SDCARD_MOSI, GPIO_FUNC_SPI);
	gpio_set_function(SDCARD_MISO, GPIO_FUNC_SPI);
	printf("[picoclock] Init SD card OK\r\n");

	printf("[picoclock] System Clock: %lu\n", clock_get_hz(clk_sys));
}

void system_putc(char ch) {
	uart_putc_raw(UART_ID, ch);
}


// ----------------------------------------------------------------------------
// SDCARD HAL
// ----------------------------------------------------------------------------
void ost_hal_sdcard_set_slow_clock() {
	spi_set_baudrate(spi1, 1000000UL);
}
void ost_hal_sdcard_set_fast_clock() {
	spi_set_baudrate(spi1, 40000000UL);
}
void ost_hal_sdcard_cs_high() {
	gpio_put(SD_CARD_CS, 1);
}
void ost_hal_sdcard_cs_low() {
	gpio_put(SD_CARD_CS, 0);
}
void ost_hal_sdcard_spi_exchange(const uint8_t *buffer, uint8_t *out, uint32_t size) {
	spi_write_read_blocking(spi1, buffer, out, size);
}
void ost_hal_sdcard_spi_write(const uint8_t *buffer, uint32_t size) {
	spi_write_blocking(spi1, buffer, size);
}
void ost_hal_sdcard_spi_read(uint8_t *out, uint32_t size) {
	spi_read_blocking(spi1, 0xFF, out, size);
}
uint8_t ost_hal_sdcard_get_presence() {
	return 1; // TODO
}


void core1_entry() {
	int last_min = 0;
	int last_sync = -1;
	time_struct dt;

	printf("[core1] check RTC\r\n");
	dt = pcf8563_getDateTime();
	if (dt.volt_low) {
		request_remote_sync();
	}

	printf("[core1] start loop\r\n");
	while (1) {
		dt = pcf8563_getDateTime();
		//printf("[core1] got time:[%d] [%d:%d]\r\n", dt.volt_low, dt.hour, dt.min);
		if (!dt.volt_low) {
			// Check alarm
			if (dt.min != last_min) {
				if (check_alarm(dt) == 1) {
					ts_reset_alarm_screen = dt.hour * 3600 + dt.min * 60 + dt.sec + 300;  // To reset screen after a while

					// Set screen to alarm
					current_screen = SCREEN_ALARM;
					refresh_screen = true;
					refresh_screen_clear = true;
				}

				refresh_screen = true;
				refresh_screen_clear = false;

				last_min = dt.min;
			}

			// Trigger sync periodically
			int ts = dt.hour * 3600 + dt.min * 60 + dt.sec;
			if (last_sync == -1) {
				// First run, initialize as if sync nearly 1 hour ago
				last_sync = ts - 3600 + 30;
			}
			if ((((ts - last_sync) > 3600) || ((ts - last_sync) < 0)) && !audio_ctx.playing) {
				printf("ts:[%d] last_sync:[%d] ts - last_sync:[%d] => Trigger server sync\r\n", ts, last_sync, ts - last_sync);
				request_remote_sync();
				last_sync = ts;
				sprintf(last_sync_str, "%02d:%02d:%02d", dt.hour, dt.min, dt.sec);
				printf("sync trigger done\r\n");
			}

			// Reset alarm screen after a while
			if (ts_reset_alarm_screen > 0 && ts > ts_reset_alarm_screen) {
				printf("ts:[%d] ts_reset_alarm_screen:[%d] => Reset alarm screen\r\n", ts, ts_reset_alarm_screen);
				ts_reset_alarm_screen = 0;
				// Set screen back to normal mode
				current_screen = SCREEN_MAIN;
				// And force refresh
				refresh_screen = true;
				refresh_screen_clear = true;
			}
		}
		if (refresh_screen) {
			ui_refresh_screen(refresh_screen_clear);
			refresh_screen = false;
		}
		sleep_ms(100);
	}
}

// ===========================================================================================================
// MAIN ENTRY POINT
// ===========================================================================================================
int main() {
	// Call the platform initialization
	system_initialize();

	// Test the printf output
	printf("[picoclock] Starting: V%d.%d\r\n", 1, 0);

	// Init config with default values
	strncpy(global_config.wifi_ssid, WIFI_SSID, 50);
	strncpy(global_config.wifi_key, WIFI_PASSWORD, 50);
	strncpy(global_config.remote_host, SERVER_IP, 50);
	strncpy(global_config.screen, "4", 2); // "4" (color) or "B" (B/W)

	// Filesystem / SDCard initialization
	printf("[picoclock] Check SD card\r\n");
	filesystem_mount();
	// List files on sdcard (test)
	filesystem_read_config_file();

	// Init screen
	//------------------- Init LCD
	printf("[picoclock] Init e-Paper module...\r\n");
	if (DEV_Module_Init() == 0) {
		if (strcmp(global_config.screen, "B") == 0) {
			printf("[picoclock] e-Paper module: B\r\n");
			//EPD_2IN13BC_Init();
			EPD_2IN13B_V4_Init();
			printf("[picoclock] e-Paper module: B init OK\r\n");
			//EPD_2IN13BC_Clear();
			EPD_2IN13B_V4_Clear();
			printf("[picoclock] e-Paper module: B clear OK\r\n");
			DEV_Delay_ms(500);
		} else {
			printf("[picoclock] e-Paper module: V4\r\n");
			EPD_2in13_V4_Init();
			printf("[picoclock] e-Paper module: V4 init OK\r\n");
			EPD_2in13_V4_Clear();
			printf("[picoclock] e-Paper module: V4 clear OK\r\n");
		}
	}

	watchdog_enable(8200, false);
	printf("[picoclock] Init UI\r\n");
	init_ui();

	printf("[picoclock] UI: refresh screen\r\n");
	ui_refresh_screen(false);

	// Start the operating system!
	printf("[picoclock] Start\r\n");

	multicore_launch_core1(core1_entry);

	while (true) {
		//printf("[picoclock] In loop\r\n");
		if (request_btn_push >= 0) {
			ui_btn_click(request_btn_push);
			request_btn_push = -1;
		}
		uint64_t sleep_dur = 10000;
		if (audio_ctx.playing) {
			sleep_dur = 50;		// I2S audio data cannot wait
		}
		sleep_us(sleep_dur);
		if (request_audio_read > 0) {
			fs_audio_next_samples();
			request_audio_read = -1;
		}
		watchdog_update();
	}

	return 0;
}
