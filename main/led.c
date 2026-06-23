#include "led.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define LED_GPIO   3
#define LEDC_SPEED LEDC_LOW_SPEED_MODE
#define LEDC_TIMER LEDC_TIMER_1
#define LEDC_CHAN  LEDC_CHANNEL_1
#define LEDC_FREQ  1000
#define LEDC_BITS  LEDC_TIMER_8_BIT
#define LEDC_MAX   ((1 << 8) - 1)   /* 255 */

static const char *TAG = "led";
static uint8_t s_pct = 0;

void led_init(void) {
    ledc_timer_config_t t = {
        .speed_mode      = LEDC_SPEED,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_BITS,
        .freq_hz         = LEDC_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);

    ledc_channel_config_t ch = {
        .gpio_num   = LED_GPIO,
        .speed_mode = LEDC_SPEED,
        .channel    = LEDC_CHAN,
        .timer_sel  = LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ledc_channel_config(&ch);
    ESP_LOGI(TAG, "init, LED OFF");
}

void led_set(uint8_t pct) {
    if (pct > 100) pct = 100;
    s_pct = pct;
    uint32_t duty = ((uint32_t)pct * LEDC_MAX) / 100;
    ledc_set_duty(LEDC_SPEED, LEDC_CHAN, duty);
    ledc_update_duty(LEDC_SPEED, LEDC_CHAN);
    ESP_LOGI(TAG, "pct=%u duty=%lu", pct, duty);
}

uint8_t led_get_power(void) { return s_pct; }
