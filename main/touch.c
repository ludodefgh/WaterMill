#include "touch.h"
#include "pump.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

TaskHandle_t g_touch_task_handle = NULL;

#define TOUCH_GPIO   GPIO_NUM_4
#define POLL_MS      10     /* sampling interval */
#define COOLDOWN_MS  500    /* ignore everything after a toggle */

static const char *TAG = "touch";

static void touch_task(void *arg) {
    /* Initialize to current state to avoid false edge at boot */
    bool last = gpio_get_level(TOUCH_GPIO);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
        bool current = gpio_get_level(TOUCH_GPIO);

        if (current && !last) {
            bool    new_on = !pump_is_on();
            uint8_t pct    = pump_get_power();
            if (new_on && pct == 0) pct = 100;
            pump_set(new_on, pct);
            ESP_LOGI(TAG, "toggle → %s %u%%", new_on ? "ON" : "OFF", pct);

            /* Fixed cooldown — ignore all glitches and hold duration */
            vTaskDelay(pdMS_TO_TICKS(COOLDOWN_MS));
            last = gpio_get_level(TOUCH_GPIO);  /* sync after cooldown */
            continue;
        }

        last = current;
    }
}

void touch_init(void) {
    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << TOUCH_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);

    xTaskCreate(touch_task, "touch", 3072, NULL, 5, &g_touch_task_handle);
    ESP_LOGI(TAG, "init GPIO%d", TOUCH_GPIO);
}
