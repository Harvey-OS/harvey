#include	"all.h"

enum
{
	Nblock		= 20,	/* blocking factor */
};

static	Dev	dev;

static	int	xopen(void);
static	int	xgettoc(void);
static	void	xcopytrack(int, int, int, int);

static	int
xopen(void)
{
	if(dev.chan.open == 0) {
		doprobe(Dplex, &dev.chan);
		if(dev.chan.open == 0) {
			print("plextor not on scsi\n");
			return 1;
		}
	}
	if(xgettoc())	/* toc address are adjusted by block size */
		return 1;
	return 0;
}

void
xload(void)
{
	int fromtrack, type, totrack;
	int bs, i;

	if(xopen())
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
			xcopytrack(i, totrack, bs, type);
			totrack++;
		}
		return;
	}
	xcopytrack(fromtrack, totrack, bs, type);
}

void
xverif(void)
{
	int fromtrack, type, totrack;
	int bs, i;

	if(xopen())
		return;
	fromtrack = gettrack("CD track [number]/*");
	if(fromtrack < dev.firsttrack || fromtrack > dev.lasttrack)
		if(fromtrack != Trackall) {
			print("from track out of range %d-%d\n", dev.firsttrack, dev.lasttrack);
			return;
		}

	type = Tcdda;
	bs = Bcdda;

	totrack = gettrack("disk track [number]");

	if(fromtrack == Trackall) {
		for(i=dev.firsttrack; i<=dev.lasttrack; i++) {
			print("track %d->%d\n", i, totrack);
			xcopytrack(i, totrack, bs, type);
			for(;;) {
				print("track %d->t\n", i);
				xcopytrack(i, Tracktmp, bs, type);
				if(dverify(totrack, Tracktmp))
					break;
				print("track %d->%d\n", i, totrack);
				xcopytrack(i, totrack, bs, type);
				if(dverify(totrack, Tracktmp))
					break;
			}
			dclearentry(Tracktmp, 1);
			totrack++;
		}
		return;
	}

	print("track %d->%d\n", fromtrack, totrack);
	xcopytrack(fromtrack, totrack, bs, type);
	for(;;) {
		print("track %d->t\n", fromtrack);
		xcopytrack(fromtrack, Tracktmp, bs, type);
		if(dverify(totrack, Tracktmp))
			break;
		print("track %d->%d\n", fromtrack, totrack);
		xcopytrack(fromtrack, totrack, bs, type);
		if(dverify(totrack, Tracktmp))
			break;
	}
	dclearentry(Tracktmp, 1);
}

void
xtoc(void)
{
	int i;

	if(xopen())
		return;

	print("plextor table of contents\n");
	for(i=0; i<Ntrack; i++)
		if(dev.table[i].size)
			print("%3d %7ld %6ld\n", i,
				dev.table[i].start,
				dev.table[i].size);
}

static	void
xcopytrack(int from, int to, int bs, int type)
{
	int ioc;
	long sz, ad, n;
	uchar *buf;

	if(to < 0 || to >= Maxtrack) {
		print("disk track not in range %d\n", to);
		return;
	}
	buf = dbufalloc(Nblock, bs);
	if(buf == 0)
		return;

	ioc = 0xd8;
	if(type == Tcdrom)
		ioc = 0x28;

	/* dummy read to break readahead check condition */
	readscsi(dev.chan, buf, 0, bs, bs, ioc, 0);

	sz = dev.table[from].size;
	ad = dev.table[from].start - dev.table[dev.firsttrack].start;
	while(sz > 0) {
		n = Nblock;
		if(sz < Nblock)
			n = sz;
		if(readscsi(dev.chan, buf, ad, n*bs, bs, ioc, 0))
			return;
		if(dwrite(buf, n))
			return;
		ad += n;
		sz -= n;
	}
	dcommit(to, type);
}

static	int
xgettoc(void)
{
	uchar cmd[10], resp[Ntrack*8 + 4];
	int i, n, t, ft, lt;
	long a;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x43;		/* disk info */
	cmd[7] = sizeof(resp)>>8;
	cmd[8] = sizeof(resp);
	n = scsi(dev.chan, cmd, sizeof(cmd), resp, sizeof(resp), 0);
	if(n < 4) {
		print("cant issue in disk info command\n");
		return 1;
	}

	dev.firsttrack = 0;
	dev.lasttrack = 0;
	for(i=0; i<Ntrack; i++)
		dev.table[i].start = -1;

	ft = resp[2];
	lt = resp[3];
	if(ft <= 0 || ft >= Maxtrack) {
		print("first table %d not in range\n", ft);
		return 1;
	}
	if(lt <= 0 || lt >= Maxtrack) {
		print("last table %d not in range\n", lt);
		return 1;
	}
	n -= 7;
	for(i=4; i<n; i+=8) {
		t = resp[i+2];
		if(t >= 0xa0)
			t = Maxtrack;
		else
		if(t < ft || t > lt) {
			print("table %d not in range\n", t);
			return 1;
		}
		dev.table[t].type = resp[i+1];
		dev.table[t].start = bige(&resp[i+4]);
		/* for some reason, last track is under reported */
		if(t == Maxtrack)
			dev.table[t].start--;
	}

	a = 0;
	for(i=Ntrack-1; i>=0; i--) {
		if(dev.table[i].start == -1) {
			dev.table[i].start = a;
			dev.table[i].size = 0;
			continue;
		}
		a -= dev.table[i].start;
		if(a < 0)
			a = 0;
		dev.table[i].size = a;
		a = dev.table[i].start;
	}

	dev.firsttrack = ft;
	dev.lasttrack = lt;
	return 0;
}
