#include "pump.h"
#include "driver/ledc.h"
#include "esp_log.h"

/* 4N35 bandwidth is limited — 500 Hz gives clean switching */
#define PUMP_GPIO    5
#define LEDC_SPEED   LEDC_LOW_SPEED_MODE
#define LEDC_TIMER   LEDC_TIMER_0
#define LEDC_CHAN    LEDC_CHANNEL_0
#define LEDC_FREQ    500
#define LEDC_BITS    LEDC_TIMER_10_BIT
#define LEDC_MAX     ((1 << 10) - 1)   /* 1023 */

static const char *TAG = "pump";
static bool    s_on  = false;
static uint8_t s_pct = 0;

void pump_init(void) {
    ledc_timer_config_t t = {
        .speed_mode      = LEDC_SPEED,
        .timer_num       = LEDC_TIMER,
        .duty_resolution = LEDC_BITS,
        .freq_hz         = LEDC_FREQ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&t);

    ledc_channel_config_t ch = {
        .gpio_num   = PUMP_GPIO,
        .speed_mode = LEDC_SPEED,
        .channel    = LEDC_CHAN,
        .timer_sel  = LEDC_TIMER,
        .duty       = LEDC_MAX,   /* HIGH → 4N35 LED on → gate pulled low → pump OFF */
        .hpoint     = 0,
    };
    ledc_channel_config(&ch);
    ESP_LOGI(TAG, "init, pump OFF");
}

void pump_set(bool on, uint8_t pct) {
    if (pct > 100) pct = 100;
    s_on  = on;
    s_pct = pct;

    uint32_t duty;
    if (!on || pct == 0) {
        duty = LEDC_MAX;                        /* full HIGH → pump OFF */
    } else {
        /* Inverted by optocoupler: pct% pump → (100-pct)% duty */
        duty = ((100 - pct) * (uint32_t)LEDC_MAX) / 100;
    }

    ledc_set_duty(LEDC_SPEED, LEDC_CHAN, duty);
    ledc_update_duty(LEDC_SPEED, LEDC_CHAN);
    ESP_LOGI(TAG, "on=%d pct=%u duty=%lu", on, pct, duty);
}

bool    pump_is_on(void)     { return s_on;  }
uint8_t pump_get_power(void) { return s_pct; }
