#include	<u.h>
#include	<libc.h>

int
zioread(int fd, Zio io[], int nio, usize count)
{
	return ziopread(fd, io, nio, count, -1LL);
}
