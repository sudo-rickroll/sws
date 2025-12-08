# Group Project - Simple Web Server

Liam Rust, Dimitri Stamoutsos, Rangasai Kumbhashi Raghavendra

Created 2025-11-12

Submitted 2025-12-08

## Project

Our goal was to create a simple web server as a group in accordance to the given man page
and other specifications such as RFC1945.

## Work Split Flow

Below is how the work was split up amongst group members. For each major segment, each
group member had created their own working branch in git to work from, and it later was
communicated that a merge would take place in main whenever that branch's active task
was complete.

## Pre-Snapshot

Dimitri - Input Validation

Liam - Program Startup, Basic Networking

Rangasai - Logging, Basic Networking

For the snapshot, we are aware that there was not much development for our project. This was
due to a lot of other coursework piling on each of us to where it was difficult to make time
and contribute. However, through all of this, we were closely communicating these needs through
our private Discord channel and were able to work on what was necessary for the snapshot.

## Post-Snapshot

Dimitri - Directory Indexing, Status Code Printing, Magic, If-Modified-Since, Input Parsing and Validation, File
Serving, various bug fixes.

Liam - File path traversal, user directory access, cross-platform support and build configuration, many bug fixes,
daemonization and further startup improvements.

Rangasai - Connection Protocols, Logging, CGI, Process Management (except daemonization), Debug Mode, Usage Help,
Utility functions, Bug fixes.

## Known Issues & Limiations

We did unfortunately run up against a bit of a time crunch, but we appear to have gotten most of the functionality.

Currently, this program takes CGI files with headers related to content type and the body itself. It cannot include the
Response Status code or any of the other server-related information.

If the port the server wishes to connect to is taken, the server will infinitely loop on "accept: Bad file number".

We are sure there are more, but those are for you to find out! Thank you for reading this, and have fun with the
server :)