#include <sys/wait.h>

#include <fts.h>
#include <strings.h>

#include "handlers.h"

void
reap_connection(int signo){
	int status;
	
	(void)signo;

	while(waitpid(-1, &status, WNOHANG) > 0);
}	

/* strcasecmp is not case sensitive, like how normal web servers are */
int fts_compare(const FTSENT **a, const FTSENT **b) {
	return strcasecmp((*a)->fts_name, (*b)->fts_name);
}
