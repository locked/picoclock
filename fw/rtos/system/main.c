#include "ost_hal.h"
#include "debug.h"
#include "filesystem.h"

#include "qor.h"

#include "sdcard.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "system.h"
#include "ui_task.h"

void ost_hal_panic()
{
}

// ===========================================================================================================
// IDLE TASK
// ===========================================================================================================
static qor_tcb_t IdleTcb;
static uint32_t IdleStack[1024];
void IdleTask(void *args)
{
    while (1)
    {
        // Instrumentation, power saving, os functions won't work here
        __asm volatile("wfi");
    }
}

qor_mbox_t NetMailBox;
wakeup_alarm_struct wakeup_alarm;


// ===========================================================================================================
// MAIN ENTRY POINT
// ===========================================================================================================
int main()
{
    // 1. Call the platform initialization
    ost_system_initialize();

    // 2. Test the printf output
    debug_printf("\r\n[picoclock] Starting picoclock tests: V%d.%d\r\n", 1, 0);

    // 3. Filesystem / SDCard initialization
    //filesystem_mount();

    // 4. Initialize OS before all other OS calls
    qor_init(125000000UL);

    // 5. Initialize the tasks
    net_task_initialize();
    ui_task_initialize();

    // 6. Start the operating system!
    qor_start(&IdleTcb, IdleTask, IdleStack, 1024);

    return 0;
}
