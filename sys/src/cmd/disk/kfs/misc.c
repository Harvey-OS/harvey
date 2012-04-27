#include "all.h"

extern int cmdfd;

Float
famd(Float a, int b, int c, int d)
{
	ulong x, m;

	x = (a + b) * c;
	m = x % d;
	x /= d;
	if(m >= d / 2)
		x++;
	return x;
}

ulong
fdf(Float a, int d)
{
	ulong x, m;

	m = a % d;
	x = a / d;
	if(m >= d / 2)
		x++;
	return x;
}

long
belong(char *s)
{
	uchar *x;

	x = (uchar *)s;
	return (x[0] << 24) + (x[1] << 16) + (x[2] << 8) + x[3]; 
}

void
panic(char *fmt, ...)
{
	char buf[8192], *s;
	va_list arg;


	s = buf;
	s += sprint(s, "%s %s %d: ", progname, procname, getpid());
	va_start(arg, fmt);
	s = vseprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, arg);
	va_end(arg);
	*s++ = '\n';
	write(2, buf, s - buf);
abort();
	exits(buf);
}

#define	SIZE	4096

void
cprint(char *fmt, ...)
{
	char buf[SIZE], *out;
	va_list arg;

	va_start(arg, fmt);
	out = vseprint(buf, buf+SIZE, fmt, arg);
	va_end(arg);
	write(cmdfd, buf, (long)(out-buf));
}

/*
 * print goes to fd 2 [sic] because fd 1 might be
 * otherwise preoccupied when the -s flag is given to kfs.
 */
int
print(char *fmt, ...)
{
	va_list arg;
	int n;

	va_start(arg, fmt);
	n = vfprint(2, fmt, arg);
	va_end(arg);
	return n;
}

