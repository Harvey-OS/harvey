#include <u.h>
#include <libc.h>
#include "httpd.h"
#include "httpsrv.h"

void
usage(void)
{
	fprint(2, "usage: httpd [-b inbuf] [-d domain] [-r remoteip] [-w webroot] [-N netdir] [-R reqline] [-L logfd0 logfd1] method version uri [search]\n");
	exits("usage");
}

char	*netdir;
char	*webroot;
char	*HTTPLOG = "httpd/log";

static	HConnect	connect;
static	HSPriv		priv;

HConnect*
init(int argc, char **argv)
{
	char *s, *vs;

	hinit(&connect.hin, 0, Hread);
	hinit(&connect.hout, 1, Hwrite);
	hmydomain = nil;
	connect.replog = writelog;
	connect.private = &priv;
	priv.remotesys = nil;
	priv.remoteserv = nil;
	fmtinstall('D', hdatefmt);
	fmtinstall('H', httpfmt);
	fmtinstall('U', hurlfmt);
	netdir = "/net";
	ARGBEGIN{
	case 'b':
		s = ARGF();
		if(s != nil)
			hload(&connect.hin, s);
		break;
	case 'd':
		hmydomain = ARGF();
		break;
	case 'r':
		priv.remotesys = ARGF();
		break;
	case 'w':
		webroot = ARGF();
		break;
	case 'N':
		netdir = ARGF();
		break;
	case 'L':
		s = ARGF();
		if(s == nil)
			usage();
		logall[0] = strtol(s, nil, 10);
		s = ARGF();
		if(s == nil)
			usage();
		logall[1] = strtol(s, nil, 10);
		break;
	case 'R':
		s = ARGF();
		if(s == nil)
			usage();
		snprint((char*)connect.header, sizeof(connect.header), "%s", s);
		break;
	default:
		usage();
	}ARGEND

	if(priv.remotesys == nil)
		priv.remotesys = "unknown";
	if(priv.remoteserv == nil)
		priv.remoteserv = "unknown";
	if(hmydomain == nil)
		hmydomain = "unknown";
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
	connect.hpos = (uchar*)strchr((char*)connect.header, '\0');
	connect.hstop = connect.hpos;
	connect.reqtime = time(nil);	/* not quite right, but close enough */
	return &connect;
}
