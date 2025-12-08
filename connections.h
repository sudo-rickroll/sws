#ifndef CONNECTIONS_H
#define CONNECTIONS_H

#include <stdint.h>

#include "types.h"

int create_connections(char *address, uint16_t port);

void accept_connections(int sock, sws_options_t *config);

#endif
