#include <u.h>
#include <libc.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

#define	SIZE	1024
int	chatty;
extern int	doabort;

void
chat(char *fmt, ...)
{
	va_list arg;

	if (!chatty)
		return;
	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);
}

void
panic(char *fmt, ...)
{
	va_list arg;

	fprint(2, "%s %d: panic ", argv0, getpid());
	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);
	fprint(2, ": %r\n");
	if(doabort)
		abort();
	exits("panic");
}
