#include "cgi.h"

#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "handlers.h"

int
is_cgi(const char *path)
{
	if (path == NULL) {
		return 0;
	}

	if (strncmp(path, "/cgi-bin/", 9) == 0) {
		return 1;
	}

	return 0;
}

static void
cgi_init_env(const char *method, const char *version, const char *query,
             const char *script, const char *server, const char *port,
             const char *address)
{
	/* Setting meta-variables according to RFC3875 */

	if (setenv("GATEWAY_INTERFACE", "CGI/1.1", 1) != 0) {
		perror("setenv interface");
	}

	if (setenv("SERVER_SOFTWARE", "sws/1.0", 1) != 0) {
		perror("setenv software");
	}

	if (setenv("REQUEST_METHOD", method, 1) != 0) {
		perror("setenv request");
	}

	if (setenv("SCRIPT_NAME", script, 1) != 0) {
		perror("setenv script");
	}

	if (setenv("SERVER_PROTOCOL", version, 1) != 0) {
		perror("setenv protocol");
	}

	if (server != NULL && setenv("SERVER_NAME", server, 1) != 0) {
		perror("setenv server");
	}

	if (port != NULL && setenv("SERVER_PORT", port, 1) != 0) {
		perror("setenv port");
	}

	if (address != NULL && setenv("REMOTE_ADDR", address, 1) != 0) {
		perror("setenv address");
	}

	if (query != NULL && query[0] != '\0') {
		if (setenv("QUERY_STRING", query, 1) != 0) {
			perror("setenv query");
		}
	} else {
		if (setenv("QUERY_STRING", "", 1) != 0) {
			perror("setenv query");
		}
	}

	/* Set path for the binaries. I have assumed that it would be present at
	 * these locations always, like typical Unix system binaries */

	if (setenv("PATH", "/bin:/usr/bin:/usr/local/bin", 1) != 0) {
		perror("setenv bin path");
	}
}

int
cgi_exec(int sock, const char *path, const char *method, const char *version,
         const char *query, const char *script, const char *port,
         const char *address)
{
	pid_t pid;
	int status;

	/* I am using signal handler to reap child in the grandparent, so I will
	 * have to mask SIGCHLD here. Otherwise, waitpid here will fail. */
	sigset_t old_mask;

	if (block_sig(SIGCHLD, &old_mask) < 0) {
		perror("cgi block SIGCHLD");
		return -1;
	}


	if ((pid = fork()) < 0) {
		perror("cgi fork");
		if (restore_sig(&old_mask) < 0) {
			perror("cgi restore sigchld");
		}
		return -1;
	}

	if (pid == 0) {
		if (restore_sig(&old_mask) < 0) {
			perror("cgi child SIGCHLD restore");
		}
		if (dup2(sock, STDOUT_FILENO) < 0) {
			perror("dup2 stdout");
			exit(EXIT_FAILURE);
		}

		(void)close(sock);

		cgi_init_env(method, version, query, script, "localhost", port,
		             address);
		execl(path, path, (char *)NULL);

		perror("execl");
		exit(EXIT_FAILURE);
	}

	if (waitpid(pid, &status, 0) < 0) {
		perror("waitpid");
		if (restore_sig(&old_mask) < 0) {
			perror("cgi parent SIGCHLD restore");
		}
		return -1;
	}

	if (restore_sig(&old_mask) < 0) {
		perror("cgi parent SIGCHLD restore");
	}

	return inspect_status(pid, status);
}
