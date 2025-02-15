#include <stdbool.h>
#include <stdio.h>
#include <ff.h>
#include "debug.h"
#include "main.h"

#include "filesystem.h"
#include "mini_qoi.h"
#include "serializers.h"
#include "sdcard.h"

#include "ini.h"

#ifdef OST_USE_FF_LIBRARY
#include "ff.h"
#include "diskio.h"
typedef FIL file_t;
#else

// Use standard library
typedef FILE *file_t;
typedef int FRESULT;
#define F_OK
#endif

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
#ifdef OST_USE_FF_LIBRARY
    file_t fil;
    FRESULT fr = f_open(&fil, filename, FA_READ);
    if (fr != FR_OK)
    {
        printf("ERROR: f_open %d\n\r", (int)fr);
    }
    return fil;
#else
    return fopen(filename, "r");
#endif
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
                printf("/%s\r\n", Finfo.fname);
                if (fr != FR_OK)
                    break;
            } else {
                printf("%s\r\n", Finfo.fname);
            }
        }
    } else {
        printf("Cannot open directory\r\n");
    }

    return fr;
}

void filesystem_mount() {
    uint32_t retries = 3;
    FRESULT fr;
    do {
        fr = f_mount(&fs, "", 1); // 0: mount successful ; 1: mount failed
        ost_system_delay_ms(10);
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
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
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
			// TODO: parse from ConfigBuf
            ini_parse_string(ConfigBuf, config_handler, &global_config);
			/*
			wifi_ssid=xxx
			wifi_key=xxx
			remote_host=x.x.x.x
			*/
			//strncpy(global_config.wifi_ssid, WIFI_SSID, 50);
			//strncpy(global_config.wifi_key, WIFI_PASSWORD, 50);
			printf("Set config wifi_ssid:[%s] wifi_key:[%s]\r\n", global_config.wifi_ssid, global_config.wifi_key);

			printf("Config:[%s]\r\n", ConfigBuf);
		}
	} else {
		printf("ERROR: config file not found\r\n");
    }
    return true;
}
