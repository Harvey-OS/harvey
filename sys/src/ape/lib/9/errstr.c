#include <lib9.h>

extern	int	_ERRSTR(char*);

int
errstr(char *err)
{
	return _ERRSTR(err);
}
