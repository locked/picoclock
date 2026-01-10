#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <malloc.h>
#include "pico/unique_id.h"

uint32_t getTotalHeap(void);
uint32_t getFreeHeap(void);
void get_uniq_id(char *board_id);

void getWeekdayStr(int weekday, char *weekdays_str);
void getWeekdaysStr(int weekdays, char *weekdays_str);

#endif
