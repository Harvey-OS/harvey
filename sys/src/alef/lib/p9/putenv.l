#include <alef.h>

int
putenv(byte *name, byte *val)
{
	int f, s;
	byte ename[NAMELEN+6];

	sprint(ename, "/env/%s", name);
	f = create(ename, OWRITE, 0664);
	if(f < 0)
		return -1;
	s = strlen(val);
	if(write(f, val, s) != s) {
		close(f);
		return -1;
	}
	close(f);
	return 0;
}
