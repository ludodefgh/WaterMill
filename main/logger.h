#pragma once
#include <stdint.h>

/* RAM ring buffer — current session */
#define LOGGER_ENTRIES    60
#define LOGGER_ENTRY_MAX  120

/* RTC ring buffer — survives soft resets (panic/WDT), cleared on power-off */
#define LOGGER_RTC_ENTRIES    15
#define LOGGER_RTC_ENTRY_MAX 100

#define LOGGER_HIST_SIZE  5

typedef struct { char s[LOGGER_ENTRY_MAX];     } log_line_t;
typedef struct { char s[LOGGER_RTC_ENTRY_MAX]; } rtc_log_line_t;

void logger_init(void);

/* current session logs, oldest first */
int  logger_snapshot(log_line_t *out, int max);

/* previous session logs (pre-crash), oldest first; 0 if none */
int  logger_prev_snapshot(rtc_log_line_t *out, int max);

/* last LOGGER_HIST_SIZE reset reasons, most recent first */
void logger_reset_history(uint8_t *out, int *count);
