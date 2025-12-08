#include "util.h"

#include <pwd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

const char *CGI_PATH = "/cgi-bin";
const char *USER_PATH = "/~";

/**
 * Take a user's URI, resolves all . and .. components contained in it,
 * and resolves it relative to its respective root - either the docroot,
 * the CGI bin folder, or a user's home folder.
 *
 * @param uri The request URI to resolve
 * @param docroot The absolute path of the standard web server root
 * @param cgi The absolute path of the CGI bin.
 * @param resolved_uri: A pointer to a char * of length at least PATH_MAX.
 * @param query: A pointer to a char * of at least PATH_MAX.
 * if applicable.
 * @return 0 on success
 * @return -1 on server error
 * @return 1 on client error (bad request)
 */
int
resolve_request_uri(char *uri, char *docroot, char *cgi, char *resolved_uri,
                    char *query, size_t query_len)
{
	struct passwd *user_info;
	char *dedot_buf;
	char *_uri_initial;
	char *_uri;
	char *root;
	char *full_path;
	char *tmp;
	size_t full_path_len;
	size_t dedot_pos = 0;
	bool is_cgi_dir = false;
	bool is_user_dir = false;

	if (uri == NULL || (_uri = _uri_initial = strdup(uri)) == NULL) {
		return -1;
	}

	/* Chop off the fragment, if it exists. We don't need it. */
	if ((tmp = strchr(_uri, '#')) != NULL) {
		*tmp = '\0';
	}
	/* If there is a query string, copy it to the query pointer and chop it off.
	 */
	if ((tmp = strrchr(_uri, '?')) != NULL) {
		(void)strncpy(query, tmp, query_len);
		*tmp = '\0';
	} else {
		query[0] = '\0';
	}

	/* If CGI, mark that for later and skip it. */
	if (strncmp(_uri, CGI_PATH, strlen(CGI_PATH)) == 0) {
		is_cgi_dir = true;
		_uri += strlen(CGI_PATH);
	}
	/* Else, if it starts with the user directory /~, find that user's home dir.
	   and save for later */
	else if (strncmp(_uri, USER_PATH, strlen(USER_PATH)) == 0) {
		is_user_dir = true;
		_uri += strlen(USER_PATH);
		if ((tmp = strchr(_uri, '/')) != NULL) {
			*tmp = '\0';
		}
		if ((user_info = getpwnam(_uri)) == NULL || user_info->pw_dir == NULL) {
			free(_uri_initial);
			return 1;
		}
		/* Go to after the next slash if there was one, or until the end (or
		 * slash?) if there wasn't */
		if (tmp != NULL) {
			_uri = tmp + 1;
		} else {
			while (_uri[0] != '\0' && _uri[0] != '/') {
				_uri++;
			}
		}
	}

	/* Removing ../ and ./ shouldn't make it larger than the original. */
	if ((dedot_buf = calloc(strlen(_uri), sizeof(char))) == NULL) {
		free(_uri_initial);
		return -1;
	};

	/* Ok. Now we have to do some funky logic. Essentially, we need to keep
	 * traversing and collapsing "/./"s. We also need to traverse upwards when
	 * we see a "/../".
	 *
	 * This RFC describes what we need to do:
	 * https://datatracker.ietf.org/doc/html/rfc3986#section-5.2.4
	 *
	 * While it technically came after HTTP/1.0, I'm going to follow it anyway.
	 *
	 * However, we do all the logic above because we need to also keep inside
	 * whatever root we're currently in no matter how many "/../"s we see. For
	 * regular requests, that's docroot. For user requests, that's the home
	 * directory of the user + "/sws". For cgi-bin, that's "/cgi-bin"
	 *
	 * I know it was stated that we don't have to specially treat "." and "..":
	 * https://cs631apue2025.slack.com/archives/C0413N0EMGS/p1764431749518809
	 * But, given the requests and examples shown, I'm not sure how we can,
	 * outside chroot? But that would bring its own issues.
	 *
	 * For now, we are going to traverse and ignore our root until later. We can
	 * append to that root afterward! */

	/* Loop while input buffer is not empty */
	while (_uri[0] != '\0') {
		/* Note: We're going to be using a lot of magic numbers, simply because
		 * otherwise I would need to run str(n)len an unreasonable amount of
		 * time for known, constant string literals. I hope that's ok with you.
		 * If not, feel free to make a pull request :) */

		/* A. If the input buffer begins with a prefix of "../" or "./", then
		 * remove that prefix from the input buffer, otherwise... */
		if (strncmp(_uri, "../", 3) == 0) {
			_uri += 3; /* strlen(../) */
		} else if (strncmp(_uri, "./", 2) == 0) {
			_uri += 2; /* strlen(./) */
		}
		/* B. If the input buffer begins with a prefix of "/./" or "/.", where
		 * "." is a complete path segment, then replace that  prefix with "/" in
		 * the input buffer, otherwise... */
		else if (strncmp(_uri, "/./", 3) == 0) {
			_uri += 2; /* strlen(/./) - 1, so we end on that slash */
		} else if (strcmp(_uri, "/.") == 0) {
			_uri[1] = '\0'; /* uri = "/" */
		}
		/* C. if the input buffer begins with a prefix of "/../" or "/..", where
		 * ".." is a complete path segment, then replace that prefix with "/" in
		 * the input buffer and remove the last segment and its preceding "/"
		 * (if any) from the output buffer, otherwise... */
		else if (strncmp(_uri, "/../", 4) == 0) {
			_uri += 3; /* strlen(/../) - 1, so we end on that slash */
			while (dedot_pos > 0 && dedot_buf[dedot_pos] != '/') {
				dedot_buf[dedot_pos--] = '\0';
			}
		} else if (strcmp(_uri, "/..") == 0) {
			_uri[1] = '\0'; /* uri = "/" */
			while (dedot_pos > 0 && dedot_buf[dedot_pos] != '/') {
				dedot_buf[dedot_pos--] = '\0';
			}
		}
		/* D. if the input buffer consists only of "." or "..", then remove that
		   from the input buffer, otherwise... */
		else if (strcmp(_uri, ".") == 0) {
			_uri += 1;
		} else if (strcmp(_uri, "..") == 0) {
			_uri += 2;
		}
		/* E. move the first path segment in the input buffer to the end of the
		 * output buffer, including the initial "/" character (if any) and any
		 * subsequent characters up to, but not including, the next "/"
		 * character or the end of the input buffer. */
		else {
			if (_uri[0] == '/') {
				dedot_buf[dedot_pos++] = '/';
				_uri++;
			}
			while (_uri[0] != '\0' && _uri[0] != '/') {
				dedot_buf[dedot_pos++] = _uri[0];
				_uri++;
			}
		}
	}
	free(_uri_initial);

	if (is_cgi_dir && cgi != NULL) {
		root = cgi;
	} else if (is_user_dir && user_info->pw_dir != NULL) {
		root = user_info->pw_dir;
	} else {
		root = docroot;
	}

	/* must be at least len(dst) + len(src) + 1, + 4 for /sws */
	full_path_len = strlen(root) + strlen(dedot_buf) + 6;
	if ((full_path = calloc(full_path_len, sizeof(char))) == NULL) {
		free(dedot_buf);
		return -1;
	}

	/* concatenate paths! */
	full_path[0] = '\0';
	(void)strncat(full_path, root, full_path_len - strlen(full_path) - 1);
	if (is_user_dir) {
		(void)strncat(full_path, "/sws", full_path_len - strlen(full_path) - 1);
	}
	if (dedot_buf[0] != '/') {
		(void)strncat(full_path, "/", full_path_len - strlen(full_path) - 1);
	}
	(void)strncat(full_path, dedot_buf, full_path_len - strlen(full_path) - 1);
	free(dedot_buf);

	/* get the *real* real path? */
	(void)realpath(full_path, resolved_uri);
	free(full_path);
	return resolved_uri == NULL ? 1 : 0;
}