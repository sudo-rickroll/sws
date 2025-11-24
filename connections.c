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

int
create_connections(char *address, uint16_t port){
	int sock, sockopt;
	struct addrinfo server, *next, *current;
	char port_cast[6];

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
		if((sock = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt))) < 0){
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
			if((sock = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &sockopt, sizeof(sockopt))) < 0){
				perror("setsockopt IPV6_V6ONLY");
				(void)close(sock);
				continue;
			}
		}

		if((sock = bind(sock, current->ai_addr, current->ai_addrlen)) < 0){
			perror("bind");
			(void)close(sock);
			continue;
		}

		if((sock = listen(sock, BACKLOG)) < 0){
			perror("listen");
			(void)close(sock);
			continue;
		}

		break;
	}

	freeaddrinfo(next);

	return sock;
}


