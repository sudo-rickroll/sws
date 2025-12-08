#include <sys/stat.h>

#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
input_validation(int argc, char **argv, sws_options_t *config)
{
	int opt;
	long user_port;
	struct stat st;
	char *end, *last_slash;

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
			if (stat(optarg, &st) == 0 && S_ISDIR(st.st_mode)) {
				fprintf(stderr, "Log path \"%s\" cannot be a directory",
				        optarg);
				return -1;
			}

			last_slash = strrchr(optarg, '/');
			if (last_slash != NULL) {
				size_t dir_len;
				char *dir_path;

				dir_len = (size_t)(last_slash - optarg);

				if (dir_len == 0) {
					dir_len = 1;
				}

				if ((dir_path = malloc(dir_len + 1)) == NULL) {
					perror("log dir malloc");
					return -1;
				}

				strncpy(dir_path, optarg, dir_len);
				dir_path[dir_len] = '\0';

				if (stat(dir_path, &st) != 0) {
					perror("stat log directory");
					free(dir_path);
					return -1;
				}
				if (!S_ISDIR(st.st_mode)) {
					fprintf(stderr, "\"%s\" is not a directory", dir_path);
					free(dir_path);
					return -1;
				}
				free(dir_path);
			}

			if ((config->log = strdup(optarg)) == NULL) {
				perror("dup log");
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
	sws_options_t config;
	int sock;

	if (input_validation(argc, argv, &config) < 0) {
		return EXIT_FAILURE;
	}

	initialize_logging(config.log, config.debug);

	if ((sock = create_connections(config.address, config.port)) < 0) {
		err(EXIT_FAILURE, "Unable to establish a connection");
	}

	if (!config.debug && daemon(0, 0)) {
		err(EXIT_FAILURE, "daemon");
	}

	accept_connections(sock, &config);
	free(config.docroot);
	/* Even if these are NULL, it's fine to free them. */
	free(config.cgi);
	free(config.log);
	(void)close(sock);
	end_logging();

	return EXIT_SUCCESS;
}
