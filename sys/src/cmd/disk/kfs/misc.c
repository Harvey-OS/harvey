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

	s = buf;
	s += sprint(s, "%s %s %d: ", progname, procname, getpid());
	s = doprint(s, buf + sizeof(buf) / sizeof(*buf), fmt, &fmt + 1);
	*s++ = '\n';
	write(2, buf, s - buf);
	exits(buf);
}

#define	SIZE	4096
#define	DOTDOT	(&fmt+1)

void
cprint(char *fmt, ...)
{
	char buf[SIZE], *out;

	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	write(cmdfd, buf, (long)(out-buf));
}

int
print(char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	n = write(2, buf, (long)(out-buf));
	return n;
}

int
fprint(int f, char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	n = write(f, buf, (long)(out-buf));
	return n;
}

int
sprint(char *buf, char *fmt, ...)
{
	char *out;
	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	return out-buf;
}

void
perror(char *msg)
{
	fprint(2, "%s", msg);
}
