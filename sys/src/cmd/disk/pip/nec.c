#include	"all.h"

enum
{
	Nblock		= 12,	/* blocking factor */
};

static	Dev	dev;
static	int	mod;

static	int	nopen(void);
static	int	ngentoc(void);
static	int	nmodeset(int);
static	void	ncopytrack(int, int, int, int);

void
nspecial(void)
{

	getword();
	mod = atoi(word);
	print("mod = %d\n", mod);
}

static	int
nopen(void)
{
	if(dev.chan.open == 0) {
		doprobe(Dnec, &dev.chan);
		if(dev.chan.open == 0) {
			print("nec not on scsi\n");
			return 1;
		}
	}
	if(nmodeset(Bcdda))
		return 1;
	if(ngentoc())	/* toc address are adjusted by block size */
		return 1;
	return 0;
}

void
nload(void)
{
	int fromtrack, type, totrack;
	int bs, i;
	long t;

	if(nopen())
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
			ncopytrack(i, totrack, bs, type);
			totrack++;
		}
		return;
	}
	t = time(0);
	ncopytrack(fromtrack, totrack, bs, type);
	t = time(0) - t;
	print("time = %ld\n", t);
}

void
ntoc(void)
{
	int i;

	if(nopen())
		return;

	print("nec table of contents\n");
	for(i=0; i<Ntrack; i++)
		if(dev.table[i].size)
			print("%2d %7ld %6ld\n", i,
				dev.table[i].start,
				dev.table[i].size);
}

static	void
ncopytrack(int from, int to, int bs, int type)
{
	long sz, ad, n;
	uchar *buf;

	buf = dbufalloc(Nblock, bs);
	if(buf == 0)
		return;
	sz = dev.table[from].size;
	ad = dev.table[from].start;
	while(sz > 0) {
		n = Nblock;
		if(sz < Nblock)
			n = sz;
		if(readscsi(dev.chan, buf, ad, n*bs, bs, 0xd4, mod))
			return;
		if(dwrite(buf, n))
			return;
		ad += n;
		sz -= n;
	}
	dcommit(to, type);
}

static	int
ngentoc(void)
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
	if(ft <= 0 || ft >= Ntrack) {
		print("first table %d not in range\n", ft);
		return 1;
	}
	if(lt <= 0 || lt >= Ntrack) {
		print("last table %d not in range\n", lt);
		return 1;
	}
	n -= 4;
	for(i=4; i<n; i+=8) {
		t = resp[i+2];
		if(t >= 0xa0)
			t = Ntrack-1;
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
		dev.table[i].start = dev.table[Ntrack-1].start;
		dev.table[i].size = 0;
	}
	for(i=ft; i<=lt; i++) {
		if(dev.table[i].start == -1) {
			print("table %d not in toc\n", i);
			return 1;
		}
		dev.table[i].size = dev.table[i+1].start - dev.table[i].start;
	}
	if(dev.table[Ntrack-1].start == -1) {
		print("table %d not in toc\n", Ntrack-1);
		return 1;
	}

	dev.firsttrack = ft;
	dev.lasttrack = lt;
	return 0;
}

int
nmodeset(int bs)
{
	uchar cmd[6], resp[Bcdda];

USED(bs);
return 0;
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
