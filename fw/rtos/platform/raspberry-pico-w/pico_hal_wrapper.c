#define PICO_MAX_SHARED_IRQ_HANDLERS 5u

// C library
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

#include "mcp4652/mcp4652.h"
#include "mcp9808/mcp9808.h"
#include "mcp23009/mcp23009.h"


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

// Audio (no PIO)
/*
#include <math.h>
#include "hardware/structs/clocks.h"
#include "pico/audio_i2s.h"
#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(I2S_DATA_PIN, "I2S DIN", I2S_CLOCK_PIN_BASE, "I2S BCK", I2S_CLOCK_PIN_BASE+1, "I2S LRCK"));
*/

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

//static PIO pio = pio0;
//static uint8_t sm = 0;

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




/*
#define SINE_WAVE_TABLE_LEN 2048
#define SAMPLES_PER_BUFFER 512

static int16_t sine_wave_table[SINE_WAVE_TABLE_LEN];


struct audio_buffer_pool *init_audio() {

    static audio_format_t audio_format = {
            .sample_freq = 24000,
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .channel_count = 1,
    };

    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 2
    };

    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                      SAMPLES_PER_BUFFER); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;
    struct audio_i2s_config config = {
            .data_pin = I2S_DATA_PIN,
            .clock_pin_base = I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };

    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }

    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);
    return producer_pool;
}

void play_sine() {
    printf("starting core 1\r\n");
    for (int i = 0; i < SINE_WAVE_TABLE_LEN; i++) {
        sine_wave_table[i] = 32767 * cosf(i * 2 * (float) (M_PI / SINE_WAVE_TABLE_LEN));
    }

    struct audio_buffer_pool *ap = init_audio();
    printf("init_audio OK [%d] [%d]\r\n", I2S_DATA_PIN, I2S_CLOCK_PIN_BASE);
    uint32_t step = 0x200000;
    uint32_t pos = 0;
    uint32_t pos_max = 0x10000 * SINE_WAVE_TABLE_LEN;
    uint vol = 70;
    int wiper = 0;
    int wiper_dir = 1;
    while (true) {
        struct audio_buffer *buffer = take_audio_buffer(ap, true);
        int16_t *samples = (int16_t *) buffer->buffer->bytes;
        for (uint i = 0; i < buffer->max_sample_count; i++) {
            samples[i] = (vol * sine_wave_table[pos >> 16u]) >> 8u;
            pos += step;
            if (pos >= pos_max) pos -= pos_max;
        }
        buffer->sample_count = buffer->max_sample_count;
        //printf("give_audio_buffer, wiper:[%d]..\r\n", wiper);
        give_audio_buffer(ap, buffer);
        //sleep_ms(1);
        wiper += wiper_dir;
        if (wiper >= 0x100) {
            wiper_dir = -1;
        }
        if (wiper == 0) {
            wiper_dir = 1;
        }
        mcp4652_set_wiper(wiper);
    }
    printf("DONE\r\n");
}
*/



void init_i2c() {
    i2c_init(I2C_CHANNEL, I2C_BAUD_RATE);

    //printf("sensors: setting I2C pins: SDA=%d SCL=%d\n", SENSORS_SDA_PIN, SENSORS_SCL_PIN);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);

    //printf("sensors: pulling I2C pins up\n");
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

static void audio_callback(void) {
    ost_audio_process();
}

void ost_system_initialize()
{
    stdio_init_all();

    set_sys_clock_khz(125000, true);

    //stdio_init_all();
    sleep_ms(500);

    // Init DEBUG LED
    //gpio_init(LED_PIN);
    //gpio_set_dir(LED_PIN, GPIO_OUT);

    // Init DEBUG PIN
    //gpio_init(DEBUG_PIN);
    //gpio_set_dir(DEBUG_PIN, GPIO_OUT);

    // Init UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    printf("START\r\n");

    // Init I2C
    init_i2c();

    // Init GPIO extender
    mcp23009_set_i2c(I2C_CHANNEL);
    bool mcp23009_status = mcp23009_is_connected();
    mcp23009_set_direction(0b00100000);
    printf("mcp23009_is_connected: [%d]\r\n", mcp23009_status);

    // Mute
    mcp23009_set(0, 0);

    // Init Sound
    mcp4652_set_i2c(I2C_CHANNEL);
    mcp4652_set_wiper(0x100);
    i2s_program_setup(pio0, audio_i2s_dma_irq_handler, &i2s, &config);
    audio_init(&audio_ctx);
    ost_audio_register_callback(audio_callback);

    // Init time
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
    //gpio_init(FRONT_PANEL_LED_PIN);
    //gpio_set_dir(FRONT_PANEL_LED_PIN, GPIO_OUT);

    //------------------- Init LCD
    printf("Init e-Paper module...\r\n");
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


    //gpio_set_function(GPIO_PWM, GPIO_FUNC_PWM);

    // ------------ Everything is initialized, print stuff here
    printf("System Clock: %lu\n", clock_get_hz(clk_sys));
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
void ost_hal_sdcard_set_slow_clock()
{
  spi_set_baudrate(spi1, 1000000UL);
}

void ost_hal_sdcard_set_fast_clock()
{
  spi_set_baudrate(spi1, 40000000UL);
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
  spi_write_read_blocking(spi1, buffer, out, size);
}

void ost_hal_sdcard_spi_write(const uint8_t *buffer, uint32_t size)
{
  spi_write_blocking(spi1, buffer, size);
}

void ost_hal_sdcard_spi_read(uint8_t *out, uint32_t size)
{
  spi_read_blocking(spi1, 0xFF, out, size);
}

uint8_t ost_hal_sdcard_get_presence()
{
  return 1; // not wired
}


// ----------------------------------------------------------------------------
// AUDIO HAL
// ----------------------------------------------------------------------------

/**/
void ost_audio_play(const char *filename) {
    mcp23009_set(0, 1); // Unmute

    debug_printf("audio_play... [%s]\r\n", filename);
    audio_play(&audio_ctx, filename);
    config.freq = audio_ctx.audio_info.sample_rate;
    config.channels = audio_ctx.audio_info.channels;
    debug_printf("pico_i2s_set_frequency...\r\n");
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
    debug_printf("register audio callback\r\n");
    AudioCallBack = cb;
}

void ost_hal_audio_new_frame(const void *buff, int size) {
    if (size > STEREO_BUFFER_SIZE) {
        // Problème
        debug_printf("ost_hal_audio_new_frame size:[%d] > STEREO_BUFFER_SIZE\n", size);
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

