/*
 *	SB 16 driver
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"devtab.h"
#include	"io.h"
#include	"audio.h"

#define	NPORT		(sizeof audiodir/sizeof(Dirtab))

typedef struct	AQueue	AQueue;
typedef struct	Buf	Buf;

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
	Vsynth,
	Vcd,
	Vline,
	Vmic,
	Vspeaker,
	Vtreb,
	Vbass,
	Vspeed,
	Nvol,

	Speed		= 44100,
	Ncmd		= 50,		/* max volume command words */
};

Dirtab
audiodir[] =
{
	"audio",	{Qaudio},		0,	0666,
	"volume",	{Qvolume},		0,	0666,
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
	int	major;		/* SB16 major version number (sb 4) */
	int	minor;		/* SB16 minor version number */

	Buf	buf[Nbuf];	/* buffers and queues */
	AQueue	empty;
	AQueue	full;
	Buf*	current;
	Buf*	filling;
} audio;

static	struct
{
	char*	name;
	int	flag;
	int	ilval;		/* initial values */
	int	irval;
} volumes[] =
{
	[Vaudio]	"audio",	Fout, 		50,	50,
	[Vsynth]	"synth",	Fin|Fout,	0,	0,
	[Vcd]		"cd",		Fin|Fout,	0,	0,
	[Vline]		"line",		Fin|Fout,	0,	0,
	[Vmic]		"mic",		Fin|Fout|Fmono,	0,	0,
	[Vspeaker]	"speaker",	Fout|Fmono,	0,	0,

	[Vtreb]		"treb",		Fout, 		50,	50,
	[Vbass]		"bass",		Fout, 		50,	50,

	[Vspeed]	"speed",	Fin|Fout|Fmono,	Speed,	Speed,
	0
};

static struct
{
	Lock;
	int	reset;		/* io ports to the sound blaster */
	int	read;
	int	write;
	int	wstatus;
	int	rstatus;
	int	mixaddr;
	int	mixdata;
	int	clri8;
	int	clri16;
	int	clri401;
	int	dma;
} blaster;

static	void	swab(uchar*);

static	char	Emajor[]	= "soundblaster not responding/wrong version";
static	char	Emode[]		= "illegal open mode";
static	char	Evolume[]	= "illegal volume specifier";

static	int
sbcmd(int val)
{
	int i, s;

	for(i=1<<16; i!=0; i--) {
		s = inb(blaster.wstatus);
		if((s & 0x80) == 0) {
			outb(blaster.write, val);
			return 0;
		}
	}
	return 1;
}

static	int
sbread(void)
{
	int i, s;

	for(i=1<<16; i!=0; i--) {
		s = inb(blaster.rstatus);
		if((s & 0x80) != 0) {
			return inb(blaster.read);
		}
	}
	return 0xbb;
}

static	int
mxcmd(int addr, int val)
{

	outb(blaster.mixaddr, addr);
	outb(blaster.mixdata, val);
	return 1;
}

static	int
mxread(int addr)
{
	int s;

	outb(blaster.mixaddr, addr);
	s = inb(blaster.mixdata);
	return s;
}

static	void
mxcmds(int s, int v)
{

	if(v > 100)
		v = 100;
	if(v < 0)
		v = 0;
	mxcmd(s, (v*255)/100);
}

static	void
mxcmdt(int s, int v)
{

	if(v > 100)
		v = 100;
	if(v <= 0)
		mxcmd(s, 0);
	else
		mxcmd(s, 255-100+v);
}

static	void
mxcmdu(int s, int v)
{

	if(v > 100)
		v = 100;
	if(v <= 0)
		v = 0;
	mxcmd(s, 128-50+v);
}

static	void
mxvolume(void)
{
	int *left, *right;
	int source;

	if(audio.amode == Aread){
		left = audio.livol;
		right = audio.rivol;
	}else{
		left = audio.lovol;
		right = audio.rovol;
	}

	ilock(&blaster);

	mxcmd(0x30, 255);		/* left master */
	mxcmd(0x31, 255);		/* right master */
	mxcmd(0x3f, 0);			/* left igain */
	mxcmd(0x40, 0);			/* right igain */
	mxcmd(0x41, 0);			/* left ogain */
	mxcmd(0x42, 0);			/* right ogain */

	mxcmds(0x32, left[Vaudio]);
	mxcmds(0x33, right[Vaudio]);

	mxcmds(0x34, left[Vsynth]);
	mxcmds(0x35, right[Vsynth]);

	mxcmds(0x36, left[Vcd]);
	mxcmds(0x37, right[Vcd]);

	mxcmds(0x38, left[Vline]);
	mxcmds(0x39, right[Vline]);

	mxcmds(0x3a, left[Vmic]);
	mxcmds(0x3b, left[Vspeaker]);

	mxcmdu(0x44, left[Vtreb]);
	mxcmdu(0x45, right[Vtreb]);

	mxcmdu(0x46, left[Vbass]);
	mxcmdu(0x47, right[Vbass]);

	source = 0;
	if(left[Vsynth])
		source |= 1<<6;
	if(right[Vsynth])
		source |= 1<<5;
	if(left[Vaudio])
		source |= 1<<4;
	if(right[Vaudio])
		source |= 1<<3;
	if(left[Vcd])
		source |= 1<<2;
	if(right[Vcd])
		source |= 1<<1;
	if(left[Vmic])
		source |= 1<<0;
	if(audio.amode == Aread)
		mxcmd(0x3c, 0);		/* output switch */
	else
		mxcmd(0x3c, source);
	mxcmd(0x3d, source);		/* input left switch */
	mxcmd(0x3e, source);		/* input right switch */
	iunlock(&blaster);
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

/*
 * move the dma to the next buffer
 */
static	void
contindma(void)
{
	Buf *b;

	if(!audio.active)
		goto shutdown;

	b = audio.current;
	if(audio.amode == Aread) {
		if(b)	/* shouldnt happen */
			putbuf(&audio.full, b);
		b = getbuf(&audio.empty);
	} else {
		if(b)	/* shouldnt happen */
			putbuf(&audio.empty, b);
		b = getbuf(&audio.full);
	}
	audio.current = b;
	if(b == 0)
		goto shutdown;

	dmasetup(blaster.dma, b->virt, Bufsize, audio.amode == Aread);
	return;

shutdown:
	dmaend(blaster.dma);
	sbcmd(0xd9);				/* exit at end of count */
	sbcmd(0xd5);				/* pause */
	audio.curcount = 0;
	audio.active = 0;
}

/*
 * cause sb to get an interrupt per buffer.
 * start first dma
 */
static	void
startdma(void)
{
	ulong count;
	int speed;

	ilock(&blaster);
	dmaend(blaster.dma);
	if(audio.amode == Aread) {
		sbcmd(0x42);			/* input sampling rate */
		speed = audio.livol[Vspeed];
	} else {
		sbcmd(0x41);			/* output sampling rate */
		speed = audio.lovol[Vspeed];
	}
	sbcmd(speed>>8);
	sbcmd(speed);

	count = (Bufsize >> 1) - 1;
	if(audio.amode == Aread)
		sbcmd(0xbe);			/* A/D, autoinit */
	else
		sbcmd(0xb6);			/* D/A, autoinit */
	sbcmd(0x30);				/* stereo, 16 bit */
	sbcmd(count);
	sbcmd(count>>8);

	audio.active = 1;
	contindma();
	iunlock(&blaster);
}

/*
 * if audio is stopped,
 * start it up again.
 */
static	void
pokeaudio(void)
{
	if(!audio.active)
		startdma();
}

void
audiosbintr(void)
{
	int stat, dummy;

	stat = mxread(0x82) & 7;		/* get irq status */
	if(stat) {
		dummy = 0;
		if(stat & 2) {
			ilock(&blaster);
			dummy = inb(blaster.clri16);
			contindma();
			iunlock(&blaster);
			audio.intr = 1;
			wakeup(&audio.vous);
		}
		if(stat & 1) {
			dummy = inb(blaster.clri8);
		}
		if(stat & 4) {
			dummy = inb(blaster.clri401);
		}
		USED(dummy);
	}
}

void
pcaudiosbintr(Ureg *ureg, void *rock)
{
	USED(ureg, rock);
	audiosbintr();
}

void
audiodmaintr(void)
{
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
		audio.active = 0;
		pokeaudio();
	}
}

static void
sbbufinit(void)
{
	int i;
	void *p;

	for(i=0; i<Nbuf; i++) {
		p = xspanalloc(Bufsize, CACHELINESZ, 64*1024);
		audio.buf[i].virt = UNCACHED(uchar, p);
		audio.buf[i].phys = (ulong)PADDR(p);
	}
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

void
audioreset(void)
{
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
audioinit(void)
{
	ISAConf sbconf;
	int i;

	sbconf.port = 0x220;
	sbconf.dma = 5;
	sbconf.irq = 7;
	if(isaconfig("audio", 0, &sbconf) == 0)
		return;
	if(strcmp(sbconf.type, "sb16") != 0)
		return;

	switch(sbconf.port){
	case 0x220:
	case 0x240:
	case 0x260:
	case 0x280:
		break;
	default:
		print("devaudio: bad sb16 port 0x%x\n", sbconf.port);
		return;
	}
	switch(sbconf.irq){
	case 2:
	case 5:
	case 7:
	case 10:
		break;
	default:
		print("devaudio: bad sb16 irq %d\n", sbconf.irq);
		return;
	}

	blaster.dma = sbconf.dma;

	blaster.reset = sbconf.port + 0x6;
	blaster.read = sbconf.port + 0xa;
	blaster.write = sbconf.port + 0xc;
	blaster.wstatus = sbconf.port + 0xc;
	blaster.rstatus = sbconf.port + 0xe;
	blaster.mixaddr = sbconf.port + 0x4;
	blaster.mixdata = sbconf.port + 0x5;
	blaster.clri8 = sbconf.port + 0xe;
	blaster.clri16 = sbconf.port + 0xf;
	blaster.clri401 = sbconf.port + 0x100;

	seteisadma(blaster.dma, audiodmaintr);
	setvec(Int0vec+sbconf.irq, pcaudiosbintr, 0);

	audio.amode = Aclosed;
	resetlevel();

	outb(blaster.reset, 1);
	delay(1);			/* >3 υs */
	outb(blaster.reset, 0);
	delay(1);

	i = sbread();
	if(i != 0xaa) {
		print("sound blaster didnt respond #%.2x\n", i);
		return;
	}

	sbcmd(0xe1);			/* get version */
	audio.major = sbread();
	audio.minor = sbread();

	if(audio.major != 4) {
		print("bad soundblaster model #%.2x #%.2x; not SB 16\n", audio.major, audio.minor);
		return;
	}
	/*
	 * initialize the mixer
	 */
	mxcmd(0x00, 0);			/* Reset mixer */
	mxvolume();

	/*
	 * set up irq/dma chans
	 */
	mxcmd(0x80,			/* irq */
		(sbconf.irq==2)? 1:
		(sbconf.irq==5)? 2:
		(sbconf.irq==7)? 4:
		(sbconf.irq==10)? 8:
		0);
	mxcmd(0x81, 1<<blaster.dma);	/* dma */
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
	int amode;

	if(audio.major != 4)
		error(Emajor);

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
			sbbufinit();
		}
		audio.amode = amode;
		setempty();
		audio.curcount = 0;
		qunlock(&audio);
		mxvolume();
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
			qunlock(&audio);
		}
		break;
	}
}

long
audioread(Chan *c, char *a, long n, ulong offset)
{
	int liv, riv, lov, rov;
	long m, n0;
	char buf[300];
	Buf *b;
	int j;

	n0 = n;
	switch(c->qid.path & ~CHDIR) {
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, a, n, audiodir, NPORT, devgen);

	case Qaudio:
		if(audio.amode != Aread)
			error(Emode);
		qlock(&audio);
		if(waserror()){
			qunlock(&audio);
			nexterror();
		}
		while(n > 0) {
			b = audio.filling;
			if(b == 0) {
				b = getbuf(&audio.full);
				if(b == 0) {
					waitaudio();
					continue;
				}
				audio.filling = b;
				swab(b->virt);
				audio.curcount = 0;
			}
			m = Bufsize-audio.curcount;
			if(m > n)
				m = n;
			memmove(a, b->virt+audio.curcount, m);

			audio.curcount += m;
			n -= m;
			a += m;
			if(audio.curcount >= Bufsize) {
				audio.filling = 0;
				putbuf(&audio.empty, b);
			}
		}
		poperror();
		qunlock(&audio);
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
	return n0-n;
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
				mxvolume();
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
				mxvolume();
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
		break;

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
				swab(b->virt);
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
swab(uchar *a)
{
	ulong *p, *ep, b;

	if(!SBswab)
		return;
	p = (ulong*)a;
	ep = p + (Bufsize>>2);
	while(p < ep) {
		b = *p;
		b = (b>>24) | (b<<24) |
			((b&0xff0000) >> 8) |
			((b&0x00ff00) << 8);
		*p++ = b;
	}
}
