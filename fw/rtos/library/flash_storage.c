#include <hardware/sync.h>
#include <hardware/flash.h>
#include "pico/stdlib.h"
#include <string.h>
#include <stdio.h>


#include "flash_storage.h"


#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)


int flash_store_alarms(wakeup_alarm_struct *wakeup_alarms) {
	int buffer[FLASH_PAGE_SIZE/sizeof(int)];

	// 32 = size(struct wakeup_alarm_struct) ?
	printf("flash_store_alarms: memcpy\r\n");
    memcpy(buffer, wakeup_alarms, sizeof(int) * 4 * 4);

	printf("flash_store_alarms: flash_range_erase...\r\n");

	//stdio_set_driver_enabled(&stdio_usb, false);

	uint32_t ints = save_and_disable_interrupts();
	flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
	// This is the line that cause qor_sleep() to hang (even if not called):
    flash_range_program(FLASH_TARGET_OFFSET, (uint8_t *)buffer, FLASH_PAGE_SIZE);
	restore_interrupts(ints);

	//stdio_set_driver_enabled(&stdio_usb, true);

	printf("flash_store_alarms: flash_range_program OK\r\n");

    return 0;
}


int flash_read_alarms(wakeup_alarm_struct *wakeup_alarms) {
	int *p, addr, value;

	addr = XIP_BASE + FLASH_TARGET_OFFSET;
    p = (int *)addr;

	memcpy(wakeup_alarms, p, 32 * 4);
	printf("flash_read_alarms: memcpy OK\r\n");

    return 0;
}
