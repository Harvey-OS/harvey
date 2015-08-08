/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "lib.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int
system(const char* s)
{
	int w, status;
	pid_t pid;
	char cmd[30], *oty;

	oty = getenv("cputype");
	if(!oty)
		return -1;
	if(!s)
		return 1; /* a command interpreter is available */
	pid = fork();
	snprintf(cmd, sizeof cmd, "/%s/bin/ape/sh", oty);
	if(pid == 0) {
		execl(cmd, "sh", "-c", s, NULL);
		_exit(1);
	}
	if(pid < 0) {
		_syserrno();
		return -1;
	}
	for(;;) {
		w = wait(&status);
		if(w == -1 || w == pid)
			break;
	}

	if(w == -1) {
		_syserrno();
		return w;
	}
	return status;
}
