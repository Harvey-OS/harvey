#include "lib.h"
#include <unistd.h>
#include "sys9.h"

int
chdir(const char *f)
{
	int n;

	n = _CHDIR(f);
	if(n < 0)
		_syserrno();
	return n;
}
