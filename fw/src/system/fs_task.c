#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "debug.h"
#include "audio_player.h"
#include "filesystem.h"
#include "system.h"
#include "fs_task.h"
#include "sdcard.h"

// ===========================================================================================================
// DEFINITIONS
// ===========================================================================================================
typedef enum
{
    FS_NO_EVENT,
    FS_PLAY_SOUND,
    FS_DISPLAY_IMAGE,
    FS_READ_SDCARD_BLOCK,
    FS_WRITE_SDCARD_BLOCK,
    FS_AUDIO_NEXT_SAMPLES
} fs_state_t;

#define ASSETS_DIR "/"
#define STORY_DIR_OFFSET (UUID_SIZE + 1) // +1 for the first '/' (filesystem root)


static int PacketCounter = 0;

static char ScratchFile[260];

int isPlaying = 0;


void fs_audio_next_samples() {
	isPlaying = main_audio_process();
	PacketCounter++;
	if (PacketCounter % 200 == 0) {
		debug_printf("isPlaying:%d Packets: %d\r\n", isPlaying, PacketCounter);
	}

	if (isPlaying == 0) {
		//debug_printf("STOP AUDIO Packets: %d\r\n", PacketCounter);
		main_audio_stop();
	}
}

void fs_task_read_block(uint32_t addr, uint8_t *block, fs_result_cb_t cb) {
	sdcard_sector_read(addr, block);
	if (cb != NULL) {
		cb(true);
	}
}

void fs_task_sound_start(char *sound) {
	sprintf(ScratchFile, "%s%s", ASSETS_DIR, sound);

	debug_printf("--------------------------\r\nPlaying: %s\r\n", ScratchFile);

	main_audio_play(ScratchFile);
	debug_printf("audio_play...OK\r\n");
	PacketCounter = 0;
	isPlaying = 1;
}
