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
#include <hardware/sync.h>

#include "mcp9808/mcp9808.h"

#include "pcf8563/pcf8563.h"

#include "net_task.h"
#include "fs_task.h"

#include "graphics.h"

#include <malloc.h>

#include "flash_storage.h"

#include "pwm_sound.h"

#include "mcp23009/mcp23009.h"
#include "mcp4652/mcp4652.h"


uint32_t getTotalHeap(void) {
   extern char __StackLimit, __bss_end__;
   
   return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
   struct mallinfo m = mallinfo();

   return getTotalHeap() - m.uordblks;
}

// ===========================================================================================================
// DEFINITIONS
// ===========================================================================================================

typedef struct {
    uint8_t ev;
    uint32_t btn;
    bool clear;
} ost_ui_event_t;

// ===========================================================================================================
// GLOBAL VARIABLES
// ===========================================================================================================
static qor_tcb_t UiTcb;
static uint32_t UiStack[4096];
static qor_mbox_t UiMailBox;
static ost_ui_event_t UiEvent;
static ost_ui_event_t *UiQueue[10];
static ost_system_state_t OstState = OST_SYS_NO_EVENT;
static ost_context_t OstContext;

// Externs
extern qor_mbox_t NetMailBox;
extern wakeup_alarm_struct wakeup_alarms[4];
extern weather_struct weather;
extern int current_screen;


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
    bool backlight_on = false;
    char temp_str[100];
    char temp2_str[10];
    char last_sync_str[9] = "";
    int screen_x = 34;
    int icon_left_x = 0;
    int icon_right_x = 214;
    int last_sync = -1;

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

    // Load alarms
    //flash_read_alarms(wakeup_alarms);

    while (1) {
        res = qor_mbox_wait(&UiMailBox, (void **)&message, 5);
        if (res == QOR_MBOX_OK) {
            // Buttons click
            if (message->ev == OST_SYS_BUTTON) {
                debug_printf("/!\\ Received event OST_SYS_BUTTON:[%d]\r\n", message->btn);
                if (current_screen == SCREEN_ALARM) {
                    // In alarm
                    stop_melody();
                    current_screen = SCREEN_MAIN;
                    static ost_ui_event_t Ev = {
                        .ev = OST_SYS_REFRESH_SCREEN,
                        .clear = true
                    };
                    qor_mbox_notify(&UiMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
                } else {
                    // Sound tests:
                    char SoundFile[260] = "Tellement.wav";
                    if (message->btn == 5) {
                        fs_task_sound_start(SoundFile);
                    } else if (message->btn == 4) {
                        mcp23009_set(0, 0);
                    } else if (message->btn == 3) {
                        mcp23009_set(0, 1);
                    }
                    if (message->btn == 2) {
                        mcp4652_set_wiper(0x100);
                    } else if (message->btn == 1) {
                        mcp4652_set_wiper(0x80);
                    } else if (message->btn == 0) {
                        mcp4652_set_wiper(0x0);
                    }
                    // Normal mode
                    /*
                    if (message->btn == 0) {
                        if (current_screen < 3) {
                            if (backlight_on) {
                                gpio_put(FRONT_PANEL_LED_PIN, 0);
                                backlight_on = false;
                            } else {
                                gpio_put(FRONT_PANEL_LED_PIN, 1);
                                backlight_on = true;
                            }
                        } else if (current_screen == 3) {
                            request_play_song(0);
                        }
                    }
                    if (message->btn == 1) {
                        if (current_screen < 3) {
                            request_remote_sync();
                        } else if (current_screen == 3) {
                            request_play_song(1);
                        }
                    }
                    if (message->btn == 2 || message->btn == 3) {
                        current_screen += message->btn == 2 ? 1 : -1;
                        if (current_screen > MAX_SCREEN_ID) {
                            current_screen = 0;
                        }
                        if (current_screen < 0) {
                            current_screen = MAX_SCREEN_ID;
                        }
                        static ost_ui_event_t Ev = {
                            .ev = OST_SYS_REFRESH_SCREEN,
                            .clear = true
                        };
                        qor_mbox_notify(&UiMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
                    }
                    */
                }
            }

            // Refresh screens
            if (message->ev == OST_SYS_REFRESH_SCREEN) {
                debug_printf("/!\\ Received event OST_SYS_REFRESH_SCREEN:[%d] current_screen:[%d]\r\n", message->btn, current_screen);
                if (!module_initialized) {
                    EPD_2in13_V4_Init();
                    message->clear = true;
                    module_initialized = true;
                }
                if (message->clear) {
                    EPD_2in13_V4_Clear();
                    Paint_Clear(WHITE);
                }

                // Display screen X
                bool shutdown_screen = false;
                int _y = 8;
                display_icon(icon_left_x, 90, ICON_LEFTARROW);
                display_icon(icon_right_x, 90, ICON_RIGHTARROW);
                if (current_screen == SCREEN_MAIN) {
                    display_icon(icon_right_x, 10, ICON_LIGHT);
                    display_icon(icon_right_x, 50, ICON_WIFI);

                    time_struct dt = pcf8563_getDateTime();
                    sPaint_time.Year = dt.year;
                    sPaint_time.Month = dt.month;
                    sPaint_time.Day = dt.day;
                    sPaint_time.Hour = dt.hour;
                    sPaint_time.Min = dt.min;
                    sPaint_time.Sec = dt.sec;
                    sprintf(temp_str, "%02d/%02d (%d)", dt.day, dt.month, dt.weekday);
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font24, WHITE, BLACK);

                    _y += Font24.Height + 1;
                    Paint_ClearWindows(screen_x, _y, screen_x + Font24.Width * 8, 40 + Font24.Height, WHITE);
                    Paint_DrawTime(screen_x, 40, &sPaint_time, &Font24, WHITE, BLACK);

                    mcp9808_get_temperature(temp2_str);
                    sprintf(temp_str, "Temp: %s C", temp2_str);
                    _y += Font24.Height + 3;
                    Paint_ClearWindows(screen_x + Font12.Width * 6, _y, screen_x + Font12.Width * 12, 80 + Font12.Height, WHITE);
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font12, WHITE, BLACK);

                    //sprintf(temp_str, "Alarm: %02d:%02d (%d)", dt.alarm_hour, dt.alarm_min, dt.alarm_weekday);
                    //Paint_ClearWindows(screen_x + Font12.Width * 7, 100, screen_x + Font12.Width * 17, 100 + Font12.Height, WHITE);
                    //Paint_DrawString_EN(screen_x, 100, temp_str, &Font12, WHITE, BLACK);
                    for (int i=0; i<4; i++) {
                        sprintf(temp_str, "[%d] %d:%d (%d)", wakeup_alarms[i].isset, wakeup_alarms[i].hour, wakeup_alarms[i].min, wakeup_alarms[i].weekdays);
                        Paint_DrawString_EN(screen_x + Font12.Width * 12, _y, temp_str, &Font12, WHITE, BLACK);
                        _y += Font12.Height + 1;
                    }

                    if (message->clear) {
                        EPD_2in13_V4_Display_Base(BlackImage);
                        message->clear = false;
                    } else {
                        EPD_2in13_V4_Display_Partial(BlackImage);
                    }
                    shutdown_screen = loop_count++ > 10;
                } else if (current_screen == SCREEN_WEATHER) {
                    display_icon(icon_right_x, 10, ICON_LIGHT);
                    display_icon(icon_right_x, 50, ICON_WIFI);

                    sprintf(temp_str, "Weather");
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font16, WHITE, BLACK);
                    _y += Font16.Height + 1;

                    mcp9808_get_temperature(temp2_str);
                    sprintf(temp_str, "Clock temp: %s C", temp2_str);
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 0, temp_str, &Font12, WHITE, BLACK);

                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 1, weather.code_desc, &Font12, WHITE, BLACK);
                    sprintf(temp_str, "Temp: %s", weather.temperature_2m);
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 2, temp_str, &Font12, WHITE, BLACK);
                    sprintf(temp_str, "Wind: %s", weather.wind_speed_10m);
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 3, temp_str, &Font12, WHITE, BLACK);
                    sprintf(temp_str, "Precipitation: %s", weather.precipitation);
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 4, temp_str, &Font12, WHITE, BLACK);

                    sprintf(temp_str, " %02dC   %02dC   %02dC", weather.day_0_temp_min, weather.day_1_temp_min, weather.day_2_temp_min);
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 5, temp_str, &Font12, WHITE, BLACK);

                    sprintf(temp_str, " %02dC   %02dC   %02dC", weather.day_0_temp_max, weather.day_1_temp_max, weather.day_2_temp_max);
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 6, temp_str, &Font12, WHITE, BLACK);

                    sprintf(temp_str, "%s %s %s", weather.day_0_sunrise, weather.day_1_sunrise, weather.day_2_sunrise);
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 7, temp_str, &Font12, WHITE, BLACK);

                    EPD_2in13_V4_Display_Base(BlackImage);
                    shutdown_screen = loop_count++ > 10;
                } else if (current_screen == SCREEN_LIST_ALARMS) {
                    sprintf(temp_str, "Alarms");
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font16, WHITE, BLACK);
                    for (int i=0; i<4; i++) {
                        _y += Font16.Height + 1;
                        sprintf(temp_str, "[%d] %d:%d (%d)", wakeup_alarms[i].isset, wakeup_alarms[i].hour, wakeup_alarms[i].min, wakeup_alarms[i].weekdays);
                        Paint_DrawString_EN(screen_x, _y + Font12.Height * 0, temp_str, &Font12, WHITE, BLACK);
                    }
                    EPD_2in13_V4_Display_Base(BlackImage);
                    shutdown_screen = true;
                } else if (current_screen == SCREEN_DEBUG) {
                    sprintf(temp_str, "Debug");
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font16, WHITE, BLACK);

                    _y += Font16.Height + 1;
                    sprintf(temp_str, "ID: %d", get_uniq_id());
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 0, temp_str, &Font12, WHITE, BLACK);

                    sprintf(temp_str, "Last sync: %s", last_sync_str);
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 1, temp_str, &Font12, WHITE, BLACK);

                    sprintf(temp_str, "Mem(free/tot): %d/%d", getFreeHeap(), getTotalHeap());
                    Paint_DrawString_EN(screen_x, _y + Font12.Height * 2, temp_str, &Font12, WHITE, BLACK);

                    EPD_2in13_V4_Display_Base(BlackImage);
                    shutdown_screen = true;
                } else if (current_screen == SCREEN_ALARM) {
                    sprintf(temp_str, "ALARM");
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font16, WHITE, BLACK);

                    time_struct dt = pcf8563_getDateTime();
                    _y += Font16.Height + 1;
                    sprintf(temp_str, "%02d:%02d", dt.hour, dt.min);
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font24, WHITE, BLACK);

                    EPD_2in13_V4_Display_Base(BlackImage);
                    shutdown_screen = true;
                }

                // Shutdown screen periodically
                if (shutdown_screen) {
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
                for (int i=0; i<4; i++) {
                    wakeup_alarm_struct alarm = wakeup_alarms[i];
                    debug_printf("Check alarm:[%d] [%d:%d] vs [%d:%d]\r\n", i, alarm.hour, alarm.min, dt.hour, dt.min);
                    if (alarm.isset == 1 && alarm.hour == dt.hour && alarm.min == dt.min) {
                        // Check weekday
                        if (alarm.weekdays & (1 << (7 - dt.weekday))) {
                            debug_printf("Trigger alarm dt.weekday:[%d] alarm.weekdays:[%d]\r\n", dt.weekday, alarm.weekdays);
                            static ost_net_event_t NetEv = {
                                .ev = OST_SYS_ALARM
                            };
                            qor_mbox_notify(&NetMailBox, (void **)&NetEv, QOR_MBOX_OPTION_SEND_BACK);

                            current_screen = SCREEN_ALARM;
                            static ost_ui_event_t Ev = {
                                .ev = OST_SYS_REFRESH_SCREEN,
                                .clear = true
                            };
                            qor_mbox_notify(&UiMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
                        } else {
                            debug_printf("NO trigger alarm dt.weekday:[%d] alarm.weekdays:[%d]\r\n", dt.weekday, alarm.weekdays);
                        }
                    }
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
                printf("ts:[%d] last_sync:[%d] ts - last_sync:[%d] => Trigger server sync\r\n", ts, last_sync, ts - last_sync);
                request_remote_sync();
                last_sync = ts;
                sprintf(last_sync_str, "%02d:%02d:%02d", dt.hour, dt.min, dt.sec);
                printf("sync trigger done\r\n");
            }
        }

        qor_sleep(500);
        //printf("[UI] task woke up\r\n");
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
