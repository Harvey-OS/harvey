/*
 * pANS stdio -- strerror (not really in stdio)
 *
 * Shouldn't really call this sys_errlist or make it
 * externally visible, but too many programs in X assume it...
 */
#include <string.h>
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
	"Inappropriate ioctl",
	"No such device or address",
	"Permission denied",
	"Broken pipe",
	"Read-only file system",
	"Illegal seek",
	"No such process",
	"Cross-device link",

	/* bsd networking software */
	"Not a socket",
	"Protocol not supported",	/* EPROTONOSUPPORT, EPROTOTYPE */
/*	"Protocol wrong type for socket",	/* EPROTOTYPE */
	"Connection refused",
	"Address family not supported",
	"No buffers",
	"OP not supported",
	"Address in use",
	"Destination address required",
	"Message size",
	"Protocol option not supported",
	"Socket option not supported",
	"Protocol family not supported",	/* EPFNOSUPPORT */
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
	"Host unreachable",
	"Unknown error",		/* EGREG */

	/* These added in 1003.1b-1993 */
	"Operation canceled",
	"Operation in progress"
};
#define	_IO_nerr	(sizeof sys_errlist/sizeof sys_errlist[0])
int sys_nerr = _IO_nerr;
extern char _plan9err[];

char *
strerror(int n)
{
	if(n == EPLAN9)
		return _plan9err;
	if(n >= 0 && n < _IO_nerr)
		return sys_errlist[n];
	if(n == EDOM)
		return "Domain error";
	else if(n == ERANGE)
		return "Range error";
	else
		return "Unknown error";
}

char *
strerror_r(int n, char *buf, int len)
{
	strncpy(buf, strerror(n), len);
	buf[len-1] = 0;
	return buf;
}
