#include	<u.h>
#include	<libc.h>

char*
sysname(void)
{
	int f, n;
	static char b[128];

	if(b[0])
		return b;

	f = open("#c/sysname", 0);
	if(f >= 0) {
		n = read(f, b, sizeof(b)-1);
		if(n > 0)
			b[n] = 0;
		close(f);
	}
	return b;
}
