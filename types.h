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
	/* Port number will be between 1 and 65535 */
	uint16_t port;
	char *docroot;
} sws_options_t;

typedef enum {
	FORMAT_HTTP,
	FORMAT_ISO8601
} timeformat_t;

#endif
