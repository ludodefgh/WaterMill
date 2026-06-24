#pragma once

#define LOGGER_ENTRIES   20
#define LOGGER_ENTRY_MAX 120

typedef struct { char s[LOGGER_ENTRY_MAX]; } log_line_t;

void logger_init(void);
int  logger_snapshot(log_line_t *out, int max);  /* oldest first */
