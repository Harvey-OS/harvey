#include <u.h>
#include <libc.h>
#include <oventi.h>

void
vtFatal(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	fprint(2, "fatal error: ");
	vfprint(2, fmt, arg);
	fprint(2, "\n");
	va_end(arg);
	exits("vtFatal");
}
