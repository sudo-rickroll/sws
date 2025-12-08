#include <sys/stat.h>

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "connections.h"
#include "logger.h"
#include "types.h"

void
usage(char *program_name)
{
	printf("Usage: %s [-dh] [-c dir] [-i address] [-l file] [-p port] dir\n",
	       program_name);
}

int
input_validation(int argc, char **argv, sws_options *config)
{
	int opt;
	long user_port;
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
			if ((config->cgi = realpath(optarg, NULL)) == NULL) {
				warn("failed to get realpath of \"%s\"", optarg);
				return -1;
			}
			if (stat(config->cgi, &st) != 0) {
				warn("cannot access \"%s\"", config->cgi);
				return -1;
			}
			if (!S_ISDIR(st.st_mode)) {
				warnx("\"%s\" is not directory", config->cgi);
				return -1;
			}
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
			if ((config->log = realpath(optarg, NULL)) == NULL) {
				warn("failed to get realpath of \"%s\"", optarg);
				return -1;
			}
			if (stat(config->log, &st) != 0) {
				warn("cannot access \"%s\"", config->log);
				return -1;
			}
			if (!S_ISDIR(st.st_mode)) {
				warnx("\"%s\" is not directory", config->log);
				return -1;
			}
			break;
		case 'p':
			user_port = strtol(optarg, &end, 10);
			if (*end != '\0' || user_port < 0 || user_port > 65535) {
				warnx("invalid port \"%s\"", optarg);
				return -1;
			}
			config->port = user_port;

			break;
		/* getopt prints error messages for us, just exit on unknown. */
		case '?':
		default:
			return -1;
		}
	}

	if (config->help) {
		usage(argv[0]);
		exit(EXIT_SUCCESS);
		/* NOTREACHED */
	}
	if (optind >= argc) {
		usage(argv[0]);
		return -1;
	}

	if ((config->docroot = realpath(argv[optind], NULL)) == NULL) {
		warn("failed to get realpath of \"%s\"", argv[optind]);
		return -1;
	}
	if (stat(config->docroot, &st) != 0) {
		warn("cannot access \"%s\"", config->docroot);
		return -1;
	}
	if (!S_ISDIR(st.st_mode)) {
		warnx("\"%s\" is not directory", config->docroot);
		return -1;
	}

	return 0;
}

int
main(int argc, char *argv[])
{
	sws_options config;
	int sock;

	if (input_validation(argc, argv, &config) < 0) {
		return EXIT_FAILURE;
	}

	initialize_logging(config.log, config.debug);

	if ((sock = create_connections(config.address, config.port)) < 0) {
		err(EXIT_FAILURE, "Unable to establish a connection");
	}

	accept_connections(sock, &config);
	free(config.docroot);
	/* Even if these are NULL, it's fine to free them. */
	free(config.cgi);
	free(config.log);
	(void)close(sock);

	return EXIT_SUCCESS;
}
