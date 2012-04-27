#include <u.h>
#include <libc.h>

int
confpmc(int core, int index, int mode, char *desc)
{
	int fd;
	char name[32];
	
	if(core < 0)
		core = getcoreno(nil);
	snprint(name, sizeof(name), "/dev/core%4.4d/ctr%2.2dctl", core, index);

	fd = open(name, OWRITE);
	if (fd < 0)
		return -1;

	if (fprint(fd, "reset\n") < 0) {
		close(fd);
		return -1;
	}

	if (mode&PmcOs)
		if (fprint(fd, "os\n") < 0) {
			close(fd);
			return -1;
		}
	if (mode&PmcUser)
		if (fprint(fd, "user\n") < 0) {
			close(fd);
			return -1;
		}

	if (desc != nil)
		if (fprint(fd, "set %s\n", desc) < 0) {
			close(fd);
			return -1;
		}
	if (mode&PmcEnable)
		if (fprint(fd, "enable\n") < 0) {
			close(fd);
			return -1;
		}

	close(fd);
	return 0;
}
