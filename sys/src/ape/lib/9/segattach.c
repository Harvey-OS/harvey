#include <lib9.h>

extern	void	*_SEGATTACH(int, char*, void*, unsigned long);

void *
segattach(int attr, char *class, void *va, unsigned long len)
{
	return _SEGATTACH(attr, class, va, len);
}

