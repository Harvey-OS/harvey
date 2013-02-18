#include <u.h>
#include <libc.h>

Rune*
runeseprint(Rune *buf, Rune *e, char *fmt, ...)
{
	Rune *p;
	va_list args;

	va_start(args, fmt);
	p = runevseprint(buf, e, fmt, args);
	va_end(args);
	return p;
}
