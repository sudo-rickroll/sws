#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/stat.h>

#include "types.h"

int input_validation(int argc, char **argv, sws_options *config) {
	/* Default assignments */
	int opt;
	struct stat st;
	char *end;

	config->port = 8080;
	config->debug = false;
	config->help = false;
	config->address = NULL;
	config->log = NULL;
	config->docroot = NULL;
	config->cgi = NULL;

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

				config->port = strtol(optarg, &end, 10);

				if (*end != '\0' || config->port < 1 || config->port > 65535) {
					fprintf(stderr, "Error: invalid port '%s'\n", optarg);
					return -1;
				}

				break;
			/* Unknown, pass */
			case '?':
			default:
				/* Cases with arguments */
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

	config->docroot = argv[optind];

	/* Check docroot */
	if (stat(config->docroot, &st) != 0) {
		fprintf(stderr, "Error: cannot access '%s'\n", config->docroot);
		perror("stat");
		return -1;
	}
	if (!S_ISDIR(st.st_mode)) {
		fprintf(stderr, "Error: '%s' is not dir\n", config->docroot);
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[]) {
	sws_options config;

	input_validation(argc, argv, &config);

	return EXIT_SUCCESS;
}
