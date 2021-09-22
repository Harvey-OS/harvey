/*
 *	SB 16 driver
 */
#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"
#include	"io.h"
#include	"audio.h"

typedef struct	AQueue	AQueue;
typedef struct	Buf	Buf;

enum
{
	Qdir		= 0,
	Qaudio,
	Qvolume,
	Qstatus,

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
	".",	{Qdir, 0, QTDIR},		0,	DMDIR|0555,
	"audio",	{Qaudio},		0,	0666,
	"volume",	{Qvolume},		0,	0666,
	"audiostat",{Qstatus},		0,	0444,
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
	int	buffered;		/* number of bytes en route */
	int	bufinit;		/* boolean if buffers allocated */
	int	curcount;		/* how much data in current buffer */
	int	active;		/* boolean dma running */
	int	intr;			/* boolean an interrupt has happened */
	int	amode;		/* Aclosed/Aread/Awrite for /audio */
	int	rivol[Nvol];	/* right/left input/output volumes */
	int	livol[Nvol];
	int	rovol[Nvol];
	int	lovol[Nvol];
	int	major;		/* SB16 major version number (sb 4) */
	int	minor;		/* SB16 minor version number */
	ulong	totcount;	/* how many bytes processed since open */
	vlong	tottime;	/* time at which totcount bytes were processed */

	Buf	buf[Nbuf];		/* buffers and queues */
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
[Vaudio]		"audio",	Fout, 		50,	50,
[Vsynth]		"synth",	Fin|Fout,	0,	0,
[Vcd]		"cd",		Fin|Fout,	0,	0,
[Vline]		"line",	Fin|Fout,	0,	0,
[Vmic]		"mic",	Fin|Fout|Fmono,	0,	0,
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

	void	(*startdma)(void);
	void	(*intr)(void);
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
/*	print("#A: sbcmd (%#.2x) timeout\n", val);	/**/
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
/*	print("#A: sbread did not respond\n");	/**/
	return -1;
}

static int
ess1688w(int reg, int val)
{
	if(sbcmd(reg) || sbcmd(val))
		return 1;

	return 0;
}

static int
ess1688r(int reg)
{
	if(sbcmd(0xC0) || sbcmd(reg))
		return -1;

	return sbread();
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
	mxcmd(0x3f, 0);		/* left igain */
	mxcmd(0x40, 0);		/* right igain */
	mxcmd(0x41, 0);		/* left ogain */
	mxcmd(0x42, 0);		/* right ogain */

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
	if(b){
		audio.totcount += Bufsize;
		audio.tottime = todget(nil);
	}
	if(audio.amode == Aread) {
		if(b){
			putbuf(&audio.full, b);
			audio.buffered += Bufsize;
		}
		b = getbuf(&audio.empty);
	} else {
		if(b){
			putbuf(&audio.empty, b);
			audio.buffered -= Bufsize;
		}
		b = getbuf(&audio.full);
	}
	audio.current = b;
	if(b == 0)
		goto shutdown;

	if(dmasetup(blaster.dma, b->virt, Bufsize, audio.amode == Aread) >= 0)
		return;
	print("#A: dmasetup fail\n");
	putbuf(&audio.empty, b);

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
sb16startdma(void)
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

static int
ess1688reset(void)
{
	int i;

	outb(blaster.reset, 3);
	delay(1);			/* >3 υs */
	outb(blaster.reset, 0);
	delay(1);

	i = sbread();
	if(i != 0xAA) {
		print("#A: no response %#.2x\n", i);
		return 1;
	}

	if(sbcmd(0xC6)){		/* extended mode */
		print("#A: barf 3\n");
		return 1;
	}

	return 0;
}

static	void
ess1688startdma(void)
{
	ulong count;
	int speed, x;

	ilock(&blaster);
	dmaend(blaster.dma);

	if(audio.amode == Awrite)
		ess1688reset();
	if(audio.amode == Aread)
		sbcmd(0xD3);			/* speaker off */

	/*
	 * Set the speed.
	 */
	if(audio.amode == Aread)
		speed = audio.livol[Vspeed];
	else
		speed = audio.lovol[Vspeed];
	if(speed < 4000)
		speed = 4000;
	else if(speed > 48000)
		speed = 48000;

	if(speed > 22000)
		  x = 0x80|(256-(795500+speed/2)/speed);
	else
		  x = 128-(397700+speed/2)/speed;
	ess1688w(0xA1, x & 0xFF);

	speed = (speed * 9) / 20;
	x = 256 - 7160000 / (speed * 82);
	ess1688w(0xA2, x & 0xFF);

	if(audio.amode == Aread)
		ess1688w(0xB8, 0x0E);		/* A/D, autoinit */
	else
		ess1688w(0xB8, 0x04);		/* D/A, autoinit */
	x = ess1688r(0xA8) & ~0x03;
	ess1688w(0xA8, x|0x01);			/* 2 channels */
	ess1688w(0xB9, 2);			/* demand mode, 4 bytes per request */

	if(audio.amode == Awrite)
		ess1688w(0xB6, 0);
	ess1688w(0xB7, 0x71);
	ess1688w(0xB7, 0xBC);

	x = ess1688r(0xB1) & 0x0F;
	ess1688w(0xB1, x|0x50);
	x = ess1688r(0xB2) & 0x0F;
	ess1688w(0xB2, x|0x50);
	if(audio.amode == Awrite)
		sbcmd(0xD1);			/* speaker on */

	count = -Bufsize;
	ess1688w(0xA4, count & 0xFF);
	ess1688w(0xA5, (count>>8) & 0xFF);
	x = ess1688r(0xB8);
	ess1688w(0xB8, x|0x05);

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
		blaster.startdma();
}

static void
sb16intr(void)
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

static void
ess1688intr(void)
{
	int dummy;

	if(audio.active){
		ilock(&blaster);
		contindma();
		dummy = inb(blaster.clri8);
		iunlock(&blaster);
		audio.intr = 1;
		wakeup(&audio.vous);
		USED(dummy);
	}
	else
		print("#A: unexpected ess1688 interrupt\n");
}

void
audiosbintr(void)
{
	/*
	 * Carrera interrupt interface.
	 */
	blaster.intr();
}

static void
pcaudiosbintr(Ureg*, void*)
{
	/*
	 * x86 interrupt interface.
	 */
	blaster.intr();
}

void
audiodmaintr(void)
{
/*	print("#A: dma interrupt\n");	/**/
}

static int
anybuf(void*)
{
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
	tsleep(&audio.vous, anybuf, 0, 10000);
	if(audio.intr == 0) {
/*		print("#A: audio timeout\n");	/**/
		audio.active = 0;
		pokeaudio();
	}
}

static void
sbbufinit(void)
{
	int i;
	uchar *p;

	p = (uchar*)(((ulong)xalloc((Nbuf+1) * Bufsize) + Bufsize-1) &
		~(Bufsize-1));
	if (p == nil)
		panic("sbbufinit: no memory");
	for(i=0; i<Nbuf; i++) {
		dcflush(p, Bufsize);
		audio.buf[i].virt = UNCACHED(uchar, p);
		audio.buf[i].phys = (ulong)PADDR(p);
		p += Bufsize;
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
	audio.buffered = 0;
	for(i=0; i<Nbuf; i++)
		putbuf(&audio.empty, &audio.buf[i]);
	audio.totcount = 0;
	audio.tottime = 0LL;
	iunlock(&blaster);
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

static int
ess1688(ISAConf* sbconf)
{
	int i, major, minor;

	/*
	 * Try for ESS1688.
	 */
	sbcmd(0xE7);			/* get version */
	major = sbread();
	minor = sbread();
	if(major != 0x68 || minor != 0x8B){
		print("#A: model %#.2x %#.2x; not ESS1688 compatible\n", major, minor);
		return 1;
	}

	ess1688reset();

	switch(sbconf->irq){
	case 2:
	case 9:
		i = 0x50|(0<<2);
		break;
	case 5:
		i = 0x50|(1<<2);
		break;
	case 7:
		i = 0x50|(2<<2);
		break;
	case 10:
		i = 0x50|(3<<2);
		break;
	default:
		print("#A: bad ESS1688 irq %d\n", sbconf->irq);
		return 1;
	}
	ess1688w(0xB1, i);

	switch(sbconf->dma){
	case 0:
		i = 0x50|(1<<2);
		break;
	case 1:
		i = 0xF0|(2<<2);
		break;
	case 3:
		i = 0x50|(3<<2);
		break;
	default:
		print("#A: bad ESS1688 dma %lud\n", sbconf->dma);
		return 1;
	}
	ess1688w(0xB2, i);

	ess1688reset();

	blaster.startdma = ess1688startdma;
	blaster.intr = ess1688intr;

	return 0;
}

static void
audioinit(void)
{
	ISAConf sbconf;
	int i, x;
	static int irq[] = {2,5,7,10};

	sbconf.port = 0x220;
	sbconf.dma = Dma;
	sbconf.irq = IrqAUDIO;
	if(isaconfig("audio", 0, &sbconf) == 0)
		return;
	if(sbconf.type == nil ||
		(cistrcmp(sbconf.type, "sb16") != 0 && 
		 cistrcmp(sbconf.type, "ess1688") != 0))
		return;
	switch(sbconf.port){
	case 0x220:
	case 0x240:
	case 0x260:
	case 0x280:
		break;
	default:
		print("#A: bad port %#lux\n", sbconf.port);
		return;
	}

	if(ioalloc(sbconf.port, 0x10, 0, "audio") < 0){
		print("#A: cannot ioalloc range %lux+0x10\n", sbconf.port);
		return;
	}
	if(ioalloc(sbconf.port+0x100, 1, 0, "audio.mpu401") < 0){
		iofree(sbconf.port);
		print("#A: cannot ioalloc range %lux+0x01\n", sbconf.port+0x100);
		return;
	}

	switch(sbconf.irq){
	case 2:
	case 5:
	case 7:
	case 9:
	case 10:
		break;
	default:
		print("#A: bad irq %d\n", sbconf.irq);
		iofree(sbconf.port);
		iofree(sbconf.port+0x100);
		return;
	}

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
	blaster.dma = sbconf.dma;

	blaster.startdma = sb16startdma;
	blaster.intr = sb16intr;

	audio.amode = Aclosed;
	resetlevel();

	outb(blaster.reset, 1);
	delay(1);			/* >3 υs */
	outb(blaster.reset, 0);
	delay(1);

	i = sbread();
	if(i != 0xaa) {
		print("#A: no response #%.2x\n", i);
		iofree(sbconf.port);
		iofree(sbconf.port+0x100);
		return;
	}

	sbcmd(0xe1);			/* get version */
	audio.major = sbread();
	audio.minor = sbread();

	if(audio.major != 4) {
		if(audio.major != 3 || audio.minor != 1 || ess1688(&sbconf)){
			print("#A: model %#.2x %#.2x; not SB 16 compatible\n",
				audio.major, audio.minor);
			iofree(sbconf.port);
			iofree(sbconf.port+0x100);
			return;
		}
		audio.major = 4;
	}

	/*
	 * initialize the mixer
	 */
	mxcmd(0x00, 0);			/* Reset mixer */
	mxvolume();

	/*
	 * Attempt to set IRQ/DMA channels.
	 * On old ISA boards, these registers are writable.
	 * On Plug-n-Play boards, these are read-only.
	 *
	 * To accomodate both, we write to the registers,
	 * but then use the contents in case the write is
	 * disallowed.
	 */
	mxcmd(0x80,			/* irq */
		(sbconf.irq==2)? 1:
		(sbconf.irq==5)? 2:
		(sbconf.irq==7)? 4:
		(sbconf.irq==9)? 1:
		(sbconf.irq==10)? 8:
		0);

	mxcmd(0x81, 1<<blaster.dma);	/* dma */

	x = mxread(0x81);
	for(i=5; i<=7; i++)
		if(x & (1<<i)){
			blaster.dma = i;
			break;
		}

	x = mxread(0x80);
	for(i=0; i<=3; i++)
		if(x & (1<<i)){
			sbconf.irq = irq[i];
			break;
		}

	seteisadma(blaster.dma, audiodmaintr);
	setvec(Int0vec+sbconf.irq, pcaudiosbintr, 0);
}

static Chan*
audioattach(char *param)
{
	return devattach('A', param);
}

static Walkqid*
audiowalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, audiodir, nelem(audiodir), devgen);
}

static int
audiostat(Chan *c, uchar *db, int n)
{
	audiodir[Qaudio].length = audio.buffered;
	return devstat(c, db, n, audiodir, nelem(audiodir), devgen);
}

static Chan*
audioopen(Chan *c, int omode)
{
	int amode;

	if(audio.major != 4)
		error(Emajor);

	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qstatus:
		if((omode&7) != OREAD)
			error(Eperm);
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
	c = devopen(c, omode, audiodir, nelem(audiodir), devgen);
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;

	return c;
}

static void
audioclose(Chan *c)
{
	Buf *b;

	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qdir:
	case Qvolume:
	case Qstatus:
		break;

	case Qaudio:
		if(c->flag & COPEN) {
			qlock(&audio);
			if(audio.amode == Awrite) {
				/* flush out last partial buffer */
				b = audio.filling;
				if(b) {
					audio.filling = 0;
					memset(b->virt+audio.curcount, 0, Bufsize-audio.curcount);
					audio.buffered += Bufsize-audio.curcount;
					swab(b->virt);
					putbuf(&audio.full, b);
				}
				if(!audio.active && audio.full.first)
					pokeaudio();
			}
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

static long
audioread(Chan *c, void *v, long n, vlong off)
{
	int liv, riv, lov, rov;
	long m, n0;
	char buf[300];
	Buf *b;
	int j;
	ulong offset = off;
	char *a;

	n0 = n;
	a = v;
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, a, n, audiodir, nelem(audiodir), devgen);

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
			audio.buffered -= m;
			if(audio.curcount >= Bufsize) {
				audio.filling = 0;
				putbuf(&audio.empty, b);
			}
		}
		poperror();
		qunlock(&audio);
		break;

	case Qstatus:
		buf[0] = 0;
		snprint(buf, sizeof(buf), "bufsize %6d buffered %6d offset  %10lud time %19lld\n",
			Bufsize, audio.buffered, audio.totcount, audio.tottime);
		return readstr(offset, a, n, buf);

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
						j += snprint(buf+j, sizeof(buf)-j,
							" in %d", liv);
					if(volumes[m].flag & Fout)
						j += snprint(buf+j, sizeof(buf)-j,
							" out %d", lov);
				}
			}else{
				if((volumes[m].flag&(Fin|Fout))==(Fin|Fout) &&
				    liv==lov && riv==rov)
					j += snprint(buf+j, sizeof(buf)-j,
						" left %d right %d",
						liv, riv);
				else{
					if(volumes[m].flag & Fin)
						j += snprint(buf+j, sizeof(buf)-j,
							" in left %d right %d",
							liv, riv);
					if(volumes[m].flag & Fout)
						j += snprint(buf+j, sizeof(buf)-j,
							" out left %d right %d",
							lov, rov);
				}
			}
			j += snprint(buf+j, sizeof(buf)-j, "\n");
		}
		return readstr(offset, a, n, buf);
	}
	return n0-n;
}

static long
audiowrite(Chan *c, void *vp, long n, vlong)
{
	long m, n0;
	int i, v, left, right, in, out;
	Cmdbuf *cb;
	Buf *b;
	char *a;

	a = vp;
	n0 = n;
	switch((ulong)c->qid.path) {
	default:
		error(Eperm);
		break;

	case Qvolume:
		v = Vaudio;
		left = 1;
		right = 1;
		in = 1;
		out = 1;
		cb = parsecmd(vp, n);
		if(waserror()){
			free(cb);
			nexterror();
		}

		for(i = 0; i < cb->nf; i++){
			/*
			 * a number is volume
			 */
			if(cb->f[i][0] >= '0' && cb->f[i][0] <= '9') {
				m = strtoul(cb->f[i], 0, 10);
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
				if(strcmp(cb->f[i], volumes[m].name) == 0) {
					v = m;
					in = 1;
					out = 1;
					left = 1;
					right = 1;
					goto cont0;
				}
			}

			if(strcmp(cb->f[i], "reset") == 0) {
				resetlevel();
				mxvolume();
				goto cont0;
			}
			if(strcmp(cb->f[i], "in") == 0) {
				in = 1;
				out = 0;
				goto cont0;
			}
			if(strcmp(cb->f[i], "out") == 0) {
				in = 0;
				out = 1;
				goto cont0;
			}
			if(strcmp(cb->f[i], "left") == 0) {
				left = 1;
				right = 0;
				goto cont0;
			}
			if(strcmp(cb->f[i], "right") == 0) {
				left = 0;
				right = 1;
				goto cont0;
			}
			error(Evolume);
			break;
		cont0:;
		}
		free(cb);
		poperror();
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
			audio.buffered += m;
			if(audio.curcount >= Bufsize) {
				audio.filling = 0;
				swab(b->virt);
				putbuf(&audio.full, b);
				pokeaudio();
			}
		}
		poperror();
		qunlock(&audio);
		break;
	}
	return n0 - n;
}

static	void
swab(uchar *a)
{
	ulong *p, *ep, b;

	if(!SBswab){
		USED(a);
		return;
	}
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

Dev audiodevtab = {
	'A',
	"audio",

	devreset,
	audioinit,
	devshutdown,
	audioattach,
	audiowalk,
	audiostat,
	audioopen,
	devcreate,
	audioclose,
	audioread,
	devbread,
	audiowrite,
	devbwrite,
	devremove,
	devwstat,
};
