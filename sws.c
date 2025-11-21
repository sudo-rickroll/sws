#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#include "types.h"
#include "logger.h"
#include "connections.h"

int input_validation(int argc, char **argv, sws_options *config) {
	/* Default assignments */
	int opt;
	struct stat *st;
	(void) st;
	config->port = 8080;
	
	while ((opt = getopt(argc, argv, "c:dhi:l:p:")) != -1) {
		switch (opt) {
			case 'c':
				config->cgi = optarg;
				break;
			case 'd':
				config->debug = true;
				break;
			case 'h':
				config->help = true;
				break;
			case 'i':
				config->address = optarg;
				break;
			case 'l':
				config->log = optarg;
				break;
			case 'p': {
					  int port = atoi(optarg);
					  if(port < 1 || port > 65535){
						  err(EXIT_FAILURE, "Invalid port number");
					  }
					  config->port = port;
					  break;
				  }
			case '?':
			default:
				if (optopt == 'c' || optopt == 'i' || 
					optopt == 'l' || optopt == 'p') {
					fprintf(stderr, "Error: -%c requires an argument\n", optopt);
				}
				else if (isprint(optopt)) {
					fprintf(stderr, "Error: unknown option '-%c'\n", optopt);
				}
				else {
					fprintf(stderr, "Error: unknown option given\n");
				}

				return -1;
		}
	}

	return 0;
}

int main(int argc, char *argv[]) {
	sws_options config;
	
	if(input_validation(argc, argv, &config) < 0){
		return EXIT_FAILURE;	
	}

	if(config.help){
		printf("Usage: %s [-dh] [-c dir] [-i address] [-l file] [-p port] dir\n", argv[0]);
		return EXIT_SUCCESS;
	}

	initialize_logging(&config, config.log);
	log_stream();

	if(create_connections(config.address, config.port) < 0){
		err(EXIT_FAILURE, "Unable to establish a connection");
	}
	

	return EXIT_SUCCESS;
}
