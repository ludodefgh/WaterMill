#pragma once
#include <stdbool.h>
#include <stdint.h>

void    pump_init(void);
void    pump_set(bool on, uint8_t percent);
bool    pump_is_on(void);
uint8_t pump_get_power(void);
