#include <alef.h>

int
dirfstat(int f, Dir *dir)
{
	byte buf[DIRLEN];

	if(fstat(f, buf) == -1)
		return -1;
	convM2D(buf, dir);
	return 0;
}
