#ifndef GRAPHICS_LIB_H
#define GRAPHICS_LIB_H

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

#endif // GRAPHICS_LIB_H
