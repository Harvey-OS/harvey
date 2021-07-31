#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"
#include	"ureg.h"

#define	NPORT		(sizeof audiodir/sizeof(Dirtab))
#define	CSR		(*(ulong*)SOUNDOUTDMA)
#define	DMA1B		(*(ulong*)SOUNDOUTBASE)
#define	DMA1L		(*(ulong*)SOUNDOUTLIMIT)
#define	DMA2B		(*(ulong*)SOUNDOUTCHAINBASE)
#define	DMA2L		(*(ulong*)SOUNDOUTCHAINLIMIT)
#define	MONCMD		(*(ulong*)MONCSR)
#define	MONDAT		(*(ulong*)KMSDATA)

enum
{
	Qdir		= 0,
	Qaudio,
	Qvolume,

	Sempty		= 0,
	Sfilling,
	Sfull,
	Scurrent,
	Schained,

	Outcint		= 1<<27,
	Outrw		= 1<<26,	/* always 0 */
	Outchain	= 1<<25,
	Outenable	= 1<<24,

	Outinitbuff	= 1<<21,
	Outcreset	= 1<<20,
	Outclrcint	= 1<<19,
	Outwr		= 1<<18,	/* 0 = read */
	Outsetchain	= 1<<17,
	Outsetenable	= 1<<16,

	Bufsize		= 8*1024,
	Nbuf		= 10,
};

Dirtab
audiodir[] =
{
	"audio",	{Qaudio},		0,	0600,
	"volume",	{Qvolume},		0,	0600,
};

typedef struct	Buf	Buf;
struct	Buf
{
	uchar*	virt;
	ulong	phys;
	ulong	age;
	int	state;
};
static	struct
{
	Rendez	vous;
	int	bufinit;	/* boolean if buffers allocated */
	int	open;		/* boolean if open */
	int	curcount;	/* how much data in current buffer */
	int	active;		/* boolean dma running */
	int	intr;		/* boolean an interrupt has happened */
	ulong	agegen;		/* incremented for age in buffers */
	int	volume;		/* volume set */
	int	c4cmd;		/* various modes, ie mute */
	char	place[20];
	Buf	buf[Nbuf];
} audio;

/*
 * send a command to the
 * monitor sub-system.
 * SoundOutEnable is left on.
 */
static	void
monxmit(int cmd, long data)
{

	/*
	 * this routine suffers from usual
	 * NEXT copious documentation.
	 * 1.	other (which?) MONCMD bits/modes
	 *	are not preserved across call.
	 * 2.	the delays are arbitrary.
	 * 3.	the use of bit9 and bit31 are
	 *	mystical.
	 */
	MONCMD = (1<<9) | cmd;
	delay(1);
	MONDAT = data;
	delay(1);
	MONCMD = (1<<31)|(1<<9);
}

/*
 * get a word from the write argument
 * and return the number of bytes consumed.
 */
static	int
getcmd(char *a, long n)
{
	char *p, *ep;
	int m, c;

	p = audio.place;
	ep = p + sizeof(audio.place) - 1;
	m = 0;
	while(m < n) {
		c = *a++;
		m++;
		if(c == ' ' || c == '\n')
			continue;
		*p++ = c;
		break;
	}
	while(m < n) {
		c = *a++;
		m++;
		if(c == ' ' || c == '\n')
			break;
		if(p < ep)
			*p++ = c;
	}
	*p = 0;
	return m;
}

/*
 * find the oldest buffer of
 * a certain type. this should
 * probably be done in link lists,
 * but there arent too many buffers
 * and locking is a pain.
 */
static	Buf*
getbuf(int state)
{
	Buf *b, *p, *eb;

	b = &audio.buf[0];
	eb = b + Nbuf;
	while(b < eb) {
		if(b->state == state) {
			if(state != Sfull)
				return b;
			p = b;
			for(b++; b<eb; b++)
				if(b->state == state)
					if(b->age < p->age)
						p = b;
			return p;
		}
		b++;
	}
	return 0;
}

/*
 * if audio is stopped,
 * start it up again.
 */
static	void
pokeaudio(void)
{
	Buf *b1, *b2;

	if(audio.active == 0) {
		/*
		 * precaution, return active buffers
		 */
		while(b1 = getbuf(Scurrent))
			b1->state = Sfull;
		while(b1 = getbuf(Schained))
			b1->state = Sfull;

		/*
		 * start up audio with the
		 * two oldest full buffers.
		 * if there is not two,
		 * dont start.
		 */
		b1 = getbuf(Sfull);
		if(b1 == 0)
			return;
		b1->state = Scurrent;

		b2 = getbuf(Sfull);
		if(b2 == 0) {
			b1->state = Sfull;
			return;
		}
		b2->state = Schained;

		audio.active = 1;
		CSR = 0;
		CSR = Outcreset|Outinitbuff;
		DMA1L = b1->phys+Bufsize;
		DMA1B = b1->phys;
		DMA2L = b2->phys+Bufsize;
		DMA2B = b2->phys;
		CSR = Outsetenable|Outsetchain;
	}
}

void
audiodmaintr(void)
{
	long csr;
	Buf *b;

	csr = CSR;
	audio.intr = 1;

	b = getbuf(Scurrent);
	if(b)
		b->state = Sempty;
	b = getbuf(Schained);

	/*
	 * we took the last interrupt
	 * without a chain.
	 * just shut down.
	 */
	if((csr & Outenable) == 0) {
		if(b)
			b->state = Sfull;
		CSR = Outcreset | Outclrcint;
		audio.active = 0;
		wakeup(&audio.vous);
		return;
	}

	/*
	 * we have another full buffer,
	 * chain it on the end.
	 */
	if(b)
		b->state = Scurrent;
	b = getbuf(Sfull);
	if(b) {
		b->state = Schained;
		DMA2L = b->phys+Bufsize;
		DMA2B = b->phys;
		CSR = Outsetchain | Outclrcint;
		wakeup(&audio.vous);
		return;
	}

	/*
	 * there is no full buffer,
	 * just clear the interrupt
	 * and let the last buffer go out
	 */
	CSR = Outclrcint;
	wakeup(&audio.vous);
}

static int
anybuf(void *p)
{
	USED(p);
	return audio.intr;
}

/*
 * wait for some output to get
 * empty buffers back.
 */
static void
waitaudio(void)
{

	audio.intr = 0;
	pokeaudio();
	tsleep(&audio.vous, anybuf, 0, 10*1000);
	if(audio.intr == 0) {
		print("audio timeout\n");
		audio.active = 0;
		pokeaudio();
	}
}

static void
bufinit(void)
{
	int i;
	ulong l, p;
	Buf *b;

	b = &audio.buf[0];
	for(i=0; i<Nbuf; i++,b++) {
		p = (ulong)xspanalloc(Bufsize, BY2PG, 0);
		b->virt = (uchar*)kmappa(p);
		b->phys = (ulong)p;
		for(l = BY2PG; l < Bufsize; l += BY2PG)
			kmappa(p+l);
	}
}

void
audioreset(void)
{
}

void
audioinit(void)
{
}

Chan*
audioattach(char *param)
{
	return devattach('h', param);
}

Chan*
audioclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
audiowalk(Chan *c, char *name)
{
	return devwalk(c, name, audiodir, NPORT, devgen);
}

void
audiostat(Chan *c, char *db)
{
	devstat(c, db, audiodir, NPORT, devgen);
}

Chan*
audioopen(Chan *c, int omode)
{
	int i;
	Buf *b;

	switch(c->qid.path & ~CHDIR) {
	default:
		error(Eperm);
		break;

	case Qdir:
	case Qvolume:
		break;

	case Qaudio:
		if(audio.open)
			error(Einuse);

		if(audio.bufinit == 0) {
			/* sound out enable, analog out enable */
			monxmit(0x0f, 0);
			bufinit();
			audio.bufinit = 1;
		}
		b = &audio.buf[0];
		for(i=0; i<Nbuf; i++,b++) {
			b->state = Sempty;
			b->age = i;
		}
		audio.agegen = Nbuf;
		audio.curcount = 0;
		audio.open = 1;
		break;
	}
	c = devopen(c, omode, audiodir, NPORT, devgen);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;

	return c;
}

void
audiocreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c);
	USED(name);
	USED(omode);
	USED(perm);

	error(Eperm);
}

void
audioclose(Chan *c)
{

	switch(c->qid.path & ~CHDIR) {
	default:
		error(Eperm);
		break;

	case Qdir:
	case Qvolume:
		break;

	case Qaudio:
		while(audio.active)
			waitaudio();
		CSR = 0;
		CSR = Outcreset|Outinitbuff;
		audio.open = 0;
		break;
	}
}

long
audioread(Chan *c, char *a, long n, ulong offset)
{

	USED(offset);

	switch(c->qid.path & ~CHDIR) {
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, a, n, audiodir, NPORT, devgen);

	case Qaudio:
	case Qvolume:
		break;
	}
	return 0;
}

long
audiowrite(Chan *c, char *a, long n, ulong offset)
{
	long m, bytes;
	Buf *b;

	USED(offset);

	bytes = 0;
	switch(c->qid.path & ~CHDIR) {
	default:
		error(Eperm);
		break;

	case Qvolume:
		for(;;) {
			m = getcmd(a, n);
			if(m <= 0)
				break;
			a += m;
			bytes += m;
			n -= m;

			/*
			 * a number is volume
			 */
			if(audio.place[0] >= '0' && audio.place[0] <= '9') {
				m = strtoul(audio.place, 0, 10);
				audio.volume = m;	/* goes up to 11! */
				m *= 3;
				if(m < 0)
					m = 0;
				if(m > 31)
					m = 31;
				monxmit(0xc2, (0xdf-m)<<24);
				continue;
			}

			/*
			 * c4 command operands
			 *	[|un][mute|dec]
			 */
			if(strcmp(audio.place, "mute") == 0) {
				audio.c4cmd |= (1<<4);
				goto doc4;
			}
			if(strcmp(audio.place, "unmute") == 0) {
				audio.c4cmd &= ~(1<<4);
				goto doc4;
			}
			if(strcmp(audio.place, "dec") == 0) {
				audio.c4cmd |= (1<<3);
				goto doc4;
			}
			if(strcmp(audio.place, "undec") == 0) {
				audio.c4cmd &= ~(1<<3);
			doc4:
				monxmit(0xc4, audio.c4cmd<<24);
				continue;
			}
			break;
		}
		break;

	case Qaudio:
		while(n > 0) {
			b = getbuf(Sfilling);
			if(b == 0) {
				if(audio.curcount != 0) {
					print("no filling buffer and curcount != 0\n");
					audio.curcount = 0;
				}
				b = getbuf(Sempty);
				if(b == 0) {
					waitaudio();
					continue;
				}
				audio.agegen++;
				b->age = audio.agegen;
				b->state = Sfilling;
			}
			m = Bufsize-audio.curcount;
			if(m > n)
				m = n;
			memmove(b->virt + audio.curcount, a, m);

			audio.curcount += m;
			n -= m;
			bytes += m;
			a += m;

			if(audio.curcount >= Bufsize) {
				b->state = Sfull;
				audio.curcount = 0;
				pokeaudio();
			}
		}
		break;
	}
	return bytes;
}

void
audioremove(Chan *c)
{
	USED(c);

	error(Eperm);
}

void
audiowstat(Chan *c, char *dp)
{
	USED(c);
	USED(dp);

	error(Eperm);
}
