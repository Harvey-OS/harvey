#include <lib9.h>

extern	int	_SEGATTACH(int, char*, void*, unsigned long);

int
segattach(int attr, char *class, void *va, unsigned long len)
{
	return _SEGATTACH(attr, class, va, len);
}

