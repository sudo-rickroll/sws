#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>

#include "logger.h"

static int LOG_FD = -1;

void
initialize_logging(sws_options *config, char *path){
	if(config->debug){
		LOG_FD = STDOUT_FILENO;
	}
	else if(path != NULL){
		if((LOG_FD = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0){
			err(EXIT_FAILURE, "Unable to open log file");
		}
	}
	else{
		LOG_FD = -1;
	}
}

void
log_stream(){
	time_t now = time(NULL);
	char buf[BUFSIZ];

	int str_len = snprintf(buf, sizeof(buf), "<ip> %lu <req> <status> <res>", now);
	
	if(write(LOG_FD, buf, str_len) < 0){
		err(EXIT_FAILURE, "Unable to write to log");
	}
}	

