#ifndef FW_H
#define FW_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __
cplusplus
extern "C" {
#endif

typedef struct OTA_UPDATE_STATE_T_ {
	bool complete;
	__attribute__((aligned(4))) uint8_t buffer_recv[512];
	int recv_len;
	int num_blocks;
	int blocks_done;
	uint32_t family_id;
	uint32_t flash_update;
	int32_t write_offset;
	uint32_t write_size;
	uint32_t highest_erased_sector;
} OTA_UPDATE_STATE_T;

typedef struct uf2_block uf2_block_t;

//typedef int (*ota_segment_consumer_t)(OTA_UPDATE_STATE_T *state, uf2_block_t *block);

int process_ota_segment(char* buf);

int fw_update_init();
int fw_update_complete();

#ifdef __cplusplus
}
#endif

#endif // FW_H
