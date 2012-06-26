#include <lib9.h>

extern	int	_BIND(char*, char*, int);

int
bind(char *name, char *old, int flag)
{
	return _BIND(name, old, flag);
}
