#ifndef LOGGER_H
#define LOGGER_H

#include <sys/types.h>

void
initialize_logging(char *, int);

void
log_stream(char *, char *, int, size_t);

void 
end_logging()

#endif
