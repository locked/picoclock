// C library
#include <string.h>
#include <stdlib.h>

// OST common files
#include "ost_hal.h"
#include "debug.h"
#include "qor.h"

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

// Local
#include "audio_player.h"
#include "pico_i2s.h"
#include "button_debounce.h"

// ===========================================================================================================
// CONSTANTS / DEFINES
// ===========================================================================================================

const uint8_t BUTTONS[] = {8, 9, 10, 19, 20, 21};
uint8_t last_btn_values[6] = {1, 1, 1, 1, 1, 1};

//#define SDCARD_SCK 18
//#define SDCARD_MOSI 19
//#define SDCARD_MISO 16
//const uint8_t SD_CARD_CS = 17;
//const uint8_t SD_CARD_PRESENCE = 24;

#define UART_ID uart0
#define BAUD_RATE 115200
#define UART_TX_PIN 0

const uint LED_PIN = 25;

// PICO alarm (RTOS uses Alarm 0 and IRQ 0)
#define ALARM_NUM 1
#define ALARM_IRQ TIMER_IRQ_1

#define I2C_SDA 12
#define I2C_SCL 13
#define I2C_BAUD_RATE 400000

// ===========================================================================================================
// GLOBAL VARIABLES
// ===========================================================================================================
static __attribute__((aligned(8))) pio_i2s i2s;
static volatile uint32_t msTicks = 0;
static audio_ctx_t audio_ctx;

// Buttons
static ost_button_callback_t ButtonCallback = NULL;
static uint32_t DebounceTs = 0;
static bool IsDebouncing = false;
static uint32_t ButtonsState = 0;
static uint32_t ButtonsStatePrev = 0;

// Rotary encoder
// pio 0 is used
static PIO pio = pio1;
// state machine 0
static uint8_t sm = 0;

static int new_value, delta, old_value = 0;

static audio_i2s_config_t config = {
    .freq = 44100,
    .bps = 32,
    .data_pin = 28,
    .clock_pin_base = 26
};

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

uint8_t get_uniq_id() {
    pico_unique_board_id_t board_uid;
    pico_get_unique_board_id(&board_uid);
    return *(board_uid.id);
}

void check_buttons() {
    for (uint8_t btn=0; btn<6; btn++) {
        uint8_t btn_value = gpio_get(BUTTONS[btn]);
        if (btn_value == 1 && btn_value != last_btn_values[btn]) {
            ButtonCallback(btn);
        }
        last_btn_values[btn] = btn_value;
    }
    /*
     * Not enough PIO/SM for PIO debounce
    for (uint8_t btn=0; btn<4; btn++) {
        uint8_t btn_value = debounce_read(BUTTONS[btn]);
        if (btn_value == 1 && btn_value != last_btn_values[btn]) {
            ButtonCallback(btn);
        }
        last_btn_values[btn] = btn_value;
    }
    */
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
    i2c_init(i2c0, I2C_BAUD_RATE);

    //printf("sensors: setting I2C pins: SDA=%d SCL=%d\n", SENSORS_SDA_PIN, SENSORS_SCL_PIN);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    //printf("sensors: pulling I2C pins up\n");
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

void ost_system_initialize()
{
    set_sys_clock_khz(125000, true);

    ////------------------- Init DEBUG LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    //------------------- Init DEBUG PIN
    //gpio_init(DEBUG_PIN);
    //gpio_set_dir(DEBUG_PIN, GPIO_OUT);

    //------------------- Init UART

    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);

    // Init I2C
    init_i2c();

    //------------------- Init time
    // Set time to known values to identify it needs to be updated
    rtc_init();
    datetime_t dt;
    dt.year = 1000;
    dt.month = 1;
    dt.day = 1;
    dt.dotw = 1;
    dt.hour = 1;
    dt.min = 1;
    dt.sec = 1;
    rtc_set_datetime(&dt);

    // Front panel LEDs
    gpio_init(FRONT_PANEL_LED_PIN);
    gpio_set_dir(FRONT_PANEL_LED_PIN, GPIO_OUT);

    //------------------- Init LCD
    debug_printf("Init e-Paper module...\r\n");
    if(DEV_Module_Init()!=0){
      return;
    }

    debug_printf("e-Paper Init and Clear...\r\n");
    EPD_2in13_V4_Init();
    EPD_2in13_V4_Clear();

    //------------------- Buttons init
    for (uint8_t btn=0; btn<6; btn++) {
        gpio_init(BUTTONS[btn]);
        gpio_set_dir(BUTTONS[btn], GPIO_IN);
    }
    /*debounce_debounce();
    for (uint8_t btn=0; btn<4; btn++) {
        debug_printf("Debounce button [%d] GPIO [%d]...\r\n", btn, BUTTONS[btn]);
        if (debounce_gpio(BUTTONS[btn]) == -1) {
            debug_printf("Error debouncing GPIO [%d]...\r\n", BUTTONS[btn]);
        }
    }*/

    // Set irq handler for alarm irq
    irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
    // Enable the alarm irq
    irq_set_enabled(ALARM_IRQ, true);

    alarm_in_us(100000);

    //------------------- Init SDCARD
/*
    gpio_init(SD_CARD_CS);
    gpio_put(SD_CARD_CS, 1);
    gpio_set_dir(SD_CARD_CS, GPIO_OUT);

    gpio_init(SD_CARD_PRESENCE);
    gpio_set_dir(SD_CARD_PRESENCE, GPIO_IN);

    spi_init(spi0, 1000 * 1000); // slow clock

    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(SDCARD_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(SDCARD_MISO, GPIO_FUNC_SPI);
*/

    //------------------- Init Sound
/*
    i2s_program_setup(pio0, audio_i2s_dma_irq_handler, &i2s, &config);
    audio_init(&audio_ctx);
    char fakefile[6] = "fake";
    ost_audio_play(fakefile);
*/
    gpio_set_function(16, GPIO_FUNC_PWM);

    // ------------ Everything is initialized, print stuff here
    debug_printf("System Clock: %lu\n", clock_get_hz(clk_sys));
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

int ost_hal_gpio_get(ost_hal_gpio_t gpio) {
    int value = 0;
    switch (gpio) {
        break;
    default:
        break;
    }

    return value;
}

void ost_hal_gpio_set(ost_hal_gpio_t gpio, int value) {
    switch (gpio) {
    case OST_GPIO_DEBUG_LED:

        break;
    case OST_GPIO_DEBUG_PIN:

        break;
    default:
        break;
    }
}

// ----------------------------------------------------------------------------
// SDCARD HAL
// ----------------------------------------------------------------------------
/*
void ost_hal_sdcard_set_slow_clock()
{
  spi_set_baudrate(spi0, 1000000UL);
}

void ost_hal_sdcard_set_fast_clock()
{
  spi_set_baudrate(spi0, 40000000UL);
}

void ost_hal_sdcard_cs_high()
{
  gpio_put(SD_CARD_CS, 1);
}

void ost_hal_sdcard_cs_low()
{
  gpio_put(SD_CARD_CS, 0);
}

void ost_hal_sdcard_spi_exchange(const uint8_t *buffer, uint8_t *out, uint32_t size)
{
  spi_write_read_blocking(spi0, buffer, out, size);
}

void ost_hal_sdcard_spi_write(const uint8_t *buffer, uint32_t size)
{
  spi_write_blocking(spi0, buffer, size);
}

void ost_hal_sdcard_spi_read(uint8_t *out, uint32_t size)
{
  spi_read_blocking(spi0, 0xFF, out, size);
}

uint8_t ost_hal_sdcard_get_presence()
{
  return 1; // not wired
}
*/

// ----------------------------------------------------------------------------
// AUDIO HAL
// ----------------------------------------------------------------------------

void ost_audio_play(const char *filename)
{
    audio_play(&audio_ctx, filename);
    config.freq = audio_ctx.audio_info.sample_rate;
    config.channels = audio_ctx.audio_info.channels;
    pico_i2s_set_frequency(&i2s, &config);

    i2s.buffer_index = 0;

    // On appelle une première fois le process pour récupérer et initialiser le premier buffer...
    audio_process(&audio_ctx);

    // Puis le deuxième ... (pour avoir un buffer d'avance)
    audio_process(&audio_ctx);

    // On lance les DMA
    i2s_start(&i2s);
}

void ost_audio_stop() {
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
