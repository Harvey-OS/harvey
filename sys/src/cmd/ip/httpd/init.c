#include <u.h>
#include <libc.h>
#include "httpd.h"
#include "httpsrv.h"

void
usage(void)
{
	fprint(2, "usage: %s [-b inbuf] [-d domain] [-p localport]"
		" [-r remoteip] [-s uri-scheme] [-w webroot]"
		" [-L logfd0 logfd1] [-N netdir] [-R reqline]"
		" method version uri [search]\n", argv0);
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
	char *vs;

	hinit(&connect.hin, 0, Hread);
	hinit(&connect.hout, 1, Hwrite);
	hmydomain = nil;
	connect.replog = writelog;
	connect.scheme = "http";
	connect.port = "80";
	connect.private = &priv;
	priv.remotesys = nil;
	priv.remoteserv = nil;
	fmtinstall('D', hdatefmt);
	fmtinstall('H', httpfmt);
	fmtinstall('U', hurlfmt);
	netdir = "/net";
	ARGBEGIN{
	case 'b':
		hload(&connect.hin, EARGF(usage()));
		break;
	case 'd':
		hmydomain = EARGF(usage());
		break;
	case 'p':
		connect.port = EARGF(usage());
		break;
	case 'r':
		priv.remotesys = EARGF(usage());
		break;
	case 's':
		connect.scheme = EARGF(usage());
		break;
	case 'w':
		webroot = EARGF(usage());
		break;
	case 'L':
		logall[0] = strtol(EARGF(usage()), nil, 10);
		logall[1] = strtol(EARGF(usage()), nil, 10);
		break;
	case 'N':
		netdir = EARGF(usage());
		break;
	case 'R':
		snprint((char*)connect.header, sizeof(connect.header), "%s",
			EARGF(usage()));
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
