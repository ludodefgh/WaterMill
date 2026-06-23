idf_component_register(
    SRCS
        "main.c"
        "pump.c"
        "led.c"
        "ota.c"
        "touch.c"
        "wifi_manager.c"
        "webserver.c"
    INCLUDE_DIRS "."
    EMBED_TXTFILES "index.html"
    REQUIRES
        esp_driver_ledc
        esp_driver_gpio
        esp_wifi
        esp_event
        esp_netif
        esp_http_server
        json
        nvs_flash
        mdns
        app_update
)
