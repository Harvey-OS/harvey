#include <lib9.h>

extern	int	_SEGFREE(void*, unsigned long);

int
segfree(void *va, unsigned long len)
{
	return _SEGFREE(va, len);
}

