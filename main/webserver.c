#include "webserver.h"
#include "pump.h"
#include "led.h"
#include "ota.h"
#include "wifi_manager.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "web";

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");

static esp_err_t handle_index(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static esp_err_t handle_state(httpd_req_t *req) {
    cJSON *j = cJSON_CreateObject();
    cJSON_AddBoolToObject(j,   "on",        pump_is_on());
    cJSON_AddNumberToObject(j, "power",     pump_get_power());
    cJSON_AddNumberToObject(j, "led_power", led_get_power());
    cJSON_AddStringToObject(j, "ip",        wifi_manager_get_ip());
    char *s = cJSON_PrintUnformatted(j);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, s);
    free(s);
    cJSON_Delete(j);
    return ESP_OK;
}

static esp_err_t handle_pump(httpd_req_t *req) {
    char buf[128] = {0};
    int  len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");   return ESP_FAIL; }
    buf[len] = '\0';

    cJSON *j = cJSON_Parse(buf);
    if (!j)  { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid JSON"); return ESP_FAIL; }

    cJSON *on_item  = cJSON_GetObjectItemCaseSensitive(j, "on");
    cJSON *pct_item = cJSON_GetObjectItemCaseSensitive(j, "power");
    bool  on  = cJSON_IsTrue(on_item);
    int   pct = cJSON_IsNumber(pct_item) ? (int)pct_item->valuedouble : 0;
    cJSON_Delete(j);

    pump_set(on, (uint8_t)pct);

    cJSON *r = cJSON_CreateObject();
    cJSON_AddBoolToObject(r,   "on",    pump_is_on());
    cJSON_AddNumberToObject(r, "power", pump_get_power());
    char *s = cJSON_PrintUnformatted(r);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, s);
    free(s);
    cJSON_Delete(r);
    return ESP_OK;
}

static esp_err_t handle_led(httpd_req_t *req) {
    char buf[64] = {0};
    int  len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "empty body");   return ESP_FAIL; }
    buf[len] = '\0';

    cJSON *j = cJSON_Parse(buf);
    if (!j)  { httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid JSON"); return ESP_FAIL; }

    cJSON *pct_item = cJSON_GetObjectItemCaseSensitive(j, "power");
    int   pct = cJSON_IsNumber(pct_item) ? (int)pct_item->valuedouble : 0;
    cJSON_Delete(j);

    led_set((uint8_t)pct);

    cJSON *r = cJSON_CreateObject();
    cJSON_AddNumberToObject(r, "led_power", led_get_power());
    char *s = cJSON_PrintUnformatted(r);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, s);
    free(s);
    cJSON_Delete(r);
    return ESP_OK;
}

void webserver_start(void) {
    httpd_config_t cfg     = HTTPD_DEFAULT_CONFIG();
    cfg.stack_size         = 8192;
    cfg.max_uri_handlers   = 8;
    cfg.recv_wait_timeout  = 60;   /* OTA uploads can take a while */
    cfg.send_wait_timeout  = 10;
    httpd_handle_t srv     = NULL;

    if (httpd_start(&srv, &cfg) != ESP_OK) {
        ESP_LOGE(TAG, "start failed");
        return;
    }

    httpd_uri_t uris[] = {
        { .uri="/",          .method=HTTP_GET,  .handler=handle_index },
        { .uri="/api/state", .method=HTTP_GET,  .handler=handle_state },
        { .uri="/api/pump",  .method=HTTP_POST, .handler=handle_pump  },
        { .uri="/api/led",   .method=HTTP_POST, .handler=handle_led   },
        { .uri="/api/ota",   .method=HTTP_POST, .handler=handle_ota   },
    };
    for (int i = 0; i < 5; i++) httpd_register_uri_handler(srv, &uris[i]);
    ESP_LOGI(TAG, "ready on port %d", cfg.server_port);
}
