#include <u.h>
#include <libc.h>

void
werrstr(char *fmt, ...)
{
	int psave;
	va_list arg;
	char buf[ERRLEN];

	extern int printcol;

	psave = printcol;
	va_start(arg, fmt);
	doprint(buf, buf+ERRLEN, fmt, arg);
	va_end(arg);
	errstr(buf);
	printcol = psave;
}
