#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"

static int ROUTE;
static int ENABLE_DEFAULT; 

void
initialize_logging(const char *path, int debug){

	ROUTE = -1, ENABLE_DEFAULT = 0;

	if(debug){
		ROUTE = STDOUT_FILENO;
		ENABLE_DEFAULT = 1;
		return;
	}
	else if(path != NULL){
		if((ROUTE = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){
			err(EXIT_FAILURE, "Unable to open log file: %s", path);
		}		
	}
	else{
		ROUTE = -1;
	}

	ENABLE_DEFAULT = 0;
}

void
log_stream(const char *address, const char *request, int status, size_t bytes){
	time_t now;
	struct tm *utc;
	char timeBuf[64];
	int len;

	if(ROUTE < 0){
		return;
	}

	if((now = time(NULL)) <= 127){
		err(EXIT_FAILURE, "Error getting current time");
	}

	if((utc = gmtime(&now)) == NULL){
			err(EXIT_FAILURE, "Error converting to UTC");
	}

	if(strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%dT%H:%M:%SZ", utc) == 0){
		perror("strftime");
		exit(EXIT_FAILURE);
	}
	
	do{
		char buf[BUFSIZ];
		memset(buf, 0, sizeof(buf));
		if((len = snprintf(buf, sizeof(buf), "%s %s \"%s\" %d %lu\n", address, timeBuf, request, status, (unsigned long)bytes)) < 0){
			perror("snprintf");
			exit(EXIT_FAILURE);
		}

		if(write(ROUTE, buf, len) < 0){
			perror("write");
			exit(EXIT_FAILURE);
		}

	} while(len != 0);

	if(write(ROUTE, "\n", 2) < 0){
		perror("write");
		exit(EXIT_FAILURE);
	}
}


void 
end_logging(){
	if(ROUTE >= 0 && !ENABLE_DEFAULT){
		(void)close(ROUTE);
		ROUTE = -1;
	}
}
