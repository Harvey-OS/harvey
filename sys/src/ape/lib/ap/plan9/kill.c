#include "lib.h"
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

static int
note(int pid, char *msg, char *fmt)
{
	int f;
	char pname[50];

	sprintf(pname, fmt, pid);
	f = open(pname, O_WRONLY);
	if(f < 0){
		errno = ESRCH;
		return -1;
	}
	if(msg != 0 && write(f, msg, strlen(msg)) < 0){
		close(f);
		errno = EPERM;
		return -1;
	}
	close(f);
	return 0;
}

int
kill(pid_t pid, int sig)
{
	char *msg;
	int sid, r, mpid;

	if(sig == 0)
		msg = 0;
	else {
		msg = _sigstring(sig);
		if(msg == 0) {
			errno = EINVAL;
			return -1;
		} 
	}

	if(pid < 0) {
		sid = getpgrp();
		mpid = getpid();
		if(setpgid(mpid, -pid) == 0) {
			r = note(mpid, msg, "/proc/%d/notepg");
			setpgid(mpid, sid);
		} else {
			r = -1;
		}
	} else if(pid == 0)
		r = note(getpid(), msg, "/proc/%d/notepg");
	else
		r = note(pid, msg, "/proc/%d/note");
	return r;
}
