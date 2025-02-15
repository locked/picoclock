#ifndef ALARMS_H
#define ALARMS_H

#include "pcf8563/pcf8563.h"

// Wakeup alarm global structure
typedef struct {
  int isset;
  int weekdays;	// This is a bitmask
  int hour;
  int min;
  int in_mins;
  char chime[20];
} wakeup_alarm_struct;

void get_next_alarm(wakeup_alarm_struct **next_alarm, time_struct dt);

uint8_t check_alarm(time_struct dt);

#endif
