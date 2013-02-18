#include <u.h>
#include <libc.h>

static char *nsgetwd(char*, int);

char*
getwd(char *buf, int nbuf)
{
	int n, fd;

	fd = open(".", OREAD);
	if(fd < 0)
		return nil;
	n = fd2path(fd, buf, nbuf);
	close(fd);
	if(n < 0)
		return nil;
	return buf;
}
