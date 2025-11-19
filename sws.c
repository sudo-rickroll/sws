#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "types.h"
#include "logger.h"

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
			case 'p':
				/* There is a default assignment, but -p
				 * still requires a port
				 */
				if (!optarg) {
					fprintf(stderr, "Error: -p requires a port value\n");
					return -1;
				}
				break;
			/* Unknown, pass */
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

	input_validation(argc, argv, &config);
	// placeholder path
	initialize_logging(&config, "./");
	log_stream();

	return EXIT_SUCCESS;
}
