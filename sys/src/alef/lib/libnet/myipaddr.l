#include <alef.h>
#include <bio.h>
#include <net.h>

int
myipaddr(byte *to, byte *dev)
{
	int n, fd, clone;
	byte buf[256], *ptr;

	/* Opening clone ensures the 0 connection exists */
	sprint(buf, "%s/clone", dev);
	clone = open(buf, OREAD);
	if(clone < 0)
		return -1;

	sprint(buf, "%s/0/local", dev);
	fd = open(buf, OREAD);
	close(clone);
	if(fd < 0)
		return -1;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;

	ptr = strchr(buf, ' ');
	if(ptr)
		*ptr = 0;

	parseip(to, buf);
	return 0;
}
