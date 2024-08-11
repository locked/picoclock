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

#include "net_task.h"


// ===========================================================================================================
// DEFINITIONS
// ===========================================================================================================

typedef struct
{
    uint8_t ev;
    uint32_t btn;
    char msg[30];
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
void UiTask(void *args)
{
    ost_ui_event_t *message = NULL;
    uint32_t res = 0;

    //double b = 2.0;
    //char buf[50];
    //floatToString(buf, b, 1);
    //debug_printf("TEST: %d %d [%s]\r\n", b, 2.0, buf);

    datetime_t dt;
    rtc_get_datetime(&dt);
    debug_printf("Hour:[%d] Min:[%d] Sec:[%d]\r\n", dt.hour, dt.min, dt.sec);

    PAINT_TIME sPaint_time;
    bool module_initialized = true;
    bool redraw_screen = false;
    bool need_screen_clear = false;
    int loop_count = 0;
    int last_min = 0;
    char temp_str[10];

    DEV_Module_Init();
    EPD_2in13_V4_Init();

    debug_printf("Paint_NewImage\r\n");
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

    while (1) {
        redraw_screen = false;

        res = qor_mbox_wait(&UiMailBox, (void **)&message, 5);
        if (res == QOR_MBOX_OK) {
            if (message->ev == OST_SYS_BUTTON) {
                if (message->btn == 1) {
                    debug_printf("/!\\ Received event OST_SYS_BUTTON1\r\n");
                    redraw_screen = true;
                }
                if (message->btn == 3) {
                    debug_printf("/!\\ Received event OST_SYS_BUTTON3\r\n");
                    static ost_net_event_t Ev = {
                        .ev = OST_SYS_PLAY_SOUND
                    };
                    qor_mbox_notify(&NetMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
                }
            }
        }

        mcp9808_get_temperature(temp_str);
        //debug_printf("Current temp:[%s]\r\n", temp_str);

        rtc_get_datetime(&dt);
        //debug_printf("rtc_get_datetime year:[%d]\r\n", dt.year);
        bool time_initialized = dt.year != 1000;
        if (!time_initialized) {
            Paint_DrawString_EN(10, 40, "Connecting...", &Font20, WHITE, BLACK);
            //EPD_2in13_V4_Display_Partial(BlackImage);
            EPD_2in13_V4_Display_Base(BlackImage);
            need_screen_clear = true;

            ost_request_update_time();
            redraw_screen = true;
        }
        if (time_initialized && dt.min != last_min) {
            static ost_net_event_t Ev = {
                .ev = OST_SYS_MINUTE_CHANGE
            };
            qor_mbox_notify(&NetMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
            redraw_screen = true;
            //last_sec = dt.sec;
            last_min = dt.min;
        }

        if (redraw_screen) {
            if (!module_initialized) {
                DEV_Module_Init();
                EPD_2in13_V4_Init();
            }

            //Paint_NewImage(BlackImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
            //Paint_SelectImage(BlackImage);
            //Paint_SetRotate(270);

            if (need_screen_clear) {
                EPD_2in13_V4_Clear();
                Paint_Clear(WHITE);
                need_screen_clear = false;
            }

            char date_str[100];
            if (time_initialized) {
                sPaint_time.Year = dt.year;
                sPaint_time.Month = dt.month;
                sPaint_time.Day = dt.day;
                sPaint_time.Hour = dt.hour;
                sPaint_time.Min = dt.min;
                sPaint_time.Sec = dt.sec;
                sprintf(date_str, "%d-%02d-%02d", dt.year, dt.month, dt.day);
                Paint_DrawString_EN(10, 10, date_str, &Font24, WHITE, BLACK);

                Paint_ClearWindows(10, 40, 150 + Font20.Width * 7, 80 + Font20.Height, WHITE);
                Paint_DrawTime(10, 40, &sPaint_time, &Font24, WHITE, BLACK);

                sprintf(date_str, "Temp: %s C", temp_str);
                Paint_DrawString_EN(10, 80, date_str, &Font12, WHITE, BLACK);

                //ost_get_wakeup_alarm(&wakeup_alarm);
                sprintf(date_str, "Alarm: %02d:%02d", wakeup_alarm.hour, wakeup_alarm.min);
                Paint_DrawString_EN(10, 100, date_str, &Font12, WHITE, BLACK);

                EPD_2in13_V4_Display_Partial(BlackImage);
            }

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
