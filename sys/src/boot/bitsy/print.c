#include "u.h"
#include "lib.h"
#include "fns.h"
#include "dat.h"


#define	SIZE	1024

int
print(char *fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg;

	va_start(arg, fmt);
	out = donprint(buf, buf+SIZE, fmt, arg);
	va_end(arg);
	serialputs(buf, out-buf);
	return out-buf;
}

int
sprint(char *buf, char *fmt, ...)
{
	char *out;
	va_list arg;

	va_start(arg, fmt);
	out = donprint(buf, buf+SIZE, fmt, arg);
	va_end(arg);
	return out-buf;
}

int
snprint(char *buf, int len, char *fmt, ...)
{
	char *out;
	va_list arg;

	va_start(arg, fmt);
	out = donprint(buf, buf+len, fmt, arg);
	va_end(arg);
	return out-buf;
}

char*
seprint(char *buf, char *e, char *fmt, ...)
{
	char *out;
	va_list arg;

	va_start(arg, fmt);
	out = donprint(buf, e, fmt, arg);
	va_end(arg);
	return out;
}
