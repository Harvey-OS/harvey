#include	"all.h"

enum
{
	Nblock		= 12,	/* blocking factor */
};

static	Dev	dev;
static	int	romode;
static	int	type;

static	int	popen(void);
static	int	pgettoc(void);
static	void	firstwaddr(int);
static	int	dtrackinfo(int, int);
static	void	pcopytrack(int, int, int, int);

int
popen(void)
{
	if(dev.chan.open == 0) {
		doprobe(Dphil, &dev.chan);
		if(dev.chan.open == 0) {
			print("philips not on scsi\n");
			return 1;
		}
	}
	if(pgettoc())
		return 1;
	return 0;
}

void
pload(void)
{
	int fromtrack, type, totrack;
	int bs, i;

	if(popen())
		return;
	fromtrack = gettrack("CD track [number]/*");
	if(fromtrack < dev.firsttrack || fromtrack > dev.lasttrack)
		if(fromtrack != Trackall) {
			print("from track out of range %d-%d\n", dev.firsttrack, dev.lasttrack);
			return;
		}

	type = gettype("type [cdda/cdrom]");
	switch(type) {
	default:
		print("bad type\n");
		return;
	case Tcdrom:
		bs = Bcdrom;
		break;
	case Tcdda:
		bs = Bcdda;
		break;
	}

	totrack = gettrack("disk track [number]");

	if(fromtrack == Trackall) {
		for(i=dev.firsttrack; i<=dev.lasttrack; i++) {
			print("track %d\n", i);
			pcopytrack(i, totrack, bs, type);
			totrack++;
		}
		return;
	}
	pcopytrack(fromtrack, totrack, bs, type);
}

static	void
pcopytrack(int from, int to, int bs, int type)
{
	long sz, ad, n;
	uchar *buf;

	if(to < 0 || to >= Maxtrack) {
		print("disk track not in range %d\n", to);
		return;
	}
	buf = dbufalloc(Nblock, bs);
	if(buf == 0)
		return;
	sz = dev.table[from].size;
	ad = dev.table[from].start;
	while(sz > 0) {
		n = Nblock;
		if(sz < Nblock)
			n = sz;
		if(readscsi(dev.chan, buf, ad, n*bs, bs, 0x28, 0))
			break;
		if(dwrite(buf, n))
			return;
		ad += n;
		sz -= n;
	}
	dcommit(to, type);
}

void
ptoc(void)
{
	int i;

	if(popen())
		return;

	print("philips table of contents\n");
	for(i=0; i<=Maxtrack; i++) {
		if(i >= dev.firsttrack && i <= dev.lasttrack)
			print("%3d %7ld %6ld (%ld %x %x)\n", i,
				dev.table[i].start, dev.table[i].size,
				dev.table[i].xsize,
				dev.table[i].tmode,
				dev.table[i].dmode);
		else
		if(dev.table[i].size)
			print("%3d %7ld %6ld  (%ld %x %x)**\n", i,
				dev.table[i].start, dev.table[i].size,
				dev.table[i].xsize,
				dev.table[i].tmode,
				dev.table[i].dmode);
	}
}

int
pinit(long bs, long sz)
{
	uchar cmd[10];

	if(popen())
		return 1;

	if(romode) {
		print("CD not writable\n");
		return 1;
	}
	if(sz+300 >= dev.table[Maxtrack].size) {
		print("not enough room on this CD\n");
		return 1;
	}

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xe6;		/* write track */
	cmd[5] = 0;		/* track number (next) */
	switch(bs) {
	default:
		print("bad block size %ld\n", bs);
		return 1;
	case Bcdda:
		type = Tcdda;
		cmd[6] = 4;	/* audio 2 chan no pre-emphasis */
		break;
	case Bcdrom:
		type = Tcdrom;
		cmd[6] = 1;	/* mode 1, cdrom */
		break;
	}
	if(scsi(dev.chan, cmd, sizeof(cmd), cmd, 0, Snone) < 0) {
		print("cant write track\n");
		return 1;
	}
	return 0;
}

long
pwrite(uchar *b, long n)
{
	uchar cmd[10];
	long m;

	switch(type) {
	default:
		print("unknown type %d\n", type);
		return -1;
	case Tcdda:
		swab(b, n);
		m = n/Bcdda;
		break;
	case Tcdrom:
		m = n/Bcdrom;
		break;
	}
	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x2a;	/* write */
	cmd[7] = m>>8;
	cmd[8] = m>>0;
	return scsi(dev.chan, cmd, sizeof(cmd), b, n, Swrite);
}

void
pflush(void)
{
	uchar cmd[10];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x35;	/* flush */
	scsi(dev.chan, cmd, sizeof(cmd), cmd, 0, Snone);
}

void
pfixate(void)
{
	uchar cmd[10];

	type = gettype("type [cdda/cdrom]");

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xe9;	/* fixate */
	switch(type) {
	default:
		print("bad type\n");
		return;
	case Tcdda:
		cmd[8] = 0;	/* toc type + open next session */
		break;
	case Tcdrom:
		cmd[8] = 1;	/* toc type + open next session */
		break;
	}
	scsi(dev.chan, cmd, sizeof(cmd), cmd, 0, Snone);
	return;
}

void
psession(void)
{
	uchar cmd[10];

	type = gettype("type [cdda/cdrom]");

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xe9;	/* fixate */
	switch(type) {
	default:
		print("bad type\n");
		return;
	case Tcdda:
		cmd[8] = 8;	/* toc type + open next session */
		break;
	case Tcdrom:
		cmd[8] = 9;	/* toc type + open next session */
		break;
	}
	scsi(dev.chan, cmd, sizeof(cmd), cmd, 0, Snone);
	return;
}

static	int
pgettoc(void)
{
	uchar cmd[10], resp[50*8 + 4];
	int i, n, t, ft, lt;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x43;		/* disk info */
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp);
	n = scsi(dev.chan, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < 4) {
		print("cant issue in disk info command\n");
		return 1;
	}

	dev.firsttrack = 0;
	dev.lasttrack = 0;
	memset(dev.table, 0, sizeof(dev.table));
	for(i=1; i<nelem(dev.table); i++)
		dev.table[i].start = -1;

	ft = resp[2];
	lt = resp[3];
	if(ft != 0 || lt != 0) {
		if(ft <= 0 || ft >= Maxtrack) {
			print("first table %d not in range\n", ft);
			return 1;
		}
		if(lt <= 0 || lt >= Maxtrack) {
			print("last table %d not in range\n", lt);
			return 1;
		}
	} else
		ft = 1;

	n -= 7;
	for(i=4; i<n; i+=8) {
		t = resp[i+2];
		if(t == 0xaa) {
			romode = 0;
			if(dtrackinfo(lt+1, Maxtrack))
				romode = 1;
			dev.table[Maxtrack].size = dev.table[Maxtrack].xsize;
		} else
		if(t >= ft && t <= lt) {
			dtrackinfo(t, t);
			if(!romode && dev.table[t].xsize)
				firstwaddr(t);
		} else
			print("unknown track %d\n", t);
	}

	dev.firsttrack = ft;
	dev.lasttrack = lt;
	return 0;
}

static	int
dtrackinfo(int t, int i)
{
	uchar cmd[10], resp[255];
	int n;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xe5;	/* get track info */
	cmd[5] = t;
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp);
	n = scsi(dev.chan, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < 19)
		return 1;

	dev.table[i].start = bige(&resp[2]);
	dev.table[i].size = bige(&resp[6]);
	dev.table[i].xsize = bige(&resp[12]);
	dev.table[i].tmode = resp[10];
	dev.table[i].dmode = resp[11];
	return 0;
}

static	void
firstwaddr(int t)
{
	uchar cmd[10], resp[6];
	int n;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0xe2;	/* first writable addr */
	cmd[2] = t;
	cmd[3] = 4;	/* audio, no preemp */
	cmd[8] = sizeof(resp);
	n = scsi(dev.chan, cmd, sizeof(cmd), resp, sizeof(resp), Sread);
	if(n < sizeof(resp)) {
		print("scsi firstwadd bad\n");
		return;
	}

	dev.table[t].firstw = bige(&resp[1]);
	print("fwa[%d] = %ld\n", t, dev.table[t].firstw);
}
