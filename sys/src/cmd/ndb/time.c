#include <u.h>
#include <libc.h>

static Lock tlock;

long
time(long *tp)
{
	char b[20];
	static int f = -1;
	long t;

	lock(&tlock);
	memset(b, 0, sizeof(b));
	f = open("/dev/time", OREAD|OCEXEC);
	if(f >= 0) {
		seek(f, 0, 0);
		read(f, b, sizeof(b));
	}
	t = atol(b);
	if(tp)
		*tp = t;
	unlock(&tlock);

	return t;
}
