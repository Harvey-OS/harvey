/* posix */
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

int
gethostname(char *name, int namelen)
{
	int n, fd;
	char buf[128];

	fd = open("/dev/sysname", O_RDONLY);
	if(fd < 0)
		return -1;
	n = read(fd, buf, sizeof(buf)-1);
	close(fd);
	if(n <= 0)
		return -1;
	buf[n] = 0;
	strncpy(name, buf, namelen);
	name[namelen-1] = 0;
	return 0;
}
