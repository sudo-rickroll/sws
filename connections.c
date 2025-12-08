#include "connections.h"

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <err.h>
#include <errno.h>
#include <fts.h>
#include <limits.h>
#include <magic.h>
#include <netdb.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>

#include "cgi.h"
#include "handlers.h"
#include "logger.h"
#include "types.h"
#include "util.h"

#define BACKLOG 10
#define DEFAULT_PORT "8080"
#define ENABLE_OPTION 1
#define DISABLE_OPTION 0
#define PORT_NUMBER_MAX 6
/* Safe increment for dynamic allocation of HTML body length */
#define INCREMENT 512

int
is_http(const char *buf)
{
	if (buf == NULL) {
		return 0;
	}

	/* Does not have GET/HEAD and version somewhere, isn't HTTP */
	if ((strncmp(buf, "GET ", 4) == 0 || strncmp(buf, "HEAD ", 5) == 0) &&
	    (strstr(buf, "HTTP/1.0") || strstr(buf, "HTTP/1.1"))) {
		return 1;
	}

	return 0;
}

time_t
if_modified_since(const char *buf)
{
	char *ims = strcasestr(buf, "If-Modified-Since:");
	struct tm gmt;
	char date[BUFSIZ];
	int i = 0;

	memset(&gmt, 0, sizeof(gmt));
	
	if (ims == NULL) {
		return (time_t)-1;
	}

	ims += strlen("If-Modified-Since:");

	/* Skip nullspace */
	while (*ims == ' ' || *ims == '\t') {
		ims++;
	}

	/* Put given IMS into date buffer */
	while (*ims != '\0' && i < (int)sizeof(date)-1) {
		if (*ims == '\r' || *ims == '\n') {
			break;
		}

		date[i] = *ims;
		i++;
		ims++;
	}
	date[i] = '\0';

	/* Parse and check format */
	if (strptime(date, "%a, %d %b %Y %H:%M:%S GMT", &gmt) == NULL) {
		return (time_t)-1;
	}

	return timegm(&gmt);
}

char *
index_directory(const char *dir_path, const char *request_path)
{
	char *paths[2];
	char *html;
	char link_name[PATH_MAX];
	FTS *fts;
	FTSENT *entry;
	size_t len = 0;
	size_t capacity = BUFSIZ;

	paths[0] = (char *)dir_path;
	paths[1] = NULL;

	html = malloc(capacity);
	if (html == NULL) {
		return NULL;
	}

	/* Initial HTML format */
	len += snprintf(html + len, capacity - len,
	                "<!DOCTYPE html>\n"
	                "<html><head><meta charset=\"UTF-8\">"
	                "<title>Index of %s</title></head><body>\n"
	                "<h1>Index of %s</h1>\n"
	                "<ul>\n",
	                request_path, request_path);

	/* Error if fail */
	fts = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, fts_compare);
	if (fts == NULL) {
		len +=
			snprintf(html + len, capacity - len,
		             "<li>Error indexing directory.</li>\n</ul></body></html>");
		return html;
	}

	/* Read directory */
	while ((entry = fts_read(fts)) != NULL) {
		/* Only top-level */
		if (entry->fts_level != 1) {
			continue;
		}

		/* Each dir was listed twice, skipping postorder */
		if (entry->fts_info == FTS_DP) {
			continue;
		}

		/* Skip files starting with . */
		if (entry->fts_name[0] == '.') {
			continue;
		}

		/* Dynamically expand */
		if (len + INCREMENT > capacity) {
			capacity *= 2;
			html = realloc(html, capacity);
			if (html == NULL) {
				fts_close(fts);
				return NULL;
			}
		}

		/* End with / if directory */
		if (entry->fts_info == FTS_D) {
			snprintf(link_name, sizeof(link_name), "%s/", entry->fts_name);
		} else {
			snprintf(link_name, sizeof(link_name), "%s", entry->fts_name);
		}

		/* Add entry to body */
		len +=
			snprintf(html + len, capacity - len,
		             "<li><a href=\"%s\">%s</a></li>\n", link_name, link_name);
	}

	/* Cleanup, return */
	fts_close(fts);

	len += snprintf(html + len, capacity - len, "</ul>\n</body></html>\n");

	return html;
}

void
status_print(int sock, const char *version, const char *request,
             int status_code, const char *message, const char *mime_type,
             const struct stat *st, char *client_ip)
{
	FILE *sock_fp;
	size_t length;
	char *elabReq;

	if ((sock_fp = fdopen(sock, "w")) == NULL) {
		perror("fdopen sock");
		return;
	}

	length = strlen(version) + strlen(request) + 2;
	if ((elabReq = malloc(length * sizeof(char))) == NULL) {
		perror("request size allocation");
		return;
	}
	if (snprintf(elabReq, length, "%s %s", request, version) < 0) {
		fprintf(stderr, "Unable to concatenate request type and version");
		free(elabReq);
		return;
	}

	fprintf(sock_fp, "%s %d %s\r\n", version, status_code, message);
	fprintf(sock_fp, "Date: %s\r\n", get_time(0, FORMAT_HTTP));
	fprintf(sock_fp, "Server: sws/1.0\r\n");
	if (st != NULL) {
		fprintf(sock_fp, "Last-Modified: %s\r\n",
		        get_time(st->st_mtime, FORMAT_HTTP));
	}
	if (mime_type != NULL) {
		fprintf(sock_fp, "Content-Type: %s\r\n", mime_type);
	}
	if (st != NULL) {
		fprintf(sock_fp, "Content-Length: %ld\r\n", st->st_size);
	}
	fprintf(sock_fp, "\r\n");
	fflush(sock_fp);
	log_stream(client_ip, elabReq, status_code, st ? st->st_size : 0);
	free(elabReq);
}

void
header_print(int sock, const char *version, const char *request){
	char buf[1024];
	const char *terminator;
	int len;

	terminator = (strcmp(request, "HEAD") == 0) ? "\r\n" : "";

	len = snprintf(buf, sizeof(buf),
			"%s 200 OK \r\n"
			"Date: %s\r\n"
			"Server: sws/1.0\r\n"
			"%s",
			version,
			get_time(0, FORMAT_HTTP),
			terminator);

	if(len > 0){
		if(write(sock, buf, len) != len){
			perror("write headers");
		}
	}
}

int
create_connections(char *address, uint16_t port)
{
	int sock, sockopt;
	struct addrinfo server, *next, *current;
	char port_cast[PORT_NUMBER_MAX];

	snprintf(port_cast, sizeof(port_cast), "%hu", port);

	memset(&server, 0, sizeof(server));

	server.ai_family = AF_UNSPEC;
	server.ai_socktype = SOCK_STREAM;
	server.ai_flags = AI_PASSIVE;

	if (getaddrinfo(address, port_cast, &server, &next) > 0) {
		err(EXIT_FAILURE, "Unable to resolve address %s:%hu", address, port);
	}

	/* If -i isn't specified, try list of addresses until one works */
	for (current = next; current != NULL; current = current->ai_next) {
		if ((sock = socket(current->ai_family, current->ai_socktype,
		                   current->ai_protocol)) < 0) {
			perror("socket");
			continue;
		}

		/* To make the port available instantly after
		 * a quick server restart, using SO_REUSEADDR.
		 * Otherwise, it'll be in TIME_WAIT.
		 */
		sockopt = ENABLE_OPTION;
		if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt,
		               sizeof(sockopt)) < 0) {
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
		if (current->ai_family == AF_INET6) {
			sockopt = DISABLE_OPTION;
			if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &sockopt,
			               sizeof(sockopt)) < 0) {
				perror("setsockopt IPV6_V6ONLY");
				(void)close(sock);
				continue;
			}
		}

		if (bind(sock, current->ai_addr, current->ai_addrlen) < 0) {
			perror("bind");
			(void)close(sock);
			continue;
		}

		if (listen(sock, BACKLOG) < 0) {
			perror("listen");
			(void)close(sock);
			continue;
		} else {
			struct sockaddr_storage server_address;
			socklen_t length;
			char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

			length = sizeof(server_address);

			if (getsockname(sock, (struct sockaddr *)&server_address,
			                &length) == 0 &&
			    getnameinfo((struct sockaddr *)&server_address, length, hbuf,
			                sizeof(hbuf), sbuf, sizeof(sbuf),
			                NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
				(void)printf("Server listening on address %s and port %s\n",
				             hbuf, sbuf);
			}
		}

		break;
	}

	freeaddrinfo(next);

	return sock;
}

static void
display_client_details(struct sockaddr_storage *address, socklen_t length,
                       char *ip, char *port, size_t ip_len, size_t port_len)
{

	if (getnameinfo((struct sockaddr *)address, length, ip, ip_len, port,
	                port_len, NI_NUMERICHOST | NI_NUMERICSERV)) {
		(void)snprintf(ip, ip_len, "Unknown");
		return;
	}

	(void)printf("Connection request has arrived from: %s:%s\n", ip, port);
}

static void
handle_connections(int sock, char *docroot, char *ip, char *cgidir,
                   uint16_t port)
{
		char buf[BUFSIZ];
	/* Choose 16 because neither version nor request can possibly be over this amount. Choose PATH_MAX+1 because the sscanf macro stringify trick doesn't allow us to get *exactly* PATH_MAX-1 characters + 1 null terminator. */
	char version[16], request[16], path[PATH_MAX+1];
	char canonic_filepath[PATH_MAX];
	char query_string[PATH_MAX];

	FILE *sock_fp;
	ssize_t rval;
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

	do {
		bzero(buf, sizeof(buf));
		bzero(canonic_filepath, sizeof(canonic_filepath));
		bzero(query_string, sizeof(query_string));
		bzero(path, sizeof(path));

		if ((rval = read(sock, buf, BUFSIZ)) < 0) {
			warn("Stream read error");
			break;
		}

		if (rval == 0) {
			break;
		}

		buf[rval] = '\0';

		/* If not normal HTTP, send and done */
		if (!is_http(buf)) {
			break;
		}

		/* Validate parts of request */
		/* https://gcc.gnu.org/onlinedocs/cpp/Stringizing.html */
#define xstr(s) str(s)
#define str(s) #s
		if (sscanf(buf, "%15s %" xstr(PATH_MAX) "s %15s", request, path, version) != 3) {
			status_print(sock, "HTTP/1.0", request, 400, "Bad Request", NULL,
						 NULL, ip);
			log_stream(ip, path, 400, 0);
			break;
		}

		if (strcmp(version, "HTTP/1.0") != 0 &&
			strcmp(version, "HTTP/1.1") != 0) {
			perror("strcmp");
			break;
			}

		if (strcmp(request, "GET") != 0 && strcmp(request, "HEAD") != 0) {
			perror("strcmp");
			break;
		}
		errno = 0;
		rval = resolve_request_uri(path, docroot, cgidir, canonic_filepath, query_string, sizeof(query_string));
		if (rval == -1) {
			status_print(sock, version, request, 500,
							 "Internal Server Error", NULL, NULL, ip);
			log_stream(ip, path, 500, 0);
		} else if (rval == 1) {
			if (errno == ENOENT) {
				status_print(sock, version, request, 404, "Not Found", NULL,
							 NULL, ip);
				log_stream(ip, path, 404, 0);
			} else {
				status_print(sock, version, request, 400, "Bad Request", NULL,
							 NULL, ip);
				log_stream(ip, path, 400, 0);
			}
		}
		if (cgidir != NULL && is_cgi(path)) {
			char port_cast[PORT_NUMBER_MAX];

			if (stat(canonic_filepath, &st) != 0) {
				status_print(sock, version, request, 404, "Not Found", NULL,
				             NULL, ip);
				log_stream(ip, path, 404, 0);
				break;
			}

			/* Check if regular file and can be executed */
			if (!S_ISREG(st.st_mode) || access(canonic_filepath, X_OK) != 0) {
				status_print(sock, version, request, 403, "Forbidden", NULL,
				             NULL, ip);
				log_stream(ip, path, 403, 0);
				break;
			}

			if (snprintf(port_cast, sizeof(port_cast), "%hu", port) < 0) {
				perror("snprintf cgi port");
				break;
			}
			
			/* This will break if cgi contains response code. Going by professor's slack response */ 
			header_print(sock, version, request);

			if(strcmp(request, "HEAD") == 0){
				char elabReq[BUFSIZ];
				snprintf(elabReq, sizeof(elabReq), "%s %s", request, version);
				log_stream(ip, path, 200, 0);
				break;
			}


			if (cgi_exec(sock, canonic_filepath, request, version, query_string, path,
			             port_cast, ip) < 0) {
				status_print(sock, version, request, 500,
				             "Internal Server Error", NULL, NULL, ip);
				log_stream(ip, path, 500, 0);
			}

			else{
				char elabReq[BUFSIZ];
				snprintf(elabReq, sizeof(elabReq), "%s %s", request, version);
				/* using 0 because I don't know the bytes read from cgi */
				log_stream(ip, elabReq, 200, 0);
			}

			break;
		}

		/* At this point it is a good request. Can serve */

		if (stat(canonic_filepath, &st) != 0) {
			perror("stat");
			break;
		}

		/* Check for IMS */
		time_t ims = if_modified_since(buf);

		/* Has not changed, 304 */
		if (ims != (time_t)-1 && st.st_mtime <= ims) {
			if ((sock_fp = fdopen(sock, "w")) == NULL) {
				perror("fdopen sock");
				break;
			}
			fprintf(sock_fp, "%s 304 Not Modified\r\n", version);
			fprintf(sock_fp, "Date: %s\r\n", get_time(0, FORMAT_HTTP));
			fprintf(sock_fp, "Server: sws/1.0\r\n");
			fprintf(sock_fp, "Last-Modified: %s\r\n", get_time(st.st_mtime, FORMAT_HTTP));
			fprintf(sock_fp, "\r\n");
			fflush(sock_fp);

			log_stream(ip, path, 304, 0);

			break;
		}

		/* Is a regular file */
		if (S_ISREG(st.st_mode)) {
			/* More magic! */
			mime_type = magic_file(magic_cookie, canonic_filepath);
			if (mime_type == NULL) {
				perror("magic");
				break;
			}

			/* Print SUCCESSFUL request details */
			status_print(sock, version, request, 200, "OK", mime_type, &st, ip);

			/* Serve file if GET */
			if (strcmp(request, "GET") == 0) {
				char filebuf[BUFSIZ];
				size_t n;

				FILE *fp = fopen(canonic_filepath, "r");
				if (fp == NULL) {
					status_print(sock, version, request, 500,
					             "Internal Server Error", NULL, NULL, ip);
					log_stream(ip, path, 500, 0);
					break;
				}

				while ((n = fread(filebuf, 1, sizeof(filebuf), fp)) > 0) {
					write(sock, filebuf, n);
				}

				fclose(fp);
			}
		}

		/* Is a directory */
		if (S_ISDIR(st.st_mode)) {
			char index_path[PATH_MAX];
			struct stat index_st;

			if (snprintf(index_path, sizeof(index_path), "%s/index.html",
			             canonic_filepath) < 0) {
				perror("snprintf");
				break;
			}

			/* index.html exists, serve it */
			if (stat(index_path, &index_st) == 0 && S_ISREG(index_st.st_mode)) {
				/* More magic! */
				mime_type = magic_file(magic_cookie, index_path);
				if (mime_type == NULL) {
					perror("magic");
					break;
				}

				/* Print SUCCESSFUL request details */
				status_print(sock, version, request, 200, "OK", mime_type,
				             &index_st, ip);

				if (strcmp(request, "GET") == 0) {
					char filebuf[BUFSIZ];
					size_t n;

					FILE *fp = fopen(index_path, "r");
					if (fp == NULL) {
						status_print(sock, version, request, 500,
						             "Internal Server Error", NULL, NULL, ip);
						log_stream(ip, path, 500, 0);
						break;
					}

					while ((n = fread(filebuf, 1, sizeof(filebuf), fp)) > 0) {
						write(sock, filebuf, n);
					}

					fclose(fp);
				}
			}
			/* index.html does not exist, print directory index in HTML format
			 */
			else {
				char *html = index_directory(canonic_filepath, path);
				off_t html_len = (off_t)strlen(html);

				if (html == NULL) {
					status_print(sock, version, request, 500,
					             "Internal Server Error", NULL, NULL, ip);
					break;
				}

				/* Manually print headers because dir length must be sent
				 * accurately and status_print cannot handle this along
				 */
				if ((sock_fp = fdopen(sock, "w")) == NULL) {
					perror("fdopen sock");
					break;
				}
				fprintf(sock_fp, "%s 200 OK\r\n", version);
				fprintf(sock_fp, "Date: %s\r\n", get_time(0, FORMAT_HTTP));
				fprintf(sock_fp, "Server: sws/1.0\r\n");
				fprintf(sock_fp, "Content-Type: text/html\r\n");
				fprintf(sock_fp, "Content-Length: %ld\r\n", html_len);
				fprintf(sock_fp, "\r\n");

				fflush(sock_fp);

				if (strcmp(request, "GET") == 0) {
					write(sock, html, html_len);
				}

				free(html);
			}
		}

	} while (rval != 0);

	/* Cleanup */
	magic_close(magic_cookie);
}

void
accept_connections(int sock, sws_options_t *config)
{
	struct sockaddr_storage address;
	char client_ip[NI_MAXHOST], client_port[NI_MAXSERV];
	int fd;
	socklen_t length;
	pid_t pid;

	if(signal(SIGINT, handle_term) == SIG_ERR){
		perror("sigint handler");
	}

	if(signal(SIGTERM, handle_term) == SIG_ERR){
		perror("sigterm handler");
	}
	
	if(!config->debug){
		if(signal(SIGCHLD, handle_sig) == SIG_ERR){
			perror("sigchld handler");
		}
	}
	else{
		printf("Server is now running in debug mode\n");
	}

	for (;;) {
		length = sizeof(address);
		if ((fd = accept(sock, (struct sockaddr *)&address, &length)) < 0) {
			(errno == EINTR) ? perror("signal") : perror("accept");
			continue;
		}

		display_client_details(&address, length, client_ip, client_port,
		                       sizeof(client_ip), sizeof(client_port));

		if(config->debug){
			handle_connections(fd, config->docroot, client_ip, config->cgi, config->port);
			(void)printf("Client %s:%s has closed connection.\n\n", client_ip, client_port);
		}
		else{
			if((pid = fork()) < 0){
				perror("fork");
				close(fd);
				continue;
			}


			if(pid == 0){
				(void)close(sock);
				handle_connections(fd, config->docroot, client_ip, config->cgi, config->port);
				(void)printf("Client %s:%s has closed connection...\n\n", client_ip, client_port);
				(void)close(fd);
				exit(EXIT_SUCCESS);
			}

			(void)close(fd);
		}
	}
}
