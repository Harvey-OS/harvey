#include <lib9.h>

extern	int	_UNMOUNT(char*, char*);

int
unmount(char *name, char *old)
{
	return _UNMOUNT(name, old);
}
