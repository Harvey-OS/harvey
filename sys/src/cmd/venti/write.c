#include "stdinc.h"
#include "dat.h"
#include "fns.h"

char *host;

void
usage(void)
{
	fprint(2, "usage: write [-z] [-h host] [-t type] <datablock\n");
	exits("usage");
}

int
main(int argc, char *argv[])
{
	uchar *p, score[VtScoreSize];
	int type, n, dotrunc;
	VtSession *z;

	dotrunc = 0;
	type = VtDataType;
	ARGBEGIN{
	case 'z':
		dotrunc = 1;
		break;
	case 'h':
		host = EARGF(usage());
		break;
	case 't':
		type = atoi(EARGF(usage()));
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

	p = vtMemAllocZ(VtMaxLumpSize+1);
	n = readn(0, p, VtMaxLumpSize+1);
	if(n > VtMaxLumpSize)
		vtFatal("data too big");
	z = vtDial(host, 0);
	if(z == nil)
		vtFatal("could not connect to server: %R");
	if(!vtConnect(z, 0))
		vtFatal("vtConnect: %r");
	if(dotrunc)
		n = vtZeroTruncate(type, p, n);
	if(!vtWrite(z, score, type, p, n))
		vtFatal("vtWrite: %R");
	vtClose(z);
	print("%V\n", score);
	vtDetach();
	exits(0);
	return 0;	/* shut up compiler */
}
