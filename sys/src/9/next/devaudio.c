#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"

#include	"io.h"
#include	"ureg.h"

typedef struct	AQueue	AQueue;
typedef struct	Buf	Buf;
typedef struct	Level	Level;

#define	NAUDIO		(sizeof audiodir/sizeof(Dirtab))
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


	Fmono		= 1,
	Fin		= 2,
	Fout		= 4,


	Aclosed		= 0,
	Aread,
	Awrite,

	Vaudio		= 0,
	Vdeemp,
	Nvol,

	Ncmd		= 50,		/* max volume command words */

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

struct	Buf
{
	uchar*	virt;
	ulong	phys;
	Buf*	next;
};
struct	AQueue
{
	Lock;
	Buf*	first;
	Buf*	last;
};
static	struct
{
	QLock;
	Rendez	vous;
	int	bufinit;	/* boolean if buffers allocated */
	int	curcount;	/* how much data in current buffer */
	int	active;		/* boolean dma running */
	int	intr;		/* boolean an interrupt has happened */
	int	amode;		/* Aclosed/Aread/Awrite for /audio */
	int	rivol[Nvol];		/* right/left input/output volumes */
	int	livol[Nvol];
	int	rovol[Nvol];
	int	lovol[Nvol];
	int	c4cmd;

	Buf	buf[Nbuf];	/* buffers and queues */
	AQueue	empty;
	AQueue	full;
	Buf*	current;
	Buf*	chain;
	Buf*	filling;
} audio;

static	struct
{
	char*	name;
	int	flag;
	int	ilval;
	int	irval;
} volumes[] =
{
[Vaudio]	{"audio",		Fmono|Fout,	50,	50,},
[Vdeemp]	{"deemp",		Fmono|Fout,	0,	0,},
	0
};

static	char	Emode[]		= "illegal open mode";
static	char	Evolume[]	= "illegal volume specifier";

static	Lock	blaster;

static	void	swab(uchar*, int);

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

static void
setvolume(int tag, int isin, int isleft, int val)
{
	if(!isleft || isin)
		return;
	if(tag == Vaudio){
		val = val*31/100;
		if(val < 0)
			val = 0;
		if(val > 31)
			val = 31;
		audio.c4cmd |= (1<<4);
		if(val != 0)
			audio.c4cmd &= ~(1<<4);
		monxmit(0xc4, audio.c4cmd<<24);
		monxmit(0xc2, (0xdf-val)<<24);
	}
	if(tag == Vdeemp){
		audio.c4cmd |= (1<<3);
		if(val == 0)
			audio.c4cmd &= ~(1<<3);
		monxmit(0xc4, audio.c4cmd<<24);
	}
}

static	Buf*
getbuf(AQueue *q)
{
	Buf *b;

	ilock(q);
	b = q->first;
	if(b)
		q->first = b->next;
	iunlock(q);

	return b;
}

static	void
putbuf(AQueue *q, Buf *b)
{

	ilock(q);
	b->next = 0;
	if(q->first)
		q->last->next = b;
	else
		q->first = b;
	q->last = b;
	iunlock(q);
}

static	void
setempty(void)
{
	int i;

	ilock(&blaster);
	audio.empty.first = 0;
	audio.empty.last = 0;
	audio.full.first = 0;
	audio.full.last = 0;
	audio.current = 0;
	audio.filling = 0;
	for(i=0; i<Nbuf; i++)
		putbuf(&audio.empty, &audio.buf[i]);
	iunlock(&blaster);
}

/*
 * if audio is stopped,
 * start it up again.
 */
static	void
pokeaudio(void)
{
	Buf *b1, *b2;

	if(audio.active)
		return;
	ilock(&blaster);
	b1 = audio.current;
	if(b1)
		putbuf(&audio.empty, b1);
	audio.current = 0;
	b1 = audio.chain;
	if(b1 == 0){
		b1 = getbuf(&audio.full);
		audio.chain = b1;
	}
	b2 = getbuf(&audio.full);
	if(b2 == 0){
		iunlock(&blaster);
		return;
	}

	audio.chain = b2;
	audio.current = b1;

	audio.active = 1;
	CSR = 0;
	CSR = Outcreset|Outinitbuff;
	DMA1L = b1->phys+Bufsize;
	DMA1B = b1->phys;
	DMA2L = b2->phys+Bufsize;
	DMA2B = b2->phys;
	CSR = Outsetenable|Outsetchain;
	iunlock(&blaster);
}

void
audiodmaintr(void)
{
	long csr;
	Buf *b;

	ilock(&blaster);
	csr = CSR;
	audio.intr = 1;

	b = audio.current;
	if(b)
		putbuf(&audio.empty, b);

	/*
	 * we took the last interrupt
	 * without a chain.
	 * just shut down.
	 */
	if((csr & Outenable) == 0) {
		CSR = Outcreset | Outclrcint;
		audio.current = 0;
		audio.active = 0;
		iunlock(&blaster);
		wakeup(&audio.vous);
		return;
	}

	b = audio.chain;
	audio.current = b;
	b = getbuf(&audio.full);
	audio.chain = b;
	if(b) {
		/*
		 * we have another full buffer,
		 * chain it on the end.
		 */
		DMA2L = b->phys+Bufsize;
		DMA2B = b->phys;
		CSR = Outsetchain | Outclrcint;
	}else{
		/*
		 * there is no full buffer,
		 * just clear the interrupt
		 * and let the last buffer go out
		 */
		CSR = Outclrcint;
	}
	iunlock(&blaster);
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
/*		print("audio timeout\n");	/**/
		audio.active = 0;
		pokeaudio();
	}
}

static void
audiobufinit(void)
{
	int i;
	ulong l, p;

	/* sound out enable, analog out enable */
	monxmit(0x0f, 0);

	for(i=0; i<Nbuf; i++) {
		p = (ulong)xspanalloc(Bufsize, BY2PG, 0);
		audio.buf[i].virt = (uchar*)kmappa(p);
		audio.buf[i].phys = (ulong)p;
		for(l = BY2PG; l < Bufsize; l += BY2PG)
			kmappa(p+l);
	}
}

static void
setvolumes(void)
{
	int v;

	for(v=0; volumes[v].name; v++){
		setvolume(v, 1, 1, audio.livol[v]);
		setvolume(v, 0, 1, audio.lovol[v]);
		if(volumes[v].flag != Fmono){
			setvolume(v, 1, 0, audio.rivol[v]);
			setvolume(v, 0, 0, audio.rovol[v]);
		}
	}
}

static	void
resetlevel(void)
{
	int i;

	for(i=0; volumes[i].name; i++) {
		audio.lovol[i] = volumes[i].ilval;
		audio.rovol[i] = volumes[i].irval;
		audio.livol[i] = volumes[i].ilval;
		audio.rivol[i] = volumes[i].irval;
	}
}

void
audioreset(void)
{
}

void
audioinit(void)
{
	resetlevel();
	setvolumes();
	audio.amode = Aclosed;
}

Chan*
audioattach(char *param)
{
	return devattach('A', param);
}

Chan*
audioclone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
audiowalk(Chan *c, char *name)
{
	return devwalk(c, name, audiodir, NAUDIO, devgen);
}

void
audiostat(Chan *c, char *db)
{
	devstat(c, db, audiodir, NAUDIO, devgen);
}

Chan*
audioopen(Chan *c, int omode)
{
	int amode;

	switch(c->qid.path & ~CHDIR) {
	default:
		error(Eperm);
		break;

	case Qvolume:
	case Qdir:
		break;

	case Qaudio:
		amode = Awrite;
		if((omode&7) == OREAD)
			amode = Aread;
		qlock(&audio);
		if(audio.amode != Aclosed){
			qunlock(&audio);
			error(Einuse);
		}
		if(audio.bufinit == 0) {
			audio.bufinit = 1;
			audiobufinit();
		}
		audio.amode = amode;
		setempty();
		audio.curcount = 0;
		qunlock(&audio);
		break;
	}
	c = devopen(c, omode, audiodir, NAUDIO, devgen);
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
		if(c->flag & COPEN) {
			qlock(&audio);
			audio.amode = Aclosed;
			if(waserror()){
				qunlock(&audio);
				nexterror();
			}
			while(audio.active)
				waitaudio();
			setempty();
			poperror();
			ilock(&blaster);
			CSR = 0;
			CSR = Outcreset|Outinitbuff;
			iunlock(&blaster);
			qunlock(&audio);
		}
		break;
	}
}

long
audioread(Chan *c, char *a, long n, ulong offset)
{
	int liv, riv, lov, rov;
	long m;
	char buf[300];
	int j;

	switch(c->qid.path & ~CHDIR) {
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, a, n, audiodir, NAUDIO, devgen);

	case Qaudio:
		break;

	case Qvolume:
		j = 0;
		buf[0] = 0;
		for(m=0; volumes[m].name; m++){
			liv = audio.livol[m];
			riv = audio.rivol[m];
			lov = audio.lovol[m];
			rov = audio.rovol[m];
			j += snprint(buf+j, sizeof(buf)-j, "%s", volumes[m].name);
			if((volumes[m].flag & Fmono) || liv==riv && lov==rov){
				if((volumes[m].flag&(Fin|Fout))==(Fin|Fout) && liv==lov)
					j += snprint(buf+j, sizeof(buf)-j, " %d", liv);
				else{
					if(volumes[m].flag & Fin)
						j += snprint(buf+j, sizeof(buf)-j, " in %d", liv);
					if(volumes[m].flag & Fout)
						j += snprint(buf+j, sizeof(buf)-j, " out %d", lov);
				}
			}else{
				if((volumes[m].flag&(Fin|Fout))==(Fin|Fout) && liv==lov && riv==rov)
					j += snprint(buf+j, sizeof(buf)-j, " left %d right %d",
						liv, riv);
				else{
					if(volumes[m].flag & Fin)
						j += snprint(buf+j, sizeof(buf)-j, " in left %d right %d",
							liv, riv);
					if(volumes[m].flag & Fout)
						j += snprint(buf+j, sizeof(buf)-j, " out left %d right %d",
							lov, rov);
				}
			}
			j += snprint(buf+j, sizeof(buf)-j, "\n");
		}

		return readstr(offset, a, n, buf);
	}
	return 0;
}

long
audiowrite(Chan *c, char *a, long n, ulong offset)
{
	long m, n0;
	int i, nf, v, left, right, in, out;
	char buf[255], *field[Ncmd];
	Buf *b;

	USED(offset);

	n0 = n;
	switch(c->qid.path & ~CHDIR) {
	default:
		error(Eperm);
		break;

	case Qvolume:
		v = Vaudio;
		left = 1;
		right = 1;
		in = 1;
		out = 1;
		if(n > sizeof(buf)-1)
			n = sizeof(buf)-1;
		memmove(buf, a, n);
		buf[n] = '\0';

		nf = getfields(buf, field, Ncmd, " \t\n");
		for(i = 0; i < nf; i++){
			/*
			 * a number is volume
			 */
			if(field[i][0] >= '0' && field[i][0] <= '9') {
				m = strtoul(field[i], 0, 10);
				if(left && out)
					audio.lovol[v] = m;
				if(left && in)
					audio.livol[v] = m;
				if(right && out)
					audio.rovol[v] = m;
				if(right && in)
					audio.rivol[v] = m;
				setvolumes();
				goto cont0;
			}

			for(m=0; volumes[m].name; m++) {
				if(strcmp(field[i], volumes[m].name) == 0) {
					v = m;
					in = 1;
					out = 1;
					left = 1;
					right = 1;
					goto cont0;
				}
			}

			if(strcmp(field[i], "reset") == 0) {
				resetlevel();
				setvolumes();
				goto cont0;
			}
			if(strcmp(field[i], "in") == 0) {
				in = 1;
				out = 0;
				goto cont0;
			}
			if(strcmp(field[i], "out") == 0) {
				in = 0;
				out = 1;
				goto cont0;
			}
			if(strcmp(field[i], "left") == 0) {
				left = 1;
				right = 0;
				goto cont0;
			}
			if(strcmp(field[i], "right") == 0) {
				left = 0;
				right = 1;
				goto cont0;
			}
			error(Evolume);
			break;
		cont0:;
		}
		return n;

	case Qaudio:
		if(audio.amode != Awrite)
			error(Emode);
		qlock(&audio);
		if(waserror()){
			qunlock(&audio);
			nexterror();
		}
		while(n > 0) {
			b = audio.filling;
			if(b == 0) {
				b = getbuf(&audio.empty);
				if(b == 0) {
					waitaudio();
					continue;
				}
				audio.filling = b;
				audio.curcount = 0;
			}

			m = Bufsize-audio.curcount;
			if(m > n)
				m = n;
			memmove(b->virt+audio.curcount, a, m);

			audio.curcount += m;
			n -= m;
			a += m;
			if(audio.curcount >= Bufsize) {
				audio.filling = 0;
				swab(b->virt, Bufsize);
				putbuf(&audio.full, b);
			}
		}
		poperror();
		qunlock(&audio);
		break;
	}
	return n0 - n;
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

static	void
swab(uchar *a, int n)
{
	ulong *p, b;

	p = (ulong*)a;
	while(n >= 4) {
		b = *p;
		b = (b>>24) | (b<<24) |
			((b&0xff0000) >> 8) |
			((b&0x00ff00) << 8);
		*p++ = b;
		n -= 4;
	}
}
