/*
 * pANS stdio -- strerror (not really in stdio)
 */
#include <errno.h>

#include "iolib.h"
static char *_IO_errlist[] = {
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
};
#define	_IO_nerr	(sizeof _IO_errlist/sizeof _IO_errlist[0])
char *strerror(int n){
	if(n >= 0 && n < _IO_nerr)
		return _IO_errlist[n];
	if(n == EDOM)
		return "Domain error";
	else if(n == ERANGE)
		return "Range error";
	else
		return "Unknown error";
}
