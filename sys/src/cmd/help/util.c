#include <u.h>
#include <libc.h>
#include <libg.h>
#include <frame.h>
#include "dat.h"
#include "fns.h"

#define	PRINTSIZE	256
char	printbuf[PRINTSIZE];

long
min(long a, long b)
{
	if(a < b)
		return a;
	return b;
}

long
max(long a, long b)
{
	if(a > b)
		return a;
	return b;
}

void
error(char *s)
{
	char err[ERRLEN];

	errstr(err);
	fprint(2, "help: %s: %s\n", s, err);
	bflush();
	abort();
	exits(s);
}

int			/* special print to keep stacks small */
print(char *fmt, ...)
{
	int n;

	n = doprint(printbuf, printbuf+sizeof(printbuf), fmt, (&fmt+1)) - printbuf;
	write(1, printbuf, n);
	return n;
}

int			/* special fprint to keep stacks small */
fprint(int fd, char *fmt, ...)
{
	int n;

	n = doprint(printbuf, printbuf+sizeof(printbuf), fmt, (&fmt+1)) - printbuf;
	write(fd, printbuf, n);
	return n;
}

int
sprint(char *s, char *fmt, ...)
{
	return doprint(s, s+PRINTSIZE, fmt, (&fmt+1)) - s;
}

void			/* special print to keep stacks small */
err(char *fmt, ...)
{
	doprint(printbuf, printbuf+sizeof(printbuf), fmt, (&fmt+1));
	errs((uchar*)printbuf);
}

void *
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == 0)
		error("malloc: out of memory");
	return p;
}

void *
erealloc(void *p, ulong n)
{
	p = realloc(p, n);
	if(p == 0)
		error("realloc: out of memory");
	return p;
}

ulong
gettime(void)	/* doesn't work if second click is exactly 49.71027... days after first */
{
	static int fd = 0;
	char x[3*12+1];

	if(fd==0 && (fd=open("/dev/cputime", OREAD|OCEXEC))<0)
		error("/dev/cputime");
	x[0] = 0;
	seek(fd, 0, 0);
	read(fd, x, 3*12);
	x[3*12] = 0;
	return strtoul(x+2*12, 0, 0);
}

#ifdef asdf

void *xx[100000];
int maxxx;

void
check(void)
{
	int i;
	for(i=0; i<maxxx; i++)
		if(xx[i] && ((ulong*)xx[i])[0]!=0x12345678){
			fprint(2, "check: %lux\n", xx[i]);
			abort();
		}
}

void
free(void *b)
{
	ulong *a = b;
	int i;

	check();
	if(a == 0)
		abort();
	--a;
	for(i=0; i<maxxx; i++)
		if(xx[i] == a)
			goto found;
	fprint(2, "bad f %lux %lux\n", a, (&b)[-1]);
	abort();
    found:
	xx[i] = 0;
	if(a[0] != 0x12345678)
		abort();
	fprint(2, "f %lux %lux\n", a, (&b)[-1]);
}

void*
malloc(long n)
{
	ulong *a;
	int i;

	check();
	for(i=0; i<maxxx; i++)
		if(xx[i] == 0)
			break;
	if(i == maxxx){
		maxxx++;
		fprint(2, "new max: %d ", maxxx);
		if(maxxx==100000)
			abort();
	}
	a = sbrk(n+4);
	xx[i] = a;
	fprint(2, "m %lux %lux\n", a, (&n)[-1]);
	*a = 0x12345678;
	return a+1;
}

void*
realloc(void *oa, long n)
{
	ulong *a, *o = oa;

	fprint(2, "r %lux %lux ", o, (&oa)[-1]);
	a = malloc(n);
	if(o){
		free(o);
		memcpy(a, o, n);
	}
	return a;
}
#endif
