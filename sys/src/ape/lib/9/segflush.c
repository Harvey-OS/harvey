#include <lib9.h>

extern	int	_SEGFLUSH(void*, unsigned long);

int
segflush(void *va, unsigned long len)
{
	return _SEGFLUSH(va, len);
}
