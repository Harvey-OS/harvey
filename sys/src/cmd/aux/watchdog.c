#include <u.h>
#include <libc.h>

static int wdog;

int
procctl(int pid)
{
	int ctlfd;
	char *ctl;

	ctl = smprint("/proc/%d/ctl", pid);
	ctlfd = open(ctl, OWRITE);
	if (ctlfd < 0)
		sysfatal("open %s: %r", ctl);
	free(ctl);
	return ctlfd;
}

void
main(int, char **)
{
	int ctl;

	wdog = open("#w/wdctl", ORDWR);
	if (wdog < 0)
		sysfatal("open #w/wdctl: %r");

	switch(rfork(RFPROC|RFNOWAIT|RFFDG)){
	case 0:
		break;
	default:
		exits(0);
	}

	ctl = procctl(getpid());
	fprint(ctl, "pri 18");
	close(ctl);

	if (fprint(wdog, "enable") < 0)
		sysfatal("write #w/wdctl enable: %r");
	for(;;){
		sleep(300);		/* allows 4.2GHz CPU, with some slop */
		seek(wdog, 0, 0);
		if (fprint(wdog, "restart") < 0)
			sysfatal("write #w/wdctl restart: %r");
	}
}
