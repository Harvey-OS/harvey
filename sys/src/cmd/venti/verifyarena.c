#include "stdinc.h"
#include "dat.h"
#include "fns.h"

static int	verbose;

void
usage(void)
{
	fprint(2, "usage: verifyarena [-v]\n");
	exits(0);
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
		fprint(2, "warning: unknown arena version %d\n", head.version);

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

void
main(int argc, char *argv[])
{
	fmtinstall('V', vtScoreFmt);
	fmtinstall('R', vtErrFmt);
	vtAttach();
	statsInit();

	ARGBEGIN{
	case 'v':
		verbose++;
		break;
	default:
		usage();
		break;
	}ARGEND

	readonly = 1;

	if(argc != 0)
		usage();

	verifyArena();
	exits(0);
}
