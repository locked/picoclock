#include "graphics.h"

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"

#include "icons.h"


void display_icon(int x, int y, int icon_id) {
	icon_struct icon = icons[icon_id];
	line_struct line;
	size_t count = icon.size / sizeof(line_struct);
	for(size_t i = 0; i < count; i++) {
		line = icon.lines[i];
		Paint_DrawLine(line.xstart + x, line.y + y, line.xend + x, line.y + y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
	}
}
