#include <lib9.h>

extern	int	_RFORK(int);

int
rfork(int flags)
{
	return _RFORK(flags);
}
