#include <u.h>
#include <libc.h>
#include <bio.h>
#include "awk.h"

#define MAXFORKS	20
#define NSYSFILE	3
#define	tst(a,b)	(mode == OREAD? (b) : (a))
#define	RDR	0
#define	WTR	1

struct a_fork {
	short	done;
	short	fd;
	int	pid;
	char status[128];
};
static struct a_fork the_fork[MAXFORKS];

Biobuf*
popen(char *cmd, int mode)
{
	int p[2];
	int myside, hisside, pid;
	int i, ind;

	for (ind = 0; ind < MAXFORKS; ind++)
		if (the_fork[ind].pid == 0)
			break;
	if (ind == MAXFORKS)
		return nil;
	if(pipe(p) < 0)
		return nil;
	myside = tst(p[WTR], p[RDR]);
	hisside = tst(p[RDR], p[WTR]);
	switch (pid = fork()) {
	case -1:
		return nil;
	case 0:
		/* myside and hisside reverse roles in child */
		close(myside);
		dup(hisside, tst(0, 1));
		for (i=NSYSFILE; i<FOPEN_MAX; i++)
			close(i);
		execl("/bin/rc", "rc", "-c", cmd, nil);
		exits("exec failed");
	default:
		the_fork[ind].pid = pid;
		the_fork[ind].fd = myside;
		the_fork[ind].done = 0;
		close(hisside);
		return(Bfdopen(myside, mode));
	}
}

int
pclose(Biobuf *ptr)
{
	int f, r, ind;
	Waitmsg *status;

	f = Bfildes(ptr);
	Bterm(ptr);
	for (ind = 0; ind < MAXFORKS; ind++)
		if (the_fork[ind].fd == f && the_fork[ind].pid != 0)
			break;
	if (ind == MAXFORKS)
		return -1;
	if (!the_fork[ind].done) {
		do {
			if((status = wait()) == nil)
				r = -1;
			else
				r = status->pid;
			for (f = 0; f < MAXFORKS; f++) {
				if (r == the_fork[f].pid) {
					the_fork[f].done = 1;
					strecpy(the_fork[f].status, the_fork[f].status+512, status->msg);
					break;
				}
			}
			free(status);
		} while(r != the_fork[ind].pid && r != -1);
		if(r == -1)
			strcpy(the_fork[ind].status, "No loved ones to wait for");
	}
	the_fork[ind].pid = 0;
	if(the_fork[ind].status[0] != '\0')
		return 1;
	return 0;
}
