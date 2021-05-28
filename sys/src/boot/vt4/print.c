#include "include.h"

int
print(char* fmt, ...)
{
	int n;
	va_list args;
	char buf[PRINTSIZE];

	va_start(args, fmt);
	n = vseprint(buf, buf+sizeof(buf), fmt, args) - buf;
	va_end(args);

	vuartputs(buf, n);
	return n;
}

void
_fmtlock(void)
{
}

void
_fmtunlock(void)
{
}

int
_efgfmt(Fmt*)
{
	return -1;
}

int
errfmt(Fmt*)
{
	return -1;
}
