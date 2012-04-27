#include	<u.h>
#include	<libc.h>

long
read(int fd, void *buf, long n)
{
	return pread(fd, buf, n, -1LL);
}
