#include "stdinc.h"
#include "dat.h"
#include "fns.h"

char *host;

void
usage(void)
{
	fprint(2, "usage: sync [-h host]\n");
	exits("usage");
}

int
main(int argc, char *argv[])
{
	VtSession *z;

	ARGBEGIN{
	case 'h':
		host = EARGF(usage());
		if(host == nil)
			usage();
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 0)
		usage();

	vtAttach();

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	z = vtDial(host, 0);
	if(z == nil)
		vtFatal("could not connect to server: %R");

	if(!vtConnect(z, 0))
		sysfatal("vtConnect: %r");

	if(!vtSync(z))
		sysfatal("vtSync: %r");

	vtClose(z);
	vtDetach();
	exits(0);
	return 0;	/* shut up compiler */
}
