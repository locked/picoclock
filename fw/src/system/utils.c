#include "utils.h"
#include <string.h>
#include <stdio.h>



uint32_t getTotalHeap(void) {
   extern char __StackLimit, __bss_end__;
   
   return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
   struct mallinfo m = mallinfo();

   return getTotalHeap() - m.uordblks;
}

void get_uniq_id(char *board_id) {
	pico_get_unique_board_id_string(board_id, 20);
}


void getWeekdayStr(int weekday, char *weekdays_str) {
    if (weekday == 0) {
        strcat(weekdays_str, "Sun");
    } else if (weekday == 1) {
        strcat(weekdays_str, "Mon");
    } else if (weekday == 2) {
        strcat(weekdays_str, "Tue");
    } else if (weekday == 3) {
        strcat(weekdays_str, "Wed");
    } else if (weekday == 4) {
        strcat(weekdays_str, "Thu");
    } else if (weekday == 5) {
        strcat(weekdays_str, "Fri");
    } else if (weekday == 6) {
        strcat(weekdays_str, "Sat");
    }
}

void getWeekdaysStr(int weekdays, char *weekdays_str) {
    sprintf(weekdays_str, "");
    for (int chk_weekday=0; chk_weekday<7; chk_weekday++) {
        if (weekdays & (1 << (7 - chk_weekday))) {
            getWeekdayStr(chk_weekday, weekdays_str);
        }
    }
}
