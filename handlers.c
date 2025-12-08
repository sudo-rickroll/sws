#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "handlers.h"

void
handle_sig(int signo){
	int status;
	pid_t pid;
	
	(void)signo;

	while((pid = waitpid(-1, &status, WNOHANG)) > 0){
		(void)inspect_status(pid, status);
	}
	
}

void
handle_term(int signo){
	const char *prompt = "\nReceived signal for shutdown\n";
	(void)signo;

	write(STDOUT_FILENO, prompt, strlen(prompt));

	_exit(EXIT_SUCCESS);
}

int 
block_sig(int signum, sigset_t *old_mask){
	sigset_t mask;

	if(sigemptyset(&mask) < 0 || sigaddset(&mask, signum) < 0 || sigprocmask(SIG_BLOCK, &mask, old_mask) < 0){
	       return -1;
	}	       
	
	return 0;
}

int 
restore_sig(sigset_t *old_mask){

	if(sigprocmask(SIG_SETMASK, old_mask, NULL) < 0){
		return -1;
	}

	return 0;
}

int
inspect_status(pid_t pid, int status){
	int exit_status;

	if(WIFEXITED(status) && (exit_status = WEXITSTATUS(status)) != 0){
		(void)fprintf(stderr, "Process %d exited with status %d\n", pid, exit_status);
		return -1;
	}

	if(WIFSIGNALED(status)){
		(void)fprintf(stderr, "Process %d was terminated by signal %d\n", pid, WTERMSIG(status));
		return -1;
	}

	return 0;
}


/* strcasecmp is not case sensitive, like how normal web servers are */
int fts_compare(const FTSENT **a, const FTSENT **b) {
	return strcasecmp((*a)->fts_name, (*b)->fts_name);
}
