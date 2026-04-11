#include <stdbool.h>
#include <stdio.h>
#include <ff.h>
#include "debug.h"
#include "main.h"

#include "filesystem.h"
#include "mini_qoi.h"
#include "serializers.h"
#include "sdcard.h"
#include "fw.h"

#include "ini.h"

#include "ff.h"
#include "diskio.h"
typedef FIL file_t;

extern config_struct global_config;


// SD_FAT_TYPE = 0 for SdFat/File as defined in SdFatConfig.h,
// 1 for FAT16/FAT32, 2 for exFAT, 3 for FAT16/FAT32 and exFAT.
#define SD_FAT_TYPE 2

static FATFS fs;
static FIL File[2]; /* File object */
static DIR Dir;     /* Directory object */
static FILINFO Finfo;

static SD_CardInfo cardinfo;

file_t file_open(const char *filename)
{
    file_t fil;
    FRESULT fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK)
    {
        printf("ERROR: f_open %d\n\r", (int)fr);
    }
    return fil;
}

static FRESULT scan_files(
    char *path /* Pointer to the working buffer with start path */
) {
    DIR dirs;
    FRESULT fr;

    fr = f_opendir(&dirs, path);
    if (fr == FR_OK) {
        while (((fr = f_readdir(&dirs, &Finfo)) == FR_OK) && Finfo.fname[0]) {
            if (Finfo.fattrib & AM_DIR) {
                //				i = strlen(path);
                printf("/%s [%d]\r\n", Finfo.fname, Finfo.fsize);
                if (fr != FR_OK)
                    break;
            } else {
                printf("%s [%d]\r\n", Finfo.fname, Finfo.fsize);
            }
        }
    } else {
        printf("Cannot open directory\r\n");
    }

    return fr;
}

void filesystem_unmount() {
	f_mount(NULL, "", 1);
}

void filesystem_mount() {
    uint32_t retries = 3;
    FRESULT fr;
    do {
        fr = f_mount(&fs, "", 1); // 0: mount successful ; 1: mount failed
        busy_wait_ms(10);
    } while (--retries && fr);

    if (fr) {
        printf("No Card Found! Err=%d\r\n", (int)fr);
    }

    printf("SD Card File System = %d\r\n", fs.fs_type); // FS_EXFAT = 4

    // Get SD Card info
    if (sdcard_get_card_info(&cardinfo) == SD_RESPONSE_NO_ERROR) {
        printf("Sectors: %d, sector size: %d\r\n", cardinfo.CardCapacity, cardinfo.CardBlockSize);
    } else {
        printf("Cannot read SDCard info\r\n");
    }

    scan_files("");
}

uint32_t filesystem_get_capacity()
{
    return cardinfo.CardCapacity;
}

/*
// Loop in all directories
void disk_start()
{
      // default is current directory
  const char* dpath = ".";
  if (argc) dpath = args;

  DIR dir;
  if ( FR_OK != f_opendir(&dir, dpath) )
  {
    printf("cannot access '%s': No such file or directory\r\n", dpath);
    return;
  }

  FILINFO fno;
  while( (f_readdir(&dir, &fno) == FR_OK) && (fno.fname[0] != 0) )
  {
    if ( fno.fname[0] != '.' ) // ignore . and .. entry
    {
      if ( fno.fattrib & AM_DIR )
      {
        // directory
        printf("/%s\r\n", fno.fname);
      }else
      {
        printf("%-40s", fno.fname);
        if (fno.fsize < 1024)
        {
          printf("%lu B\r\n", fno.fsize);
        }else
        {
          printf("%lu KB\r\n", fno.fsize / 1024);
        }
      }
    }
  }

  f_closedir(&dir);
}
*/


static file_t ConfigFile;
static uint8_t ConfigBuf[2000];
static const char *ConfigFileName = "config.ini";

#define FS_MAX_SIZE_READ 256



static int config_handler(void* user, const char* section, const char* name,
                   const char* value)
{
    config_struct* pconfig = (config_struct*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("", "wifi_ssid")) {
        printf("Set config wifi_ssid:[%s]\r\n", value);
        strncpy(pconfig->wifi_ssid, value, 50);
    } else if (MATCH("", "wifi_key")) {
        printf("Set config wifi_key:[%s]\r\n", value);
        strncpy(pconfig->wifi_key, value, 50);
    } else if (MATCH("", "remote_host")) {
        printf("Set config remote_host:[%s]\r\n", value);
        strncpy(pconfig->remote_host, value, 50);
    } else if (MATCH("", "screen")) {
        printf("Set config screen:[%s]\r\n", value);
        strncpy(pconfig->screen, value, 2);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}



bool __no_inline_not_in_flash_func(filesystem_read_fw_file)(ota_segment_consumer_t process_buffer) {
	file_t FwFile;
	const char *FwFileName = "picoclock.uf2";

    FILINFO fno;
    FRESULT fr = f_stat(FwFileName, &fno);

    if (fr == FR_OK) {
		printf("[fs] SUCCESS: found fw file:[%s]\r\n", FwFileName);
        UINT br;

		FRESULT fr = f_open(&FwFile, FwFileName, FA_READ);
		if (fr == FR_OK) {
			int total_size = fno.fsize;
			int copied_size = 0;

			int tmp_buf_size = 512;
			char tmp_buf[512];

			do {
				int read_size = total_size > tmp_buf_size ? tmp_buf_size : total_size;

				f_read(&FwFile, tmp_buf, read_size, &br);
				int rc = process_buffer(tmp_buf);
				if (rc != 0) {
					printf("[fs] Error processing buffer, rc:[%d]\r\n", rc);
					return false;
				}

				if (br == read_size) {
					total_size -= read_size;
					copied_size += read_size;
				} else {
					printf("[fs] Read file error\r\n");
					total_size = 0; // force exit
					return false;
				}
			} while (total_size > 0);

			f_close(&FwFile);
		}
	} else {
		printf("[fs] ERROR: fw file:[%s] not found\r\n", FwFileName);
    }
    return true;
}

bool filesystem_read_config_file() {
    FILINFO fno;
    FRESULT fr = f_stat(ConfigFileName, &fno);

    if (fr == FR_OK) {
		printf("SUCCESS: found config file:[%s]\r\n", ConfigBuf);
        UINT br;

		FRESULT fr = f_open(&ConfigFile, ConfigFileName, FA_READ);
		if (fr == FR_OK) {
			int total_size = fno.fsize;
			int copied_size = 0;

			do {
				int read_size = total_size > FS_MAX_SIZE_READ ? FS_MAX_SIZE_READ : total_size;

				f_read(&ConfigFile, &ConfigBuf[copied_size], read_size, &br);

				if (br == read_size) {
					total_size -= read_size;
					copied_size += read_size;
				} else {
					printf("Read file error\n");
					total_size = 0; // force exit
					return false;
				}
			} while (total_size > 0);

			f_close(&ConfigFile);

			// Set config values
            ini_parse_string(ConfigBuf, config_handler, &global_config);
			/*
			wifi_ssid=xxx
			wifi_key=xxx
			remote_host=x.x.x.x
			*/
			printf("Set config wifi_ssid:[%s] wifi_key:[%s]\r\n", global_config.wifi_ssid, global_config.wifi_key);

			printf("Config:[%s]\r\n", ConfigBuf);
		}
	} else {
		printf("ERROR: config file not found\r\n");
    }
    return true;
}


file_t current_file;

bool filesystem_write_file(char *file_name) {
	FRESULT fr = f_open(&current_file, file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr == FR_OK) {
		printf("[filesystem] New file:[%s] created\r\n", file_name);
	}
	//f_lseek(&current_file, 0);
}

void filesystem_write_bytes(char *buffer, uint16_t buffer_length) {
	UINT bytes_written;
	f_write(&current_file, buffer, buffer_length, &bytes_written);
	//printf("[filesystem] Bytes length:[%d] written:[%d]\r\n", buffer_length, bytes_written);
}

void filesystem_close() {
	f_close(&current_file);
}
