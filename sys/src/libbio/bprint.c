#include	<u.h>
#include	<libc.h>
#include	<bio.h>

int
Bprint(Biobufhdr *bp, char *fmt, ...)
{
	va_list arg;
	int n;

	va_start(arg, fmt);
	n = Bvprint(bp, fmt, arg);
	va_end(arg);
	return n;
}
