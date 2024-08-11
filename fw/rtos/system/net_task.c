/**
 * @file net_task.c
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

#include "hardware/rtc.h"
#include "hardware/clocks.h"

#include "tiny-json.h"

#include "wifi.h"

#include "pwm_sound.h"
#include "pwm_sound_melodies.h"



// ===========================================================================================================
// DEFINITIONS
// ===========================================================================================================

// ===========================================================================================================
// GLOBAL STORY VARIABLES
// ===========================================================================================================

static qor_tcb_t NetTcb;
static uint32_t NetStack[4096];

extern qor_mbox_t NetMailBox;
extern wakeup_alarm_struct wakeup_alarm;

static ost_net_event_t NetEvent;

static ost_net_event_t *NetQueue[10];

static ost_system_state_t OstState = OST_SYS_NO_EVENT;

static ost_context_t OstContext;


int get_response_from_server(char *response) {
	debug_printf("Connecting to wifi...\r\n");
	int ret = wifi_connect(WIFI_SSID, WIFI_PASSWORD);
    if (ret == 0) {
        ret = send_tcp(SERVER_IP, atoi(SERVER_PORT), "\n", 1, response);
        debug_printf("RESPONSE FROM SERVER ret:[%d] Server:[%s:%d] => [%s]\n", ret, SERVER_IP, atoi(SERVER_PORT), response);
        if (ret != 0) {
            return ret;
        }
    }
    wifi_disconnect();
    debug_printf("Disconnected from wifi, ret:[%d]\n", ret);
    return ret;
}

void set_date_from_response(char *response) {
	enum { MAX_FIELDS = 20 };
	json_t pool[MAX_FIELDS];

	json_t const* root_elem = json_create(response, pool, MAX_FIELDS);
	json_t const* status_elem = json_getProperty(root_elem, "status");
	int status = json_getInteger(status_elem);
	debug_printf("Parsing date from [%s]...\r\n", response);
	json_t const* date_elem = json_getProperty(root_elem, "date");
	json_t const* year_elem = json_getProperty(date_elem, "year");
	int year_full = json_getInteger(year_elem);
	/*
	int year;
	bool century;
	if (year_full >= 2000) {
	year = year_full - 2000;
	century = false;
	} else {
	year = year_full - 1900;
	century = true;
	}
	*/
	json_t const* day_elem = json_getProperty(date_elem, "day");
	datetime_t dt;
	dt.year = year_full;
	dt.month = json_getInteger(json_getProperty(date_elem, "month"));
	dt.day = json_getInteger(day_elem);
	dt.dotw = json_getInteger(json_getProperty(date_elem, "weekday"));
	dt.hour = json_getInteger(json_getProperty(date_elem, "hour"));
	dt.min = json_getInteger(json_getProperty(date_elem, "minute"));
	dt.sec = json_getInteger(json_getProperty(date_elem, "second"));
    debug_printf("rtc_set_datetime SET year:[%d]\r\n", dt.year);
	rtc_set_datetime(&dt);
	// Save alarm
	json_t const* wakeup_alarm_elem = json_getProperty(root_elem, "wakeup_alarm");
	wakeup_alarm.isset = json_getInteger(json_getProperty(wakeup_alarm_elem, "isset"));
	wakeup_alarm.dotw = json_getInteger(json_getProperty(wakeup_alarm_elem, "weekday"));
	wakeup_alarm.hour = json_getInteger(json_getProperty(wakeup_alarm_elem, "hour"));
	wakeup_alarm.min = json_getInteger(json_getProperty(wakeup_alarm_elem, "minute"));
	debug_printf("STATUS FROM SERVER:[%d] year:[%d] day:[%d] weekday:[%d] month:[%d] hour:[%d] wakeup_alarm:[%02d:%02d]\r\n", status, dt.year, dt.day, dt.dotw, dt.month, dt.hour, wakeup_alarm.hour, wakeup_alarm.min);
}

// ===========================================================================================================
// Network TASK (wifi, alarm)
// ===========================================================================================================
void NetTask(void *args)
{
    ost_net_event_t *message = NULL;
    uint32_t res = 0;

    bool time_initialized = false;
    datetime_t dt;

    while (1) {
        res = qor_mbox_wait(&NetMailBox, (void **)&message, 5);
        if (res == QOR_MBOX_OK) {
            if (message->ev == OST_SYS_MINUTE_CHANGE) {
                debug_printf("\r\n[NET] OST_SYS_MINUTE_CHANGE\r\n");
                rtc_get_datetime(&dt);
                if (dt.hour == wakeup_alarm.hour && dt.min == wakeup_alarm.min) {
                    play_melody(16, HarryPotter, 200);
                }
            }

            if (message->ev == OST_SYS_UPDATE_TIME) {
                debug_printf("\r\n[NET] OST_SYS_UPDATE_TIME\r\n");
                char response[300] = "";
                if (get_response_from_server(response) == 0) {
                    set_date_from_response(response);
                }
            }

            if (message->ev == OST_SYS_PLAY_SOUND) {
                debug_printf("\r\n[NET] OST_SYS_PLAY_SOUND\r\n");
                play_melody(16, HappyBirday, 140);
                //play_melody(16, HarryPotter, 140);
            }
        }

        qor_sleep(5000);
        debug_printf("\r\n[NET] task woke up\r\n");
    }
}

/*void ost_get_wakeup_alarm(wakeup_alarm_struct *wa) {
	wa->hour = wakeup_alarm.hour;
	wa->min = wakeup_alarm.min;
}*/

void ost_request_update_time() {
	// Notify for wifi update
    static ost_net_event_t UpdateTimeEv = {
        .ev = OST_SYS_UPDATE_TIME
    };
    qor_mbox_notify(&NetMailBox, (void **)&UpdateTimeEv, QOR_MBOX_OPTION_SEND_BACK);
}

void net_task_initialize() {
    OstState = OST_SYS_NO_EVENT;
    qor_mbox_init(&NetMailBox, (void **)&NetQueue, 10);

    qor_create_thread(&NetTcb, NetTask, NetStack, sizeof(NetStack) / sizeof(NetStack[0]), NET_TASK_PRIORITY, "NetTask");
}
