#include	"all.h"

enum
{
	Nblock		= 12,	/* blocking factor */
};

static	Dev	dev;

static	int	topen(void);
static	int	tgettoc(void);
static	int	tmodeset(int);
static	void	tcopytrack(int, int, int, int);

static	int
topen(void)
{
	if(dev.chan.open == 0) {
		doprobe(Dtosh, &dev.chan);
		if(dev.chan.open == 0) {
			print("toshiba not on scsi\n");
			return 1;
		}
	}
	if(tmodeset(Bcdda))
		return 1;
	if(tgettoc())	/* toc address are adjusted by block size */
		return 1;
	return 0;
}

void
tload(void)
{
	int fromtrack, type, totrack;
	int bs, i;

	if(topen())
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
			tcopytrack(i, totrack, bs, type);
			totrack++;
		}
		return;
	}
	tcopytrack(fromtrack, totrack, bs, type);
}

void
tverif(void)
{
	int fromtrack, type, totrack;
	int bs, i;

	if(topen())
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
			tcopytrack(i, totrack, bs, type);
			for(;;) {
				print("track %d->t\n", i);
				tcopytrack(i, Tracktmp, bs, type);
				if(dverify(totrack, Tracktmp))
					break;
				print("track %d->%d\n", i, totrack);
				tcopytrack(i, totrack, bs, type);
				if(dverify(totrack, Tracktmp))
					break;
			}
			dclearentry(Tracktmp, 1);
			totrack++;
		}
		return;
	}

	print("track %d->%d\n", fromtrack, totrack);
	tcopytrack(fromtrack, totrack, bs, type);
	for(;;) {
		print("track %d->t\n", fromtrack);
		tcopytrack(fromtrack, Tracktmp, bs, type);
		if(dverify(totrack, Tracktmp))
			break;
		print("track %d->%d\n", fromtrack, totrack);
		tcopytrack(fromtrack, totrack, bs, type);
		if(dverify(totrack, Tracktmp))
			break;
	}
	dclearentry(Tracktmp, 1);
}

void
ttoc(void)
{
	int i;

	if(topen())
		return;

	print("toshiba table of contents\n");
	for(i=0; i<Ntrack; i++)
		if(dev.table[i].size)
			print("%3d %7ld %6ld\n", i,
				dev.table[i].start,
				dev.table[i].size);
}

static	void
tcopytrack(int from, int to, int bs, int type)
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
			return;
		if(dwrite(buf, n))
			return;
		ad += n;
		sz -= n;
	}
	dcommit(to, type);
}

static	int
tgettoc(void)
{
	uchar cmd[10], resp[50*8 + 4];
	int i, n, t, ft, lt;

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
	}

	dev.table[0].start = 0;
	dev.table[0].size = dev.table[ft].start;
	for(i=1; i<ft; i++) {
		dev.table[i].start = dev.table[ft].start;
		dev.table[i].size = 0;
	}
	for(i=lt+1; i<Ntrack; i++) {
		dev.table[i].start = dev.table[Maxtrack].start;
		dev.table[i].size = 0;
	}
	for(i=ft; i<=lt; i++) {
		if(dev.table[i].start == -1) {
			print("table %d not in toc\n", i);
			return 1;
		}
		dev.table[i].size = dev.table[i+1].start - dev.table[i].start;
	}
	if(dev.table[Maxtrack].start == -1) {
		print("table %d not in toc\n", Maxtrack);
		return 1;
	}

	dev.firsttrack = ft;
	dev.lasttrack = lt;
	return 0;
}

int
tmodeset(int bs)
{
	uchar cmd[6], resp[28];

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x15;		/* mode select */
	cmd[1] = 0x10;
	cmd[4] = sizeof(resp);

	memset(resp, 0, sizeof(resp));
	resp[3] = 8;
	resp[4] = 0x82;
	resp[8] = bs>>24;
	resp[9] = bs>>16;
	resp[10] = bs>>8;
	resp[11] = bs>>0;
	resp[12] = 2;		/* mode select page 2 */
	resp[13] = 0xe;		/* length of page 2 */
	resp[14] = 0x6f;	/* reconnect after 16 blocks of data */

	if(scsi(dev.chan, cmd, sizeof(cmd), resp, sizeof(resp), 1) != sizeof(resp)) {
		print("scsi mode select\n");
		return 1;
	}
	return 0;
}
