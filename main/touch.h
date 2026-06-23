#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void touch_init(void);
extern TaskHandle_t g_touch_task_handle;
