#include <u.h>
#include <libc.h>
#include "iotrack.h"
#include "dat.h"
#include "fns.h"

#define	SIZE	1024
#define	DOTDOT	(&fmt+1)

int	chatty;

void
chat(char *fmt, ...)
{
	char buf[SIZE], *out;

	if (!chatty)
		return;

	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	write(2, buf, (long)(out-buf));
}

void
panic(char *fmt, ...)
{
	char buf[SIZE]; int n;

	n = sprint(buf, "%s %d: ", argv0, getpid());
	doprint(buf+n, buf+SIZE-n, fmt, DOTDOT);
	perror(buf);
	exits("panic");
}
