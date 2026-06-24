#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

/* ── RAM ring buffer (current session) ────────────────────────────── */
static log_line_t   s_buf[LOGGER_ENTRIES];
static int          s_head  = 0;
static int          s_count = 0;
static portMUX_TYPE s_mux   = portMUX_INITIALIZER_UNLOCKED;

/* ── RTC ring buffer (survives soft resets) ───────────────────────── */
#define RTC_MAGIC 0xA5C3E7F1

RTC_DATA_ATTR static char     s_rtc_buf[LOGGER_RTC_ENTRIES][LOGGER_RTC_ENTRY_MAX];
RTC_DATA_ATTR static int      s_rtc_head;
RTC_DATA_ATTR static int      s_rtc_count;
RTC_DATA_ATTR static uint8_t  s_rst_hist[LOGGER_HIST_SIZE];
RTC_DATA_ATTR static int      s_rst_hist_n;
RTC_DATA_ATTR static uint32_t s_rtc_magic;

/* ── Snapshot of previous session (RAM copy, valid this boot only) ── */
static rtc_log_line_t s_prev[LOGGER_RTC_ENTRIES];
static int            s_prev_count = 0;
static uint8_t        s_hist_snap[LOGGER_HIST_SIZE];
static int            s_hist_snap_n = 0;

/* ── helpers ──────────────────────────────────────────────────────── */
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

/* ── vprintf hook ─────────────────────────────────────────────────── */
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
        int ram_slot, rtc_slot;

        portENTER_CRITICAL_SAFE(&s_mux);
        /* capture slots atomically, write outside CS */
        ram_slot = s_head;
        s_head   = (s_head + 1) % LOGGER_ENTRIES;
        if (s_count < LOGGER_ENTRIES) s_count++;
        rtc_slot   = s_rtc_head;
        s_rtc_head = (s_rtc_head + 1) % LOGGER_RTC_ENTRIES;
        if (s_rtc_count < LOGGER_RTC_ENTRIES) s_rtc_count++;
        portEXIT_CRITICAL_SAFE(&s_mux);

        /* write to RAM */
        memcpy(s_buf[ram_slot].s, tmp, LOGGER_ENTRY_MAX);
        s_buf[ram_slot].s[LOGGER_ENTRY_MAX - 1] = '\0';

        /* write to RTC (outside CS — slow memory, occasional torn write acceptable) */
        strncpy(s_rtc_buf[rtc_slot], tmp, LOGGER_RTC_ENTRY_MAX - 1);
        s_rtc_buf[rtc_slot][LOGGER_RTC_ENTRY_MAX - 1] = '\0';
    }

    return vprintf(fmt, args);
}

/* ── public API ───────────────────────────────────────────────────── */
void logger_init(void) {
    if (s_rtc_magic != RTC_MAGIC) {
        /* first power-on: RTC memory uninitialized */
        memset(s_rtc_buf, 0, sizeof(s_rtc_buf));
        s_rtc_head  = 0;
        s_rtc_count = 0;
        memset(s_rst_hist, 0, sizeof(s_rst_hist));
        s_rst_hist_n = 0;
        s_rtc_magic  = RTC_MAGIC;
    }

    /* snapshot previous session's RTC buffer before overwriting */
    int count = s_rtc_count;
    int start = (count < LOGGER_RTC_ENTRIES) ? 0 : s_rtc_head;
    s_prev_count = count < LOGGER_RTC_ENTRIES ? count : LOGGER_RTC_ENTRIES;
    for (int i = 0; i < s_prev_count; i++) {
        strncpy(s_prev[i].s,
                s_rtc_buf[(start + i) % LOGGER_RTC_ENTRIES],
                LOGGER_RTC_ENTRY_MAX - 1);
        s_prev[i].s[LOGGER_RTC_ENTRY_MAX - 1] = '\0';
    }

    /* snapshot reset history for this boot */
    s_hist_snap_n = s_rst_hist_n;
    memcpy(s_hist_snap, s_rst_hist, sizeof(s_rst_hist));

    /* update history: push current reset reason to front */
    uint8_t rr = (uint8_t)esp_reset_reason();
    for (int i = LOGGER_HIST_SIZE - 1; i > 0; i--)
        s_rst_hist[i] = s_rst_hist[i - 1];
    s_rst_hist[0] = rr;
    if (s_rst_hist_n < LOGGER_HIST_SIZE) s_rst_hist_n++;

    /* clear RTC buffer — fresh start for this session */
    s_rtc_head  = 0;
    s_rtc_count = 0;

    esp_log_set_vprintf(my_vprintf);
}

int logger_snapshot(log_line_t *out, int max) {
    int count = s_count;
    int head  = s_head;
    if (count > max) count = max;
    int start = (s_count < LOGGER_ENTRIES) ? 0 : head;
    for (int i = 0; i < count; i++)
        memcpy(&out[i], &s_buf[(start + i) % LOGGER_ENTRIES], sizeof(log_line_t));
    return count;
}

int logger_prev_snapshot(rtc_log_line_t *out, int max) {
    int n = s_prev_count < max ? s_prev_count : max;
    for (int i = 0; i < n; i++)
        memcpy(&out[i], &s_prev[i], sizeof(rtc_log_line_t));
    return n;
}

void logger_reset_history(uint8_t *out, int *count) {
    *count = s_rst_hist_n;
    memcpy(out, s_rst_hist, s_rst_hist_n);
}
