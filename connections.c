#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <err.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "types.h"
#include "connections.h"

#define BACKLOG 10
#define DEFAULT_PORT "8080"
#define ENABLE_OPTION 1
#define DISABLE_OPTION 0
#define PORT_NUMBER_MAX 6

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
	char hbuf[NI_MAXHOST];

	if(getnameinfo((struct sockaddr *)address, length, hbuf, sizeof(hbuf), NULL, 0, NI_NUMERICHOST)){
		errx(EXIT_FAILURE, "getnameinfo: Unable to get numeric hostname");
	}

	(void)printf("Connection request has arrived from: %s\n", hbuf);
}

static void
handle_connections(int sock){
	int rval;
	do{
		char buf[BUFSIZ];
		bzero(buf, sizeof(buf));

		if((rval = read(sock, buf, BUFSIZ)) < 0){
			perror("Stream read error");
		}

		if(rval == 0){
			(void)printf("Ending connection\n");
		}
		else{
			(void)printf("Client sent \n%s\n", buf);
		}
	} while(rval != 0);
}

void
accept_connections(int sock){
	struct sockaddr_storage address;
	int fd;
	socklen_t length;

	(void)printf("Server is open for connections!!!! \n");

	for(;;){
		length = sizeof(address);
		if((fd = accept(sock, (struct sockaddr *)&address, &length)) < 0){
			perror("accept");
			continue;
		}

		display_client_details(&address, length);
		handle_connections(fd);

		(void)close(fd);
		(void)printf("Closing connection...\n\n");
	}
}



