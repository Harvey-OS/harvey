#include <u.h>
#include <libc.h>
#include "time.h"

int		T = 201;
int		D = 179;
int		C = 11;
int		verbose, debug, tset, dset, cset, sset;
int		nproc = 1;
int		umpteenth;
char *resources = "a 4ms";

uvlong v;
uvlong onems;

char *clonedev = "#R/realtime/clone";
char missstr[] = "sys: deadline miss: runtime";

int notefd;
int
rthandler(void*, char *s)
{
	umpteenth++;
	if (strncmp(missstr, s, strlen(missstr)) == 0)
		return 1;

	fprint(2, "note received: %s\n", s);
	/* On any other note */
	if (fprint(notefd, "remove") < 0)
		sysfatal("Could not remove task: %r");
	sysfatal("Removed task");
	return 1;
}

static void
usage(void)
{
	fprint(2, "Usage: %s [-T period] [-D deadline] [-D cost] [-v] [resource...]\n", argv0);
	exits(nil);
}

void
main(int argc, char *argv[])
{
	int fd;

	quotefmtinstall();

	ARGBEGIN {
	case 'T':
		T = strtoul(EARGF(usage()), nil, 0);
		tset++;
		break;
	case 'D':
		D = strtoul(EARGF(usage()), nil, 0);
		dset++;
		break;
	case 'C':
		C = strtoul(EARGF(usage()), nil, 0);
		cset++;
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

	if ((fd = open(clonedev, ORDWR)) < 0) 
		sysfatal("%s: %r", clonedev);
	fprint(2, "T=%dms D=%dms C=%dms procs=self  admit\n", 100, 100, 10);
	notefd = fd;
	atnotify(rthandler, 1);
	if (fprint(fd, "T=%dms D=%dms C=%dms procs=self admit", 100, 100, 10) < 0)
			sysfatal("%s: %r", clonedev);
	while(umpteenth ==0)
		v+=1LL;
	if (fprint(fd, "expel") < 0)
		sysfatal("%s: expel: %r", clonedev);
	onems = v/10;
	if (verbose)
		print("One millisecond is %llud cycles\n", v);

	if (tset && !dset) D = T;

	fprint(2, "T=%dms D=%dms C=%dms procs=%d resources=%q %sadmit\n",
		T, D, C, getpid(), resources, verbose?"verbose ":"");
	notefd = fd;
	atnotify(rthandler, 1);
	if (fprint(fd, "T=%dms D=%dms C=%dms procs=%d resources=%q %sadmit",
		T, D, C, getpid(), resources, verbose?"verbose ":"") < 0)
			sysfatal("%s: %r", clonedev);

	for (;;){
		if (fprint(fd, "acquire=a") < 0)
			sysfatal("%s: acquire=a: %r", clonedev);
		v = 3*onems;
		while(v != 0LL)
			v -= 1LL;
		if (fprint(fd, "release=a") < 0)
			sysfatal("%s: release=a: %r", clonedev);
		v = 5*onems;
		while(v != 0LL)
			v -= 1LL;
		if (fprint(fd, "yield") < 0)
			sysfatal("%s: yield a: %r", clonedev);
	}
}
