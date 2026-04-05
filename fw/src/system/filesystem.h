#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include "system.h"

bool filesystem_read_config_file();
void filesystem_mount();
uint32_t filesystem_get_capacity();

bool filesystem_write_file(char *file_name);
void filesystem_write_bytes(char *buffer, uint16_t buffer_length);
void filesystem_close();

#endif // FILESYSTEM_H
