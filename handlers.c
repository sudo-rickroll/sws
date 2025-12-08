#include "handlers.h"

#include <sys/wait.h>

#include <stddef.h>
#include <strings.h>

void
reap_connection(int signo)
{
	int status;

	(void)signo;

	while (waitpid(-1, &status, WNOHANG) > 0)
		;
}

int
block_sig(int signum, sigset_t *old_mask)
{
	sigset_t mask;

	if (sigemptyset(&mask) < 0 || sigaddset(&mask, signum) < 0 ||
	    sigprocmask(SIG_BLOCK, &mask, old_mask) < 0) {
		return -1;
	}

	return 0;
}

int
restore_sig(sigset_t *old_mask)
{

	if (sigprocmask(SIG_SETMASK, old_mask, NULL) < 0) {
		return -1;
	}

	return 0;
}

/* strcasecmp is not case sensitive, like how normal web servers are */
int
fts_compare(const FTSENT **a, const FTSENT **b)
{
	return strcasecmp((*a)->fts_name, (*b)->fts_name);
}