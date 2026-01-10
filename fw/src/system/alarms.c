#include "alarms.h"
#include "fs_task.h"

#include "mcp46XX/mcp46XX.h"
#include "audio_player.h"
#include <string.h>


wakeup_alarm_struct wakeup_alarms[10];
int wakeup_alarms_count = 10;


void get_next_alarm(wakeup_alarm_struct **next_alarm, time_struct dt) {
	*next_alarm = &(wakeup_alarms[0]);
	(*next_alarm)->in_mins = 999999;
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

					//printf("  alarm:[%d] chk_weekday:[%d] in_mins:[%d] (*next_alarm).in_mins:[%d]\r\n", i, chk_weekday, in_mins, (*next_alarm)->in_mins);
					if (in_mins < (*next_alarm)->in_mins) {
						//printf("  => OVERWRITE (*next_alarm) with alarm:[%d]\r\n", i);
						(*next_alarm) = &(wakeup_alarms[i]);
						(*next_alarm)->in_mins = in_mins;
					}
				}
			}
		}
	}
}


void ring_alarm(wakeup_alarm_struct *alarm) {
	//extern qor_mbox_t UiMailBox;

	// Set volume
	//mcp4651_set_wiper(0x90);
	//set_audio_volume_factor(50);

	// Start sound
	//fs_task_sound_start(alarm->chime);
}


uint8_t check_alarm(time_struct dt) {
	// Minute change
	for (int i=0; i<wakeup_alarms_count; i++) {
		wakeup_alarm_struct alarm = wakeup_alarms[i];
		//printf("Check alarm:[%d] [%d:%d] vs [%d:%d]\r\n", i, alarm.hour, alarm.min, dt.hour, dt.min);
		if (alarm.isset == 1 && alarm.hour == dt.hour && alarm.min == dt.min) {
			// Check weekday
			if (alarm.weekdays & (1 << (7 - dt.weekday))) {
				printf("Trigger alarm dt.weekday:[%d] alarm.weekdays:[%d]\r\n", dt.weekday, alarm.weekdays);
				ring_alarm(&alarm);
				return 1;
			} else {
				printf("NO trigger alarm dt.weekday:[%d] alarm.weekdays:[%d]\r\n", dt.weekday, alarm.weekdays);
			}
		}
	}
	return 0;
}
