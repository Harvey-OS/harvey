#include "stdinc.h"
#include "dat.h"
#include "fns.h"

char *host;

void
usage(void)
{
	fprint(2, "usage: read [-h host] score [type]\n");
	exits("usage");
}

int
parseScore(uchar *score, char *buf, int n)
{
	int i, c;

	memset(score, 0, VtScoreSize);

	if(n < VtScoreSize*2)
		return 0;
	for(i=0; i<VtScoreSize*2; i++) {
		if(buf[i] >= '0' && buf[i] <= '9')
			c = buf[i] - '0';
		else if(buf[i] >= 'a' && buf[i] <= 'f')
			c = buf[i] - 'a' + 10;
		else if(buf[i] >= 'A' && buf[i] <= 'F')
			c = buf[i] - 'A' + 10;
		else {
			return 0;
		}

		if((i & 1) == 0)
			c <<= 4;
	
		score[i>>1] |= c;
	}
	return 1;
}

int
main(int argc, char *argv[])
{
	int type, n;
	uchar score[VtScoreSize];
	uchar *buf;
	VtSession *z;

	ARGBEGIN{
	case 'h':
		host = EARGF(usage());
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 1 && argc != 2)
		usage();

	vtAttach();

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	if(!parseScore(score, argv[0], strlen(argv[0])))
		vtFatal("could not parse score: %s", vtGetError());

	buf = vtMemAllocZ(VtMaxLumpSize);

	z = vtDial(host, 0);
	if(z == nil)
		vtFatal("could not connect to server: %R");

	if(!vtConnect(z, 0))
		sysfatal("vtConnect: %r");

	if(argc == 1){
		n = -1;
		for(type=0; type<VtMaxType; type++){
			n = vtRead(z, score, type, buf, VtMaxLumpSize);
			if(n >= 0){
				fprint(2, "venti/read%s%s %V %d\n", host ? " -h" : "", host ? host : "",
					score, type);
				break;
			}
		}
	}else{
		type = atoi(argv[1]);
		n = vtRead(z, score, type, buf, VtMaxLumpSize);
	}
	vtClose(z);
	if(n < 0)
		vtFatal("could not read block: %s", vtGetError());
	if(write(1, buf, n) != n)
		vtFatal("write: %r");

	vtDetach();
	exits(0);
	return 0;	/* shut up compiler */
}
