#include <u.h>
#include <libc.h>


void
sysfatal(char *fmt, ...)
{
	char buf[1024];
	va_list arg;

	va_start(arg, fmt);
	doprint(buf, buf+sizeof(buf), fmt, arg);
	va_end(arg);
	if(argv0)
		fprint(2, "%s: %s\n", argv0, buf);
	else
		fprint(2, "%s\n", buf);
	exits(buf);
}
