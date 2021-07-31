#include <lib9.h>

extern	int	_MOUNT(int, char*, int, char*, char*);

int
mount(int fd, char *old, int flag, char *aname, char *authserv)
{
	return _MOUNT(fd, old, flag, aname, authserv);
}
