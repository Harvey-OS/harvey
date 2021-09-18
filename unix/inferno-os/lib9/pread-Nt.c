#include "lib9.h"

long
pread(int fd, void *buf, long n, vlong offset)
{
	if(seek(fd, offset, 0) < 0)
		return -1;
	return read(fd, buf, n);
}
