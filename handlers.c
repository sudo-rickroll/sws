#include <sys/wait.h>

#include "handlers.h"

void
reap_connection(int signo){
	int status;
	
	(void)signo;

	while(waitpid(-1, &status, WNOHANG) > 0);
}	
