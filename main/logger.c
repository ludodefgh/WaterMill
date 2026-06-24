#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

static log_line_t   s_buf[LOGGER_ENTRIES];
static int          s_head  = 0;
static int          s_count = 0;
static portMUX_TYPE s_mux   = portMUX_INITIALIZER_UNLOCKED;

/* strip ANSI colour codes emitted by ESP-IDF logging */
static void strip_ansi(char *s) {
    char *r = s, *w = s;
    while (*r) {
        if (*r == '\033' && *(r + 1) == '[') {
            r += 2;
            while (*r && *r != 'm') r++;
            if (*r == 'm') r++;
        } else {
            *w++ = *r++;
        }
    }
    *w = '\0';
}

static int my_vprintf(const char *fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    char tmp[LOGGER_ENTRY_MAX];
    vsnprintf(tmp, sizeof(tmp), fmt, copy);
    va_end(copy);

    strip_ansi(tmp);

    size_t len = strlen(tmp);
    while (len > 0 && (tmp[len - 1] == '\n' || tmp[len - 1] == '\r'))
        tmp[--len] = '\0';

    if (len > 0) {
        portENTER_CRITICAL_SAFE(&s_mux);
        memcpy(s_buf[s_head].s, tmp, LOGGER_ENTRY_MAX);
        s_buf[s_head].s[LOGGER_ENTRY_MAX - 1] = '\0';
        s_head = (s_head + 1) % LOGGER_ENTRIES;
        if (s_count < LOGGER_ENTRIES) s_count++;
        portEXIT_CRITICAL_SAFE(&s_mux);
    }

    return vprintf(fmt, args);
}

void logger_init(void) {
    esp_log_set_vprintf(my_vprintf);
}

int logger_snapshot(log_line_t *out, int max) {
    int count = s_count;  /* atomic 32-bit read */
    int head  = s_head;
    if (count > max) count = max;
    int start = (s_count < LOGGER_ENTRIES) ? 0 : head;
    for (int i = 0; i < count; i++)
        memcpy(&out[i], &s_buf[(start + i) % LOGGER_ENTRIES], sizeof(log_line_t));
    return count;
}
