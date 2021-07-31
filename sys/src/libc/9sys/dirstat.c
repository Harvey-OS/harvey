#include <u.h>
#include <libc.h>
#include <fcall.h>

int
dirstat(char *name, Dir *dir)
{
	char buf[DIRLEN];

	if(stat(name, buf) == -1)
		return -1;
	convM2D(buf, dir);
	return 0;
}
