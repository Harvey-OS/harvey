#include "../lib9.h"

#include "../libdraw/draw.h"
#include "../libmemdraw/memdraw.h"

#undef write
int drawdebug;
int
iprint(char *fmt,...)
{
	char buf[128];
	int n;
	va_list va;

	va_start(va, fmt);
	n = doprint(buf, buf+sizeof buf, fmt, va) - buf;
	va_end(va);
	write(1, buf, n);
	return n;
}
