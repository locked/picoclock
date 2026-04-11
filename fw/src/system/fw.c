#include "fw.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "boot/picoboot.h"
#include "boot/uf2.h"
//#include "pico/sha256.h"

#include "hardware/sync.h"
#include "hardware/watchdog.h"
#include "hardware/flash.h"

#define FLASH_SECTOR_ERASE_SIZE 4096u

//typedef int (*ota_segment_consumer_t)(OTA_UPDATE_STATE_T *state, uf2_block_t *block);

typedef struct uf2_block uf2_block_t;

static OTA_UPDATE_STATE_T* state;


static OTA_UPDATE_STATE_T* ota_update_init(void) {
	OTA_UPDATE_STATE_T *state = calloc(1, sizeof(OTA_UPDATE_STATE_T));
	if (!state) {
		printf("failed to allocate state\n");
		return NULL;
	}
	return state;
}


static __attribute__((aligned(4))) uint8_t workarea[4 * 1024];


int __no_inline_not_in_flash_func(process_ota_segment)(char* buf) {
	uf2_block_t* block = (uf2_block_t*)buf;
	watchdog_update();

	if (block->magic_start0 != UF2_MAGIC_START0 || block->magic_start1 != UF2_MAGIC_START1) {
		printf("[OTA] ERROR: Block doesn't have valid UF2 magic numbers\n");
		return -1;
	}
	if (block->num_blocks > 0 && (block->block_no < 0 || block->block_no >= block->num_blocks)) {
		printf("[ERROR] Invalid block number: %d. Total blocks: %d.\n", block->block_no, block->num_blocks);
		return -1;
	}

	if (state->num_blocks == 0 || (block->num_blocks > state->num_blocks && state->blocks_done < 2)) {
		state->num_blocks = block->num_blocks;
		state->family_id = block->file_size;

		if (state->flash_update == 0) {
			boot_info_t current_boot_info = {};
			int boot_info_result = rom_get_boot_info(&current_boot_info);

			uint32_t partition_a_start = 0x2000;
			uint32_t partition_b_start = 0x202000;
			uint32_t partition_size = 0x200000;

			uint32_t target_partition_offset;

			if (current_boot_info.partition == 0) {
				target_partition_offset = partition_b_start;
				state->flash_update = XIP_BASE + partition_b_start;
				printf("[OTA] Will flash to partition 1, start:[%x]\n", target_partition_offset);
			} else if (current_boot_info.partition == 1) {
				target_partition_offset = partition_a_start;
				state->flash_update = XIP_BASE + partition_a_start;
				printf("[OTA] Will flash to partition 0, start:[%x]\n", target_partition_offset);
			} else {
				target_partition_offset = partition_b_start;
				state->flash_update = XIP_BASE + partition_b_start;
				printf("[OTA] Will flash to DEFAULT partition 1, start:[%x]\n", target_partition_offset);
			}

			state->write_offset = target_partition_offset;
			state->write_size = partition_size;
		}
	}

	if (state->blocks_done < 2 && state->family_id != block->file_size) {
		state->family_id = block->file_size;
	} else if (state->blocks_done >= 2 && state->family_id != block->file_size) {
		printf("[ERROR] Family ID mismatch: expected 0x%08X, got 0x%08X\n", state->family_id, block->file_size);
		return -1;
	}

	int8_t ret;
	(void)ret;

	uint32_t flash_addr = block->target_addr;

	if (flash_addr >= XIP_BASE && flash_addr < (XIP_BASE + 0x200000)) {
		uint32_t offset_in_partition = flash_addr - XIP_BASE;
		flash_addr = state->flash_update + offset_in_partition;
	}

	if (flash_addr < XIP_BASE || flash_addr >= (XIP_BASE + 0x800000)) {
		printf("[WARN] flash_addr:[%x] < XIP_BASE:[%x] || flash_addr >= (XIP_BASE + 0x800000)\n", flash_addr, XIP_BASE);
		return 0;
	}

	uint32_t flash_sector = flash_addr / FLASH_SECTOR_ERASE_SIZE;

	struct cflash_flags flags;

	if (flash_sector > state->highest_erased_sector) {
		uint32_t erase_addr = flash_addr & ~(FLASH_SECTOR_ERASE_SIZE-1);
		flags.flags =
			(CFLASH_OP_VALUE_ERASE << CFLASH_OP_LSB) | 
			(CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB) |
			(CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB);
		//printf("[fw] erase_addr:[%x]\n", erase_addr);
		ret = rom_flash_op(flags, erase_addr, FLASH_SECTOR_ERASE_SIZE, NULL);

		state->highest_erased_sector = flash_sector;

		if (ret != 0) {
			printf("[ERROR] Flash erase failed with code: %d erase_addr: %x\n", ret, erase_addr);
			return -1;
		}
	}

	flags.flags =
		(CFLASH_OP_VALUE_PROGRAM << CFLASH_OP_LSB) | 
		(CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB) |
		(CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB);
	//printf("[fw] flash_addr:[%x]\n", flash_addr);
	ret = rom_flash_op(flags, flash_addr, 256, (void*)block->data);

	if (ret != 0) {
		printf("[ERROR] Flash program failed with code: %d\n", ret);
		return -1;
	}
	if (ret != 0) {
		printf("[ERROR] Flash program failed with code: %d\n", ret);
		return -1;
	}

	state->blocks_done++;

	if (state->blocks_done >= state->num_blocks) {
		int ret = rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_FLASH_UPDATE, 1000, state->flash_update, 0);
		free(state);
		sleep_ms(3000);

		printf("[fw] *** DOWNLOAD COMPLETE! ***\n");
		state->complete = true;
		return 0;
	}

	if (state->blocks_done % 10 == 0) {
		printf("[fw] Segment written flash_addr:[%x] state->blocks_done:[%d/%d]\r\n", flash_addr, state->blocks_done, state->num_blocks);
	}
	return 0;
}


int fw_update_init() {
	/*boot_info_t boot_info = {};
	int ret = rom_get_boot_info(&boot_info);

	if (rom_get_last_boot_type() == BOOT_TYPE_FLASH_UPDATE) {
		if (boot_info.reboot_params[0]);
		if (boot_info.tbyb_and_update_info);
		ret = rom_explicit_buy(workarea, sizeof(workarea));
		if (ret);
		ret = rom_get_boot_info(&boot_info);
		if (boot_info.tbyb_and_update_info);
	}*/

	state = ota_update_init();
	if (!state) {
		return -1;
	}

	return 0;
}
