#include	<u.h>
#include	<libc.h>

long
write(int fd, void *buf, long n)
{
	return pwrite(fd, buf, n, -1LL);
}
