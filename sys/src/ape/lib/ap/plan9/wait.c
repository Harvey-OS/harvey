#include "lib.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "sys9.h"
#include "dir.h"

/*
 * status not yet collected for processes that have exited
 */
typedef struct Waited Waited;
struct Waited {
	Waitmsg*	msg;
	Waited*	next;
};
static Waited *wd;

static Waitmsg *
lookpid(int pid)
{
	Waited **wl, *w;
	Waitmsg *msg;

	for(wl = &wd; (w = *wl) != nil; wl = &w->next)
		if(pid <= 0 || w->msg->pid == pid){
			msg = w->msg;
			*wl = w->next;
			free(w);
			return msg;
		}
	return 0;
}

static void
addpid(Waitmsg *msg)
{
	Waited *w;

	w = malloc(sizeof(*w));
	if(w == nil){
		/* lost it; what can we do? */
		free(msg);
		return;
	}
	w->msg = msg;
	w->next = wd;
	wd = w;
}

static int
waitstatus(Waitmsg *w)
{
	int r, t;
	char *bp, *ep;

	r = 0;
	t = 0;
	if(w->msg[0]){
		/* message is 'prog pid:string' */
		bp = w->msg;
		while(*bp){
			if(*bp++ == ':')
				break;
		}
		if(*bp == 0)
			bp = w->msg;
		r = strtol(bp, &ep, 10);
		if(*ep == 0){
			if(r < 0 || r >= 256)
				r = 1;
		}else{
			t = _stringsig(bp);
			if(t == 0)
				r = 1;
		}
	}
	return (r<<8) | t;
}

static void
waitresource(struct rusage *ru, Waitmsg *w)
{
	memset(ru, 0, sizeof(*ru));
	ru->ru_utime.tv_sec = w->time[0]/1000;
	ru->ru_utime.tv_usec = (w->time[0]%1000)*1000;
	ru->ru_stime.tv_sec = w->time[1]/1000;
	ru->ru_stime.tv_usec = (w->time[1]%1000)*1000;
}

pid_t
wait(int *status)
{
	return wait4(-1, status, 0, nil);
}

pid_t
waitpid(pid_t wpid, int *status, int options)
{
	return wait4(wpid, status, options, nil);
}

pid_t
wait3(int *status, int options, struct rusage *res)
{
	return wait4(-1, status, options, res);
}

pid_t
wait4(pid_t wpid, int *status, int options, struct rusage *res)
{
	char pname[50];
	Dir *d;
	Waitmsg *w;

	w = lookpid(wpid);
	if(w == nil){
		if(options & WNOHANG){
			snprintf(pname, sizeof(pname), "/proc/%d/wait", getpid());
			d = _dirstat(pname);
			if(d != nil && d->length == 0){
				free(d);
				return 0;
			}
			free(d);
		}
		for(;;){
			w = _WAIT();
			if(w == nil){
				_syserrno();
				return -1;
			}
			if(wpid <= 0 || w->pid == wpid)
				break;
			addpid(w);
		}
	}
	if(res != nil)
		waitresource(res, w);
	if(status != nil)
		*status = waitstatus(w);
	wpid = w->pid;
	free(w);
	return wpid;
}
