#include <u.h>
#include <libc.h>
#include "time.h"

char *	T = "10s";
char *	D = "5s";
char *	C = "1.5s";
char *	S = nil;
char *	s = "250ms";
char *	R = "";
int		verbose, debug, tset, dset, cset, sset;
int		nproc = 1;

uvlong v;

char *clonedev = "#R/realtime/clone";

int notefd;
int
rthandler(void*, char*)
{
	/* On any note */
	if (fprint(notefd, "remove") < 0)
		sysfatal("Could not remove task: %r");
	sysfatal("Removed task");
	return 1;
}

static int
schedpars(char *period, char *deadline, char *cost)
{
	int fd;

	if ((fd = open(clonedev, ORDWR)) < 0) 
		sysfatal("%s: %r", clonedev);

	fprint(2, "T=%s D=%s C=%s procs=%d resources=%q %sadmit\n",
		period, deadline, cost, getpid(), R, verbose?"verbose ":"");
	if (fprint(fd, "T=%s D=%s C=%s procs=%d resources=%q %sadmit",
		period, deadline, cost, getpid(), R, verbose?"verbose ":"") < 0)
			sysfatal("%s: %r", clonedev);
	notefd = fd;
	atnotify(rthandler, 1);
	return fd;
}

static void
usage(void)
{
	fprint(2, "Usage: %s [-T period] [-D deadline] [-D cost] [-S sleepinterval] [-s sleeptime] [-R resources] [-v]\n", argv0);
	exits(nil);
}

void
main(int argc, char *argv[])
{
	Time sleepinterval;
	Time sleeptime;
	Time now, when;
	int i, j, fd;
	char *e;

	quotefmtinstall();

	ARGBEGIN {
	case 'T':
		T = EARGF(usage());
		tset++;
		break;
	case 'D':
		D = EARGF(usage());
		dset++;
		break;
	case 'C':
		C = EARGF(usage());
		cset++;
		break;
	case 'S':
		S = EARGF(usage());
		break;
	case 's':
		s = EARGF(usage());
		sset++;
		break;
	case 'R':
		R = EARGF(usage());
		break;
	case 'n':
		nproc = atoi(EARGF(usage()));
		if (nproc <= 0 || nproc > 10)
			sysfatal("%d processes?", nproc);
		break;
	case 'v':
		verbose++;
		break;
	case 'd':
		debug++;
		break;
	default:
		usage();
	}
	ARGEND;

	if (S){
		if (e = parsetime(&sleepinterval, S))
			sysfatal("%s: Sleep interval: %s\n", argv0, e);
		if (e = parsetime(&sleeptime, s))
			sysfatal("%s: Sleep time: %s\n", argv0, e);
	}

	sleeptime /= 1000000;
	if (S && sleeptime == 0)
		sysfatal("%s: Sleep time too small\n", argv0);

	if (tset && !dset) D = T;

	fd = schedpars(T, D, C);

	for (j = 1; j < nproc; j++){
		switch(fork()){
		case -1:
			sysfatal("fork: %r");
		case 0:
			fprint(2, "procs+=%d\n", getpid());
			if (fprint(fd, "procs+=%d", getpid()) < 0)
			sysfatal("%s: %r", clonedev);
			for (;;){
				for(i = 0; i < 10000; i++)
					v+=1LL;
				if (S){
					now = nsec();
					if (now >= when){
						sleep((long)sleeptime);
						when = now + sleepinterval;
					}
				}
			}
		default:
			break;
		}
	}
	when = nsec() + sleepinterval;
	for (;;){
		for(i = 0; i < 100000; i++)
			v+=1LL;
		if (verbose)
			fprint(2, ".");
		if (S){
			now = nsec();
			if (now >= when){
				sleep((long)sleeptime);
				when = now + sleepinterval;
			}
		}
	}
}
