#include <alef.h>
#include <auth.h>
#include "authlocal.h"

int
_asrdfile(byte *file, byte *buf, int len)
{
	int n, fd;

	memset(buf, 0, len);
	fd = open(file, OREAD);
	if(fd < 0)
		return -1;
	n = read(fd, buf, len-1);
	close(fd);
	return n;
}
