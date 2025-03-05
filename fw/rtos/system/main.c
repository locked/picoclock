#define PICO_MAX_SHARED_IRQ_HANDLERS 5u

// C library
#include "main.h"
#include "debug.h"
#include "filesystem.h"
#include "qor.h"
#include "sdcard.h"

#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "system.h"
#include "ui_task.h"
#include "net_task.h"
#include "fs_task.h"

// Raspberry Pico SDK
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/rtc.h"
#include "hardware/i2c.h"
#include "pico/unique_id.h"
#include "pico.h"

// Screen
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"

#include "mcp46XX/mcp46XX.h"
#include "mcp23009/mcp23009.h"
#include "pcf8563/pcf8563.h"

// Audio (PIO)
#include "audio_player.h"
#include "pico_i2s.h"
static __attribute__((aligned(8))) pio_i2s i2s;
static volatile uint32_t msTicks = 0;
static audio_ctx_t audio_ctx;
static audio_i2s_config_t config = {
    .freq = 22050,
    .bps = 32,
    .data_pin = I2S_DATA_PIN,
    .clock_pin_base = I2S_CLOCK_PIN_BASE
};


//
// Globals
//
weather_struct weather = {"", "", "", ""};
int current_screen = 0;
config_struct global_config;

qor_mbox_t NetMailBox;


void ost_hal_panic() {
}

// ===========================================================================================================
// IDLE TASK
// ===========================================================================================================
static qor_tcb_t IdleTcb;
static uint32_t IdleStack[1024];
void IdleTask(void *args) {
    while (1) {
        // Instrumentation, power saving, os functions won't work here
        __asm volatile("wfi");
    }
}



// ===========================================================================================================
// CONSTANTS / DEFINES
// ===========================================================================================================

const uint8_t BUTTONS[] = {BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6};
uint8_t last_btn_values[6] = {1, 1, 1, 1, 1, 1};

// PICO alarm (RTOS uses Alarm 0 and IRQ 0)
#define ALARM_NUM 1
#define ALARM_IRQ TIMER_IRQ_1

// ===========================================================================================================
// GLOBAL VARIABLES
// ===========================================================================================================

// Buttons
static ost_button_callback_t ButtonCallback = NULL;
static uint32_t DebounceTs = 0;
static bool IsDebouncing = false;
static uint32_t ButtonsState = 0;
static uint32_t ButtonsStatePrev = 0;

static int new_value, delta, old_value = 0;


// ===========================================================================================================
// PROTOTYPES
// ===========================================================================================================
extern void init_spi(void);
void dma_init();
void __isr __time_critical_func(audio_i2s_dma_irq_handler)();

// ===========================================================================================================
// OST HAL IMPLEMENTATION
// ===========================================================================================================
void ost_system_delay_ms(uint32_t delay) {
    busy_wait_ms(delay);
}

void check_buttons() {
    for (uint8_t btn=0; btn<6; btn++) {
        uint8_t btn_value = gpio_get(BUTTONS[btn]);
        if (btn_value == 1 && btn_value != last_btn_values[btn]) {
            ButtonCallback(btn);
        }
        last_btn_values[btn] = btn_value;
    }
}

static void alarm_in_us(uint32_t delay_us);

static void alarm_irq(void) {
    // Clear the alarm irq
    hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
    alarm_in_us(10000);

    check_buttons();
}

static void alarm_in_us(uint32_t delay_us) {
    // Enable the interrupt for our alarm (the timer outputs 4 alarm irqs)
    hw_set_bits(&timer_hw->inte, 1u << ALARM_NUM);

    // Enable interrupt in block and at processor

    // Alarm is only 32 bits so if trying to delay more
    // than that need to be careful and keep track of the upper
    // bits
    uint64_t target = timer_hw->timerawl + delay_us;

    // Write the lower 32 bits of the target time to the alarm which
    // will arm it
    timer_hw->alarm[ALARM_NUM] = (uint32_t)target;
}


void init_i2c() {
    i2c_init(I2C_CHANNEL, I2C_BAUD_RATE);

    //printf("[picoclock] sensors: setting I2C pins: SDA=%d SCL=%d\n", SENSORS_SDA_PIN, SENSORS_SCL_PIN);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    //printf("[picoclock] sensors: pulling I2C pins up\n");
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

static void audio_callback(void) {
    ost_audio_process();
}

void system_initialize() {
    stdio_init_all();

    set_sys_clock_khz(125000, true);

    //stdio_init_all();
    sleep_ms(500);

    // Init UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    printf("[picoclock] START\r\n");

    // Init I2C
    init_i2c();
    printf("[picoclock] init_i2c OK\r\n");

	pcf8563_set_i2c(I2C_CHANNEL);
    printf("[picoclock] pcf8563_set_i2c OK\r\n");

    // Init GPIO extender
#ifndef PCBV1
    mcp23009_set_i2c(I2C_CHANNEL);
    bool mcp23009_enabled = mcp23009_is_connected();
    printf("[picoclock] mcp23009_is_connected: [%d]\r\n", mcp23009_enabled);
    if (mcp23009_enabled) {
        //mcp23009_config(true, false, false, false);
        // All output except GP1 (SD_DET)
        mcp23009_set_direction(0b01000000);

        // Mute
        mcp23009_set(0, 0);
        // Screen off
        mcp23009_set(FRONT_PANEL_LED_PIN, 0);
    }
#endif

    // Init Sound
#ifndef PCBV1
    mcp46XX_set_i2c(I2C_CHANNEL);
    mcp4651_set_wiper(0x100);
#endif
    i2s_program_setup(pio0, audio_i2s_dma_irq_handler, &i2s, &config);
    audio_init(&audio_ctx);
    ost_audio_register_callback(audio_callback);
    printf("[picoclock] init sound OK\r\n");

    // Init time
    // Set time to known values to identify it needs to be updated
    /*rtc_init();
    datetime_t dt;
    dt.year = 1000;
    dt.month = 1;
    dt.day = 1;
    dt.dotw = 1;
    dt.hour = 1;
    dt.min = 1;
    dt.sec = 1;
    rtc_set_datetime(&dt);
    printf("[picoclock] Init rtc OK\r\n");*/

    //------------------- Init LCD
    printf("[picoclock] Init e-Paper module...\r\n");
    if(DEV_Module_Init()!=0){
        return;
    }
    EPD_2in13_V4_Init();
    EPD_2in13_V4_Clear();

    //------------------- Buttons init
    for (uint8_t btn=0; btn<6; btn++) {
        gpio_init(BUTTONS[btn]);
        gpio_set_dir(BUTTONS[btn], GPIO_IN);
    }
    /*debounce_debounce();
    for (uint8_t btn=0; btn<4; btn++) {
        debug_printf("[picoclock] Debounce button [%d] GPIO [%d]...\r\n", btn, BUTTONS[btn]);
        if (debounce_gpio(BUTTONS[btn]) == -1) {
            debug_printf("[picoclock] Error debouncing GPIO [%d]...\r\n", BUTTONS[btn]);
        }
    }*/

    // Set irq handler for alarm irq
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
    // Enable the alarm irq
    irq_set_enabled(ALARM_IRQ, true);

    alarm_in_us(100000);

    //------------------- Init SDCARD
    gpio_init(SD_CARD_CS);
    gpio_put(SD_CARD_CS, 1);
    gpio_set_dir(SD_CARD_CS, GPIO_OUT);

    // Need to handle i2c extender
    //gpio_init(SD_CARD_PRESENCE);
    //gpio_set_dir(SD_CARD_PRESENCE, GPIO_IN);

    spi_init(spi1, 1000 * 1000); // slow clock

    spi_set_format(spi1, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(SDCARD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_MISO, GPIO_FUNC_SPI);
    printf("[picoclock] Init SD card OK\r\n");

    //gpio_set_function(GPIO_PWM, GPIO_FUNC_PWM);

    // ------------ Everything is initialized, print stuff here
    printf("[picoclock] System Clock: %lu\n", clock_get_hz(clk_sys));
}

void system_putc(char ch) {
    uart_putc_raw(UART_ID, ch);
}

#include <time.h>
clock_t clock() {
    return (clock_t)time_us_64() / 1000;
}
static uint64_t stopwatch_start_time;
static uint64_t stopwatch_end_time;

void ost_system_stopwatch_start() {
    stopwatch_start_time = clock();
}

uint32_t ost_system_stopwatch_stop() {
    stopwatch_end_time = clock();
    return (stopwatch_end_time - stopwatch_start_time);
}

void ost_button_register_callback(ost_button_callback_t cb) {
    ButtonCallback = cb;
}


// ----------------------------------------------------------------------------
// SDCARD HAL
// ----------------------------------------------------------------------------
void ost_hal_sdcard_set_slow_clock() {
  spi_set_baudrate(spi1, 1000000UL);
}

void ost_hal_sdcard_set_fast_clock() {
  spi_set_baudrate(spi1, 40000000UL);
}

void ost_hal_sdcard_cs_high() {
  gpio_put(SD_CARD_CS, 1);
}

void ost_hal_sdcard_cs_low() {
  gpio_put(SD_CARD_CS, 0);
}

void ost_hal_sdcard_spi_exchange(const uint8_t *buffer, uint8_t *out, uint32_t size) {
  spi_write_read_blocking(spi1, buffer, out, size);
}

void ost_hal_sdcard_spi_write(const uint8_t *buffer, uint32_t size) {
  spi_write_blocking(spi1, buffer, size);
}

void ost_hal_sdcard_spi_read(uint8_t *out, uint32_t size) {
  spi_read_blocking(spi1, 0xFF, out, size);
}

uint8_t ost_hal_sdcard_get_presence() {
  return 1; // not wired
}


// ----------------------------------------------------------------------------
// AUDIO HAL
// ----------------------------------------------------------------------------

void ost_audio_play(const char *filename) {
    mcp23009_set(0, 1); // Unmute

    printf("audio_play... [%s]\r\n", filename);
    audio_play(&audio_ctx, filename);
    config.freq = audio_ctx.audio_info.sample_rate;
    config.channels = audio_ctx.audio_info.channels;
    printf("pico_i2s_set_frequency...\r\n");
    pico_i2s_set_frequency(&i2s, &config);

    i2s.buffer_index = 0;

    // On appelle une première fois le process pour récupérer et initialiser le premier buffer...
    printf("audio_process 1st buffer...\r\n");
    audio_process(&audio_ctx);

    // Puis le deuxième ... (pour avoir un buffer d'avance)
    audio_process(&audio_ctx);

    // On lance les DMA
    printf("i2s_start...\r\n");
    i2s_start(&i2s);
}

void ost_audio_stop() {
    mcp23009_set(0, 0); // Mute

    debug_printf("audio stop\r\n");
    memset(i2s.out_ctrl_blocks[0], 0, STEREO_BUFFER_SIZE * sizeof(uint32_t));
    memset(i2s.out_ctrl_blocks[1], 0, STEREO_BUFFER_SIZE * sizeof(uint32_t));
    audio_stop(&audio_ctx);
    i2s_stop(&i2s);
}

int ost_audio_process() {
    return audio_process(&audio_ctx);
}

static ost_audio_callback_t AudioCallBack = NULL;

void ost_audio_register_callback(ost_audio_callback_t cb) {
    AudioCallBack = cb;
}

void ost_hal_audio_new_frame(const void *buff, int size) {
    if (size > STEREO_BUFFER_SIZE) {
        // Problème
        debug_printf("[picoclock] WARNING: audio_new_frame size:[%d] > STEREO_BUFFER_SIZE\n", size);
        return;
    }
    memcpy(i2s.out_ctrl_blocks[i2s.buffer_index], buff, size * sizeof(uint32_t));
    i2s.buffer_index = 1 - i2s.buffer_index;
}

void __isr __time_critical_func(audio_i2s_dma_irq_handler)() {
    dma_hw->ints0 = 1u << i2s.dma_ch_out_data; // clear the IRQ

    // Warn the application layer that we have done on that channel
    if (AudioCallBack != NULL) {
        AudioCallBack();
    }
}






// ===========================================================================================================
// MAIN ENTRY POINT
// ===========================================================================================================
int main() {
    // 1. Call the platform initialization
    system_initialize();

    // 2. Test the printf output
    printf("[picoclock] Starting: V%d.%d\r\n", 1, 0);

	// Init config with default values
	strncpy(global_config.wifi_ssid, WIFI_SSID, 50);
	strncpy(global_config.wifi_key, WIFI_PASSWORD, 50);
	strncpy(global_config.remote_host, SERVER_IP, 50);

    // 3. Filesystem / SDCard initialization
    printf("[picoclock] Check SD card\r\n");
    filesystem_mount();
    // List files on sdcard (test)
    filesystem_read_config_file();

    // 4. Initialize OS before all other OS calls
    printf("[picoclock] Initialize OS before all other OS calls\r\n");
    qor_init(125000000UL);

    // 5. Initialize the tasks
    printf("[picoclock] Initialize NET tasks\r\n");
    net_task_initialize();
    printf("[picoclock] Initialize UI tasks\r\n");
    ui_task_initialize();
    printf("[picoclock] Initialize FS tasks\r\n");
    fs_task_initialize();

    // 6. Start the operating system!
    printf("[picoclock] Start OS\r\n");
    qor_start(&IdleTcb, IdleTask, IdleStack, 1024);

    return 0;
}
