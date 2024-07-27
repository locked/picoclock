// C library
#include <string.h>
#include <stdlib.h>

// OST common files
#include "ost_hal.h"
#include "debug.h"
#include <ff.h>
#include "diskio.h"
#include "qor.h"

// Raspberry Pico SDK
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico.h"

// Screen
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"

// Local
#include "audio_player.h"
#include "pico_i2s.h"
// #include "pio_rotary_encoder.pio.h"

#include "wifi.h"

// ===========================================================================================================
// CONSTANTS / DEFINES
// ===========================================================================================================

//const uint8_t ROTARY_A = 6;
//const uint8_t ROTARY_B = 7;
//const uint8_t ROTARY_BUTTON = 1;

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
    .clock_pin_base = 26};

// ===========================================================================================================
// PROTOTYPES
// ===========================================================================================================
extern void init_spi(void);
void dma_init();
void __isr __time_critical_func(audio_i2s_dma_irq_handler)();

// ===========================================================================================================
// OST HAL IMPLEMENTATION
// ===========================================================================================================
void ost_system_delay_ms(uint32_t delay)
{
  busy_wait_ms(delay);
}

/*
void check_buttons()
{
  if (!gpio_get(ROTARY_BUTTON))
  {
    ButtonsState |= OST_BUTTON_OK;
  }
  else
  {
    ButtonsState &= ~OST_BUTTON_OK;
  }

  // note: thanks to two's complement arithmetic delta will always
  // be correct even when new_value wraps around MAXINT / MININT
  new_value = quadrature_encoder_get_count(pio, sm);
  delta = new_value - old_value;
  old_value = new_value;

  if (delta > 0)
  {
    ButtonsState |= OST_BUTTON_LEFT;
  }
  else if (delta < 0)
  {
    ButtonsState |= OST_BUTTON_RIGHT;
  }
  else
  {
    ButtonsState &= ~OST_BUTTON_LEFT;
    ButtonsState &= ~OST_BUTTON_RIGHT;
  }

  if (IsDebouncing)
  {
    // Même état pendant X millisecondes, on valide
    if ((ButtonsStatePrev == ButtonsState) && (ButtonCallback != NULL))
    {
      if (ButtonsState != 0)
      {
        ButtonCallback(ButtonsState);
      }
    }

    IsDebouncing = false;
    ButtonsStatePrev = ButtonsState;
  }
  else
  {
    // Changement d'état détecté
    if (ButtonsStatePrev != ButtonsState)
    {
      ButtonsStatePrev = ButtonsState;
      IsDebouncing = true;
    }
  }
}
*/

static void alarm_in_us(uint32_t delay_us);

static void alarm_irq(void)
{
  // Clear the alarm irq
  hw_clear_bits(&timer_hw->intr, 1u << ALARM_NUM);
  alarm_in_us(10000);

  //check_buttons();
}

static void alarm_in_us(uint32_t delay_us)
{
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
  //uart_puts(UART_ID, "==START==\r\n");
  //uart_default_tx_wait_blocking();
  //stdio_init_all();

  //------------------- Init Wifi and get time
  debug_printf("Connecting to wifi...\r\n");
  wifi_connect(WIFI_SSID, WIFI_PASSWORD);
  char response[200] = "";
  send_tcp(SERVER_IP, SERVER_PORT, "\n", 1, response);
  debug_printf("GOT DATE FROM SERVER:[%s]\n", response);
  wifi_disconnect();
  debug_printf("Disconnected from wifi\n");

  //------------------- Init LCD
  debug_printf("Init e-Paper module...\r\n");
  if(DEV_Module_Init()!=0){
    return -1;
  }

  debug_printf("e-Paper Init and Clear...\r\n");
  EPD_2in13_V4_Init();
  EPD_2in13_V4_Clear();

  // Create a new image cache
  UBYTE *BlackImage;
  UWORD Imagesize = ((EPD_2in13_V4_WIDTH % 8 == 0)? (EPD_2in13_V4_WIDTH / 8 ): (EPD_2in13_V4_WIDTH / 8 + 1)) * EPD_2in13_V4_HEIGHT;
  if((BlackImage = (UBYTE *)malloc(Imagesize)) == NULL) {
    debug_printf("Failed to apply for black memory...\r\n");
    return -1;
  }

  debug_printf("Paint_NewImage\r\n");
  Paint_NewImage(BlackImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  Paint_Clear(WHITE);
  EPD_2in13_V4_Display_Base(BlackImage);

  Paint_NewImage(BlackImage, EPD_2in13_V4_WIDTH, EPD_2in13_V4_HEIGHT, 90, WHITE);
  debug_printf("Partial refresh\r\n");
  Paint_SelectImage(BlackImage);
  Paint_SetRotate(270);

  Paint_DrawString_EN(70, 15, response, &Font20, WHITE, BLACK);
  //Paint_ClearWindows(10, 10, 150 + Font20.Width * 7, 80 + Font20.Height, WHITE);
  //Paint_DrawTime(10, 10, &sPaint_time, &Font20, WHITE, BLACK);

  EPD_2in13_V4_Display_Partial(BlackImage);
  DEV_Delay_ms(500);//Analog clock 1s

  debug_printf("Clear...\r\n");
  EPD_2in13_V4_Init();
  EPD_2in13_V4_Clear();

  debug_printf("Goto Sleep...\r\n");
  EPD_2in13_V4_Sleep();
  free(BlackImage);
  BlackImage = NULL;
  DEV_Delay_ms(2000);//important, at least 2s
  // close 5V
  debug_printf("close 5V, Module enters 0 power consumption ...\r\n");
  DEV_Module_Exit();

  //------------------- Rotary encoder init
/*
  gpio_init(ROTARY_BUTTON);
  gpio_set_dir(ROTARY_BUTTON, GPIO_IN);
  gpio_disable_pulls(ROTARY_BUTTON);

  // Rotary Encoder is managed by the PIO !
  // we don't really need to keep the offset, as this program must be loaded
  // at offset 0
  pio_add_program(pio, &quadrature_encoder_program);
  quadrature_encoder_program_init(pio, sm, ROTARY_A, 0);

  // Set irq handler for alarm irq
  irq_set_exclusive_handler(ALARM_IRQ, alarm_irq);
  // Enable the alarm irq
  irq_set_enabled(ALARM_IRQ, true);

  alarm_in_us(100000);

  //------------------- Init SDCARD
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

  //------------------- Init Sound
  i2s_program_setup(pio0, audio_i2s_dma_irq_handler, &i2s, &config);
  audio_init(&audio_ctx);
*/

  // ------------ Everything is initialized, print stuff here
  debug_printf("System Clock: %lu\n", clock_get_hz(clk_sys));
}

void system_putc(char ch)
{
  uart_putc_raw(UART_ID, ch);
}

#include <time.h>
clock_t clock()
{
  return (clock_t)time_us_64() / 1000;
}
static uint64_t stopwatch_start_time;
static uint64_t stopwatch_end_time;

void ost_system_stopwatch_start()
{
  stopwatch_start_time = clock();
}

uint32_t ost_system_stopwatch_stop()
{
  stopwatch_end_time = clock();
  return (stopwatch_end_time - stopwatch_start_time);
}

void ost_button_register_callback(ost_button_callback_t cb)
{
  ButtonCallback = cb;
}

int ost_hal_gpio_get(ost_hal_gpio_t gpio)
{
  int value = 0;
  switch (gpio)
  {
    break;
  default:
    break;
  }

  return value;
}

void ost_hal_gpio_set(ost_hal_gpio_t gpio, int value)
{
  switch (gpio)
  {
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

void ost_audio_stop()
{
  memset(i2s.out_ctrl_blocks[0], 0, STEREO_BUFFER_SIZE * sizeof(uint32_t));
  memset(i2s.out_ctrl_blocks[1], 0, STEREO_BUFFER_SIZE * sizeof(uint32_t));
  audio_stop(&audio_ctx);
  i2s_stop(&i2s);
}

int ost_audio_process()
{
  return audio_process(&audio_ctx);
}

static ost_audio_callback_t AudioCallBack = NULL;

void ost_audio_register_callback(ost_audio_callback_t cb)
{
  AudioCallBack = cb;
}

void ost_hal_audio_new_frame(const void *buff, int size)
{
  if (size > STEREO_BUFFER_SIZE)
  {
    // Problème
    return;
  }
  memcpy(i2s.out_ctrl_blocks[i2s.buffer_index], buff, size * sizeof(uint32_t));
  i2s.buffer_index = 1 - i2s.buffer_index;
}

void __isr __time_critical_func(audio_i2s_dma_irq_handler)()
{
  dma_hw->ints0 = 1u << i2s.dma_ch_out_data; // clear the IRQ

  // Warn the application layer that we have done on that channel
  if (AudioCallBack != NULL)
  {
    AudioCallBack();
  }
}
