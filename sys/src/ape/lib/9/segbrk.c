#include <lib9.h>

extern int	_SEGBRK(void*, void*);

int
segbrk(void *saddr, void *addr)
{
	return _SEGBRK(saddr, addr);
}
