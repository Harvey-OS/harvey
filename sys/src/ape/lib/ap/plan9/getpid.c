#include "lib.h"
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "sys9.h"

pid_t
getpid(void)
{
	int n, f;
	char pidbuf[15];

	f = _OPEN("#c/pid", 0);
	n = _READ(f, pidbuf, sizeof pidbuf);
	if(n < 0)
		_syserrno();
	else
		n = atoi(pidbuf);
	_CLOSE(f);
	return n;
}
