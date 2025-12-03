#include <arpa/inet.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>

#include <netinet/in.h>

#include <err.h>
#include <errno.h>
#include <limits.h>
#include <magic.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "types.h"
#include "connections.h"
#include "handlers.h"

#define BACKLOG 10
#define DEFAULT_PORT "8080"
#define ENABLE_OPTION 1
#define DISABLE_OPTION 0
#define PORT_NUMBER_MAX 6

int is_http(const char *buf) {
	/* Check for \r\n. If not, not HTTP */
	if (!strstr(buf, "\r\n")) {
		return 0;
	}

	/* Is HTTP */
	if ((strncmp(buf, "GET ", 4) == 0 || strncmp(buf, "HEAD ", 5) == 0)
		&& (strstr(buf, "HTTP/1.0") || strstr(buf, "HTTP/1.1"))) {
		return 1;
	}

	return 0;
}

char *http_date_display(time_t t) {
	static char buf[BUFSIZ];
	struct tm tm;

	gmtime_r(&t, &tm);

	/* RFC1945 format */
	strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", &tm);

	return buf;
}

void status_print(int sock, const char *version, int status_code, const char *message,
			const char *mime_type, const struct stat *st) {
	dprintf(sock, "%s %d %s\r\n", version, status_code, message);
	dprintf(sock, "Date: %s\r\n", http_date_display(time(NULL)));
	dprintf(sock, "Server: sws/1.0\r\n");
	if (st != NULL) {
		dprintf(sock, "Last-Modified: %s\r\n", http_date_display(st->st_mtime));
	}
	if (mime_type != NULL) {
		dprintf(sock, "Content-Type: %s\r\n", mime_type);
	}
	if (st != NULL) {
		dprintf(sock, "Content-Length: %ld\r\n", st->st_size);
	}
	dprintf(sock, "\r\n");
}

int
create_connections(char *address, uint16_t port){
	int sock, sockopt;
	struct addrinfo server, *next, *current;
	char port_cast[PORT_NUMBER_MAX];

	snprintf(port_cast, sizeof(port_cast), "%hu", port);

	memset(&server, 0, sizeof(server));

	server.ai_family = AF_UNSPEC;
	server.ai_socktype = SOCK_STREAM;
	server.ai_flags = AI_PASSIVE;

	if(getaddrinfo(address, port_cast, &server, &next) > 0){
		err(EXIT_FAILURE, "Unable to resolve address %s:%hu", address, port);
	}
	
	/* If -i isn't specified, try list of addresses
	 * until one works
	 */
	for(current = next; current != NULL; current = current->ai_next){
		if((sock = socket(current->ai_family, current->ai_socktype, current->ai_protocol)) < 0){
			perror("socket");
			continue;
		}

		/* To make the port available instantly after 
		 * a quick server restart, using SO_REUSEADDR. 
		 * Otherwise, it'll be in TIME_WAIT.
		 */
		sockopt = ENABLE_OPTION;
		if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt)) < 0){
			perror("setsockopt SO_REUSEADDR");
			(void)close(sock);
			continue;
		}

		/* For dualstack. Otherwise, we'd have to do 
		 * PF_INET and PF_INET6 separately as in the
		 * dualstack-streamread.c program from the
		 * lecture. With this, IPv6 accepts both
		 * v4 and v6
		 */
		if(current->ai_family == AF_INET6){
			sockopt = DISABLE_OPTION;
			if(setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &sockopt, sizeof(sockopt)) < 0){
				perror("setsockopt IPV6_V6ONLY");
				(void)close(sock);
				continue;
			}
		}

		if(bind(sock, current->ai_addr, current->ai_addrlen) < 0){
			perror("bind");
			(void)close(sock);
			continue;
		}

		if(listen(sock, BACKLOG) < 0){
			perror("listen");
			(void)close(sock);
			continue;
		}

		break;
	}

	freeaddrinfo(next);

	return sock;
}

static void
display_client_details(struct sockaddr_storage *address, socklen_t length){
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

	if(getnameinfo((struct sockaddr *)address, length, hbuf, sizeof(hbuf), sbuf, sizeof(sbuf), NI_NUMERICHOST)){
		errx(EXIT_FAILURE, "getnameinfo: Unable to get numeric hostname");
	}

	(void)printf("Connection request has arrived from: %s:%s\n", hbuf, sbuf);
}

static void
handle_connections(int sock, char *docroot){
	int rval;
	int filepath_len;

	char buf[BUFSIZ];
	char filepath[PATH_MAX];
	/* Choose 16 because neither version nor request can
	 * possibly be over this amount */
	char version[16], request[16], path[PATH_MAX];
	char canonic_filepath[PATH_MAX];
	char canonic_docroot[PATH_MAX];

	struct stat st;
	const char *mime_type;


	/* Magic! */
	magic_t magic_cookie = magic_open(MAGIC_MIME_TYPE);
	if (magic_cookie == NULL) {
		perror("magic");
		return;
	}
	if (magic_load(magic_cookie, NULL) != 0) {
		perror("magic");
		magic_close(magic_cookie);
		return;
	}

	do{
		bzero(buf, sizeof(buf));
		bzero(filepath, sizeof(filepath));

		if((rval = read(sock, buf, BUFSIZ)) < 0){
			perror("Stream read error");
			break;
		}

		if(rval == 0){
			(void)printf("Ending connection\n");
			break;
		}

		buf[rval] = '\0';
		(void)printf("Client sent \n%s\n", buf);

		/* If not normal HTTP, send and done */
		if (!is_http(buf)) {
			break;
		}

		/* Validate parts of request */
		/* sscanf does not allow * sizes so I explicitly write them out for it */
		if (sscanf(buf, "%15s %4095s %15s", request, path, version) != 3) {
			status_print(sock, "HTTP/1.0", 400, "Bad Request", NULL, NULL);
		}

		if (strcmp(version, "HTTP/1.0") != 0 && strcmp(version, "HTTP/1.1") != 0) {
			status_print(sock, "HTTP/1.0", 505, "HTTP Version Not Supported", NULL, NULL);
			break;
		}
		
		if (strcmp(request, "GET") != 0 && strcmp(request, "HEAD") != 0) {
			status_print(sock, version, 405, "Method Not Allowed", NULL, NULL);
			break;
		}

		/* At this point it is a good request. Can serve */
		
		filepath_len = snprintf(filepath, sizeof(filepath), "%s/%s", docroot, path + 1);
		if (filepath_len < 0) {
			perror("snprintf");
			break;
		}

		if ((size_t)filepath_len >= sizeof(filepath)) {
			status_print(sock, version, 414, "URI Too Long", NULL, NULL);
			break;
		}

		/* Should not fail but guard. For comparison later */
		if (!realpath(docroot, canonic_docroot)) {
			perror("realpath docroot");
			break;
		}

		/* Not resolved */
		if (!realpath(filepath, canonic_filepath)) {
			status_print(sock, version, 404, "Not Found", NULL, NULL);
			break;
		}

		/* Traversal prevent by checking for docroot prefix */
		if (strncmp(canonic_filepath, canonic_docroot, strlen(canonic_docroot)) != 0) {
			status_print(sock, version, 403, "Forbidden", NULL, NULL);
			break;
		}

		/* Not a regular file */
		if (stat(canonic_filepath, &st) != 0 || !S_ISREG(st.st_mode)) {
			status_print(sock, version, 404, "Not Found", NULL, NULL);
			break;
		}

		/* More magic! */
		mime_type = magic_file(magic_cookie, canonic_filepath);
		if (mime_type == NULL) {
			perror("magic");
			break;
		}

		/* Print SUCCESSFUL request details */
		status_print(sock, version, 200, "OK", mime_type, &st);

		/* Serve file if GET */
		if (strcmp(request, "GET") == 0 && filepath != NULL) {
			char filebuf[BUFSIZ];
			size_t n;

			FILE *fp = fopen(filepath, "r");
			if (fp == NULL) {
				status_print(sock, version, 500, "Internal Server Error",
						NULL, NULL);
				break;
			}

			while ((n = fread(filebuf, 1, sizeof(filebuf), fp)) > 0) {
				write(sock, filebuf, n);
			}

			fclose(fp);
		}

	} while(rval != 0);

	/* Cleanup */
	magic_close(magic_cookie);
}

void
accept_connections(int sock, char *docroot){
	struct sockaddr_storage address;
	int fd;
	socklen_t length;
	pid_t pid;

	if(signal(SIGCHLD, reap_connection) == SIG_ERR){
		perror("signal handler");
	}


	(void)printf("Server is open for connections!!!! \n");

	for(;;){
		length = sizeof(address);
		if((fd = accept(sock, (struct sockaddr *)&address, &length)) < 0){
			(errno == EINTR) ? perror("signal") : perror("accept");
			continue;
		}

		display_client_details(&address, length);

		if((pid = fork()) < 0){
			perror("fork");
			continue;
		}


		if(pid == 0){
			(void)close(sock);
			handle_connections(fd, docroot);
			(void)printf("Closing connection...\n\n");
			(void)close(fd);
			exit(EXIT_SUCCESS);
		}

		(void)close(fd);
		
	}
}



