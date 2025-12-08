#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <sys/socket.h>
#include <sys/stat.h>

#include <stdint.h>

#include "types.h"

int create_connections(char *, uint16_t);

void accept_connections(int, sws_options_t *);

void header_print(int, const char *, const char *);

int is_http(const char *);

char *index_directory(const char *, const char *);

void status_print(int, const char *, const char *, int, const char *,
                  const char *, const struct stat *, char *);

#endif
