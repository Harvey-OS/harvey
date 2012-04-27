#include <u.h>
#include <libc.h>
#include <bio.h>
#include "authcmdlib.h"

void
error(char *fmt, ...)
{
	char buf[8192], *s;
	va_list arg;

	s = buf;
	s += snprint(s, sizeof buf, "%s: ", argv0);
	va_start(arg, fmt);
	s = vseprint(s, buf + sizeof(buf), fmt, arg);
	va_end(arg);
	*s++ = '\n';
	write(2, buf, s - buf);
	exits(buf);
}
