#pragma once
#include <stdint.h>

void    led_init(void);
void    led_set(uint8_t percent);
uint8_t led_get_power(void);
