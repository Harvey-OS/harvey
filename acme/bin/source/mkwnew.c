#include <u.h>
#include <libc.h>

void
main(int argc, char *argv[])
{
	int i, fd, pid, n;
	char wdir[256];
	int dflag;

	dflag = 0;
	ARGBEGIN{
	case 'd':
		dflag = 1;
		break;
	default:
		fprint(2, "usage: wnew [-d] [label]\n");
	}ARGEND

	pid = getpid();
	wdir[0] = '\0';
	if(!dflag)
		getwd(wdir, sizeof wdir);
	if(argc>0)
		for(i=0; i<argc; i++)
			snprint(wdir, sizeof wdir, "%s%c%s", wdir, i==0? '/' : '-', argv[i]);
	else
		snprint(wdir, sizeof wdir, "%s/-win", wdir);

	if((fd = open("/dev/wnew", ORDWR)) < 0)
		sysfatal("wnew: can't open /dev/wnew: %r");

	if(fprint(fd, "%d %s", pid, wdir+dflag) < 0)
		sysfatal("wnew: can't create window: %r");

	if(seek(fd, 0, 0) != 0)
		sysfatal("wnew: can't seek: %r");

	if((n=read(fd, wdir, sizeof wdir-1)) < 0)
		sysfatal("wnew: can't read window id: %r");
	wdir[n] = '\0';

	print("%s\n", wdir);
	exits(nil);
}
