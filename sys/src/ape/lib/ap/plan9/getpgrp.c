#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "sys9.h"
#include "lib.h"

pid_t
getpgrp(void)
{
	int n, f, pid;
	char pgrpbuf[15], fname[30];

	pid = getpid();
	sprintf(fname, "/proc/%d/noteid", pid);
	f = open(fname, 0);
	n = read(f, pgrpbuf, sizeof pgrpbuf);
	if(n < 0)
		_syserrno();
	else
		n = atoi(pgrpbuf);
	close(f);
	return n;
}
