/**
 * @file ui_task.c
 *
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-07-29
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "ost_hal.h"
#include "debug.h"
#include "qor.h"
#include "system.h"

// Screen
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"

#include "hardware/rtc.h"

//#include "pwm_sound.h"
//#include "pwm_sound_melodies.h"

#include "mcp9808/mcp9808.h"

#include "pcf8563/pcf8563.h"

#include "net_task.h"

#include "graphics.h"


// ===========================================================================================================
// DEFINITIONS
// ===========================================================================================================

typedef struct {
    uint8_t ev;
    uint32_t btn;
    bool clear;
} ost_ui_event_t;

// ===========================================================================================================
// GLOBAL STORY VARIABLES
// ===========================================================================================================

static qor_tcb_t UiTcb;
static uint32_t UiStack[4096];

static qor_mbox_t UiMailBox;
extern qor_mbox_t NetMailBox;
extern wakeup_alarm_struct wakeup_alarm;

static ost_ui_event_t UiEvent;

static ost_ui_event_t *UiQueue[10];

static ost_system_state_t OstState = OST_SYS_NO_EVENT;

static ost_context_t OstContext;


// ===========================================================================================================
// UI TASK (user interface, buttons manager, LCD)
// ===========================================================================================================
void UiTask(void *args) {
    ost_ui_event_t *message = NULL;
    uint32_t res = 0;

    PAINT_TIME sPaint_time;
    bool module_initialized = true;
    bool need_screen_clear = true;
    int loop_count = 0;
    int last_min = 0;
    int current_screen = 0;
    int max_screen = 1;
    bool backlight_on = false;
    char temp_str[100];
    char temp2_str[10];

    DEV_Module_Init();
    EPD_2in13_V4_Init();

    // Create a new image cache
    UBYTE *BlackImage;
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
    EPD_2in13_V4_Display_Base(BlackImage);

    time_struct dt = pcf8563_getDateTime();
    debug_printf("GET EXTERNAL RTC volt_low:[%d] year:[%d] month:[%d] day:[%d] Hour:[%d] Min:[%d] Sec:[%d]\r\n", dt.volt_low, dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);

    bool time_initialized = !dt.volt_low;
    if (!time_initialized) {
        Paint_DrawString_EN(10, 40, "Connecting...", &Font20, WHITE, BLACK);
        EPD_2in13_V4_Display_Base(BlackImage);

        request_remote_sync();
    }
    Paint_Clear(WHITE);

    int screen_x = 34;
    int icon_left_x = 0;
    int icon_right_x = 214;
    int last_sync = -1;

    while (1) {
        res = qor_mbox_wait(&UiMailBox, (void **)&message, 5);
        if (res == QOR_MBOX_OK) {
            // Buttons click
            if (message->ev == OST_SYS_BUTTON) {
                debug_printf("/!\\ Received event OST_SYS_BUTTON:[%d]\r\n", message->btn);
                if (message->btn == 0) {
                    if (current_screen == 0) {
                        if (backlight_on) {
                            gpio_put(FRONT_PANEL_LED_PIN, 0);
                            backlight_on = false;
                        } else {
                            gpio_put(FRONT_PANEL_LED_PIN, 1);
                            backlight_on = true;
                        }
                    } else if (current_screen == 1) {
                        request_play_song(0);
                    }
                }
                if (message->btn == 1) {
                    if (current_screen == 0) {
                        request_remote_sync();
                    } else if (current_screen == 1) {
                        request_play_song(1);
                    }
                }
                if (message->btn == 2 || message->btn == 3) {
                    current_screen += message->btn == 2 ? 1 : -1;
                    if (current_screen > max_screen) {
                        current_screen = 0;
                    }
                    if (current_screen < 0) {
                        current_screen = max_screen;
                    }
                    static ost_ui_event_t Ev = {
                        .ev = OST_SYS_REFRESH_SCREEN,
                        .clear = true
                    };
                    qor_mbox_notify(&UiMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
                }
            }

            // Refresh screens
            if (message->ev == OST_SYS_REFRESH_SCREEN) {
                debug_printf("/!\\ Received event OST_SYS_REFRESH_SCREEN:[%d] current_screen:[%d]\r\n", message->btn, current_screen);
                if (!module_initialized) {
                    DEV_Module_Init();
                    EPD_2in13_V4_Init();
                    message->clear = true;
                    module_initialized = true;
                }
                if (message->clear) {
                    EPD_2in13_V4_Clear();
                    Paint_Clear(WHITE);
                }

                // Display screen X
                if (current_screen == 0) {
                    display_icon(icon_left_x, 90, ICON_LEFTARROW);
                    display_icon(icon_right_x, 10, ICON_LIGHT);
                    display_icon(icon_right_x, 50, ICON_WIFI);
                    display_icon(icon_right_x, 90, ICON_RIGHTARROW);

                    time_struct dt = pcf8563_getDateTime();
                    sPaint_time.Year = dt.year;
                    sPaint_time.Month = dt.month;
                    sPaint_time.Day = dt.day;
                    sPaint_time.Hour = dt.hour;
                    sPaint_time.Min = dt.min;
                    sPaint_time.Sec = dt.sec;
                    sprintf(temp_str, "%d-%02d-%02d", dt.year, dt.month, dt.day);
                    Paint_DrawString_EN(screen_x, 10, temp_str, &Font24, WHITE, BLACK);

                    Paint_ClearWindows(screen_x, 40, screen_x + Font24.Width * 8, 40 + Font24.Height, WHITE);
                    Paint_DrawTime(screen_x, 40, &sPaint_time, &Font24, WHITE, BLACK);

                    mcp9808_get_temperature(temp2_str);
                    sprintf(temp_str, "Temp: %s C", temp2_str);
                    Paint_ClearWindows(screen_x + Font12.Width * 6, 80, screen_x + Font12.Width * 12, 80 + Font12.Height, WHITE);
                    Paint_DrawString_EN(screen_x, 80, temp_str, &Font12, WHITE, BLACK);

                    //ost_get_wakeup_alarm(&wakeup_alarm);
                    sprintf(temp_str, "Alarm: %02d:%02d (%d)", dt.alarm_hour, dt.alarm_min, dt.alarm_weekday);
                    Paint_ClearWindows(screen_x + Font12.Width * 7, 100, screen_x + Font12.Width * 17, 100 + Font12.Height, WHITE);
                    Paint_DrawString_EN(screen_x, 100, temp_str, &Font12, WHITE, BLACK);

                    if (message->clear) {
                        EPD_2in13_V4_Display_Base(BlackImage);
                        message->clear = false;
                    } else {
                        EPD_2in13_V4_Display_Partial(BlackImage);
                    }
                } else if (current_screen == 1) {
                    sprintf(temp_str, "Weather");
                    Paint_DrawString_EN(screen_x, 10, temp_str, &Font24, WHITE, BLACK);

                    mcp9808_get_temperature(temp2_str);
                    sprintf(temp_str, "Temp: %s C", temp2_str);
                    Paint_DrawString_EN(screen_x, 80, temp_str, &Font12, WHITE, BLACK);

                    EPD_2in13_V4_Display_Base(BlackImage);
                }

                // Shutdown screen periodically
                if (loop_count++ > 10) {
                    debug_printf("Shutdown loop_count:[%d]\r\n", loop_count);
                    loop_count = 0;
                    EPD_2in13_V4_Sleep();
                    DEV_Delay_ms(2000);//important, at least 2s
                    DEV_Module_Exit();
                    module_initialized = false;
                } else {
                    module_initialized = true;
                }
            }
        }

        //rtc_get_datetime(&dt);
        time_struct dt = pcf8563_getDateTime();
        //debug_printf("GET EXTERNAL RTC volt_low:[%d] year:[%d] month:[%d] day:[%d] Hour:[%d] Min:[%d] Sec:[%d]\r\n", dt.volt_low, dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);

        time_initialized = !dt.volt_low;
        if (time_initialized) {
            if (dt.min != last_min) {
                // Minute change
                if (dt.hour == dt.alarm_hour && dt.min == dt.alarm_min) {
                    static ost_net_event_t NetEv = {
                        .ev = OST_SYS_ALARM
                    };
                    qor_mbox_notify(&NetMailBox, (void **)&NetEv, QOR_MBOX_OPTION_SEND_BACK);
                }

                static ost_ui_event_t UiEv = {
                    .ev = OST_SYS_REFRESH_SCREEN,
                    .clear = false
                };
                qor_mbox_notify(&UiMailBox, (void **)&UiEv, QOR_MBOX_OPTION_SEND_BACK);

                last_min = dt.min;
            }
            int ts = dt.hour * 3600 + dt.min * 60 + dt.sec;
            if (last_sync == -1) {
                // First run, initialize as if sync nearly 1 hour ago
                last_sync = ts - 3600 + 10;
            }
            if (((ts - last_sync) > 3600) || ((ts - last_sync) < 0)) {
                // Trigger server sync
                debug_printf("ts:[%d] last_sync:[%d] ts - last_sync:[%d] => Trigger server sync\r\n", ts, last_sync, ts - last_sync);
                request_remote_sync();
                last_sync = ts;
            }
        }

        qor_sleep(500);
        //debug_printf("\r\n[UI] task woke up\r\n");
    }
}


static void button_callback(uint32_t btn) {
    static ost_ui_event_t ButtonEv = {
        .ev = OST_SYS_BUTTON
    };
    ButtonEv.btn = btn;
    qor_mbox_notify(&UiMailBox, (void **)&ButtonEv, QOR_MBOX_OPTION_SEND_BACK);
}


void ui_task_initialize() {
    OstState = OST_SYS_NO_EVENT;
    qor_mbox_init(&UiMailBox, (void **)&UiQueue, 10);

    qor_create_thread(&UiTcb, UiTask, UiStack, sizeof(UiStack) / sizeof(UiStack[0]), HMI_TASK_PRIORITY, "UiTask");

    ost_button_register_callback(button_callback);
}
