#ifndef HANDLERS_H

#define HANDLERS_H


#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>
#include <signal.h>

void
reap_connection(int);

int
block_sig(int, sigset_t *);

int
restore_sig(sigset_t *);

int fts_compare(const FTSENT **, const FTSENT **);

#endif