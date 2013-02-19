#include <u.h>
#include <libc.h>

ulong
truerand(void)
{
	ulong x;
	static int randfd = -1;

	if(randfd < 0)
		randfd = open("/dev/random", OREAD|OCEXEC);
	if(randfd < 0)
		sysfatal("can't open /dev/random");
	if(read(randfd, &x, sizeof(x)) != sizeof(x))
		sysfatal("can't read /dev/random");
	return x;
}
