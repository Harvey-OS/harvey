#include <u.h>
#include <libc.h>
#include <fcall.h>
#include <thread.h>
#include <9p.h>
#include "dat.h"
#include "fns.h"

#include "errstr.h"

int	errno;
int rdonly;
char	*srvfile;
char	*deffile;

extern void iobuf_init(void);
extern Srv ext2srv;

void
usage(void)
{
	fprint(2, "usage: %s [-v] [-s] [-r] [-p passwd] [-g group] [-f devicefile] [srvname]\n", argv0);
	exits("usage");
}

/*void handler(void *v, char *sig)
{
	USED(v,sig);
	syncbuf();
	noted(NDFLT);
}*/

void
main(int argc, char **argv)
{
	int stdio;

	stdio = 0;
	ARGBEGIN{
	case 'D':
		++chatty9p;
		break;
	case 'v':
		++chatty;
		break;
	case 'f':
		deffile = ARGF();
		break;
	case 'g':
		gidfile(ARGF());
		break;
	case 'p':
		uidfile(ARGF());
		break;
	case 's':
		stdio = 1;
		break;
	case 'r':
		rdonly = 1;
		break;
	default:
		usage();
	}ARGEND

	if(argc == 0)
		srvfile = "ext2";
	else if(argc == 1)
		srvfile = argv[0];
	else
		usage();
	
	iobuf_init();
	/*notify(handler);*/

	if(!chatty){
		close(2);
		open("#c/cons", OWRITE);
	}
	if(stdio){
		srv(&ext2srv);
	}else{
		chat("%s %d: serving %s\n", argv0, getpid(), srvfile);
		postmountsrv(&ext2srv, srvfile, 0, 0);
	}
	exits(0);
}

char *
xerrstr(int e)
{
	if (e < 0 || e >= sizeof errmsg/sizeof errmsg[0])
		return "no such error";
	else
		return errmsg[e];
}
