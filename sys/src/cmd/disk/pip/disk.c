#include	"all.h"

enum
{
	Magic		= 0xdeadbabe,
	Tocaddr		= 100,
	Dataddr		= 200,
	Second		= 75*Bcdda/sizeof(long),
};

typedef	struct	Buf	Buf;
struct	Buf
{
	long	n;		/* longs left in buffer */
	long	a;		/* block address of beginning */
	long	da;		/* offset to current block */
	long	o;		/* total shorts read */
	long	s;		/* total size */
	long*	buf;
	long*	p;
};

static	Dev	dev;
static	long	blocksize;
static	long	numblocks;

static	uchar*	wbuf;
static	long	fortot;
static	long	forfs;
static	long	forbs;
static	long	maxlen;
static	long	wlen;
static	long	waddr;
static	long	firstwaddr;

static	int	dgetcap(void);
static	long	bestlen(long);
static	void	dumptrack(int, char*);

static	void	pubtrack(int);
static	int	Bget32(Buf*);

int
dopen(void)
{
	int i;

	if(dev.chan.open == 0) {
		doprobe(Ddisk, &dev.chan);
		if(dev.chan.open == 0) {
			print("disk not on scsi\n");
			return 1;
		}
	}
	if(dgetcap())
		return 1;
	if(readscsi(dev.chan, dev.table, Tocaddr, sizeof(dev.table), blocksize, 0x28, 0))
		return 1;
	for(i=0; i<Ntrack; i++) {
		if(dev.table[i].magic != Magic) {
			print("bad magic -- toc being cleared\n");
			if(dcleartoc())
				return 1;
			break;
		}
	}
	return 0;
}

void
dstore(void)
{
	int fromtrack, i;
	char s[100];

	fromtrack = gettrack("disk track [number]/*");
	if(fromtrack < 0 || fromtrack >= Ntrack) {
		if(fromtrack != Trackall) {
			print("from track out of range %d-%d\n", 0, Ntrack);
			return;
		}
	} else
	if(dev.table[fromtrack].size == 0) {
		print("no data on track %d\n", fromtrack);
		return;
	}
	if(eol()) {
		print("file name: ");
	}
	getword();

	if(fromtrack == Trackall) {
		for(i=0; i<Maxtrack; i++) {
			if(dev.table[i].size) {
				sprint(s, "%s%.2d", word, i);
				print("storing %d to %s\n", i, s);
				dumptrack(i, s);
			}
		}
	} else
		dumptrack(fromtrack, word);
}

void
dtoc(void)
{
	int i;
	long t;

	print("disk table of contents\n");
	t = 0;
	for(i=0; i<Ntrack; i++) {
		if(dev.table[i].size)
			print("%3d %7ld %6ld (%ld %ld %ld+%ld)\n", i,
				dev.table[i].start, dev.table[i].size,
				dev.table[i].bsize,
				dev.table[i].fsize,
				dev.table[i].tsize/dev.table[i].bsize,
				dev.table[i].tsize%dev.table[i].bsize);
		t += dev.table[i].size;
	}
	print("total %12ld (%d%%)\n", t, (t*100) / numblocks);
}

int
dclearentry(int n, int wr)
{

	memset(&dev.table[n], 0, sizeof(dev.table[n]));
	dev.table[n].magic = Magic;
	if(wr)
		if(writescsi(dev.chan, dev.table, Tocaddr, sizeof(dev.table), blocksize))
			return 1;
	return 0;
}

int
dcleartoc(void)
{
	int i;

	for(i=0; i<Ntrack; i++)
		if(dclearentry(i, 0))
			return 1;
	if(writescsi(dev.chan, dev.table, Tocaddr, sizeof(dev.table), blocksize))
		return 1;
	return 0;
}

static	int
dgetcap(void)
{
	uchar cmd[10], resp[8];
	int n;

	memset(cmd, 0, sizeof(cmd));
	cmd[0] = 0x25;	/* read capacity */
	n = scsi(dev.chan, cmd, sizeof(cmd), resp, sizeof(resp), 0);
	if(n < sizeof(resp)) {
		print("bad cap command\n");
		return 1;
	}
	numblocks = bige(&resp[0]);
	blocksize = bige(&resp[4]);
	return 0;
}

static	long
bestlen(long a)
{
	long l, m;
	int i;

	l = numblocks;
	for(i=0; i<Ntrack; i++)
		if(dev.table[i].size) {
			m = dev.table[i].start - a;
			if(m >= 0 && m < l)
				l = m;
		}
	if(l == numblocks)
		l = numblocks - a;
	return l;
}

static	void
dumptrack(int track, char *file)
{
	int f;
	long n, m, addr;

	f = create(file, OWRITE, 0666);
	if(f < 0) {
		print("cant open %s: %r\n", file);
		return;
	}
	if(wbuf)
		free(wbuf);
	forfs = dev.table[track].fsize;
	forbs = dev.table[track].bsize;
	fortot = dev.table[track].tsize;
	wlen = (forfs*forbs + blocksize - 1) / blocksize;
	wbuf = malloc(wlen*blocksize);
	if(wbuf == 0) {
		print("malloc failed\n");
		return;
	}
	addr = dev.table[track].start;
	while(fortot > 0) {
		if(readscsi(dev.chan, wbuf, addr, wlen*blocksize, blocksize, 0x28, 0))
			return;
		n = forfs*forbs;
		if(n > fortot)
			n = fortot;
		m = write(f, wbuf, n);
		if(m != n) {
			print("short write %d/%d: %r\n", m, n);
			break;
		}
		fortot -= n;
		addr += wlen;
	}
	close(f);
}

uchar*
dbufalloc(long fs, long bs)
{
	long l, a;
	int i;

	forfs = fs;
	forbs = bs;
	fortot = 0;
	wlen = (forfs*forbs + blocksize - 1) / blocksize;
	if(wbuf)
		free(wbuf);
	wbuf = malloc(wlen*blocksize);
	if(wbuf == 0) {
		print("malloc failed\n");
		return 0;
	}
	maxlen = bestlen(Dataddr);
	waddr = Dataddr;
	for(i=0; i<Ntrack; i++)
		if(dev.table[i].size) {
			a = dev.table[i].start + dev.table[i].size;
			l = bestlen(a);
			if(l > maxlen) {
				maxlen = l;
				waddr = a;
			}
		}
	firstwaddr = waddr;
	return wbuf;
}

void
dcommit(int track, int type)
{
	if(track < 0 || track >= Ntrack) {
		print("disk track not in range\n");
		return;
	}
	dev.table[track].type = type;
	dev.table[track].start = firstwaddr;
	dev.table[track].size = waddr - firstwaddr;
	dev.table[track].fsize = forfs;
	dev.table[track].bsize = forbs;
	dev.table[track].tsize = fortot;
	writescsi(dev.chan, dev.table, Tocaddr, sizeof(dev.table), blocksize);
}

int
dwrite(uchar *buf, long nb)
{

	if(wbuf != buf) {
		print("horrible\n");
		return 1;
	}
	if(waddr-firstwaddr+wlen > maxlen) {
		print("write overflow max = %ld\n", maxlen);
		return 1;
	}
	if(writescsi(dev.chan, buf, waddr, wlen*blocksize, blocksize))
		return 1;
	waddr += wlen;
	fortot += nb*forbs;
	return 0;
}

void
dpublish(void)
{
	int fromtrack, i;

	fromtrack = gettrack("disk track [number]/*");
	if(fromtrack < 0 || fromtrack >= Ntrack) {
		if(fromtrack != Trackall) {
			print("from track out of range %d-%d\n", 0, Ntrack);
			return;
		}
	} else
	if(dev.table[fromtrack].size == 0) {
		print("no data on track %d\n", fromtrack);
		return;
	}

	if(fromtrack == Trackall) {
		for(i=0; i<Maxtrack; i++) {
			if(dev.table[i].size) {
				print("publishing %d\n", i);
				pubtrack(i);
			}
		}
	} else
		pubtrack(fromtrack);
}

static	long
Bget32(Buf *b)
{
	long c;

loop:
	if(b->n > 0) {
		c = *b->p;
		b->p++;
		b->n--;
		b->o++;
		return c & 0x7fffffffL;
	}
	if(b->o >= b->s)
		return -1;

	if(readscsi(dev.chan, b->buf, b->a+b->da, wlen*blocksize, blocksize, 0x28, 0))
		return -1;
	b->da += wlen;
	b->p = b->buf;
	b->n = forfs*forbs/sizeof(*b->p);
	if(b->o + b->n > b->s)
		b->n = b->s - b->o;

	goto loop;
}

static	int
verify1(Buf *b1, Buf *b2)
{
	long c1, c2, n;

	b1->n = 0;
	b2->n = 0;
	b1->o = 0;
	b2->o = 0;
	b1->da = 0;
	b2->da = 0;

	n = 0;
	for(;;) {
		c1 = Bget32(b1);
		c2 = Bget32(b2);
		if(c2 == -1)
			break;
		if(c1 == c2) {
			if(c1 != 0) {
				n++;
				if(n > Second)
					break;
			}
			continue;
		}
		n = 0;
		c2 = Bget32(b2);
		if(c2 == -1 || b2->o > b1->o+Second)
			return 0;
	}

	n = b2->o - b1->o;

	b1->n = 0;
	b2->n = 0;
	b1->o = 0;
	b2->o = 0;
	b1->da = 0;
	b2->da = 0;

	for(; n>0; n--)
		Bget32(b2);
	for(;;) {
		c1 = Bget32(b1);
		c2 = Bget32(b2);
		if(c2 == -1)
			return 1;
		if(c1 != c2)
			return 0;
	}
}

int
dverify(int track1, int track2)
{
	Buf b1, b2;
	int f;

	if(dev.table[track1].fsize != dev.table[track2].fsize ||
	   dev.table[track1].bsize != dev.table[track2].bsize ||
	   dev.table[track1].tsize != dev.table[track2].tsize) {
		print("verify -- tracks not compat\n");
		return 0;
	}

	forfs = dev.table[track1].fsize;
	forbs = dev.table[track1].bsize;
	fortot = dev.table[track1].tsize;
	wlen = (forfs*forbs + blocksize - 1) / blocksize;
	b1.buf = malloc(wlen*blocksize);
	b2.buf = malloc(wlen*blocksize);
	if(b1.buf == 0 || b2.buf == 0) {
		print("malloc failed\n");
		exits("malloc");
	}

	b1.a = dev.table[track1].start;
	b2.a = dev.table[track2].start;
	b1.s = fortot/sizeof(*b1.p);
	b2.s = fortot/sizeof(*b2.p);

	f = verify1(&b2, &b1) || verify1(&b1, &b2);

	free(b1.buf);
	free(b2.buf);

	if(f)
		print("tracks do not verify\n");

	return f;

}

static	void
leadin(int n)
{
	int i;

	memset(wbuf, 0, forbs);
	for(i=0; i<n; i++)
		pwrite(wbuf, forbs);
}

static	void
pubtrack(int track)
{
	long n, m, addr;

	if(wbuf)
		free(wbuf);
	forfs = dev.table[track].fsize;
	forbs = dev.table[track].bsize;
	fortot = dev.table[track].tsize;
	wlen = (forfs*forbs + blocksize - 1) / blocksize;
	wbuf = malloc(wlen*blocksize);
	if(wbuf == 0) {
		print("malloc failed\n");
		return;
	}
	if(pinit(forbs, fortot/forbs))
		return;

	addr = dev.table[track].start;
	while(fortot > 0) {
		if(readscsi(dev.chan, wbuf, addr, wlen*blocksize, blocksize, 0x28, 0))
			break;
		n = forfs*forbs;
		if(n > fortot)
			n = fortot;
		m = pwrite(wbuf, n);
		if(m != n) {
			print("short write %d/%d: %r\n", m, n);
			break;
		}
		fortot -= n;
		addr += wlen;
	}

	pflush();
}

static	void
sumtrack(int track)
{
	long n, i, w, addr, sum;

	if(wbuf)
		free(wbuf);
	forfs = dev.table[track].fsize;
	forbs = dev.table[track].bsize;
	fortot = dev.table[track].tsize;
	wlen = (forfs*forbs + blocksize - 1) / blocksize;
	wbuf = malloc(wlen*blocksize);
	if(wbuf == 0) {
		print("malloc failed\n");
		return;
	}

	sum  = 0;
	addr = dev.table[track].start;
	while(fortot > 0) {
		if(readscsi(dev.chan, wbuf, addr, wlen*blocksize, blocksize, 0x28, 0))
			break;
		n = forfs*forbs;
		if(n > fortot)
			n = fortot;
		for(i=0; i<n; i+=4) {
			w = (wbuf[i+0]<<24) | (wbuf[i+1]<<16) |
				(wbuf[i+2]<<8) | wbuf[i+3];
			sum += w;
		}
		fortot -= n;
		addr += wlen;
	}
	print("sum = %.8lux\n", sum);
}

void
dsum(void)
{
	int fromtrack, i;

	fromtrack = gettrack("disk track [number]/*");
	if(fromtrack < 0 || fromtrack >= Ntrack) {
		if(fromtrack != Trackall) {
			print("from track out of range %d-%d\n", 0, Ntrack);
			return;
		}
	} else
	if(dev.table[fromtrack].size == 0) {
		print("no data on track %d\n", fromtrack);
		return;
	}

	if(fromtrack == Trackall) {
		for(i=0; i<Maxtrack; i++) {
			if(dev.table[i].size) {
				print("summing %d\n", i);
				sumtrack(i);
			}
		}
	} else
		sumtrack(fromtrack);
}
