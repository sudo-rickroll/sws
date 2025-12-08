SWS (1) NetBSD General Commands Manual SWS (1)
Dimitri, Liam, Rangasai

## NAME

```
sws —asimple web server
```
**SYNOPSIS
sws** [ **−dh** ][ **−c** _dir_ ][ **−i** _address_ ][ **−l** _file_ ][ **−p** _port_ ] _dir_

**DESCRIPTION
sws** is a very simple web server.Itbehavesquite likeyou would expect from anyregular web server,inthat
it binds to a givenport on the givenaddress and waits for incoming HTTP/1.0 requests. It serves content
from the givendirectory.That is, anyrequests for documents is resolved relative tothis directory (the docu-
ment root).

**OPTIONS**
The following options are supported by **sws** :
**−c** _dir_ Allowexecution of CGIs from the givendirectory.SeeCGIsfor details.
**−d** Enter debugging mode. That is, do not daemonize, only accept one connection at a time
and enable logging to stdout.
**−h** Print a short usage summary and exit.
**−i** _address_ Bind to the givenIPv4 or IPv6 address. If not provided, **sws** will listen on all IPv4 and
IPv6 addresses on this host.
**−l** _file_ Log all requests to the givenfile. SeeLOGGINGfor details.
**−p** _port_ Listen on the givenport. Ifnot provided, **sws** will listen on port 8080.

**DETAILS
sws** speaks a crippled dialect of HTTP/1.0: it responds to GET and HEAD requests according to RFC1945,
and only supports the If-Modified-Since Request-Header.
When a connection is made, **sws** will respond with the appropriate HTTP/1.0 status code and the following
headers:
Date The current timestamp in GMT.
Server Astring identifying this server and version.
Last-Modified The timestamp in GMT of the file’slast modification date.
Content-Type The correct content-type for the file in question as determined viamagic(5) patterns.
Content-Length The size in bytes of the data returned.
If the request type was a GET,then it will subsequently return the data of the requested file. After serving
the request, the connection is terminated.

**FEATURES
sws** supports a number of interesting features:
CGIs Execution of CGIs as described inCGIs.
Directory Indexing If the request was for a directory and the directory does not contain a file
named "index.html", then **sws** will generate a directory index, listing the con-
tents of the directory in alphanumeric order.Files starting with a "." are
ignored.

NetBSD 9.3 November 12, 2021 1


SWS (1) NetBSD General Commands Manual SWS (1)

```
User Directories If the request begins with a " ̃", then the following string up to the first slash is
translated into that user’s sws directory (ie /home/<user>/sws/ ).
```
**CGIs**
If a URI begins with the string "/cgi-bin", and the **−c** flag was specified, then the remainder of the resource
path will be resolved relative tothe directory specified using this flag. The resulting file will then be
executed and anyoutput generated is returned instead. Execution of CGIs follows the specification in
RFC3875.

**LOGGING**
Per default, **sws** does not do anylogging. Ifexplicitly enabled via the **−l** flag, **sws** will log every request
in a slight variation of Apache’sso-called "common" format: ’%a %t "%r" %>s %b’. That is, it will log:
%a The remote IP address.
%t The time the request was received(in GMT).
%r The (quoted) first line of the request.
%>s The status of the request.
%b Size of the response in bytes. Ie, "Content-Length".
Example log lines might look likeso:
155.246.89.8 2019-10-28T12:12:12Z "GET / HTTP/1.0" 200 1406
2001::fe72:3900 2019-10-28T12:12:12Z "GET /file HTTP/1.0" 404 312
All lines will be appended to the givenfile unless **−d** wasgiv en, in which case all lines will be printed to std-
out.

**EXAMPLE INVOCATIONS**
In order to test **sws** ,you may wish to compare to e.g.,bozohttpd(8). For this, let us startbozohttpd(8)
in the background ( **−b** ), listen on port 8080 ( **−I** _8080_ ), enable user translation using the "sws" directory
name ( **−u −p** _sws_ ), enable CGI execution from the "cgi-bin" directory ( **−c** _cgi-bin_ )under the docu-
ment root, and servedata from the directory "docroot":
/usr/libexec/httpd -b -I 8080 -X -u -p sws -c cgi-bin docroot
The equivalent invocation for **sws** to run side-by-side on port 8081 would then be:
sws -p 8081 -c cgi-bin docroot

**EXIT STATUS**
The **sws** utility exits 0 on success, and >0 if an error occurs.

**SEE ALSO**
RFC

**HISTORY**
Asimple http server has been a frequent final project assignment for the class _Advanced Programming
in the UNIX Environment_ at Stevens Institute of Technology.This variation was first assigned in the
Fall 2008 by Jan Schaumann.

NetBSD 9.3 November 12, 2021 2


**LIMITATIONS**
Currently, this program takes CGI files with headers related to content type and the body itself. It cannot include the Response Status code or any of the other server-related information.

**CONTRIBUTIONS**
Rangasai - Connection Protocols, Logging, CGI, Process Management (except daemonization), Debug Mode, Usage Help