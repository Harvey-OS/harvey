#include "lib.h"
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include "sys9.h"

/*
**	PID cache
*/
typedef struct wdesc wdesc;
struct wdesc {
	pid_t w_pid;
	Waitmsg *w_msg;
	wdesc *w_next;
};
static wdesc *wd = 0;

static Waitmsg *
lookpid (pid_t pid) {
	wdesc **wp0 = &wd, *wp;
	Waitmsg *msg;

	if (pid == -1) {
		if (wd == 0)
			return 0;
		pid = wd->w_pid;
	}
	for (wp = wd; wp; wp = wp->w_next) {
		if (wp->w_pid == pid) {
			msg = wp->w_msg;
			*wp0 = wp->w_next;
			free (wp);
			return msg;
		}
		wp0 = &(wp->w_next);
	}
	return 0;
}

static void
addpid (Waitmsg *msg) {
	wdesc *wp = malloc (sizeof (wdesc));

	wp->w_msg = msg;
	wp->w_pid = msg->pid;
	wp->w_next = wd;
	wd = wp;
}

pid_t
wait (int *status) {
	return wait4(-1, status, 0, 0);
}

pid_t
waitpid (pid_t wpid, int *status, int options) {
	return wait4(wpid, status, options, 0);
}

pid_t
wait3 (int *status, int options, Waitmsg *waitmsg) {
	return wait4(-1, status, options, waitmsg);
}

pid_t
wait4 (pid_t wpid, int *status, int options, Waitmsg *waitmsg) {
	Waitmsg *w;

	if (options & WNOHANG) {
		char pname[128];
		int i;
		struct stat buf;

		snprintf (pname, sizeof (pname), "/proc/%d/wait", getpid());
		i = stat (pname, &buf);
		if (i >= 0 && buf.st_size == 0)
			return 0;
	}
	if (w = lookpid (wpid)) {
		waitmsg = w;
		wpid = w->pid;
		return wpid;
	}
	w = _WAIT();
	while (w) {
		if (wpid <= 0) {
			waitmsg = w;
			wpid = w->pid;
			return wpid;
		}
		if (w->pid == wpid) {
			if (status) {
				int r = 0;
				int t = 0;
				char *bp, *ep;

				if (w->msg[0]) {
					/* message is 'prog pid:string' */
					bp = w->msg;
					while (*bp) {
						if (*bp++ == ':')
							break;
					}
					if (*bp == 0)
						bp = w->msg;
					r = strtol (bp, &ep, 10);
					if (*ep == 0) {
						if (r < 0 || r >= 256)
							r = 1;
					} else {
						t = _stringsig (bp);
						if (t == 0)
							r = 1;
					}
				}
				*status = (r << 8) | t;
			}
			waitmsg = w;
			wpid = w->pid;
			return wpid;
		} else {
			addpid (w);
		}
		w = _WAIT();
	}
	if (w == 0) {
		_syserrno ();
	}
}
