#include "lib9.h"
#include <sys/types.h>
#include <fcntl.h>

int
create(char *f, int mode, int perm)
{
	int m;

	m = 0;
	switch(mode & 3){
	case OREAD:
	case OEXEC:
		m = O_RDONLY;
		break;
	case OWRITE:
		m = O_WRONLY;
		break;
	case ORDWR:
		m = O_RDWR;
		break;
	}
	m |= O_CREAT|O_TRUNC;

	if(perm & DMDIR){
		if(mkdir(f, perm&0777) < 0)
			return -1;
		perm &= ~DMDIR;
		m &= 3;
	}
	return open(f, m, perm);
}
