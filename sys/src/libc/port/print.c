#include <u.h>
#include <libc.h>

#define	SIZE	4096
#define	DOTDOT	(&fmt+1)
extern	int	printcol;
static	int	errcount = { 0 };
static	char	errmsg[] = "print errors";

int
print(char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	n = write(1, buf, (long)(out-buf));
	if(n < 0)
		if(++errcount >= 10)
			exits(errmsg);
	return n;
}

int
fprint(int f, char *fmt, ...)
{
	char buf[SIZE], *out;
	int n;

	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	n = write(f, buf, (long)(out-buf));
	if(n < 0)
		if(++errcount >= 10)
			exits(errmsg);
	return n;
}

int
sprint(char *buf, char *fmt, ...)
{
	char *out;
	int scol;

	scol = printcol;
	out = doprint(buf, buf+SIZE, fmt, DOTDOT);
	printcol = scol;
	return out-buf;
}

int
snprint(char *buf, int len, char *fmt, ...)
{
	char *out;
	int scol;

	scol = printcol;
	out = doprint(buf, buf+len, fmt, DOTDOT);
	printcol = scol;
	return out-buf;
}
