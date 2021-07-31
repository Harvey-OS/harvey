#include <u.h>
#include <libc.h>
#include "ip.h"

int
myetheraddr(uchar *to, char *dev)
{
	char buf[256];
	int n, fd;
	char *ptr;

	sprint(buf, "%s/1/stats", dev);
	fd = open(buf, OREAD);
	if(fd < 0)
		return -1;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;

	ptr = strstr(buf, "addr: ");
	if(!ptr)
		return -1;
	ptr += 6;

	parseether(to, ptr);
	return 0;
}
