#ifndef NET_TASK_H
#define NET_TASK_H

#include <stdint.h>

void net_task_initialize();
void net_task_connected();

void request_remote_sync();
void request_play_song(uint8_t song);

#endif // NET_TASK_H
