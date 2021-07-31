#include <u.h>
#include <libc.h>
#include <fcall.h>

int
dirwstat(char *name, Dir *dir)
{
	char buf[DIRLEN];

	convD2M(dir, buf);
	return wstat(name, buf);
}
