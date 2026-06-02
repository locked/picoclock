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

/* ============================================================================
 * H1 instrumentation + repair: QMI / XIP cache integrity around rom_flash_op
 *
 * Hypothesis: SDK's flash_safe_execute saves QMI state before the ROM flash
 * op and restores it afterwards. Boot2 (which the ROM re-runs internally)
 * may produce a slightly different auto-calibrated timing each run, so the
 * "restore" puts back stale parameters and subsequent XIP reads return
 * shuffled bytes -> printf garbage.
 *
 * Strategy:
 *   1. Snapshot QMI M0 timing/format/cmd + ATRANS regs ONCE at OTA init,
 *      when XIP is known good.
 *   2. After every rom_flash_op: re-read QMI, count mismatches, force-restore
 *      from the baseline, then flush the XIP cache.
 *   3. Cross-check a flash-resident "canary" string by reading it through XIP
 *      and comparing against a RAM copy taken at init.
 *   4. Persist counters in watchdog scratch[4..7] so they survive the reboot
 *      and can be inspected by the new firmware (or here on next OTA).
 *
 * All helpers are forced into RAM so they remain callable while XIP is in any
 * questionable state.
 * ==========================================================================*/

/* RP2350 QMI register block (datasheet 12.14) */
#define QMI_BASE_ADDR        0x400d0000u
#define QMI_M0_TIMING_OFF    0x0cu
#define QMI_M0_RFMT_OFF      0x10u
#define QMI_M0_RCMD_OFF      0x14u
#define QMI_M0_WFMT_OFF      0x18u
#define QMI_M0_WCMD_OFF      0x1cu
#define QMI_ATRANS_OFF       0x34u  /* ATRANS0..7 are at 0x34, 0x38 ... 0x50 */

/* RP2350 watchdog scratch registers (raw access; do not pull a new SDK include) */
#define WATCHDOG_SCRATCH_BASE 0x400d802cu /* scratch[0] */

/* UART0 raw register access for fallback direct prints (no flash needed) */
#define UART0_DR_ADDR        0x40070000u
#define UART0_FR_ADDR        0x40070018u
#define UART_FR_TXFF         (1u << 5)

typedef struct {
	uint32_t m0_timing;
	uint32_t m0_rfmt;
	uint32_t m0_rcmd;
	uint32_t m0_wfmt;
	uint32_t m0_wcmd;
	uint32_t atrans[8];
} qmi_snapshot_t;

static qmi_snapshot_t qmi_baseline;
static bool qmi_baseline_valid = false;

/* Flash-resident canary string. Lives in .rodata of the *running* partition,
 * which the OTA never writes (writes always target the other partition). Any
 * change observed when reading via XIP therefore indicates cache corruption,
 * not flash corruption. */
__attribute__((used, aligned(64)))
static const char fw_xip_canary[64] =
	"OTA_XIP_CANARY__DO_NOT_EDIT__abcdefghijklmnopqrstuvwxyz_0123456";
static uint8_t canary_ram_copy[64];

/* Diagnostic counters (also mirrored to watchdog scratch[4..7]) */
static uint32_t diag_flash_ops = 0;
static uint32_t diag_qmi_mismatches = 0;
static uint32_t diag_canary_mismatches = 0;
static uint32_t diag_last_mismatch_code = 0; /* upper 8 bits = which reg, low 24 = current m0_timing low bits */

static inline volatile uint32_t *qmi_reg(uint32_t off) {
	return (volatile uint32_t *)(QMI_BASE_ADDR + off);
}

static void __no_inline_not_in_flash_func(qmi_capture)(qmi_snapshot_t *s) {
	s->m0_timing = *qmi_reg(QMI_M0_TIMING_OFF);
	s->m0_rfmt   = *qmi_reg(QMI_M0_RFMT_OFF);
	s->m0_rcmd   = *qmi_reg(QMI_M0_RCMD_OFF);
	s->m0_wfmt   = *qmi_reg(QMI_M0_WFMT_OFF);
	s->m0_wcmd   = *qmi_reg(QMI_M0_WCMD_OFF);
	for (int i = 0; i < 8; i++) {
		s->atrans[i] = *qmi_reg(QMI_ATRANS_OFF + (uint32_t)(i * 4));
	}
}

/* Returns 0 if identical, otherwise an opaque non-zero code identifying which
 * register first diverged (1..13). */
static int __no_inline_not_in_flash_func(qmi_compare)(const qmi_snapshot_t *a, const qmi_snapshot_t *b) {
	if (a->m0_timing != b->m0_timing) return 1;
	if (a->m0_rfmt   != b->m0_rfmt  ) return 2;
	if (a->m0_rcmd   != b->m0_rcmd  ) return 3;
	if (a->m0_wfmt   != b->m0_wfmt  ) return 4;
	if (a->m0_wcmd   != b->m0_wcmd  ) return 5;
	for (int i = 0; i < 8; i++) {
		if (a->atrans[i] != b->atrans[i]) return 6 + i;
	}
	return 0;
}

static void __no_inline_not_in_flash_func(qmi_restore)(const qmi_snapshot_t *s) {
	*qmi_reg(QMI_M0_TIMING_OFF) = s->m0_timing;
	*qmi_reg(QMI_M0_RFMT_OFF)   = s->m0_rfmt;
	*qmi_reg(QMI_M0_RCMD_OFF)   = s->m0_rcmd;
	*qmi_reg(QMI_M0_WFMT_OFF)   = s->m0_wfmt;
	*qmi_reg(QMI_M0_WCMD_OFF)   = s->m0_wcmd;
	for (int i = 0; i < 8; i++) {
		*qmi_reg(QMI_ATRANS_OFF + (uint32_t)(i * 4)) = s->atrans[i];
	}
	__dsb();
	__isb();
}

static void __no_inline_not_in_flash_func(xip_invalidate_all)(void) {
	/* RP2350 XIP cache invalidate via maintenance aperture.
	 * The 16 KiB / 8 B/line = 2048 set/way walks use 8-bit writes
	 * at addresses outside the downstream QMI flash range, followed
	 * by a full memory barrier.  From pico-sdk xip_cache.c:xip_cache_invalidate_all. */
	volatile uint8_t *base = (volatile uint8_t *)0x1BFFC000u;
	for (uint32_t off = 0x0u; off < 0x4000u; off += 0x8u) {
		base[off] = 0;   /* XIP_CACHE_INVALIDATE_BY_SET_WAY = 0 */
	}
	__dsb();
	__isb();
}

/* Direct-UART hex dump for diagnostics that MUST not rely on flash-resident
 * printf format strings or libc state. */
static void __no_inline_not_in_flash_func(uart0_putc_raw)(char c) {
	volatile uint32_t *fr = (volatile uint32_t *)UART0_FR_ADDR;
	volatile uint32_t *dr = (volatile uint32_t *)UART0_DR_ADDR;
	while ((*fr) & UART_FR_TXFF) {}
	*dr = (uint32_t)(uint8_t)c;
}

static void __no_inline_not_in_flash_func(uart0_puts_raw)(const char *s) {
	/* Caller passes either a stack/RAM string or a flash one; if flash, it
	 * may be unreadable. We accept that risk and prefer RAM strings here. */
	while (*s) uart0_putc_raw(*s++);
}

static void __no_inline_not_in_flash_func(uart0_put_hex32)(uint32_t v) {
	for (int i = 7; i >= 0; i--) {
		uint32_t n = (v >> (i * 4)) & 0xfu;
		uart0_putc_raw((char)(n < 10 ? ('0' + n) : ('a' + n - 10)));
	}
}

static void __no_inline_not_in_flash_func(uart0_put_hex8)(uint8_t v) {
	uint32_t hi = (v >> 4) & 0xfu;
	uint32_t lo = v & 0xfu;
	uart0_putc_raw((char)(hi < 10 ? ('0' + hi) : ('a' + hi - 10)));
	uart0_putc_raw((char)(lo < 10 ? ('0' + lo) : ('a' + lo - 10)));
}

/* Dump a region of flash both as ASCII (printable chars) and as hex via
 * direct UART. This bypasses printf entirely. The bytes are read through XIP
 * but the printing path uses only RAM + UART regs. If the ASCII / hex shows
 * garbage, the flash at that address really is returning corrupted bytes. */
static char dump_s_addr[] = "[FDUMP ";
static char dump_s_sep[]  = " @";
static char dump_s_mid[]  = " bytes=";
static char dump_s_pipe[] = " |";
static char dump_s_eol[]  = "|\r\n";

static void __no_inline_not_in_flash_func(fw_dump_flash_region)(const char *tag, const uint8_t *addr, unsigned n) {
	uart0_puts_raw(dump_s_addr);
	/* tag string itself lives in flash at the caller's site; print at most
	 * 8 chars best-effort. If it garbles that's also a data point. */
	for (int i = 0; i < 8 && tag[i]; i++) uart0_putc_raw(tag[i]);
	uart0_puts_raw(dump_s_sep);
	uart0_put_hex32((uint32_t)addr);
	uart0_puts_raw(dump_s_mid);
	for (unsigned i = 0; i < n; i++) {
		uart0_put_hex8(addr[i]);
	}
	uart0_puts_raw(dump_s_pipe);
	for (unsigned i = 0; i < n; i++) {
		uint8_t c = addr[i];
		uart0_putc_raw((char)((c >= ' ' && c < 127) ? c : '.'));
	}
	uart0_puts_raw(dump_s_eol);
}

static void __no_inline_not_in_flash_func(scratch_write)(unsigned idx, uint32_t v) {
	volatile uint32_t *p = (volatile uint32_t *)(WATCHDOG_SCRATCH_BASE + idx * 4u);
	*p = v;
}

/* Initialize the baseline. Must be called when XIP is known good and BEFORE
 * any rom_flash_op. */
static void __no_inline_not_in_flash_func(fw_integrity_init)(void) {
	qmi_capture(&qmi_baseline);
	qmi_baseline_valid = true;
	for (int i = 0; i < (int)sizeof(canary_ram_copy); i++) {
		canary_ram_copy[i] = (uint8_t)fw_xip_canary[i];
	}
	diag_flash_ops = 0;
	diag_qmi_mismatches = 0;
	diag_canary_mismatches = 0;
	diag_last_mismatch_code = 0;
	scratch_write(4, 0);
	scratch_write(5, 0);
	scratch_write(6, 0);
	scratch_write(7, 0);
}

/* Run after EVERY rom_flash_op. Detects, repairs, and logs corruption. */
static void __no_inline_not_in_flash_func(fw_integrity_check_repair)(void) {
	diag_flash_ops++;
	scratch_write(4, diag_flash_ops);

	if (!qmi_baseline_valid) return;

	qmi_snapshot_t cur;
	qmi_capture(&cur);
	int code = qmi_compare(&qmi_baseline, &cur);
	if (code != 0) {
		diag_qmi_mismatches++;
		diag_last_mismatch_code = ((uint32_t)code << 24) | (cur.m0_timing & 0x00ffffffu);
		scratch_write(5, diag_qmi_mismatches);
		scratch_write(6, diag_last_mismatch_code);
		/* H1 repair: forcibly restore the known-good QMI state and dump the
		 * XIP cache so the next fetch re-reads from flash via correct timing. */
		qmi_restore(&qmi_baseline);
	}
	/* Always invalidate the cache; cheap and removes any stale lines even when
	 * QMI itself didn't drift. */
	xip_invalidate_all();

	/* Now read the canary back through XIP and compare. */
	for (int i = 0; i < (int)sizeof(canary_ram_copy); i++) {
		if ((uint8_t)fw_xip_canary[i] != canary_ram_copy[i]) {
			diag_canary_mismatches++;
			scratch_write(7, diag_canary_mismatches);
			break;
		}
	}
}

/* Print one diagnostic summary line via direct UART (no flash). Safe to call
 * even when printf is unreliable. Strings live in .data (RAM) so they are not
 * re-read from flash on every call. */
static char diag_s_tag[]    = "\r\n[OTA-DIAG] ops=";
static char diag_s_qmi[]    = " qmi_mm=";
static char diag_s_canary[] = " canary_mm=";
static char diag_s_last[]   = " last_code=";
static char diag_s_eol[]    = "\r\n";

static void __no_inline_not_in_flash_func(fw_integrity_dump_uart)(void) {
	uart0_puts_raw(diag_s_tag);
	uart0_put_hex32(diag_flash_ops);
	uart0_puts_raw(diag_s_qmi);
	uart0_put_hex32(diag_qmi_mismatches);
	uart0_puts_raw(diag_s_canary);
	uart0_put_hex32(diag_canary_mismatches);
	uart0_puts_raw(diag_s_last);
	uart0_put_hex32(diag_last_mismatch_code);
	uart0_puts_raw(diag_s_eol);
}


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

	state->blocks_seen++;

	/* Skip non-flash blocks (UF2 spec FLAG_NOT_MAIN_FLASH = 0x1) and blocks
	 * whose target_addr is outside the 8 MB XIP window. These are metadata /
	 * family-id / padding blocks that some toolchains emit with synthetic
	 * addresses like 0x10FFFF00. They must not influence src_base or be
	 * written to flash. */
	if ((block->flags & 0x1u) ||
	    block->target_addr < XIP_BASE ||
	    block->target_addr >= (XIP_BASE + 0x800000)) {
		return 0;
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

	/* Lock in the source base address from the FIRST block we see. The UF2
	 * may have been authored against XIP_BASE (0x10000000), or against the
	 * actual partition-A address (0x10002000), or partition-B (0x10202000).
	 * Whatever it is, everything from here on is interpreted as
	 *   offset_in_image = block->target_addr - src_base
	 * and re-based to state->flash_update. This guarantees we only ever
	 * touch the inactive partition. */
	if (!state->src_base_valid) {
		state->src_base = block->target_addr;
		state->src_base_valid = true;
		printf("[OTA] src_base locked to:[%x] -> flash_update:[%x]\n",
			state->src_base, state->flash_update);
	}

	if (block->target_addr < state->src_base) {
		printf("[ERROR] block target_addr:[%x] below src_base:[%x]\n",
			block->target_addr, state->src_base);
		return -1;
	}
	uint32_t offset_in_image = block->target_addr - state->src_base;
	flash_addr = state->flash_update + offset_in_image;

	/* Hard bound: stay inside the target partition (state->write_size bytes
	 * starting at state->flash_update). Writing outside means we'd be
	 * overwriting the running partition - that's the bug we're fixing. */
	if (offset_in_image + 256 > state->write_size) {
		printf("[ERROR] write outside target partition: offset:[%x] size:[%x]\n",
			offset_in_image, state->write_size);
		return -1;
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
		fw_integrity_check_repair();

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
	fw_integrity_check_repair();

	if (ret != 0) {
		printf("[ERROR] Flash program failed with code: %d\n", ret);
		return -1;
	}
	if (ret != 0) {
		printf("[ERROR] Flash program failed with code: %d\n", ret);
		return -1;
	}

	state->blocks_done++;

	if (state->blocks_seen >= state->num_blocks) {
		/* Direct-UART diagnostic dump BEFORE we trust printf/stdio_flush.
		 * If counters are nonzero we know XIP/cache was disturbed. */
		fw_integrity_dump_uart();

		/* Diagnostic: take the address of a known flash-resident format
		 * string, dump it via direct UART (no printf, no libc, no flash
		 * other than the bytes themselves). If we see clean ASCII here,
		 * flash is intact at that address and printf itself is broken.
		 * If we see garbage here, flash bytes at that address are
		 * corrupted - i.e. we wrote over the running partition. */
		static const char ota_complete_fmt[] =
			"[OTA] Update complete (ops=%u qmi_mm=%u canary_mm=%u), rebooting...\r\n";
		fw_dump_flash_region("fmt", (const uint8_t *)ota_complete_fmt, sizeof(ota_complete_fmt));

		/* Also dump bytes around several spread-out addresses in the
		 * running partition's expected flash window so we can see if
		 * some regions are corrupted while others (like the canary)
		 * are intact. */
		fw_dump_flash_region("a000", (const uint8_t *)(XIP_BASE + 0x10000), 32);
		fw_dump_flash_region("a040", (const uint8_t *)(XIP_BASE + 0x40000), 32);
		fw_dump_flash_region("a080", (const uint8_t *)(XIP_BASE + 0x80000), 32);
		fw_dump_flash_region("a100", (const uint8_t *)(XIP_BASE + 0x100000), 32);
		fw_dump_flash_region("a180", (const uint8_t *)(XIP_BASE + 0x180000), 32);
		fw_dump_flash_region("a1F0", (const uint8_t *)(XIP_BASE + 0x1F0000), 32);

		/* Now try the regular printf path; if XIP is healthy this prints
		 * cleanly, if not we'll see garbage (which itself is the signal). */
		printf(ota_complete_fmt,
			(unsigned)diag_flash_ops,
			(unsigned)diag_qmi_mismatches,
			(unsigned)diag_canary_mismatches);
		stdio_flush();
		save_and_disable_interrupts();
		int ret = rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_FLASH_UPDATE | REBOOT2_FLAG_NO_RETURN_ON_SUCCESS, 100, state->flash_update, 0);
		free(state);
		sleep_ms(1000);
		while(1) {
			watchdog_reboot(0, 0, 0);
		}

		//printf("[fw] *** DOWNLOAD COMPLETE! ***\n");
		//state->complete = true;
		return 0;
	}

	if (state->blocks_done % 10 == 0) {
		printf("[fw] Segment written flash_addr:[%x] state->blocks_done:[%d/%d] qmi_mm:[%u] canary_mm:[%u]\r\n",
			flash_addr, state->blocks_done, state->num_blocks,
			(unsigned)diag_qmi_mismatches, (unsigned)diag_canary_mismatches);
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

	/* Snapshot the known-good QMI/XIP state and the canary BEFORE any flash
	 * operation. Everything that follows is compared/repaired against this. */
	fw_integrity_init();

	return 0;
}
