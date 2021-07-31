#include <u.h>
#include <libc.h>
#include <auth.h>
#include "authsrv.h"

void
error(char *fmt, ...)
{
	char buf[8192], *s;

	s = buf;
	s += sprint(s, "%s: ", argv0);
	s = doprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
	*s++ = '\n';
	write(2, buf, s - buf);
	exits(buf);
}
