#include "stdinc.h"
#include "dat.h"
#include "fns.h"
#include <disk.h>

typedef struct Label Label;

struct Label {
	ulong ver;
	ulong trailer;
	ulong tape;
	ulong slot;
};

static void usage(void);
static void verifyArena(void);
static int writeBlock(uchar *buf);
static int writeLabel(Label*);
static int readBlock(uchar *buf);
static int readLabel(Label*);
static int writeMark(void);
static int rewind(int);
static int space(int);

int tape;
int slot;
int pos;
Scsi *dev;

enum {
	LabelMagic = 0x4b5474d2,
	BlockSize = 16*1024,
	Version = 1,
};

void
main(int argc, char *argv[])
{
	char *p;
	int overwrite = 0;

	fmtregister('V', vtScoreFmt);
	fmtregister('R', vtErrFmt);
	vtAttach();
	statsInit();

	ARGBEGIN{
	default:
		usage();
		break;
	case 's':
		p = ARGF();
		if(p == nil)
			usage();
		slot = atoi(p);
		break;
	case 't':
		p = ARGF();
		if(p == nil)
			usage();
		tape = atoi(p);
		break;
	case 'o':
		overwrite++;
		break;
	case '
	}ARGEND

	readonly = 1;

	if(argc != 1)
		usage();

       	dev = openscsi(argv[0]);
	if(dev == nil)
		fatal("could not open scsi device: %r");

	if(!initPos() && !rewind())
		fatal("could not rewind: %r");

fprint(2, "pos = %d\n", pos);

	if(pos != 0) {
		if(!movetoSlot(slot)) {
			if(!rewind())
				fatal("could not rewind: %r");
			pos = 0;
		}
	}

	if(pos != slot && !movetoSlot(slot))
		fatal("could not seek to slot: %r");

	if(!overwrite) {
		if(readLabel(&lb))
			fatal("tape not empty: tape=%d", lb->tape);
	}

	memset(&lb, 0, sizeof(lb));
	lb.ver = Version;
	lb.tape = tape;
	lb.slot = slot;
	
	if(!writeLabel(&lb))
		fatal("could not write header: %r");
	
	if(!writeArena(score))
		fatal("could not write arena: %r");
	
	lb.trailer = 1;
	if(!writeLabel(&lb))
		fatal("could not write header: %r");
	if(!writeArena(score))
		fatal("could not write arena: %r");
	

	exits(0);
}

static void
usage(void)
{
	fprint(2, "usage: dumparena [-o] [-t tape] [-s slot] device\n");
	exits("usage");
}

static int
initPos(void)
{
	Label lb;
	int i;

	for(i=0; i<4; i++) {
		slot(-2);
		if(space(1))
		if(readLabel(&lb))
		if(lb->tape == tape && lb->trailer) {
			pos = lb->slot;
			return 1;
		}
	}
	return 0;
}		

static void
readBlock(uchar *buf, int n)
{
	int nr, m;

	for(nr = 0; nr < n; nr += m){
		m = n - nr;
		m = read(0, &buf[nr], m);
		if(m <= 0)
			fatal("can't read arena from standard input: %r");
	}
}

static void
verifyArena(void)
{
	Arena arena;
	ArenaHead head;
	ZBlock *b;
	VtSha1 *s;
	u64int n, e;
	u32int bs;
	u8int score[VtScoreSize];

	memset(&arena, 0, sizeof arena);

	fprint(2, "verify arena from standard input\n");
	s = vtSha1Alloc();
	if(s == nil)
		fatal("can't initialize sha1 state");
	vtSha1Init(s);

	/*
	 * read the little bit, which will included the header
	 */
	bs = MaxIoSize;
	b = allocZBlock(bs, 0);
	readBlock(b->data, HeadSize);
	vtSha1Update(s, b->data, HeadSize);
	if(!unpackArenaHead(&head, b->data))
		fatal("corrupted arena header: %R");
	if(head.version != ArenaVersion)
		fatal("unknown arena version %d", head.version);

	/*
	 * now we know how much to read
	 * read everything but the last block, which is special
	 */
	e = head.size - head.blockSize;
	for(n = HeadSize; n < e; n += bs){
		if(n + bs > e)
			bs = e - n;
		readBlock(b->data, bs);
		vtSha1Update(s, b->data, bs);
	}

	/*
	 * read the last block update the sum.
	 * the sum is calculated assuming the slot for the sum is zero.
	 */
	bs = head.blockSize;
	readBlock(b->data, bs);
	vtSha1Update(s, b->data, bs - VtScoreSize);
	vtSha1Update(s, zeroScore, VtScoreSize);
	vtSha1Final(s, score);
	vtSha1Free(s);

	/*
	 * validity check on the trailer
	 */
	arena.blockSize = head.blockSize;
	if(!unpackArena(&arena, b->data))
		fatal("corrupted arena trailer: %R");
	scoreCp(arena.score, &b->data[arena.blockSize - VtScoreSize]);

	if(!nameEq(arena.name, head.name))
		fatal("arena header and trailer names clash: %s vs. %s\n", head.name, arena.name);
	if(arena.version != head.version)
		fatal("arena header and trailer versions clash: %d vs. %d\n", head.version, arena.version);
	arena.size = head.size - 2 * head.blockSize;

	/*
	 * check for no checksum or the same
	 */
	if(!scoreEq(score, arena.score)){
		if(!scoreEq(zeroScore, arena.score))
			fprint(2, "warning: mismatched checksums for arena=%s, found=%V calculated=%V",
				arena.name, arena.score, score);
		scoreCp(arena.score, score);
	}else
		fprint(2, "matched score\n");

	printArena(2, &arena);
}

static int
writeBlock(uchar *buf)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x0a;
	cmd[2] = BlockSize>>16;
	cmd[3] = BlockSize>>8;
	cmd[4] = BlockSize;

	if(scsi(dev, cmd, 6, buf, BlockSize, Swrite) < 0)
		return 0;
	return 1;
}

static int
readBlock(uchar *buf)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x08;
	cmd[2] = BlockSize>>16;
	cmd[3] = BlockSize>>8;
	cmd[4] = BlockSize;

	if(scsi(dev, cmd, 6, buf, BlockSize, Sread) < 0)
		return 0;
	return 1;
}

static int
writeMark(void)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x10;

	if(scsi(dev, cmd, 6, buf, BlockSize, Sread) < 0)
		return 0;
	return 1;
}

static int
rewind(int n)
{
	uchar cmd[6];

	if(n > 225)
		n = 255;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x01;
	cmd[4] = n;

	if(scsi(dev, cmd, 6, buf, BlockSize, Sread) < 0)
		return 0;
	return 1;
}

static int
space(int off)
{
	uchar cmd[6];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x11;
	cmd[1] = 0x01;	/* file marks */
	cmd[2] = off>>16;
	cmd[3] = off>>8;
	cmd[4] = off;
	
	if(scsi(dev, cmd, 6, buf, BlockSize, Sread) < 0)
		return 0;
	return 1;
}

static int
writeLabel(Label *lb)
{
	uchar block[BlockSize];
	
	if(lb->ver != Version) {
		vtSerError("unknown header version");
		return 0;
	}

	memset(block, 0, blockSize);
	vtPutUint32(block+0, Magic);
	vtPutUint32(block+4, lb->ver);
	vtPutUint32(block+8, lb->trailer);
	vtPutUint32(block+12, lb->tape);
	vtPutUint32(block+16, lb->slot);

	if(!writeBlock(block))
		return 0;
	if(!writeMark(1))
		return 0;
	return 1;
}

static int
readLabel(Label *lb)
{
	uchar block[BlockSize];

	if(!readBlock(block))
		return 0;
	if(vtGetUint32(block+0) != Magic) {
		vtSetError("bad magic in header");
		return 0;
	}
	lb->ver = vtGetUint32(block+4);
	if(lb->ver != Version) {
		vtSerError("unknown header version");
		return 0;
	}
	lb->trailer = vtPutUint32(block+8);
	lb->tape = vtGetUint32(block+12);
	lb->slot = vtGetUint32(block+16);

	return 1;
}

