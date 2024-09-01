#ifndef BUTTON_DEBOUNCE_H
#define BUTTON_DEBOUNCE_H

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include "button_debounce.pio.h"

void debounce_debounce(void);

int debounce_gpio(uint gpio);

int debounce_set_debounce_time(uint gpio, float debounce_time);

int debounce_read(uint gpio);

int undebounce_gpio(uint gpio);

#endif // BUTTON_DEBOUNCE_H
