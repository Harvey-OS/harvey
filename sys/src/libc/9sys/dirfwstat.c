#include <u.h>
#include <libc.h>
#include <auth.h>
#include <fcall.h>

int
dirfwstat(int f, Dir *dir)
{
	char buf[DIRLEN];

	convD2M(dir, buf);
	return fwstat(f, buf);
}
