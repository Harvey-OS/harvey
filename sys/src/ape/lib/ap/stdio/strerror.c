/*
 * pANS stdio -- strerror (not really in stdio)
 *
 * Shouldn't really call this sys_errlist or make it
 * externally visible, but too many programs in X assume it...
 */
#include <errno.h>

#include "iolib.h"
char *sys_errlist[] = {
	"Error 0",
	"Too big",
	"Access denied",
	"Try again",
	"Bad file number",
	"In use",
	"No children",
	"Deadlock",
	"File exists",
	"Bad address",
	"File too large",
	"Interrupted system call",
	"Invalid argument",
	"I/O error",
	"Is a directory",
	"Too many open files",
	"Too many links",
	"Name too long",
	"File table overflow",
	"No such device",
	"No such file or directory",
	"Exec format error",
	"Not enough locks",
	"Not enough memory",
	"No space left on device",
	"No such system call",
	"Not a directory",
	"Directory not empty",
	"Notty",
	"Nxio",
	"Permission denied",
	"Broken pipe",
	"Read-only file system",
	"Spipe",
	"Srch",
	"Cross-device link",
	"Not a socket",
	"Protocol not supported",
	"Connection refused",
	"Address family not supported",
	"No buffers",
	"OP not supported",
	"Address in use",
	"Destination address required",
	"Message size",
	"Protocol option not supported",
	"Socket option not supported",
	"Address not available",
	"Network down",
	"Network unreachable",
	"Network reset",
	"Connection aborted",
	"Connected",
	"Not connected",
	"Shut down",
	"Too many references",
	"Timed out",
	"Host down",
	"Hout unreachable",
	"Unknown error",
	"Operation canceled",
	"Operation in progress"
};
#define	_IO_nerr	(sizeof sys_errlist/sizeof sys_errlist[0])
int sys_nerr = _IO_nerr;

char *strerror(int n){
	if(n >= 0 && n < _IO_nerr)
		return sys_errlist[n];
	if(n == EDOM)
		return "Domain error";
	else if(n == ERANGE)
		return "Range error";
	else
		return "Unknown error";
}
