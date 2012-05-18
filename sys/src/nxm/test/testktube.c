#include <u.h>
#include <libc.h>
#include <tube.h>

/*
 * NIX optimistic tubes shared with the kernel testing.
 !	6c -FTVw testktube.c && 6l -o 6.testktube testktube.6
 */

void
usage(void)
{
	fprint(2, "usage: %s [-sr] [n]\n", argv0);
	exits("usage");
}


void
main(int argc, char *argv[])
{
	int issend;
	int fd, n;
	void *addr;
	Tube *t;

	issend = 0;
	ARGBEGIN{
	case 's':
		issend = 1;
		break;
	case 'r':
		issend = 0;
		break;
	default:
		usage();
	}ARGEND;
	n = 10;
	if(argc == 1)
		n = atoi(argv[0]);
	else if(argc != 0)
		usage();


	t = newtube(sizeof(void*), 5);

	fd = open("#=/ctl", ORDWR);
	if(fd < 0)
		sysfatal("devtube: %r");
	if(issend)
		fprint(fd, "send %#p", t);
	else
		fprint(fd, "recv %#p", t);
	if(0 && issend == 0)
		sleep(15*1000);	/* let the kernel block, to check that */
	while(n-- > 0){
		print("user: %s...\n", issend?"recv":"send");
		if(issend)
			trecv(t, &addr);
		else
			tsend(t, &addr);
		print("user: %s done (-> %#p)\n", issend?"recv":"send", addr);
	}
	exits(nil);
}
