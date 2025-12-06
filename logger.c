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

char *
get_time(time_t t, char *type){
	static char buf[BUFSIZ];
	struct tm gmt;
	
	if(t == -1 && (t = time(NULL)) <= 127){
		err(EXIT_FAILURE, "Error getting current time");
	}

	if(gmtime_r(&t, &gmt) == NULL){
			err(EXIT_FAILURE, "Error converting to UTC");
	}

	if((strcmp(type, "client") == 0 && strftime(buf, sizeof(buf), "%a, %d, %b, %Y %H:%M:%S GMT", &gmt) != 0) || (strcmp(type, "server") == 0 && strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &gmt) != 0)){
		return buf;
	}
	else{
	       perror("strftime");
	       return "unknown";
	}
}	

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
	int len;

	if(ROUTE < 0){
		return;
	}
	
	do{
		char buf[BUFSIZ];
		memset(buf, 0, sizeof(buf));
		if((len = snprintf(buf, sizeof(buf), "%s %s \"%s\" %d %lu\n", address, get_time(-1, "server"), request, status, (unsigned long)bytes)) < 0){
			perror("snprintf");
			return;
		}

		if(write(ROUTE, buf, len) < 0){
			perror("write");
			return;
		}

	} while(len != 0);

	if(write(ROUTE, "\n", 2) < 0){
		perror("write");
		return;
	}
}


void 
end_logging(){
	if(ROUTE >= 0 && !ENABLE_DEFAULT){
		(void)close(ROUTE);
		ROUTE = -1;
	}
}
