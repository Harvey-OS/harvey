#include "lib9.h"

void
werrstr(char *fmt, ...)
{
	char buf[ERRLEN];
	va_list va;

	va_start(va, fmt);
	doprint(buf, buf+ERRLEN, fmt, va);
	va_end(va);
	errstr(buf);
}
