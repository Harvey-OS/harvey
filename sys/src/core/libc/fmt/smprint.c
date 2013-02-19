#include <u.h>
#include <libc.h>

char*
smprint(char *fmt, ...)
{
	va_list args;
	char *p;

	va_start(args, fmt);
	p = vsmprint(fmt, args);
	va_end(args);
	setmalloctag(p, getcallerpc(&fmt));
	return p;
}
