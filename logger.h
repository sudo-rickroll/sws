#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

void
initialize_logging(const char *, int);

void
log_stream(const char *, const char *, int, size_t);

void 
end_logging();

#endif
