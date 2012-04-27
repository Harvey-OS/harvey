#include	<u.h>
#include	<libc.h>

int
ziowrite(int fd, Zio io[], int nio)
{
	return ziopwrite(fd, io, nio, -1LL);
}
