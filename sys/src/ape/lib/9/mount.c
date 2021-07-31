#include <lib9.h>

extern	int	_MOUNT(int, char*, int, char*);

int
mount(int fd, char *old, int flag, char *aname)
{
	return _MOUNT(fd, old, flag, aname);
}
