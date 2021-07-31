#include <u.h>
#include <libc.h>

ulong
umuldiv(ulong a, ulong b, ulong c)
{
	return ((double)a*(double)b) / (double)c;
}

long
muldiv(long a, long b, long c)
{
	int s;
	long v;

	s = 0;
	if(a < 0) {
		s = !s;
		a = -a;
	}
	if(b < 0) {
		s = !s;
		b = -b;
	}
	if(c < 0) {
		s = !s;
		c = -c;
	}
	v = umuldiv(a, b, c);
	if(s)
		v = -v;
	return v;
}
