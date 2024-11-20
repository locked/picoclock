#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "debug.h"
#include "qor.h"
#include "system.h"

#include "hardware/rtc.h"
#include "hardware/clocks.h"

#include "tiny-json.h"

#include "wifi.h"

#include "pwm_sound.h"
#include "pwm_sound_melodies.h"

#include "pcf8563/pcf8563.h"

#include "tinyhttp/http.h"

#include "flash_storage.h"



static qor_tcb_t NetTcb;
static uint32_t NetStack[4096];
static ost_net_event_t NetEvent;
static ost_net_event_t *NetQueue[10];
static ost_system_state_t OstState = OST_SYS_NO_EVENT;
static ost_context_t OstContext;

// Externs
extern qor_mbox_t NetMailBox;
extern wakeup_alarm_struct wakeup_alarms[10];
extern int wakeup_alarms_count;
extern weather_struct weather;


int get_response_from_server(char *response) {
	//printf("Sleep a bit [0]...\r\n");
    qor_sleep(10);
	//printf("Connecting to wifi...\r\n");
	int ret = wifi_connect(WIFI_SSID, WIFI_PASSWORD);
	//printf("Sleep a bit [1]...\r\n");
    qor_sleep(10);
    if (ret == 0) {
        char query[100] = "GET /clock.php HTTP/1.0\r\nContent-Length: 0\r\n\r\n";
        ret = send_tcp(SERVER_IP, atoi(SERVER_PORT), query, strlen(query), response);
        qor_sleep(10);
        //debug_printf("RESPONSE FROM SERVER ret:[%d] Server:[%s:%d] => [%s]\r\n", ret, SERVER_IP, atoi(SERVER_PORT), response);
        if (ret == 0 && strlen(response) < 10) {
            ret = 1;
        }
    }
	printf("Disconnect...\r\n");
    wifi_disconnect();
    qor_sleep(10);
    printf("Disconnected from wifi, ret:[%d]\r\n", ret);
    return ret;
}

// HTTP lib
// Response data/funcs
struct HttpResponse {
	char body[4096];
    int code;
};
static void* response_realloc(void* opaque, void* ptr, int size) {
    return realloc(ptr, size);
}
static void response_body(void* opaque, const char* data, int size) {
    struct HttpResponse* response = (struct HttpResponse*)opaque;
    //debug_printf("[NET] ADD [%d] BYTES TO BODY:[%s]\r\n", size, data);
    strncpy(response->body, data, size+1);
}
static void response_header(void* opaque, const char* ckey, int nkey, const char* cvalue, int nvalue) {
}
static void response_code(void* opaque, int code) {
    struct HttpResponse* response = (struct HttpResponse*)opaque;
    //debug_printf("[NET] SET CODE [%d]\r\n", code);
    response->code = code;
}
static const struct http_funcs responseFuncs = {
    response_realloc,
    response_body,
    response_header,
    response_code,
};

int parse_http_response(char *http_response, struct HttpResponse *response) {
    debug_printf("[NET] PARSE HTTP:[%s]\r\n", http_response);

    //struct HttpResponse* response = (struct HttpResponse*)opaque;
    response->code = 0;

    struct http_roundtripper rt;
    http_init(&rt, responseFuncs, response);
    //debug_printf("[NET] PARSE HTTP: INIT OK\r\n");
    int read;
    int ndata = strlen(http_response);
    http_data(&rt, http_response, ndata, &read);
    //debug_printf("[NET] PARSE HTTP: http_data OK\r\n");

    http_free(&rt);

    if (http_iserror(&rt)) {
        debug_printf("Error parsing data\r\n");
        return -1;
    }

    if (strlen(response->body) > 0) {
        debug_printf("CODE:[%d] BODY:[%s]\r\n", response->code, response->body);
        return 0;
    }

    return -1;
}

int parse_json_response(char *response) {
    debug_printf("[NET] PARSE JSON:[%s]\r\n", response);
	enum { MAX_FIELDS = 100 };
	json_t pool[MAX_FIELDS];

	json_t const* root_elem = json_create(response, pool, MAX_FIELDS);
    if (root_elem == NULL) {
        debug_printf("[NET] Error parsing json: root_elem not found\r\n");
        return -1;
    }
    json_t const* status_elem = json_getProperty(root_elem, "status");
    if (status_elem == NULL) {
        debug_printf("[NET] Error parsing json: status_elem not found\r\n");
        return -1;
    }
	int status = json_getInteger(status_elem);
	debug_printf("Parsing date from [%s]...\r\n", response);
	json_t const* date_elem = json_getProperty(root_elem, "date");
    if (date_elem == NULL) {
        debug_printf("[NET] Error parsing json: date_elem not found\r\n");
        return -1;
    }
	json_t const* year_elem = json_getProperty(date_elem, "year");
    if (year_elem == NULL) {
        debug_printf("[NET] Error parsing json: year_elem not found in date\r\n");
        return -1;
    }
	int year_full = json_getInteger(year_elem);
	int year;
	bool century;
	if (year_full >= 2000) {
        year = year_full - 2000;
        century = false;
	} else {
        year = year_full - 1900;
        century = true;
	}
	json_t const* day_elem = json_getProperty(date_elem, "day");
	datetime_t dt;
	dt.year = year_full;
	dt.month = json_getInteger(json_getProperty(date_elem, "month"));
	dt.day = json_getInteger(day_elem);
	dt.dotw = json_getInteger(json_getProperty(date_elem, "weekday"));
	dt.hour = json_getInteger(json_getProperty(date_elem, "hour"));
	dt.min = json_getInteger(json_getProperty(date_elem, "minute"));
	dt.sec = json_getInteger(json_getProperty(date_elem, "second"));
    //debug_printf("SET in INTERNAL + EXTERNAL RTC year:[%d]\r\n", dt.year);
	rtc_set_datetime(&dt);
    pcf8563_setDateTime(dt.day, dt.dotw, dt.month, century, year, dt.hour, dt.min, dt.sec);

	// Save alarms
	json_t const* wakeup_alarms_elem = json_getProperty(root_elem, "wakeup_alarms");
    if (wakeup_alarms_elem != NULL) {
        // Store each alarms
        json_t const* alarm_child;
        int tmp;
        bool wakeup_alarms_changed = false;
        alarm_child = json_getChild(wakeup_alarms_elem);
        for (int i=0; i<wakeup_alarms_count; i++) {
            if (alarm_child == NULL) {
                // Clear alarms
                if (wakeup_alarms[i].isset != 0 || wakeup_alarms[i].weekdays != 0 || wakeup_alarms[i].hour != 0 || wakeup_alarms[i].min != 0) {
                    wakeup_alarms_changed = true;
                }
                wakeup_alarms[i].isset = 0;
                wakeup_alarms[i].weekdays = 0;
                wakeup_alarms[i].hour = 0;
                wakeup_alarms[i].min = 0;
            } else {
                tmp = json_getInteger(json_getProperty(alarm_child, "isset"));
                if (tmp != wakeup_alarms[i].isset) {
                    wakeup_alarms[i].isset = tmp;
                    wakeup_alarms_changed = true;
                }
                tmp = json_getInteger(json_getProperty(alarm_child, "weekdays"));
                if (tmp != wakeup_alarms[i].weekdays) {
                    wakeup_alarms[i].weekdays = tmp;
                    wakeup_alarms_changed = true;
                }
                tmp = json_getInteger(json_getProperty(alarm_child, "hour"));
                if (tmp != wakeup_alarms[i].hour) {
                    wakeup_alarms[i].hour = tmp;
                    wakeup_alarms_changed = true;
                }
                tmp = json_getInteger(json_getProperty(alarm_child, "minute"));
                if (tmp != wakeup_alarms[i].min) {
                    wakeup_alarms[i].min = tmp;
                    wakeup_alarms_changed = true;
                }

                alarm_child = json_getSibling(alarm_child);
            }
        }
        if (wakeup_alarms_changed) {
            // Decommenting this cause qor_sleep() to hang (even if not called)
            //flash_store_alarms(wakeup_alarms);
        }
    }

    // Save weather
    // https://open-meteo.com/en/docs#current=precipitation,weather_code&hourly=&daily=temperature_2m_max,temperature_2m_min,sunrise,sunset,precipitation_sum
	json_t const* weather_json = json_getProperty(root_elem, "weather");
    if (weather_json != NULL) {
        json_t const* current_weather_json = json_getProperty(weather_json, "current");
        sprintf(weather.code_desc, json_getPropertyValue(current_weather_json, "code_desc"));
        sprintf(weather.temperature_2m, json_getPropertyValue(current_weather_json, "temperature_2m"));
        sprintf(weather.wind_speed_10m, json_getPropertyValue(current_weather_json, "wind_speed_10m"));
        sprintf(weather.precipitation, json_getPropertyValue(current_weather_json, "precipitation"));

        json_t const* daily_weather_json = json_getProperty(weather_json, "daily");
        if (daily_weather_json != NULL) {
            json_t const* tmp_json;
            json_t const* child;
            tmp_json = json_getProperty(daily_weather_json, "precipitation_sum");
            child = json_getChild(tmp_json);
            weather.day_0_precip_sum = (int)json_getReal(child);

            tmp_json = json_getProperty(daily_weather_json, "temperature_2m_min");
            child = json_getChild(tmp_json);
            weather.day_0_temp_min = (int)json_getReal(child);
            child = json_getSibling(child);
            weather.day_1_temp_min = (int)json_getReal(child);
            child = json_getSibling(child);
            weather.day_2_temp_min = (int)json_getReal(child);

            tmp_json = json_getProperty(daily_weather_json, "temperature_2m_max");
            child = json_getChild(tmp_json);
            weather.day_0_temp_max = (int)json_getReal(child);
            child = json_getSibling(child);
            weather.day_1_temp_max = (int)json_getReal(child);
            child = json_getSibling(child);
            weather.day_2_temp_max = (int)json_getReal(child);

            tmp_json = json_getProperty(daily_weather_json, "sunrise");
            child = json_getChild(tmp_json);
            sprintf(weather.day_0_sunrise, json_getValue(child));
            child = json_getSibling(child);
            sprintf(weather.day_1_sunrise, json_getValue(child));
            child = json_getSibling(child);
            sprintf(weather.day_2_sunrise, json_getValue(child));
        }
    }

	printf("STATUS FROM SERVER:[%d] year:[%d] day:[%d] weekday:[%d] month:[%d] hour:[%d] wakeup_alarm[0]:[%02d:%02d]\r\n", status, dt.year, dt.day, dt.dotw, dt.month, dt.hour, wakeup_alarms[0].hour, wakeup_alarms[0].min);
}

// ===========================================================================================================
// Network TASK (wifi, alarm)
// ===========================================================================================================
void NetTask(void *args) {
    ost_net_event_t *message = NULL;
    uint32_t res = 0;

    while (1) {
        res = qor_mbox_wait(&NetMailBox, (void **)&message, 5);
        if (res == QOR_MBOX_OK) {
            if (message->ev == OST_SYS_ALARM) {
                printf("[NET] OST_SYS_ALARM\r\n");
                play_melody(GPIO_PWM, HarryPotter, 200);
            }

            if (message->ev == OST_SYS_UPDATE_TIME) {
                printf("[NET] REMOTE SYNC\r\n");
                char http_buffer[4096] = "";
                if (get_response_from_server(http_buffer) == 0) {
                    struct HttpResponse response;
                    if (parse_http_response(http_buffer, &response) == 0) {
                        parse_json_response(response.body);
                    }
                }
            }

            if (message->ev == OST_SYS_PLAY_SOUND) {
                printf("[NET] OST_SYS_PLAY_SOUND\r\n");
                if (message->song == 0) {
                    play_melody(GPIO_PWM, HarryPotter, 140);
                } else {
                    play_melody(GPIO_PWM, HappyBirday, 140);
                }
            }
        }

        qor_sleep(500);
        //printf("[NET] task woke up\r\n");
    }
}

void request_remote_sync() {
	// Notify for remote sync
    static ost_net_event_t UpdateTimeEv = {
        .ev = OST_SYS_UPDATE_TIME
    };
    qor_mbox_notify(&NetMailBox, (void **)&UpdateTimeEv, QOR_MBOX_OPTION_SEND_BACK);
}

void request_play_song(uint8_t song) {
    static ost_net_event_t Ev = {
        .ev = OST_SYS_PLAY_SOUND,
        .song = 0
    };
    qor_mbox_notify(&NetMailBox, (void **)&Ev, QOR_MBOX_OPTION_SEND_BACK);
}

void net_task_initialize() {
    OstState = OST_SYS_NO_EVENT;
    qor_mbox_init(&NetMailBox, (void **)&NetQueue, 10);

    qor_create_thread(&NetTcb, NetTask, NetStack, sizeof(NetStack) / sizeof(NetStack[0]), NET_TASK_PRIORITY, "NetTask");
}
