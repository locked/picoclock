/**
 * @file ui_task.c
 *
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-07-29
 *
 * @copyright Copyright (c) 2023
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "ost_hal.h"
#include "debug.h"
#include "qor.h"
#include "system.h"

// Screen
#include "DEV_Config.h"
#include "GUI_Paint.h"
#include "ImageData.h"
#include "EPD_2in13_V4.h"


// ===========================================================================================================
// DEFINITIONS
// ===========================================================================================================

typedef struct
{
    uint8_t ev;
} ost_ui_event_t;

typedef enum
{
    OST_SYS_WAIT_INDEX,
    OST_SYS_PLAY_STORY_TITLE,
    OST_SYS_WAIT_USER_EVENT
} ost_system_state_t;

// ===========================================================================================================
// GLOBAL STORY VARIABLES
// ===========================================================================================================

static qor_tcb_t UiTcb;
static uint32_t UiStack[4096];

static qor_mbox_t UiMailBox;

static ost_ui_event_t UiEvent;

static ost_ui_event_t *UiQueue[10];

static ost_system_state_t OstState = OST_SYS_WAIT_INDEX;

static ost_context_t OstContext;

// ===========================================================================================================
// UI TASK (user interface, buttons manager, LCD)
// ===========================================================================================================
void UiTask(void *args)
{
    ost_ui_event_t *message = NULL;

    // init screen
    //ui_init();

    while (1)
    {
        // res = qor_mbox_wait(&UiMailBox, (void **)&message, 5);

        // if (res == QOR_MBOX_OK)
        // {
        //     if (message->ev == UI_EV_CONNECTED)
        //     {

        //         ConnectedState = true;
        //     }
        // }

        qor_sleep(500);
        debug_printf("\r\n[UI] task woke up\r\n");
    }
}

void ui_task_initialize()
{
    OstState = OST_SYS_WAIT_INDEX;
    qor_mbox_init(&UiMailBox, (void **)&UiQueue, 10);

    qor_create_thread(&UiTcb, UiTask, UiStack, sizeof(UiStack) / sizeof(UiStack[0]), HMI_TASK_PRIORITY, "UiTask");
}
