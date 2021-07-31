#include "lib.h"
#include <sys/stat.h>
#include <errno.h>
#include "sys9.h"

/*
 * BUG: errno mapping
 */
int
mkdir(const char *name, mode_t mode)
{
	int n;

	n = _CREATE(name, 0, 0x80000000|(mode&0777));
	if(n < 0)
		_syserrno();
	else{
		_CLOSE(n);
		n = 0;
	}
	return n;
}
