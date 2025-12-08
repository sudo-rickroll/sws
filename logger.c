#include "logger.h"

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int ROUTE;
static int ENABLE_DEFAULT;

char *
get_time(time_t t, const timeformat_t format)
{
	static char buf[BUFSIZ];
	struct tm gmt;

	if (t == 0 && (t = time(NULL)) <= 127) {
		warn("Error getting current time");
		return NULL;
	}

	if (gmtime_r(&t, &gmt) == NULL) {
		warn("Error converting to UTC");
		return NULL;
	}

	if ((format == FORMAT_HTTP &&
	     strftime(buf, sizeof(buf), "%a, %d, %b, %Y %H:%M:%S GMT", &gmt) !=
	         0) ||
	    (format == FORMAT_ISO8601 &&
	     strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &gmt) != 0)) {
		return buf;
	}
	warn("strftime");
	return NULL;
}

void
initialize_logging(const char *path, bool debug)
{

	ROUTE = -1, ENABLE_DEFAULT = 0;

	if (debug) {
		ROUTE = STDOUT_FILENO;
		ENABLE_DEFAULT = 1;
		return;
	}

	if (path != NULL) {
		if ((ROUTE = open(path, O_WRONLY | O_CREAT | O_APPEND,
		                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
			err(EXIT_FAILURE, "Unable to open log file: %s", path);
		}
	} else {
		ROUTE = -1;
	}

	ENABLE_DEFAULT = 0;
}

void
log_stream(const char *address, const char *request, int status, size_t bytes)
{
	int len;
	char buf[BUFSIZ];
	if (ROUTE < 0) {
		return;
	}


	memset(buf, 0, sizeof(buf));
	if ((len = snprintf(buf, sizeof(buf), "%s %s \"%s\" %d %lu\n", address,
	                    get_time(0, FORMAT_ISO8601), request, status,
	                    (unsigned long)bytes)) < 0) {
		perror("snprintf");
		return;
	}

	if (write(ROUTE, buf, len) < 0) {
		perror("write");
		return;
	}
}

void
end_logging(void)
{
	if (ROUTE >= 0 && !ENABLE_DEFAULT) {
		(void)close(ROUTE);
		ROUTE = -1;
	}
}