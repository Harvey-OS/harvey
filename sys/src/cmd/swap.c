#include <u.h>
#include <libc.h>

void error(char *);

void
main(int argc, char **argv)
{
	Dir *d;
	int swapfd, cswfd;
	char buf[128], *p;
	int i, j;

	if(argc != 2) {
		print("Usage: swap path\n");
		exits("swap: failed");
	}

	d = dirstat(argv[1]);
	if(d == nil){
		print("swap: can't stat %s: %r\n", argv[1]);
		exits("swap: failed");
	}
	if(d->type != 'M'){		/* kernel device */
		swapfd = open(argv[1], ORDWR);
		p = argv[1];
	}
	else {
		p = getenv("sysname");
		if(p == 0)
			p = "swap";
		sprint(buf, "%s/%sXXXXXXX", argv[1], p);
		p = mktemp(buf);
		swapfd = create(p, ORDWR|ORCLOSE, 0600);
	}

	if(swapfd < 0 || (p[0] == '/' && p[1] == '\0')) {
		print("swap: failed to make a file: %r\n");
		exits("swap: failed");
	}

	i = create("/env/swap", OWRITE, 0666);
	if(i < 0) 
		error("open /env/swap");

	if(write(i, p, strlen(p)) != strlen(p))
		error("sysname");
	close(i);
	print("swap: %s\n", p);

	cswfd = open("/dev/swap", OWRITE);
	if(cswfd < 0)
		error("open: /dev/swap");

	j = sprint(buf, "%d", swapfd);
	if(write(cswfd, buf, j) != j)
		error("write: /dev/swap");
	exits(0);
}

void
error(char *s)
{
	perror(s);
	exits(s);
}
