#include <u.h>
#include <libc.h>

long
time(long *tp)
{
	char b[20];
	static int f = -1;
	long t;

	memset(b, 0, sizeof(b));
	if(f < 0)
		f = open("/dev/time", OREAD|OCEXEC);
	if(f >= 0) {
		seek(f, 0, 0);
		read(f, b, sizeof(b));
	}
	t = atol(b);
	if(tp)
		*tp = t;
	return t;
}
