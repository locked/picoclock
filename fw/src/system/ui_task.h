#ifndef UI_TASK_H
#define UI_TASK_H

#include <stdint.h>

void init_ui();
void ui_refresh_screen(bool message_clear, time_struct dt);
void ui_btn_click(int btn, time_struct dt);

#endif // UI_TASK_H
