udpftp

Server files in server folder
Client files in client folder
Log files in log folder (both client and server use same log files)

Both client and server make use of third part dirent to read files and directories on the OS

Host, Port, Package size are hardcoded in the application, look for the defines in the header.

Data is sent using the message structure, which defines headers of ints for sequence, finalBit, errorBit etc.
and a char buffer in the message structure called body is where the file contents is placed

Both Server and Client have own main class to run

Limits:
	Uses ints for header info
	List operation and filename is assumed to fit in one packet (theres a check if not, but it only exits)
	Hardcode hostname, port and buffersize
	Single threaded
	No thread protection on single log file (Server and client can attempt to write to file at the same time)

