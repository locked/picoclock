#ifndef GRAPHICS_LIB_H
#define GRAPHICS_LIB_H

#include "pcf8563/pcf8563.h"
#include "circularBuffer.h"

#define ICON_LIGHT 0
#define ICON_RIGHTARROW 1
#define ICON_LEFTARROW 2
#define ICON_WIFI 3

typedef struct {
  unsigned short y;
  unsigned short xstart;
  unsigned short xend;
} line_struct;

typedef struct {
  line_struct* lines;
  unsigned short size;
} icon_struct;

void display_icon(int x, int y, int icon_id);

#define ICON_LEFT_X 0
#define ICON_RIGHT_X 214
#define SCREEN_X 32
#define SCREEN_Y 8

#define SCREEN_WIDTH 256
#define SCREEN_HEIGHT 80

void display_screen_nav();
void display_screen_main(time_struct dt, circularBuffer_t* ring_metrics);
void display_screen_weather();
void display_screen_alarms();
void display_screen_debug(char *last_sync_str, uint8_t current_rgb_color, uint8_t* rgb_intensity);
void display_screen_alarm();
void display_screen_metrics(circularBuffer_t* ring_metrics);

#endif // GRAPHICS_LIB_H
