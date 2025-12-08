#ifndef HANDLERS_H

#define HANDLERS_H

#include <sys/types.h>

#include <fts.h>
#include <signal.h>

void
handle_sig(int);

void
handle_term(int);

int
block_sig(int, sigset_t *);

int
restore_sig(sigset_t *);

int
inspect_status(pid_t, int);

int 
fts_compare(const FTSENT **, const FTSENT **);

#endif
