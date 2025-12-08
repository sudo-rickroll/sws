#ifndef UTIL_H
#define UTIL_H
#include <stdio.h>

int resolve_request_uri(char *uri_copy, char *docroot, char *cgi,
                        char *resolved_uri, char *query, size_t query_len);

#endif