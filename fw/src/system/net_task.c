#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "main.h"
#include "debug.h"
#include "system.h"
#include "alarms.h"
#include "utils.h"
#include "wifi.h"
#include "flash_storage.h"

#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "pico/aon_timer.h"
#include "hardware/clocks.h"
#include "hardware/watchdog.h"
#include <pico/cyw43_arch.h>

#include "tiny-json.h"
#include "json-maker.h"
#include "pcf8563/pcf8563.h"
#include "tinyhttp/http.h"
#include "circularBuffer.h"


// Externs
extern wakeup_alarm_struct wakeup_alarms[10];
extern int wakeup_alarms_count;
extern weather_struct weather;
extern config_struct global_config;
extern circularBuffer_t* ring_metrics;


static size_t remLen;


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
	time_struct dt;
	dt.year = year_full;
	dt.month = json_getInteger(json_getProperty(date_elem, "month"));
	dt.day = json_getInteger(day_elem);
	dt.weekday = json_getInteger(json_getProperty(date_elem, "weekday"));
	dt.hour = json_getInteger(json_getProperty(date_elem, "hour"));
	dt.min = json_getInteger(json_getProperty(date_elem, "minute"));
	dt.sec = json_getInteger(json_getProperty(date_elem, "second"));
	pcf8563_setDateTime(dt.day, dt.weekday, dt.month, century, year, dt.hour, dt.min, dt.sec);

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
                strcpy(wakeup_alarms[i].chime, "");
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
				char *tmp_str = json_getValue(json_getProperty(alarm_child, "chime"));
				if (tmp_str != NULL) {
					if (strcmp(tmp_str, wakeup_alarms[i].chime)) {
						strncpy(wakeup_alarms[i].chime, tmp_str, 20);
					}
				}

                alarm_child = json_getSibling(alarm_child);
            }
        }
        if (wakeup_alarms_changed) {
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

	printf("STATUS FROM SERVER:[%d] year:[%d] day:[%d] weekday:[%d] month:[%d] hour:[%d] wakeup_alarm[0]:[%02d:%02d:%s]\r\n", status, dt.year, dt.day, dt.weekday, dt.month, dt.hour, wakeup_alarms[0].hour, wakeup_alarms[0].min, wakeup_alarms[0].chime);
}


void build_json(char* dest) {
	char board_id[20];
	get_uniq_id(board_id);
	uint32_t uptime = to_ms_since_boot(get_absolute_time());

	dest = json_objOpen(dest, NULL, &remLen);
	dest = json_str(dest, "board_id", board_id, &remLen);
	dest = json_int(dest, "uptime", uptime, &remLen);

	uint8_t metrics_count = 0;
	for (uint8_t i = 0; i < ring_metrics->num; i++) {
		metrics_t *m = (metrics_t*)circularBuffer_getElement(ring_metrics, i);
		if (m->eco2 > 0) {
			metrics_count++;
		}
	}
	if (metrics_count > 0) {
		dest = json_arrOpen(dest, "metrics", &remLen);
		char date[12];
		for (uint8_t i = 0; i < ring_metrics->num; i++) {
			metrics_t *m = (metrics_t*)circularBuffer_getElement(ring_metrics, i);
			if (m->eco2 > 0) {
				dest = json_objOpen(dest, NULL, &remLen);
				sprintf(date, "20%d%02d%02d_%02d%02d", m->year, m->month, m->day, m->hour, m->min);
				dest = json_str(dest, "date", date, &remLen);
				dest = json_int(dest, "tvoc", m->tvoc, &remLen);
				dest = json_int(dest, "eco2", m->eco2, &remLen);
				dest = json_int(dest, "st", m->ens160_status, &remLen);
				dest = json_objClose(dest, &remLen);
			}
		}
		dest = json_arrClose(dest, &remLen);
	}
	dest = json_objClose(dest, &remLen);
	dest = json_end(dest, &remLen);
}

void remote_sync() {
	printf("remote_sync() Connecting to wifi [%s][%s]...\r\n", global_config.wifi_ssid, global_config.wifi_key);
	int ret = wifi_connect(global_config.wifi_ssid, global_config.wifi_key);
	printf("remote_sync() ret:[%d]...\r\n", ret);
	watchdog_update();
	if (ret == 0) {
		char board_id[20];
		get_uniq_id(board_id);
		printf("remote_sync() board_id:[%s]\r\n", board_id);

		char json[8000] = "";
		remLen = sizeof(json);
		build_json(json);

		char query[8500];
		sprintf(query, "POST /clock.php?id=%s HTTP/1.0\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", board_id, strlen(json), json);
		printf("remote_sync() send_tcp [%s] [%d]...\r\n", global_config.remote_host, atoi(SERVER_PORT));

		char http_buffer[4096] = "";
		watchdog_update();
		ret = send_tcp(global_config.remote_host, 80, query, strlen(query), http_buffer);
		watchdog_update();
		//debug_printf("RESPONSE FROM SERVER ret:[%d] Server:[%s:%d] => [%s]\r\n", ret, SERVER_IP, atoi(SERVER_PORT), response);
		if (ret == 0 && strlen(http_buffer) < 10) {
			ret = 1;
		} else {
			struct HttpResponse response;
			if (parse_http_response(http_buffer, &response) == 0) {
				parse_json_response(response.body);
			}
		}
	}
	watchdog_update();
	printf("remote_sync() Disconnect...\r\n");
	wifi_disconnect();
	printf("remote_sync() Disconnected from wifi, ret:[%d]\r\n", ret);
}
