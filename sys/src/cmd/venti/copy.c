#include "stdinc.h"
#include "dat.h"
#include "fns.h"

int fast;

VtSession *zsrc, *zdst;

void
usage(void)
{
	fprint(2, "usage: copy src-host dst-host score [type]\n");
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

void
walk(uchar score[VtScoreSize], uint type, int base)
{
	int i, n, sub;
	uchar *buf;
	VtEntry e;
	VtRoot root;

	if(memcmp(score, vtZeroScore, VtScoreSize) == 0)
		return;

	buf = vtMemAllocZ(VtMaxLumpSize);
	if(fast && vtRead(zdst, score, type, buf, VtMaxLumpSize) >= 0){
		fprint(2, "%V already exists on dst server; skipping.\n", score);
		free(buf);
		return;
	}

	n = vtRead(zsrc, score, type, buf, VtMaxLumpSize);
	if(n < 0){
		fprint(2, "warning: could not read block %V %d: %R", score, type);
		return;
	}

	switch(type){
	case VtRootType:
		if(!vtRootUnpack(&root, buf)){
			fprint(2, "warning: could not unpack root in %V %d\n", score, type);
			break;
		}
		walk(root.score, VtDirType, 0);
		walk(root.prev, VtRootType, 0);
		break;

	case VtDirType:
		for(i=0; i<n/VtEntrySize; i++){
			if(!vtEntryUnpack(&e, buf, i)){
				fprint(2, "warning: could not unpack entry #%d in %V %d\n", i, score, type);
				continue;
			}
			if(!(e.flags & VtEntryActive))
				continue;
			if(e.flags&VtEntryDir)
				base = VtDirType;
			else
				base = VtDataType;
			if(e.depth == 0)
				sub = base;
			else
				sub = VtPointerType0+e.depth-1;
			walk(e.score, sub, base);
		}
		break;

	case VtDataType:
		break;

	default:	/* pointers */
		if(type == VtPointerType0)
			sub = base;
		else
			sub = type-1;
		for(i=0; i<n; i+=VtScoreSize)
			if(memcmp(buf+i, vtZeroScore, VtScoreSize) != 0)
				walk(buf+i, sub, base);
		break;
	}

	if(!vtWrite(zdst, score, type, buf, n))
		fprint(2, "warning: could not write block %V %d: %R", score, type);
	free(buf);
}

int
main(int argc, char *argv[])
{
	int type, n;
	uchar score[VtScoreSize];
	uchar *buf;

	ARGBEGIN{
	case 'f':
		fast = 1;
		break;
	default:
		usage();
		break;
	}ARGEND

	if(argc != 3 && argc != 4)
		usage();

	vtAttach();

	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);

	if(!parseScore(score, argv[2], strlen(argv[2])))
		vtFatal("could not parse score: %s", vtGetError());

	buf = vtMemAllocZ(VtMaxLumpSize);

	zsrc = vtDial(argv[0], 0);
	if(zsrc == nil)
		vtFatal("could not dial src server: %R");
	if(!vtConnect(zsrc, 0))
		sysfatal("vtConnect src: %r");

	zdst = vtDial(argv[1], 0);
	if(zdst == nil)
		vtFatal("could not dial dst server: %R");
	if(!vtConnect(zdst, 0))
		sysfatal("vtConnect dst: %r");

	if(argc == 4){
		type = atoi(argv[3]);
		n = vtRead(zsrc, score, type, buf, VtMaxLumpSize);
		if(n < 0)
			vtFatal("could not read block: %R");
	}else{
		for(type=0; type<VtMaxType; type++){
			n = vtRead(zsrc, score, type, buf, VtMaxLumpSize);
			if(n >= 0)
				break;
		}
		if(type == VtMaxType)
			vtFatal("could not find block %V of any type", score);
	}

	walk(score, type, VtDirType);

	if(!vtSync(zdst))
		vtFatal("could not sync dst server: %R");

	vtDetach();
	exits(0);
	return 0;	/* shut up compiler */
}
