#include <u.h>
#include <libc.h>
#include <bio.h>
#include "httpd.h"

void
usage(void)
{
	fprint(2, "usage: httpd [-b inbuf] [-d domain] [-r remoteip] [-w webroot] [-N netdir] method version uri [search]\n");
	exits("usage");
}

static	Connect	connect;

Connect*
init(int argc, char **argv)
{
	char *s, *vs;

	hinit(&connect.hin, 0, Hread);
	hinit(&connect.hout, 1, Hwrite);
	mydomain = nil;
	connect.remotesys = nil;
	fmtinstall('D', dateconv);
	fmtinstall('H', httpconv);
	fmtinstall('U', urlconv);
	strcpy(netdir, "/net");
	ARGBEGIN{
	case 'b':
		s = ARGF();
		if(s != nil)
			hload(&connect.hin, s);
		break;
	case 'd':
		mydomain = ARGF();
		break;
	case 'r':
		connect.remotesys = ARGF();
		break;
	case 'w':
		webroot = ARGF();
		break;
	case 'N':
		s = ARGF();
		if(s != nil)
			strcpy(netdir, s);
		break;
	default:
		usage();
	}ARGEND

	if(connect.remotesys == nil)
		connect.remotesys = "unknown";
	if(mydomain == nil)
		mydomain = "unknown";
	if(webroot == nil)
		webroot = "/usr/web";

	/*
	 * open all files we might need before castrating namespace
	 */
	time(nil);
	syslog(0, HTTPLOG, nil);

	if(argc != 4 && argc != 3)
		usage();

	connect.req.meth = argv[0];

	vs = argv[1];
	connect.req.vermaj = 0;
	connect.req.vermin = 9;
	if(strncmp(vs, "HTTP/", 5) == 0){
		vs += 5;
		connect.req.vermaj = strtoul(vs, &vs, 10);
		if(*vs == '.')
			vs++;
		connect.req.vermin = strtoul(vs, &vs, 10);
	}

	connect.req.uri = argv[2];
	connect.req.search = argv[3];
	connect.head.closeit = 1;
	connect.hpos = connect.header;
	connect.hstop = connect.header;
	return &connect;
}
