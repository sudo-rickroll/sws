# Simple Web Server

Liam Rust, Dimitri Stamoutsos, Rangasai Kumbhashi Raghavendra

## Project

Our goal was to create a simple web server as a group in accordance to
the manual page in https://stevens.netmeister.org/631/sws.1.pdf, while adhering to the specifications in RFC1945 and RFC3875.

## Work Split Flow

Below is how the work was split up amongst group members. For each major
segment, each group member had created their own working branch in git
to work from, and it later was communicated that a merge would take
place in main whenever that branch's active task was complete.

Dimitri - Directory Indexing, Status Code Printing, Magic,
If-Modified-Since, Input Parsing and Validation, File Serving, various
bug fixes.

Liam - File path traversal, user directory access, cross-platform
support and build configuration, many bug fixes, daemonization and
further startup improvements.

Rangasai - Connection Protocols, Logging, CGI, Process Management (except
daemonization), Debug Mode, Usage Help, Utility functions, Bug fixes.

## Known Issues & Limiations

Currently, this program takes CGI files with headers related to content
type and the body itself. It cannot include the Response Status code or
any of the other server-related information.

If the port the server wishes to connect to is taken, the server will
infinitely loop on "accept: Bad file number".

We are sure there are more, but those are for you to find out! Thank
you for reading this, and have fun with the server :)
