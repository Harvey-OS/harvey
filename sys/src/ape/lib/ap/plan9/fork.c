#include "lib.h"
#include <errno.h>
#include <unistd.h>
#include "sys9.h"

pid_t
fork(void)
{
	int n;

	n = _RFORK(RFENVG|RFFDG|RFPROC);
	if(n < 0)
		_syserrno();
	if(n == 0) {
		_detachbuf();
		_sessleader = 0;
	}
	return n;
}
