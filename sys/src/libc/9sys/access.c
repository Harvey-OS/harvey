#include <u.h>
#include <libc.h>

int
access(char *name, int mode)
{
	int fd;
	char db[DIRLEN];
	static char omode[] = {
		0,
		OEXEC,
		OWRITE,
		ORDWR,
		OREAD,
		ORDWR,
		ORDWR,
		ORDWR
	};

	if(mode == 0){
		if(stat(name, db) >= 0)
			return 0;
		return -1;
	}
	fd = open(name, omode[mode&7]);
	if(fd >= 0){
		close(fd);
		return 0;
	}
	return -1;
}
