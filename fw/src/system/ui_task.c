#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "debug.h"
#include "system.h"
#include "utils.h"
#include "alarms.h"
#include "graphics.h"

#include "net_task.h"
#include "fs_task.h"

// Screen
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"
#include "EPD_2in13b_V4.h"

#include "pico/util/datetime.h"
#include <hardware/sync.h>
#include "hardware/watchdog.h"

#include "audio_player.h"

#include "lp5817/lp5817.h"
#include "pcf8563/pcf8563.h"
#include "mcp46XX/mcp45XX.h"
#include "circularBuffer.h"


typedef struct {
    uint8_t ev;
    uint32_t btn;
    bool clear;
} ost_ui_event_t;

// GLOBAL VARIABLES
static ost_ui_event_t UiEvent;
static ost_ui_event_t *UiQueue[10];
static ost_system_state_t OstState = OST_SYS_NO_EVENT;
static ost_context_t OstContext;

// Externs
extern weather_struct weather;
extern int current_screen;
extern config_struct global_config;
extern int reboot_requested;
extern char last_sync_str[9];
extern bool refresh_screen;
extern bool refresh_screen_clear;
extern int ts_reset_alarm_screen;
extern bool sync_requested;
extern circularBuffer_t* ring_metrics;
extern char request_audio_start_file[260];
extern bool request_audio_start;

extern audio_ctx_t audio_ctx;

bool module_initialized = true;
bool need_screen_clear = true;
int loop_count = 0;
bool backlight_on = false;
UBYTE *BlackImage;

uint8_t current_rgb_color = 0;
uint8_t rgb_intensity[3] = {0, 0, 0};
uint8_t rgb_change[3] = {0, 0, 0};


// UI (user interface, buttons manager, LCD)
void init_ui() {
    // Create a new image cache
    UWORD Imagesize = ((EPD_2in13_V4_WIDTH % 8 == 0)? (EPD_2in13_V4_WIDTH / 8 ): (EPD_2in13_V4_WIDTH / 8 + 1)) * EPD_2in13_V4_HEIGHT;
    if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
      debug_printf("Failed to apply for black memory...\r\n");
      return;
    }
    Paint_NewImage(BlackImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
    Paint_SelectImage(BlackImage);
    Paint_Clear(WHITE);
    Paint_SetRotate(270);
    Paint_DrawString_EN(10, 10, "Starting...", &Font20, WHITE, BLACK);
    if (strcmp(global_config.screen, "B") == 0) {
        EPD_2IN13B_V4_DisplayNoColor(BlackImage);
    } else {
        EPD_2in13_V4_Display_Partial(BlackImage);
    }
}


void ui_refresh_screen(bool message_clear, time_struct dt) {
	printf("/!\\ Refresh screen:[%d]\r\n", current_screen);
	if (!module_initialized) {
		if (strcmp(global_config.screen, "B") == 0) {
			EPD_2IN13B_V4_Init();
		} else {
			EPD_2in13_V4_Init();
		}
		message_clear = true;
		module_initialized = true;
	}
	if (message_clear) {
		if (strcmp(global_config.screen, "B") == 0) {
			EPD_2IN13B_V4_Clear();
		} else {
			EPD_2in13_V4_Clear();
		}
	}
	Paint_Clear(WHITE);

	// Display screen X
	bool shutdown_screen = false;
	display_screen_nav();
	if (current_screen == SCREEN_MAIN) {
		display_screen_main(dt, ring_metrics);
	} else if (current_screen == SCREEN_WEATHER) {
		display_screen_weather();
	} else if (current_screen == SCREEN_LIST_ALARMS) {
		display_screen_alarms();
	} else if (current_screen == SCREEN_DEBUG) {
		display_screen_debug(last_sync_str, current_rgb_color, rgb_intensity);
	} else if (current_screen == SCREEN_ALARM) {
		display_screen_alarm();
	} else if (current_screen == SCREEN_METRIC) {
		display_screen_metrics(ring_metrics);
	}
	if (message_clear) {
		if (strcmp(global_config.screen, "B") == 0) {
			EPD_2IN13B_V4_DisplayNoColor(BlackImage);
		} else {
			EPD_2in13_V4_Display_Base(BlackImage);
		}
		message_clear = false;
	} else {
		if (strcmp(global_config.screen, "B") == 0) {
			EPD_2IN13B_V4_DisplayNoColor(BlackImage);
		} else {
			EPD_2in13_V4_Display_Partial(BlackImage);
		}
	}
	shutdown_screen = loop_count++ > 10;

	// Shutdown screen periodically
	if (shutdown_screen) {
		printf("Shutdown loop_count:[%d]\r\n", loop_count);
		loop_count = 0;
		if (strcmp(global_config.screen, "B") == 0) {
			EPD_2IN13B_V4_Sleep();
		} else {
			EPD_2in13_V4_Sleep();
		}
		DEV_Delay_ms(2000);//important, at least 2s
		DEV_Module_Exit();
		module_initialized = false;
	} else {
		module_initialized = true;
	}
}


void screen_anim() {
	if (current_screen == SCREEN_WEATHER) {
		if (rgb_intensity[0] > 250) {
			rgb_change[0] = -rand() % 4;
		} else if (rgb_intensity[0] < 5) {
			rgb_change[0] = rand() % 4;
		}
		if (rgb_intensity[1] > 250) {
			rgb_change[1] = -rand() % 4;
		} else if (rgb_intensity[1] < 5) {
			rgb_change[1] = rand() % 4;
		}
		if (rgb_intensity[2] > 250) {
			rgb_change[2] = -rand() % 4;
		} else if (rgb_intensity[2] < 5) {
			rgb_change[2] = rand() % 4;
		}
		for (uint8_t i = 0; i < 3; i++) {
			rgb_intensity[i] += rgb_change[i];
		}
		lp5817_turn_on(rgb_intensity[0], rgb_intensity[1], rgb_intensity[2]);
	}
}


void ui_btn_click(int btn, time_struct dt) {
	// On any button click, stop alarm
	if (current_screen == SCREEN_ALARM) {
		main_audio_stop();
		current_screen = SCREEN_MAIN;
		refresh_screen = true;
		return;
	}

	if (btn == 0) {
		//  *     [*]
		//  *      *
		//  *      *
		if (current_screen < 3) {
			if (backlight_on) {
				printf("Backlight OFF\r\n");
				gpio_put(FRONT_PANEL_LED_PIN, 0);
				lp5817_turn_off();
				backlight_on = false;
			} else {
				printf("Backlight ON\r\n");
				gpio_put(FRONT_PANEL_LED_PIN, 1);
				lp5817_turn_on(0x20, 0xff, 0x30);
				backlight_on = true;
			}
		} else if (current_screen == SCREEN_DEBUG) {
			rgb_intensity[current_rgb_color] += 20;
			lp5817_turn_on(rgb_intensity[0], rgb_intensity[1], rgb_intensity[2]);
			refresh_screen = true;
		}
	} else if (btn == 1) {
		//  *      *
		//  *     [*]
		//  *      *
		if (current_screen < 3) {
			sync_requested = true;
		} else if (current_screen == SCREEN_DEBUG) {
			rgb_intensity[current_rgb_color] -= 20;
			lp5817_turn_on(rgb_intensity[0], rgb_intensity[1], rgb_intensity[2]);
			refresh_screen = true;
		}
	} else if (btn == 2 || btn == 3) {
		//  *      *
		//  *      *
		// [*]    [*]
		current_screen += btn == 2 ? 1 : -1;
		if (current_screen > MAX_SCREEN_ID) {
			current_screen = 0;
		}
		if (current_screen < 0) {
			current_screen = MAX_SCREEN_ID;
		}
		if (current_screen == SCREEN_WEATHER) {
			// Init random rgb values
			rgb_intensity[0] = rand() % 255;
			rgb_intensity[1] = rand() % 255;
			rgb_intensity[2] = rand() % 255;
			rgb_change[0] = rand() % 2;
			rgb_change[1] = rand() % 3;
			rgb_change[2] = rand() % 4;
		}
		refresh_screen = true;
		refresh_screen_clear = false;
	} else if (btn == 4) {
		//  *      *
		// [*]     *
		//  *      *
		if (current_screen == SCREEN_DEBUG) {
			// Change led
			current_rgb_color++;
			if (current_rgb_color > 2) {
				current_rgb_color = 0;
			}
			refresh_screen = true;
		} else {
			if (gpio_get(I2S_SELECT_PIN) == 0) {
				gpio_put(I2S_SELECT_PIN, 1);	// 0 select I2S from pico, 1 from ESP32
				gpio_put(AUDIO_MUTE_PIN, 1);	// Unmute
				uart_puts(UART_ESP32_UART_ID, "START\n");
			} else {
				mcp4551_set_wiper(mcp4551_read_wiper() + 20);
			}
		}
	} else if (btn == 5) {
		// [*]     *
		//  *      *
		//  *      *
		if (current_screen == SCREEN_DEBUG) {
			// Change led
			if (current_rgb_color == 0) {
				current_rgb_color = 3;
			}
			current_rgb_color--;
			refresh_screen = true;
		} else {
			if (!audio_ctx.playing) {
				sprintf(request_audio_start_file, "Tellement.wav");
				request_audio_start = true;
			} else {
				mcp4551_set_wiper(mcp4551_read_wiper() - 20);
			}
		}
	}
}
