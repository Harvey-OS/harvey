#include <u.h>
#include <libc.h>

/*
 * Format:
  3 r  M    4 (0000000000457def 11 00)   8192      512 /rc/lib/rcmain
 */

int
iounit(int fd)
{
	int i, cfd;
	char buf[128], *args[10];

	snprint(buf, sizeof buf, "#d/%dctl", fd);
	cfd = open(buf, OREAD);
	if(cfd < 0)
		return 0;
	i = read(cfd, buf, sizeof buf-1);
	close(cfd);
	if(i <= 0)
		return 0;
	buf[i] = '\0';
	if(tokenize(buf, args, nelem(args)) != nelem(args))
		return 0;
	return atoi(args[7]);
}
