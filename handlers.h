#ifndef HANDLERS_H

#define HANDLERS_H

void
reap_connection(int signo);

int fts_compare(const FTSENT **a, const FTSENT **b);

#endif
