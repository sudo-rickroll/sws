#include <stdbool.h>

#ifndef TYPES_H
#define TYPES_H

typedef struct {
	char *cgi;
	bool debug;
	bool help;
	char *address;
	char *log;
	unsigned port;
	char *docroot;
} sws_options;

#endif
