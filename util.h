#ifndef UTIL_H
#define UTIL_H

int resolve_request_uri(char *uri, char *docroot, char *cgi,
                        char **resolved_uri, char **query);

#endif