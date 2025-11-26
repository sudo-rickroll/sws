#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>

#include "logger.h"

int route;
int enable_default; 

void
initialize_logging(char *path, int debug){

	route = -1, enable_default = 0;

	if(debug){
		route = STDOUT_FILENO;
		enable_default = 1;
		return;
	}
	else if(path != NULL){
		if((route = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){
			err(EXIT_FAILURE, "Unable to open log file: %s", path);
		}		
	}
	else{
		route = -1;
	}

	enable_default = 0;
}

void
log_stream(char *address, char *request, int status, size_t bytes){
	time_t now;
	struct tm *utc;
	char timeBuf[64];
	int len;

	if(route < 0){
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
		if((len = snprintf(buf, sizeof(buf), "%s %s \"%s\" %d %zu\n", address, timeBuf, request, status, bytes)) < 0){
			perror("snprintf");
			exit(EXIT_FAILURE);
		}

		if(write(route, buf, len) < 0){
			perror("write");
			exit(EXIT_FAILURE);
		}

	} while(len != 0);

	if(write(route, "\n", 2) < 0){
		perror("write");
		exit(EXIT_FAILURE);
	}
}


void 
end_logging(){
	if(route >= 0 && !enable_default){
		(void)close(route);
		route = -1;
	}
}
