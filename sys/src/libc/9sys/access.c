#include <u.h>
#include <libc.h>

int
access(char *name, int mode)
{
	int fd;
	Dir *db;
	static char omode[] = {
		0,
		OEXEC,
		OWRITE,
		ORDWR,
		OREAD,
		OEXEC,	/* only approximate */
		ORDWR,
		ORDWR	/* only approximate */
	};

	if(mode == AEXIST){
		db = dirstat(name);
		free(db);
		if(db != nil)
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
