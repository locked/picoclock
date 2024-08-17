#include "graphics.h"

#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"

#include "icons.h"


void display_icon(int x, int y, int icon_id) {
	line_struct *tmp;
	size_t count;
	if (icon_id == ICON_LIGHT) {
		tmp = icon_light;
		count = sizeof(icon_light) / sizeof(line_struct);
	} else if (icon_id == ICON_RIGHTARROW) {
		tmp = icon_rightarrow;
		count = sizeof(icon_rightarrow) / sizeof(line_struct);
	} else if (icon_id == ICON_LEFTARROW) {
		tmp = icon_leftarrow;
		count = sizeof(icon_leftarrow) / sizeof(line_struct);
	} else {
		return;
	}
	for(size_t i = 0; i < count; i++) {
		line_struct line;
		if (icon_id == ICON_LIGHT) {
			line = icon_light[i];
		} else if (icon_id == ICON_RIGHTARROW) {
			line = icon_rightarrow[i];
		} else if (icon_id == ICON_LEFTARROW) {
			line = icon_leftarrow[i];
		}
		Paint_DrawLine(line.xstart + x, line.y + y, line.xend + x, line.y + y, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
		//Paint_SetPixel(line.xstart + x, line.y + y, BLACK);
		//Paint_SetPixel(line.xend + x, line.y + y, BLACK);
		//Paint_SetPixel(20, i + 40, BLACK);
	}
}
