#include <u.h>
#include <libc.h>

char*
esmprint(char *fmt, ...)
{
	va_list args;
	char *p;

	va_start(args, fmt);
	p = vsmprint(fmt, args);
	va_end(args);
	if (p == nil)
		sysfatal("esmprint: out of memory: %r");
	setmalloctag(p, getcallerpc(&fmt));
	return p;
}
