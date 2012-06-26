#include "lib.h"
#include <errno.h>
#include <unistd.h>
#include "sys9.h"

int
rmdir(const char *path)
{
	int n;

	n = 0;
	if(_REMOVE(path) < 0) {
		_syserrno();
		n = -1;
	}
	return n;
}
