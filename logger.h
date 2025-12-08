#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

#include <stdbool.h>

#include "types.h"

char *get_time(time_t t, timeformat_t format);

void initialize_logging(const char *path, bool debug);

void log_stream(const char *address, const char *request, int status,
                size_t bytes);

void end_logging(void);

#endif
