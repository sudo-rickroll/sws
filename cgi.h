#ifndef CGI_H
#define CGI_H

int
is_cgi(const char *);

int
cgi_exec(int, const char *, const char *, const char *, const char *, const char *, const char *, const char *);

#endif
