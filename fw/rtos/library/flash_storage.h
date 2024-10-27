#ifndef _FLASH_STORAGE_H
#define _FLASH_STORAGE_H

#include "ost_hal.h"


int flash_store_alarms(wakeup_alarm_struct *wakeup_alarms);

int flash_read_alarms(wakeup_alarm_struct *wakeup_alarms);

#endif // _FLASH_STORAGE_H
