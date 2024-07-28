#ifndef NET_TASK_H
#define NET_TASK_H

#include <stdint.h>

void net_task_initialize();
void net_task_connected();

void ost_change_minute(datetime_t *dt);
void ost_get_wakeup_alarm(wakeup_alarm_struct *wa);
void ost_request_update_time();

#endif // NET_TASK_H
