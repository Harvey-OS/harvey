/* Example of how to use webfs */
#include <u.h>
#include <libc.h>

void
xfer(int from, int to)
{
	char buf[12*1024];
	int n;

	while((n = read(from, buf, sizeof buf)) > 0)
		if(write(to, buf, n) < 0)
			sysfatal("write failed: %r");
	if(n < 0)
		sysfatal("read failed: %r");
}

void
usage(void)
{
	fprint(2, "usage: webfsget [-b baseurl] [-m mtpt] [-p postbody] url\n");
	exits("usage");
}

void
main(int argc, char **argv)
{
	int conn, ctlfd, fd, n;
	char buf[128], *base, *mtpt, *post, *url;

	mtpt = "/mnt/web";
	post = nil;
	base = nil;
	ARGBEGIN{
	default:
		usage();
	case 'b':
		base = EARGF(usage());
		break;
	case 'm':
		mtpt = EARGF(usage());
		break;
	case 'p':
		post = EARGF(usage());
		break;
	}ARGEND;

	if (argc != 1) 
		usage();

	url = argv[0];
 
	snprint(buf, sizeof buf, "%s/clone", mtpt);
	if((ctlfd = open(buf, ORDWR)) < 0)
		sysfatal("couldn't open %s: %r", buf);
	if((n = read(ctlfd, buf, sizeof buf-1)) < 0)
		sysfatal("reading clone: %r");
	if(n == 0)
		sysfatal("short read on clone");
	buf[n] = '\0';
	conn = atoi(buf);

	if(base)
		if(fprint(ctlfd, "baseurl %s", base) < 0)
			sysfatal("baseurl ctl write: %r");

	if(fprint(ctlfd, "url %s", url) <= 0)
		sysfatal("get ctl write: %r");

	if(post){
		snprint(buf, sizeof buf, "%s/%d/postbody", mtpt, conn);
		if((fd = open(buf, OWRITE)) < 0)
			sysfatal("open %s: %r", buf);
		if(write(fd, post, strlen(post)) < 0)
			sysfatal("post write failed: %r");
		close(fd);
	}

	snprint(buf, sizeof buf, "%s/%d/body", mtpt, conn);
	if((fd = open(buf, OREAD)) < 0)
		sysfatal("open %s: %r", buf);

	xfer(fd, 1);
	exits(nil);
}
