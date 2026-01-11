#ifndef FS_TASK_H
#define FS_TASK_H

#include <stdint.h>

typedef void (*fs_result_cb_t)(bool);

void fs_audio_next_samples();

void fs_task_read_block(uint32_t addr, uint8_t *block, fs_result_cb_t cb);

void fs_task_sound_start(char *sound);

#endif // FS_TASK_H
