#include <u.h>
#include <libc.h>

void
xfer(int from, int to)
{
	char buf[8*1024];
	int n;

	while((n = read(from, buf, sizeof buf)) > 0)
		if(write(to, buf, n) < 0)
			break;
}


void
main(int argc, char **argv)
{
	int fd;

	ARGBEGIN{
	}ARGEND;

	if(argc < 1){
		fprint(2, "usage: %s dialstring\n", argv0);
		exits("usage");
	}
	fd = dial(argv[0], 0, 0, 0);
	if(fd < 0){
		fprint(2, "%s: dialing %s: %r\n", argv0, argv[0]);
		exits("dial");
	}
	rfork(RFNOTEG);
	switch(fork()){
	case -1:
		fprint(2, "%s: fork: %r\n", argv0);
		exits("dial");
	case 0:
		xfer(0, fd);
		break;
	default:
		xfer(fd, 1);
		break;
	}
	postnote(PNGROUP, getpid(), "die yankee pig dog");
	exits(0);
}
