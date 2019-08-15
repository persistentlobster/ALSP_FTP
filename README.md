# ALSP_FTP
FTP client and server for Advanced Linux Systems Programming class at PSU (Su 2019)

Authors: Micah Burnett & Joseph Venetucci

# To Run
To compile both the client and server
`$ make all`

Otherwise 

`$ make client`

`$ make server`

# Notes for Mark

Here is a snapshot of our code as it was during the demo. We ran the server on ada.cs.pdx.edu and the client on our local machines, but you should be able to do ada to babbage

Run the server first by specifying a port:

`$ ./server 7050`

Then have the client connect to it using the IP address or hostname, and the port:

`$ ./client babbage.cs.pdx.edu 7050`

## Features

Here are the featues & commands that our program supports:

### Commands

- `pwd / lpwd` Prints the working directory of the server/client
- `cd / lcd`   Changes the working directory of the server/client
- `ls / lls`   Does an ls on the server/client. Supports args and options.
- `exit`       Exits the client
- `!cmd`       Putting a '!' infront of anything you write escapes it to the shell.
- `put`        put file(s) onto the server
- `get`        get file(s) from the server

Supports wildcard and tilde expansions
