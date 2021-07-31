#include <alef.h>
#include <bio.h>
#include <net.h>

int
myetheraddr(byte *to, byte *dev)
{
	int n, fd;
	byte buf[256], *ptr;

	/* Make one exist */
	sprint(buf, "/net/%s/clone", dev);
	fd = open(buf, ORDWR);
	if(fd >= 0)
		close(fd);

	sprint(buf, "/net/%s/0/stats", dev);
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
