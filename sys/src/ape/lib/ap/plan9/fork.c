#include "lib.h"
#include <errno.h>
#include <unistd.h>
#include "sys9.h"

#define DEBUG
#ifdef DEBUG

#include <stdio.h>
#include <string.h>

static int curpid = 0;

void
_errlog(char *fmt, ...)
{
	static int f = -1;
	static int pid = 0;
	char buf[200];
	int n;
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);

	if(curpid == 0)
		curpid = getpid();
	n = curpid;
	if(f < 0 || n != pid){
		sprintf(buf, "elog.%d", n);
		f = _CREATE(buf, 1, 0666);
		if(f<0)
			return;
		_fdinfo[n].flags = FD_ISOPEN|FD_CLOEXEC;
		_fdinfo[n].oflags = O_WRONLY;
		pid = n;
	}
	vsprintf(buf, fmt, args);
	_WRITE(f, buf, strlen(buf));
	va_end(args);
}
#endif
pid_t
fork(void)
{
	int n;

	n = _RFORK(FORKEVG|FORKFDG|FORKPCS);
	if(n < 0)
		_syserrno();
	if(n == 0){
		_newcpid(-1);
		_sigclear();
		_sessleader = 0;
	}
	if(n > 0) {
		_newcpid(n);
#ifdef DEBUG
		curpid = n;
#endif
	}
	return n;
}
