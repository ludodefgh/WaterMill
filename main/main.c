#include "pump.h"
#include "led.h"
#include "touch.h"
#include "wifi_manager.h"
#include "webserver.h"
#include "logger.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mdns.h"

#define MDNS_HOSTNAME "waterpump"

static const char *TAG = "main";

void app_main(void) {
    logger_init();
    ESP_LOGI(TAG, "reset reason: %d", (int)esp_reset_reason());
    pump_init();
    led_init();
    /* blink twice at boot so resets are visible */
    for (int i = 0; i < 2; i++) {
        led_set(80);  vTaskDelay(pdMS_TO_TICKS(80));
        led_set(0);   vTaskDelay(pdMS_TO_TICKS(120));
    }
    touch_init();
    wifi_manager_init();
    ESP_LOGI(TAG, "connecting to WiFi...");
    wifi_manager_wait_connected();

    mdns_init();
    mdns_hostname_set(MDNS_HOSTNAME);
    mdns_service_add(NULL, "_http", "_tcp", 80, NULL, 0);
    ESP_LOGI(TAG, "mDNS: http://%s.local", MDNS_HOSTNAME);

    webserver_start();
    ESP_LOGI(TAG, "ready at http://%s  or  http://%s.local",
             wifi_manager_get_ip(), MDNS_HOSTNAME);
}
