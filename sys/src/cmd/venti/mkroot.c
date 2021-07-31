#include "stdinc.h"
#include "dat.h"
#include "fns.h"

char *host;

void
usage(void)
{
	fprint(2, "usage: mkroot [-h host] name type score blocksize prev\n");
	exits("usage");
}

int
main(int argc, char *argv[])
{
	uchar score[VtScoreSize];
	uchar buf[VtRootSize];
	VtSession *z;
	VtRoot root;

	ARGBEGIN{
	case 'h':
		host = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 5)
		usage();

	vtAttach();
	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	root.version = VtRootVersion;
	strecpy(root.name, root.name+sizeof root.name, argv[0]);
	strecpy(root.type, root.type+sizeof root.type, argv[1]);
	if(!vtParseScore(argv[2], strlen(argv[2]), root.score))
		vtFatal("bad score '%s'", argv[2]);
	root.blockSize = atoi(argv[3]);
	if(!vtParseScore(argv[4], strlen(argv[4]), root.prev))
		vtFatal("bad score '%s'", argv[4]);
	vtRootPack(&root, buf);

	z = vtDial(host, 0);
	if(z == nil)
		vtFatal("could not connect to server: %R");

	if(!vtConnect(z, 0))
		sysfatal("vtConnect: %r");

	if(!vtWrite(z, score, VtRootType, buf, VtRootSize))
		vtFatal("vtWrite: %R");
	vtClose(z);
	print("%V\n", score);
	vtDetach();
	exits(0);
	return 0;
}
