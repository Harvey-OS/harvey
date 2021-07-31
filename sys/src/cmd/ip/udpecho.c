#include <u.h>
#include <libc.h>
#include <ip.h>

void
usage(void)
{
	fprint(2, "usage: %s [-x netmtpt]\n", argv0);
	exits("usage");
}

void
main(int argc, char **argv)
{
	int fd, cfd, n;
	char data[128];
	char devdir[40];
	char buf[4096];
	char net[32];
	char *p;

	setnetmtpt(net, sizeof(net), nil);

	ARGBEGIN{
	case 'x':
		p = ARGF();
		if(p == nil)
			usage();
		setnetmtpt(net, sizeof(net), p);
		break;
	}ARGEND;

	sprint(data, "%s/udp!*!echo", net);
	cfd = announce(data, devdir);
	if(cfd < 0)
		sysfatal("can't announce: %r");
	if(fprint(cfd, "headers") < 0)
		sysfatal("can't set header mode: %r");

	sprint(data, "%s/data", devdir);

	fd = open(data, ORDWR);
	if(fd < 0)
		sysfatal("open udp data");
	for(;;){
		n = read(fd, buf, sizeof(buf));
		if(n < 0)
			sysfatal("error reading: %r");
		write(fd, buf, n);
	}
}
