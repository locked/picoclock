#include "fw.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "boot/picobin.h"
#include "boot/picoboot.h"
#include "boot/uf2.h"
#include "pico/sha256.h"

#include "hardware/sync.h"
#include "hardware/watchdog.h"

#define BUF_SIZE 2048
#define FLASH_SECTOR_ERASE_SIZE 4096u

typedef int (*ota_segment_consumer_t)(OTA_UPDATE_STATE_T *state, uf2_block_t *block);

typedef struct uf2_block uf2_block_t;


static OTA_UPDATE_STATE_T* ota_update_init(void) {
	OTA_UPDATE_STATE_T *state = calloc(1, sizeof(OTA_UPDATE_STATE_T));
	if (!state) {
		printf("failed to allocate state\n");
		return NULL;
	}
	return state;
}


static __attribute__((aligned(4))) uint8_t workarea[4 * 1024];

static int process_ota_segment(OTA_UPDATE_STATE_T *state, uf2_block_t *block) {
	watchdog_update();
	if (state->num_blocks == 0 || (block->num_blocks > state->num_blocks && state->blocks_done < 2)) {
		state->num_blocks = block->num_blocks;
		state->family_id = block->file_size;

		watchdog_update();
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
				printf("[OTA] Will flash to partition B, start:[%x]\n", target_partition_offset);
			} else if (current_boot_info.partition == 1) {
				target_partition_offset = partition_a_start;
				state->flash_update = XIP_BASE + partition_a_start;
				printf("[OTA] Will flash to partition A, start:[%x]\n", target_partition_offset);
			} else {
				target_partition_offset = partition_b_start;
				state->flash_update = XIP_BASE + partition_b_start;
				printf("[OTA] Will flash to DEFAULT partition B, start:[%x]\n", target_partition_offset);
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

	struct cflash_flags flags;
	int8_t ret;
	(void)ret;

	uint32_t flash_addr = block->target_addr;

	watchdog_update();
	if (flash_addr >= XIP_BASE && flash_addr < (XIP_BASE + 0x200000)) {
		uint32_t offset_in_partition = flash_addr - XIP_BASE;
		flash_addr = state->flash_update + offset_in_partition;
	}

	if (flash_addr < XIP_BASE || flash_addr >= (XIP_BASE + 0x800000)) {
		return 0;
	}

	uint32_t flash_sector = flash_addr / FLASH_SECTOR_ERASE_SIZE;

	if (flash_sector > state->highest_erased_sector) {
		uint32_t erase_addr = flash_addr & ~(FLASH_SECTOR_ERASE_SIZE-1);
		flags.flags =
			(CFLASH_OP_VALUE_ERASE << CFLASH_OP_LSB) | 
			(CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB) |
			(CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB);
		ret = rom_flash_op(flags, erase_addr, FLASH_SECTOR_ERASE_SIZE, NULL);
		state->highest_erased_sector = flash_sector;

		if (ret != 0) {
			printf("[ERROR] Flash erase failed with code: %d\n", ret);
			return -1;
		}
	}
	watchdog_update();

	flags.flags =
		(CFLASH_OP_VALUE_PROGRAM << CFLASH_OP_LSB) | 
		(CFLASH_SECLEVEL_VALUE_SECURE << CFLASH_SECLEVEL_LSB) |
		(CFLASH_ASPACE_VALUE_STORAGE << CFLASH_ASPACE_LSB);
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
		printf("[OTA] *** DOWNLOAD COMPLETE! ***\n");
		state->complete = true;
		return 0;
	}
	watchdog_update();

	return 0;
}

int fw_update(const char* buffer) {
	boot_info_t boot_info = {};
	watchdog_update();
	int ret = rom_get_boot_info(&boot_info);

	if (rom_get_last_boot_type() == BOOT_TYPE_FLASH_UPDATE) {
		if (boot_info.reboot_params[0]);
		if (boot_info.tbyb_and_update_info);
		ret = rom_explicit_buy(workarea, sizeof(workarea));
		if (ret);
		ret = rom_get_boot_info(&boot_info);
		if (boot_info.tbyb_and_update_info);
	}

	watchdog_update();
	OTA_UPDATE_STATE_T *state = ota_update_init();
	if (!state) {
		return -1;
	}

	watchdog_update();

	//process_ota_segment(state, block);

	while (!state->complete) {
		sleep_ms(250);
	}
	watchdog_update();

	printf("[INFO] OTA update completed! Preparing to reboot...\n");

	//watchdog_reboot(0, REBOOT2_FLAG_REBOOT_TYPE_FLASH_UPDATE, state->flash_update);
	ret = rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_FLASH_UPDATE, 1000, state->flash_update, 0);
	printf("[INFO] OTA reboot requested:[%d]\n", ret);
	free(state);
	sleep_ms(3000);

	printf("[INFO] OTA: hard reset\n");
	sleep_ms(1000);
	return 0;
}
