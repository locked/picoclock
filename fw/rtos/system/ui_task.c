#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "debug.h"
#include "qor.h"
#include "system.h"
#include "utils.h"
#include "alarms.h"
#include "graphics.h"

// Screen
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"

#include "hardware/rtc.h"
#include <hardware/sync.h>

#include "net_task.h"
#include "fs_task.h"

#include "pwm_sound.h"
#include "audio_player.h"

#include "pcf8563/pcf8563.h"
#include "mcp23009/mcp23009.h"
#include "mcp46XX/mcp46XX.h"


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
extern weather_struct weather;
extern int current_screen;




// ===========================================================================================================
// UI TASK (user interface, buttons manager, LCD)
// ===========================================================================================================
void UiTask(void *args) {
    ost_ui_event_t *message = NULL;
    uint32_t res = 0;

    bool module_initialized = true;
    bool need_screen_clear = true;
    int loop_count = 0;
    int last_min = 0;
    bool backlight_on = false;
    char last_sync_str[9] = "";
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
    EPD_2in13_V4_Display_Partial(BlackImage);

    printf("calling pcf8563_getDateTime()\r\n");
    dt = pcf8563_getDateTime();
    printf("GET EXTERNAL RTC volt_low:[%d] year:[%d] month:[%d] day:[%d] Hour:[%d] Min:[%d] Sec:[%d]\r\n", dt.volt_low, dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);

    bool time_initialized = !dt.volt_low;
    if (!time_initialized || dt.year == 0) {
        Paint_DrawString_EN(10, 40, "Connecting...", &Font20, WHITE, BLACK);
        EPD_2in13_V4_Display_Partial(BlackImage);

        request_remote_sync();

        printf("[startup] calling pcf8563_getDateTime()\r\n");
        dt = pcf8563_getDateTime();
        printf("[startup] pcf8563_getDateTime() year:[%d]\r\n", dt.year);
    }
    Paint_Clear(WHITE);

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
                                mcp23009_set(FRONT_PANEL_LED_PIN, 0);
                                backlight_on = false;
                            } else {
                                mcp23009_set(FRONT_PANEL_LED_PIN, 1);
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
                    //set_audio_volume_factor(1);

                    // Start sound
                    char SoundFile[260] = "Tellement.wav";
                    fs_task_sound_start(SoundFile);
                } else if (message->btn == 5) {
                    ost_audio_stop();
                    mcp4651_set_wiper(0xff);
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
                }
                Paint_Clear(WHITE);

                // Display screen X
                bool shutdown_screen = false;
                display_screen_nav();
                if (current_screen == SCREEN_MAIN) {
                    display_screen_main();
                } else if (current_screen == SCREEN_WEATHER) {
                    display_screen_weather();
                } else if (current_screen == SCREEN_LIST_ALARMS) {
                    display_screen_alarms();
                } else if (current_screen == SCREEN_DEBUG) {
                    display_screen_debug(last_sync_str);
                } else if (current_screen == SCREEN_ALARM) {
                    display_screen_alarm();
                }
                if (message->clear) {
                    EPD_2in13_V4_Display_Base(BlackImage);
                    message->clear = false;
                } else {
                    EPD_2in13_V4_Display_Partial(BlackImage);
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

        dt = pcf8563_getDateTime();
        //debug_printf("GET EXTERNAL RTC volt_low:[%d] year:[%d] month:[%d] day:[%d] Hour:[%d] Min:[%d] Sec:[%d]\r\n", dt.volt_low, dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);

        time_initialized = !dt.volt_low;
        if (time_initialized) {
            if (dt.min != last_min) {
                if (check_alarm(dt) == 1) {
                    ts_reset_alarm_screen = dt.hour * 3600 + dt.min * 60 + dt.sec + 300;  // To reset screen after a while

                    // Set screen to alarm
                    current_screen = SCREEN_ALARM;
                    static ost_ui_event_t Ev = {
                        .ev = OST_SYS_REFRESH_SCREEN,
                        .clear = true
                    };
                    qor_mbox_notify(&UiMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
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
