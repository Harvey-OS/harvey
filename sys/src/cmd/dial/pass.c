#include <u.h>
#include <libc.h>

int alarmed;

void
usage(void)
{
	fprint(2, "usage: %s [-q]\n", argv0);
	exits("usage");
}

void
ding(void*, char *s)
{
	if(strstr(s, "alarm")){
		alarmed = 1;
		noted(NCONT);
	} else
		noted(NDFLT);
}

void
main(int argc, char **argv)
{
	int fd, cfd;
	char buf[1];
	int quiet = 0;
	int done = 0;

	ARGBEGIN {
	case 'q':
		quiet = 1;
		break;
	} ARGEND;

	notify(ding);

	fd = open("/dev/cons", ORDWR);
	if(fd < 0)
		sysfatal("opening /dev/cons: %r");
	cfd = open("/dev/consctl", OWRITE);
	if(cfd >= 0)
		fprint(cfd, "rawon");

	while(!done){
		if(read(fd, buf, 1) <= 0)
			break;
		if(buf[0] == '\n' || buf[0] == '\r')
			done = 1;
		if(write(1, buf, 1) < 0)
			break;
		if(!quiet){
			alarmed = 0;
			alarm(500);
			if(read(0, buf, 1) <= 0 && !alarmed)
				break;
			alarm(0);
			if(buf[0] != '\r' && write(fd, buf, 1) < 0)
				break;
		}
	}
	exits(0);
}
