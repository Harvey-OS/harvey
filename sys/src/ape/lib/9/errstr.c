#include <lib9.h>

extern	int	_ERRSTR(char*, unsigned int);

int
errstr(char *err, unsigned int nerr)
{
	return _ERRSTR(err, nerr);
}
