#include <u.h>
#include <libc.h>
#include "ssh2.h"

void
freeptr(void **vpp)
{
	char **cpp;

	cpp = vpp;
	free(*cpp);
	*cpp = nil;
}

int
readfile(char *file, char *buf, int size)
{
	int n, fd;

	fd = open(file, OREAD);
	if (fd < 0)
		return -1;
	n = readn(fd, buf, size - 1);
	if (n < 0)
		buf[0] = '\0';
	else
		buf[n] = '\0';
	close(fd);
	return n;
}
