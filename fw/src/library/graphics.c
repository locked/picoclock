#include "graphics.h"

#include <string.h>

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"

#include "icons.h"
#include "utils.h"
#include "alarms.h"
#include "main.h"

#include "ens160/ens160.h"
#include "mcp9808/mcp9808.h"


extern weather_struct weather;
extern int wakeup_alarms_count;
extern wakeup_alarm_struct wakeup_alarms[10];


void display_icon(int x, int y, int icon_id) {
	icon_struct icon = icons[icon_id];
	line_struct line;
	size_t count = icon.size / sizeof(line_struct);
	for(size_t i = 0; i < count; i++) {
		line = icon.lines[i];
		Paint_DrawLine(line.xstart + x, line.y + y, line.xend + x, line.y + y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
	}
}


void display_screen_nav() {
	display_icon(ICON_LEFT_X, ICON_BOTTOM_Y, ICON_LEFTARROW);
	display_icon(ICON_RIGHT_X, ICON_BOTTOM_Y, ICON_RIGHTARROW);
}


void display_time(int* _y, time_struct dt, bool show_alarms) {
	char temp_str[50];
	PAINT_TIME sPaint_time;

	// Next alarm
	wakeup_alarm_struct *next_alarm;
	get_next_alarm(&next_alarm, dt);

	sPaint_time.Year = dt.year;
	sPaint_time.Month = dt.month;
	sPaint_time.Day = dt.day;
	sPaint_time.Hour = dt.hour;
	sPaint_time.Min = dt.min;
	sPaint_time.Sec = dt.sec;
	char weekday_str[3] = "";
	getWeekdayStr(dt.weekday, weekday_str);
	sprintf(temp_str, "%02d/%02d (%s)", dt.day, dt.month, weekday_str);
	Paint_DrawString_EN(SCREEN_X, *_y, temp_str, &Font24, WHITE, BLACK);
	*_y += Font24.Height + 1;

	//Paint_ClearWindows(SCREEN_X, _y, SCREEN_X + Font24.Width * 8, 40 + Font24.Height, WHITE);
	Paint_DrawTime(SCREEN_X, *_y, &sPaint_time, &Font24, WHITE, BLACK);

	display_icon(160, *_y + 2, next_alarm->in_mins != 999999 ? ICON_ALARM : ICON_DISABLED_ALARM);
	*_y += Font24.Height + 2;

	if (show_alarms) {
		if (next_alarm->in_mins != 999999) {
			char weekdays_str[21] = "";
			getWeekdaysStr(next_alarm->weekdays, weekdays_str);

			sprintf(temp_str, "Alarm: %02d:%02d %s", next_alarm->hour, next_alarm->min, weekdays_str);
			Paint_DrawString_EN(SCREEN_X, *_y, temp_str, &Font12, WHITE, BLACK);
			*_y += Font12.Height + 1;
		}
	}
}


void display_screen_main(time_struct dt, circularBuffer_t* ring_metrics) {
	char temp_str[50];
	char temp2_str[10];
	int _y = SCREEN_Y;

	display_icon(ICON_LEFT_X, ICON_TOP_Y,    ICON_MUSIC);
	display_icon(ICON_LEFT_X, ICON_MIDDLE_Y, ICON_BLUETOOTH);

	display_icon(ICON_RIGHT_X, ICON_TOP_Y,    ICON_LIGHT);
	display_icon(ICON_RIGHT_X, ICON_MIDDLE_Y, ICON_WIFI);

	display_time(&_y, dt, true);

	// Metrics
	metrics_t *m = circularBuffer_current(ring_metrics);
	int16_t co2 = 0;
	char co2_src[10] = "";
	if (m->scd43_co2 > 0) {
		co2 = m->scd43_co2;
		sprintf(co2_src, "scd43");
	} else if (m->s88_co2 > 0) {
		co2 = m->s88_co2;
		sprintf(co2_src, "s88");
	} else if (m->stcc4_co2 > 0) {
		co2 = m->stcc4_co2;
		sprintf(co2_src, "stcc4");
	} else if (m->eco2 > 0 && m->eco2 < 8000) {
		co2 = m->eco2;
		sprintf(co2_src, "ens160");
	} else {
		co2 = -1;
		sprintf(co2_src, "none");
	}
	sprintf(temp_str, "CO2:%d (%s) %0.1fC", co2, co2_src, m->temp);
	Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font12, WHITE, BLACK);
	_y += Font12.Height + 1;

	// Quick weather recap
	sprintf(temp_str, "T min/max: %02dC/%02dC", weather.day_0_temp_min, weather.day_0_temp_max);
	Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font12, WHITE, BLACK);
	_y += Font12.Height + 1;
	sprintf(temp_str, "Sunrise: %s", weather.day_0_sunrise);
	Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font12, WHITE, BLACK);
	_y += Font12.Height + 1;
}


void display_screen_weather() {
    char temp_str[50];
	int _y = SCREEN_Y;

	display_icon(ICON_RIGHT_X, 10, ICON_LIGHT);
	display_icon(ICON_RIGHT_X, 50, ICON_WIFI);

	sprintf(temp_str, "Weather");
	Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font16, WHITE, BLACK);
	_y += Font16.Height + 1;

	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 1, weather.code_desc, &Font12, WHITE, BLACK);
	sprintf(temp_str, "Temp: %s", weather.temperature_2m);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 2, temp_str, &Font12, WHITE, BLACK);
	sprintf(temp_str, "Wind: %s", weather.wind_speed_10m);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 3, temp_str, &Font12, WHITE, BLACK);
	sprintf(temp_str, "Precipitation: %s", weather.precipitation);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 4, temp_str, &Font12, WHITE, BLACK);

	sprintf(temp_str, " %02dC   %02dC   %02dC", weather.day_0_temp_min, weather.day_1_temp_min, weather.day_2_temp_min);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 5, temp_str, &Font12, WHITE, BLACK);

	sprintf(temp_str, " %02dC   %02dC   %02dC", weather.day_0_temp_max, weather.day_1_temp_max, weather.day_2_temp_max);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 6, temp_str, &Font12, WHITE, BLACK);

	sprintf(temp_str, "%s %s %s", weather.day_0_sunrise, weather.day_1_sunrise, weather.day_2_sunrise);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 7, temp_str, &Font12, WHITE, BLACK);
}


void display_screen_alarms() {
    char temp_str[50];
	int _y = SCREEN_Y;

	sprintf(temp_str, "Alarms");
	Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font16, WHITE, BLACK);
	for (int i=0; i<wakeup_alarms_count; i++) {
		if (wakeup_alarms[i].isset == 1) {
			_y += Font16.Height + 1;

			char weekdays_str[21] = "";
			getWeekdaysStr(wakeup_alarms[i].weekdays, weekdays_str);
			sprintf(temp_str, "%d:%d(%s) %s", wakeup_alarms[i].hour, wakeup_alarms[i].min, weekdays_str, wakeup_alarms[i].chime);
			Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font12, WHITE, BLACK);
		}
	}
}


void display_screen_debug(char *last_sync_str, uint8_t current_rgb_color, uint8_t* rgb_intensity) {
    char temp_str[50];
	int _y = SCREEN_Y;

	sprintf(temp_str, "Debug");
	Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font16, WHITE, BLACK);

	_y += Font16.Height + 1;
	char board_id[20];
	get_uniq_id(board_id);
	sprintf(temp_str, "ID: %s", board_id);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 0, temp_str, &Font12, WHITE, BLACK);

	sprintf(temp_str, "Last sync: %s", last_sync_str);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 1, temp_str, &Font12, WHITE, BLACK);

	sprintf(temp_str, "Mem(free/tot): %d/%d", getFreeHeap(), getTotalHeap());
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 2, temp_str, &Font12, WHITE, BLACK);

	sprintf(temp_str, "B:%02x G:%02x R:%02x Sel:%02x", rgb_intensity[0], rgb_intensity[1], rgb_intensity[2], current_rgb_color);
	Paint_DrawString_EN(SCREEN_X, _y + Font12.Height * 3, temp_str, &Font12, WHITE, BLACK);
}


void display_screen_alarm() {
    char temp_str[50];
	int _y = SCREEN_Y;

	sprintf(temp_str, "ALARM");
	Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font16, WHITE, BLACK);

	time_struct dt = pcf8563_getDateTime();
	_y += Font16.Height + 1;
	sprintf(temp_str, "%02d:%02d", dt.hour, dt.min);
	Paint_DrawString_EN(SCREEN_X, _y, temp_str, &Font24, WHITE, BLACK);
}


void display_screen_metrics(circularBuffer_t* ring_metrics) {
	metrics_t *m;
	uint8_t x = SCREEN_X;
	uint8_t y = SCREEN_HEIGHT;
	uint8_t endy = SCREEN_HEIGHT;
	// Compute max
	float max_eco2 = 0;
	for (uint8_t i = 0; i < ring_metrics->num; i++) {
		m = (metrics_t*)circularBuffer_getElement(ring_metrics, i);
		if (m->eco2 > max_eco2) {
			max_eco2 = m->eco2;
		}
	}
	max_eco2 -= 400;
	for (uint8_t i = 0; i < ring_metrics->num; i++) {
		m = (metrics_t*)circularBuffer_getElement(ring_metrics, i);
		//printf("%d %d \n", m->tvoc, m->eco2);

		x += 1;
		uint8_t norm_v = ((float)(m->eco2 - 400)/max_eco2) * SCREEN_HEIGHT;
		endy = SCREEN_HEIGHT - norm_v;
		Paint_DrawLine(x, y, x, endy, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
	}
}

void display_screen_music(time_struct dt, bool is_bt, char* bt_name) {
	int _y = SCREEN_Y;

	display_icon(ICON_LEFT_X, ICON_TOP_Y, ICON_ESCAPE);

	display_time(&_y, dt, false);

	if (is_bt) {
		Paint_DrawString_EN(SCREEN_X, _y, "Bluetooth enabled", &Font12, WHITE, BLACK);
		_y += Font12.Height + 1;

		if (strcmp(bt_name, "unknown") != 0) {
			Paint_DrawString_EN(SCREEN_X, _y, bt_name, &Font12, WHITE, BLACK);
			_y += Font12.Height + 1;
		}
	}
	Paint_DrawString_EN(ICON_RIGHT_X + 2, ICON_TOP_Y,    "+", &Font24, WHITE, BLACK);
	Paint_DrawString_EN(ICON_RIGHT_X + 2, ICON_MIDDLE_Y, "-", &Font24, WHITE, BLACK);
}
