#include <lib9.h>

extern	int	_SEGDETACH(void *);

int
segdetach(void *addr)
{
	return _SEGDETACH(addr);
}

