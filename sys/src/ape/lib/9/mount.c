#include <lib9.h>

extern	int	_MOUNT(int, int, char*, int, char*);

int
mount(int fd, int afd, char *old, int flag, char *aname)
{
	return _MOUNT(fd, afd, old, flag, aname);
}
