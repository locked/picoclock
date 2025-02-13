#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
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

#include "net_task.h"
#include "fs_task.h"

#include "graphics.h"

#include <malloc.h>

#include "flash_storage.h"

#include "pwm_sound.h"

#include "audio_player.h"

#include "mcp9808/mcp9808.h"
#include "pcf8563/pcf8563.h"
#include "mcp23009/mcp23009.h"
#include "mcp46XX/mcp46XX.h"


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
extern wakeup_alarm_struct wakeup_alarms[10];
extern int wakeup_alarms_count;
extern weather_struct weather;
extern int current_screen;


//void get_next_alarm(wakeup_alarm_struct *next_alarm, time_struct *dt) {
//}


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
    int screen_x = 32;
    int icon_left_x = 0;
    int icon_right_x = 214;
    int last_sync = -1;
    int ts_reset_alarm_screen = 0;
    time_struct dt;

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

    printf("calling pcf8563_getDateTime()\r\n");
    dt = pcf8563_getDateTime();
    printf("GET EXTERNAL RTC volt_low:[%d] year:[%d] month:[%d] day:[%d] Hour:[%d] Min:[%d] Sec:[%d]\r\n", dt.volt_low, dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);

    bool time_initialized = !dt.volt_low;
    if (!time_initialized || dt.year == 0) {
        Paint_DrawString_EN(10, 40, "Connecting...", &Font20, WHITE, BLACK);
        EPD_2in13_V4_Display_Base(BlackImage);

        request_remote_sync();

        printf("[startup] calling pcf8563_getDateTime()\r\n");
        dt = pcf8563_getDateTime();
        printf("[startup] pcf8563_getDateTime() year:[%d]\r\n", dt.year);
    }
    Paint_Clear(WHITE);

    // Load alarms
    //flash_read_alarms(wakeup_alarms);

    while (1) {
        res = qor_mbox_wait(&UiMailBox, (void **)&message, 5);
        if (res == QOR_MBOX_OK) {
            // Buttons click
            if (message->ev == OST_SYS_BUTTON) {
                printf("/!\\ Received event OST_SYS_BUTTON:[%d]\r\n", message->btn);
                if (current_screen == SCREEN_ALARM && message->btn < 4) {
                    // In alarm
                    // Stop sound
                    ost_audio_stop();
                    //stop_melody();

                    dt = pcf8563_getDateTime();
                    ts_reset_alarm_screen = dt.hour * 3600 + dt.min * 60 + dt.sec + 2;  // Force reset in 2 s
                } else {
                    // Normal mode
                    if (message->btn == 0) {
                        if (current_screen < 3) {
                            if (backlight_on) {
#ifdef PCBV1
                                gpio_put(FRONT_PANEL_LED_PIN, 0);
#else
                                mcp23009_set(FRONT_PANEL_LED_PIN, 0);
#endif
                                backlight_on = false;
                            } else {
#ifdef PCBV1
                                gpio_put(FRONT_PANEL_LED_PIN, 1);
#else
                                mcp23009_set(FRONT_PANEL_LED_PIN, 1);
#endif
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
                            .clear = false
                        };
                        qor_mbox_notify(&UiMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
                    }
                }
                if (message->btn == 4) {
                    mcp4651_set_wiper(0x20);
                    //set_audio_volume_factor(40);

                    // Start sound
                    char SoundFile[260] = "Tellement.wav";
                    fs_task_sound_start(SoundFile);
                } else if (message->btn == 5) {
                    //ost_audio_stop();
                    mcp4651_set_wiper(0x80);
                    //set_audio_volume_factor(30);
                }
            }

            // Refresh screens
            if (message->ev == OST_SYS_REFRESH_SCREEN) {
                printf("/!\\ Received event OST_SYS_REFRESH_SCREEN:[%d] current_screen:[%d]\r\n", message->btn, current_screen);
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

                    printf("[in loop] calling pcf8563_getDateTime()\r\n");
                    dt = pcf8563_getDateTime();
                    printf("[in loop] pcf8563_getDateTime() year:[%d]\r\n", dt.year);
                    sPaint_time.Year = dt.year;
                    sPaint_time.Month = dt.month;
                    sPaint_time.Day = dt.day;
                    sPaint_time.Hour = dt.hour;
                    sPaint_time.Min = dt.min;
                    sPaint_time.Sec = dt.sec;
                    char weekday_str[3] = "";
                    getWeekdayStr(dt.weekday, weekday_str);
                    sprintf(temp_str, "%02d/%02d (%s)", dt.day, dt.month, weekday_str);
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font24, WHITE, BLACK);

                    _y += Font24.Height + 1;
                    Paint_ClearWindows(screen_x, _y, screen_x + Font24.Width * 8, 40 + Font24.Height, WHITE);
                    Paint_DrawTime(screen_x, _y, &sPaint_time, &Font24, WHITE, BLACK);

                    mcp9808_get_temperature(temp2_str);
                    sprintf(temp_str, "Temp: %s C", temp2_str);
                    _y += Font24.Height + 2;
                    Paint_ClearWindows(screen_x + Font12.Width * 6, _y, screen_x + Font12.Width * 12, 80 + Font12.Height, WHITE);
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font12, WHITE, BLACK);

                    // Next alarm
                    wakeup_alarm_struct *next_alarm;
                    //get_next_alarm(next_alarm, &dt);
                    next_alarm = &wakeup_alarms[0];
                    next_alarm->in_mins = 999999;
                    for (int i=0; i<wakeup_alarms_count; i++) {
                        int in_mins = 0;
                        if (wakeup_alarms[i].isset == 1) {
                            for (int chk_weekday=0; chk_weekday<7; chk_weekday++) {
                                if (wakeup_alarms[i].weekdays & (1 << (7 - chk_weekday))) {
                                    int alarm_min = wakeup_alarms[i].hour * 60 + wakeup_alarms[i].min;
                                    int dt_min = dt.hour * 60 + dt.min;
                                    if (dt.weekday == chk_weekday) {
                                        if (alarm_min > dt_min) {
                                            //printf("  alarm:[%d] alarm_min:[%d] > dt_min:[%d]\r\n", i, alarm_min, dt_min);
                                            in_mins = alarm_min - dt_min;
                                        } else if (alarm_min < dt_min) {
                                            //printf("  alarm:[%d] alarm_min:[%d] < dt_min:[%d]\r\n", i, alarm_min, dt_min);
                                            in_mins = 6 * 1440;
                                            in_mins += 1440 - dt_min;
                                            in_mins += alarm_min;
                                        }
                                    } else if (dt.weekday > chk_weekday) {
                                        in_mins = ((chk_weekday + 7 - dt.weekday) - 1) * 1440;    // a week after
                                        in_mins += 1440 - dt_min;
                                        in_mins += alarm_min;
                                    } else {
                                        in_mins = ((chk_weekday - dt.weekday) - 1) * 1440;
                                        in_mins += 1440 - dt_min;
                                        in_mins += alarm_min;
                                    }

                                    //printf("  alarm:[%d] chk_weekday:[%d] in_mins:[%d] next_alarm.in_mins:[%d]\r\n", i, chk_weekday, in_mins, next_alarm->in_mins);
                                    if (in_mins < next_alarm->in_mins) {
                                        //printf("  => OVERWRITE next_alarm with alarm:[%d]\r\n", i);
                                        next_alarm = &(wakeup_alarms[i]);
                                        next_alarm->in_mins = in_mins;
                                    }
                                }
                            }
                        }
                    }

                    _y += Font12.Height + 1;
                    if (next_alarm->in_mins != 999999) {
                        char weekdays_str[21] = "";
                        getWeekdaysStr(next_alarm->weekdays, weekdays_str);

                        sprintf(temp_str, "Alarm: %02d:%02d (%s)", next_alarm->hour, next_alarm->min, weekdays_str);
                        Paint_ClearWindows(screen_x + Font12.Width * 7, _y, screen_x + Font12.Width * 17, _y + Font12.Height, WHITE);
                        Paint_DrawString_EN(screen_x, _y, temp_str, &Font12, WHITE, BLACK);
                        _y += Font12.Height + 1;
                    }

                    // All alarms
                    int start_y = _y;
                    int shift_x = 0;
                    for (int i=0; i<wakeup_alarms_count; i++) {
                        if (wakeup_alarms[i].isset == 1) {
                            char weekdays_str[21] = "";
                            getWeekdaysStr(wakeup_alarms[i].weekdays, weekdays_str);
                            sprintf(temp_str, "%d:%d(%s)", wakeup_alarms[i].hour, wakeup_alarms[i].min, weekdays_str);
                            Paint_DrawString_EN(screen_x + shift_x, _y, temp_str, &Font12, WHITE, BLACK);
                            _y += Font12.Height;
                        }
                        if (_y > 120) {
                            _y = start_y;
                            shift_x = Font12.Width * 13;
                        }
                    }

                    if (message->clear) {
                        EPD_2in13_V4_Display_Base(BlackImage);
                        message->clear = false;
                    } else {
                        EPD_2in13_V4_Display_Partial(BlackImage);
                    }
                    //shutdown_screen = loop_count++ > 10;
                } else if (current_screen == SCREEN_WEATHER) {
                    display_icon(icon_right_x, 10, ICON_LIGHT);
                    display_icon(icon_right_x, 50, ICON_WIFI);

                    sprintf(temp_str, "Weather");
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font16, WHITE, BLACK);
                    _y += Font16.Height + 1;

                    //mcp9808_get_temperature(temp2_str);
                    //sprintf(temp_str, "Clock temp: %s C", temp2_str);
                    //Paint_DrawString_EN(screen_x, _y + Font12.Height * 0, temp_str, &Font12, WHITE, BLACK);

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
                } else if (current_screen == SCREEN_LIST_ALARMS) {
                    sprintf(temp_str, "Alarms");
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font16, WHITE, BLACK);
                    for (int i=0; i<wakeup_alarms_count; i++) {
                        if (wakeup_alarms[i].isset == 1) {
                            _y += Font16.Height + 1;
                            sprintf(temp_str, "[%d] %d:%d (%d)", wakeup_alarms[i].isset, wakeup_alarms[i].hour, wakeup_alarms[i].min, wakeup_alarms[i].weekdays);
                            Paint_DrawString_EN(screen_x, _y + Font12.Height * 0, temp_str, &Font12, WHITE, BLACK);
                        }
                    }
                    EPD_2in13_V4_Display_Base(BlackImage);
                    //shutdown_screen = true;
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
                    //shutdown_screen = true;
                } else if (current_screen == SCREEN_ALARM) {
                    sprintf(temp_str, "ALARM");
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font16, WHITE, BLACK);

                    dt = pcf8563_getDateTime();
                    _y += Font16.Height + 1;
                    sprintf(temp_str, "%02d:%02d", dt.hour, dt.min);
                    Paint_DrawString_EN(screen_x, _y, temp_str, &Font24, WHITE, BLACK);

                    EPD_2in13_V4_Display_Base(BlackImage);
                    //shutdown_screen = true;
                }
                shutdown_screen = loop_count++ > 10;

                // Shutdown screen periodically
                if (shutdown_screen) {
                    printf("Shutdown loop_count:[%d]\r\n", loop_count);
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
        dt = pcf8563_getDateTime();
        //debug_printf("GET EXTERNAL RTC volt_low:[%d] year:[%d] month:[%d] day:[%d] Hour:[%d] Min:[%d] Sec:[%d]\r\n", dt.volt_low, dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);

        time_initialized = !dt.volt_low;
        if (time_initialized) {
            if (dt.min != last_min) {
                // Minute change
                for (int i=0; i<wakeup_alarms_count; i++) {
                    wakeup_alarm_struct alarm = wakeup_alarms[i];
                    debug_printf("Check alarm:[%d] [%d:%d] vs [%d:%d]\r\n", i, alarm.hour, alarm.min, dt.hour, dt.min);
                    if (alarm.isset == 1 && alarm.hour == dt.hour && alarm.min == dt.min) {
                        // Check weekday
                        if (alarm.weekdays & (1 << (7 - dt.weekday))) {
                            debug_printf("Trigger alarm dt.weekday:[%d] alarm.weekdays:[%d]\r\n", dt.weekday, alarm.weekdays);

                            // Set volume
#ifndef PCBV1
                            mcp4651_set_wiper(0x100);
                            set_audio_volume_factor(40);
#endif

                            // Start sound
                            char SoundFile[260]; // = "Tellement.wav";
                            strncpy(SoundFile, alarm.chime, 20);
                            fs_task_sound_start(SoundFile);

                            ts_reset_alarm_screen = dt.hour * 3600 + dt.min * 60 + dt.sec + 300;  // To reset screen after a while

                            // Set screen to alarm
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

            //
            // Period tasks
            //
            // Trigger server sync
            if (((ts - last_sync) > 3600) || ((ts - last_sync) < 0)) {
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
                static ost_ui_event_t Ev = {
                    .ev = OST_SYS_REFRESH_SCREEN,
                    .clear = true
                };
                qor_mbox_notify(&UiMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
            }
        }

        qor_sleep(200);
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
