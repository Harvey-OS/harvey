#include "lib.h"
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "sys9.h"

int
setpgid(pid_t pid, pid_t pgid)
{
	int n, f;
	char buf[50], fname[30];

	if(pid == 0)
		pid = getpid();
	if(pgid == 0)
		pgid = getpgrp();
	sprintf(fname, "/proc/%d/noteid", pid);
	f = open(fname, 1);
	if(f < 0) {
		errno = ESRCH;
		return -1;
	}
	n = sprintf(buf, "%d", pgid);
	n = write(f, buf, n);
	if(n < 0)
		_syserrno();
	else
		n = 0;
	close(f);
	return n;
}
