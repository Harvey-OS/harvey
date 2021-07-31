#include <alef.h>

int
dirfwstat(int f, Dir *dir)
{
	byte buf[DIRLEN];

	convD2M(dir, buf);
	if(fwstat(f, buf) == -1)
		return -1;
	return 0;
}
