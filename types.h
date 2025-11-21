#include <stdbool.h>

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef struct {
	char *cgi;
	bool debug;
	bool help;
	char *address;
	char *log;
	uint16_t port;
	char *docroot;
} sws_options;

#endif
