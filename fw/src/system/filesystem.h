#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include "system.h"

bool filesystem_read_config_file();
void filesystem_mount();
uint32_t filesystem_get_capacity();

#endif // FILESYSTEM_H
