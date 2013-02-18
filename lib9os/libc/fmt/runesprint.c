#include <u.h>
#include <libc.h>

int
runesprint(Rune *buf, char *fmt, ...)
{
	int n;
	va_list args;

	va_start(args, fmt);
	n = runevsnprint(buf, 256, fmt, args);
	va_end(args);
	return n;
}
