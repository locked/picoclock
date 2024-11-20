#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "main.h"
#include "debug.h"
#include "qor.h"
#include "audio_player.h"
#include "filesystem.h"
#include "system.h"
#include "fs_task.h"
#include "sdcard.h"

// ===========================================================================================================
// DEFINITIONS
// ===========================================================================================================
typedef enum
{
    FS_NO_EVENT,
    FS_PLAY_SOUND,
    FS_DISPLAY_IMAGE,
    FS_READ_SDCARD_BLOCK,
    FS_WRITE_SDCARD_BLOCK,
    FS_AUDIO_NEXT_SAMPLES
} fs_state_t;

typedef struct
{
    fs_state_t ev;
    uint8_t *mem;
    uint32_t addr;
    ost_button_t button;
    char *image;
    char *sound;
    fs_result_cb_t cb;
} ost_fs_event_t;

#define ASSETS_DIR "/"
#define STORY_DIR_OFFSET (UUID_SIZE + 1) // +1 for the first '/' (filesystem root)

// ===========================================================================================================
// PRIVATE GLOBAL VARIABLES
// ===========================================================================================================
static qor_tcb_t FsTcb;
static uint32_t FsStack[4096];

static qor_mbox_t FsMailBox;

static ost_fs_event_t *FsEventQueue[10];

static ost_context_t OstContext;

static int PacketCounter = 0;

static char ScratchFile[260];

// ===========================================================================================================
// FILE SYSTEM TASK
// ===========================================================================================================

// End of DMA transfer callback
static void audio_callback(void)
{
    static ost_fs_event_t NextSamplesEv = {
        .ev = FS_AUDIO_NEXT_SAMPLES};
    qor_mbox_notify(&FsMailBox, (void **)&NextSamplesEv, QOR_MBOX_OPTION_SEND_BACK);
}

static void show_duration(uint32_t millisecondes)
{
    uint32_t minutes, secondes, reste;

    // Calcul des minutes, secondes et millisecondes
    minutes = millisecondes / (60 * 1000);
    reste = millisecondes % (60 * 1000);
    secondes = reste / 1000;
    reste = reste % 1000;

    // Affichage du temps
    debug_printf("Temps : %d minutes, %d secondes, %d millisecondes\r\n", minutes, secondes, reste);
}

static bool UsbConnected = false;
void fs_task_usb_connected() {
    UsbConnected = true;
}
void fs_task_usb_disconnected() {
    UsbConnected = true;
}

void FsTask(void *args)
{
    ost_fs_event_t *message = NULL;
    uint32_t res = 0;

    //filesystem_read_index_file(&OstContext);

    int isPlaying = 0;

    while (1)
    {
        res = qor_mbox_wait(&FsMailBox, (void **)&message, 1000);
        if (res == QOR_MBOX_OK)
        {
            switch (message->ev)
            {
            case FS_PLAY_SOUND:
                //if (OstContext.sound != NULL)
                //{
                    sprintf(ScratchFile, "%s%s", ASSETS_DIR, message->sound);

                    debug_printf("--------------------------\r\nPlaying: %s\r\n", ScratchFile);

                    ost_system_stopwatch_start();
                    ost_audio_play(ScratchFile);
                    debug_printf("ost_audio_play...OK\r\n");
                    PacketCounter = 0;
                    isPlaying = 1;
                //}
                break;

            case FS_AUDIO_NEXT_SAMPLES:
                isPlaying = ost_audio_process();
                PacketCounter++;
                if (PacketCounter % 200 == 0) {
                    debug_printf("isPlaying:%d Packets: %d\r\n", isPlaying, PacketCounter);
                }

                if (isPlaying == 0) {
                    debug_printf("STOP AUDIO Packets: %d\r\n", PacketCounter);
                    uint32_t executionTime = ost_system_stopwatch_stop();
                    ost_audio_stop();

                    show_duration(executionTime);
                    //vm_task_sound_finished();
                }
                break;

            case FS_READ_SDCARD_BLOCK:
                sdcard_sector_read(message->addr, message->mem);
                if (message->cb != NULL)
                {
                    message->cb(true);
                }
                break;

            case FS_WRITE_SDCARD_BLOCK:
                sdcard_sector_write(message->addr, message->mem);
                if (message->cb != NULL)
                {
                    message->cb(true);
                }
                break;

            default:
                break;
            }
        }
    }
}

void fs_task_read_block(uint32_t addr, uint8_t *block, fs_result_cb_t cb)
{
    static ost_fs_event_t ReadBlockEv = {
        .ev = FS_READ_SDCARD_BLOCK};

    ReadBlockEv.mem = block;
    ReadBlockEv.addr = addr;
    ReadBlockEv.cb = cb;
    qor_mbox_notify(&FsMailBox, (void **)&ReadBlockEv, QOR_MBOX_OPTION_SEND_BACK);
}

void fs_task_write_block(uint32_t addr, uint8_t *block, fs_result_cb_t cb)
{
    static ost_fs_event_t WriteBlockEv = {
        .ev = FS_WRITE_SDCARD_BLOCK};

    WriteBlockEv.mem = block;
    WriteBlockEv.addr = addr;
    WriteBlockEv.cb = cb;
    qor_mbox_notify(&FsMailBox, (void **)&WriteBlockEv, QOR_MBOX_OPTION_SEND_BACK);
}

void fs_task_sound_start(char *sound)
{
    static ost_fs_event_t MediaStartEv = {
        .ev = FS_PLAY_SOUND,
        .cb = NULL};

    MediaStartEv.image = NULL;
    MediaStartEv.sound = sound;
	debug_printf("[fs_task] fs_task_sound_start notify:[%s]\r\n", sound);
    qor_mbox_notify(&FsMailBox, (void **)&MediaStartEv, QOR_MBOX_OPTION_SEND_BACK);
}

void fs_task_initialize()
{
    qor_mbox_init(&FsMailBox, (void **)&FsEventQueue, 10);

    ost_audio_register_callback(audio_callback);

    qor_create_thread(&FsTcb, FsTask, FsStack, sizeof(FsStack) / sizeof(FsStack[0]), FS_TASK_PRIORITY, "FsTask");
}
