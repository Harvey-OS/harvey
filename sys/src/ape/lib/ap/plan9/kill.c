#include "lib.h"
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

/*
 * BUG: negative pids (not -1) mean find the pgrp with
 * that pgrpid and kill it.
 */
#include <stdio.h>
int
kill(pid_t pid, int sig)
{
	char *msg, pname[50];
	int f;

	msg = _sigstring(sig);
	if(pid < 0) {
		errno = EINVAL;
		return -1;
	} else if(pid == 0){
		pid = getpid();
		sprintf(pname, "/proc/%d/notepg", pid);
	} else
		sprintf(pname, "/proc/%d/note", pid);
	f = open(pname, O_WRONLY);
	if(f == -1 || write(f, msg, strlen(msg)) == -1){
		close(f);
		errno = EINVAL;
		return -1;
	}
	close(f);
	return 0;
}
