#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

int	dspdebug = 0;

#define DPRINT 	if(dspdebug)kprint

typedef struct	Dspdev	Dspdev;
typedef struct	Rbh	Rbh;
typedef struct	Rbuf	Rbuf;

struct Rbh
{
	ulong	head;
	ulong	tail;
	ulong	intreq;
	ulong	level;
};

struct Rbuf
{
	Rbuf	*next;
	Rbh	*h;
	ulong	*data;
	int	ndata;
	int	type;
	int	bufid;
	int	dspid;
	QLock;
	Rendez;
};

enum
{			/* type */
	Read=	0,
	Write=	1,
	Folded=	2,
	Extend=	4,
};

struct Dspdev
{
	uchar	fill0[0x180];	/* 0000 */
	ulong	dmawdcnt;	/* 0180 DMA transfer size (SRAM words) */
	ulong	gaddrlo;	/* 0184 GIO-bus address, LSB (16 bit) */
	ulong	gaddrhi;	/* 0188 GIO-bus address, MSB (16 bit) */
	ulong	paddr;		/* 018c PBUS address (16 bit) */
	ulong	dmactrl;	/* 0190 DMA Control (2 bit) */
	ulong	counter;	/* 0194 Counter (24 bits) (ro) */
	ulong	handtx;		/* 0198 Handshake transmit (16 bit) */
	ulong	handrx;		/* 019c Handshake receive (16 bit) */
	ulong	cintstat;	/* 01a0 CPU Interrupt status (3 bit) */
	ulong	cintmask;	/* 01a4 CPU Interrupt mask (3 bit) */
	ulong	fill1[2];	/* 01a8 */
	ulong	miscsr;		/* 01b0 Misc. control and status (8 bit) */
	ulong	burstctl;	/* 01b4 DMA Ballistics register (16 bit) */
};

enum
{			/* dmactrl */
	Dmafold=	0x04,		/* mode: 0 = normal, 1 = folded */
	Dmaread=	0x02,		/* direction: 0 = MIPS->DSP */
	Dmastart=	0x01,		/* start dma */

			/* cintstat / cintmask */
	Handrx=		0x04,		/* dsp has read HANDRX */
	Handtx=		0x02,		/* dsp has written to HANDTX */
	Dmadone=	0x01,		/* end of dma */

			/* miscsr */
	P8=		0x70,		/* PBus 8 -> GIO 32 signed */
	P16=		0x60,		/* PBus 16 -> GIO 32 signed */
	P24=		0x40,		/* PBus 24 -> GIO 32 signed */
	Sram32K=	0x08,		/* SRAM size (1=32K, 0=8K) */
	Irqpol=		0x04,		/* polarity of IRQA (1 = high) */
	Irq=		0x02,		/* force /IRQA with polarity above */
	Reset=		0x01,		/* force hard reset to DSP */
};

/*	The values of Psource, ..., Prvol,
 *	are known to the dsp microcode.
 */
enum
{
	Psource = 0,
	Platten,
	Pratten,
	Pispeed,
	Pospeed,
	Plvol,
	Prvol,
	Nparms,
	Preset = Nparms,
	Pstart,
	Pinit,
};

enum
{
	Acquire = 4,
	Free = 5,
	Setparms = 7,

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
	Vaes,
	Vmic,
	Vline,
	Vspeed,
	Nvol,

	Speed		= 44100,
	Ncmd		= 50,		/* max volume command words */
};

Dirtab audiodir[]={
	"audio",	{Qaudio},	0,	0666,
	"volume",	{Qvolume},	0,	0666,
};

static	struct
{
	QLock;
	Rendez	vous;
	int	started;		/* dsp rolling */
	int	amode;		/* Aclosed/Aread/Awrite for /audio */
	int	rivol[Nvol];		/* right/left input/output volumes */
	int	livol[Nvol];
	int	rovol[Nvol];
	int	lovol[Nvol];

	Rbuf	cmd;
	Rbuf	reply;
	Rbuf	in;
	Rbuf	out;
} audio;

static	struct
{
	char*	name;
	int	flag;
	int	ilval;		/* initial values */
	int	irval;
} volumes[] =
{
[Vaudio]		"audio",		Fout, 		50,	50,
[Vline]		"line",		Fin,		0,	0,
[Vmic]		"mic",		Fin,		0,	0,
[Vaes]		"aes",		Fin,		0,	0,

[Vspeed]		"speed",		Fin|Fout|Fmono,	Speed,	Speed,
	0
};

#define	NAUDIO	(sizeof audiodir/sizeof(Dirtab))

extern	uchar	dspprog[];
static	char	Evolume[]	= "illegal volume specifier";

#define	CMDSIZE		300
#define	REPLYSIZE	20
#define	DATASIZE	65000

#define	DEV	IO(Dspdev, HPC_0_ID_ADDR)
#define	MEM	IO(ulong, HPC1MEMORY)

static	void	setvolumes(void);
static	void	resetlevel(void);
static	void	dspacquire(Rbuf*);
static	void	dspdrain(Rbuf*);
static	void	dspfree(Rbuf*);
static	void	dspinit(void);
static	int	dspraudio(Rbuf*, uchar*, int);
static	int	dsprcode(int);
static	int	dspread(Rbuf*, ulong*, int);
static	void	dspreset(void);
static	void	dsprpc(ulong*, int);
static	void	dspstart(void);
static	void	dspwaudio(Rbuf*, uchar*, int);
static	void	dspwrite(Rbuf*, ulong*, int);

static	Rbuf *	rblist;

static void
rbufalloc(Rbuf *rb, ulong ndata, int type)
{
	Rbh *r;
	static int count;
	ulong boundary = BY2PG;

	if(ndata > boundary)
		boundary = 128*1024*1024;
	r = pxspanalloc(ndata*sizeof(ulong)+sizeof(Rbh), 8, boundary);
	rb->h = r;
	rb->data = ((ulong *)r) + sizeof(Rbh)/sizeof(ulong);
	rb->ndata = ndata;
	rb->type = type;
	rb->bufid = count++;
	rb->dspid = -1;
	r->head = 0;
	r->tail = 0;
	r->intreq = 0;
	r->level = 200;
	rb->next = rblist;
	rblist = rb;
}

void
audioreset(void)
{
	Dspdev *dev = DEV;

	dev->miscsr = Reset | Sram32K;
	dev->cintstat = 0;
	dev->cintmask = 0;

	audio.started = 0;
	rbufalloc(&audio.cmd, CMDSIZE+1, Write);
	audio.cmd.dspid = 0;
	rbufalloc(&audio.reply, REPLYSIZE+1, Read);
	audio.reply.dspid = 1;
	rbufalloc(&audio.out, DATASIZE+1, Write|Extend);
	rbufalloc(&audio.in, DATASIZE+1, Read|Extend);
	*IO(uchar, LIO_1_MASK) |= LIO_DSP;

	dev->cintmask = Handtx | Handrx;
}

void
audioinit(void)
{
	dspinit();
}

Chan *
audioattach(char *param)
{
	return devattach('A', param);
}

Chan *
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

Chan *
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
		if(audio.amode != Aclosed){
			qunlock(&audio);
			error(Einuse);
		}
		audio.amode = amode;
		dspfree(&audio.out);
		break;
	}
	c = devopen(c, omode, audiodir, NAUDIO, devgen);
	return c;
}

void
audiocreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
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
			audio.amode = Aclosed;
			if(waserror()){
				qunlock(&audio);
				nexterror();
			}
			if(audio.in.dspid >= 0)
				dspfree(&audio.in);
			if(audio.out.dspid >= 0){
				dspdrain(&audio.out);
				dspfree(&audio.out);
			}
			poperror();
		}
		break;
	}
}

long
audioread(Chan *c, void *a, long n, ulong offset)
{
	int liv, riv, lov, rov, i, j;
	long m;
	char buf[300];
	uchar *ad;

	if(n <= 0)
		return 0;
	switch(c->qid.path & ~CHDIR){
	default:
		error(Eperm);
		break;

	case Qdir:
		return devdirread(c, a, n, audiodir, NAUDIO, devgen);

	case Qaudio:
		if(!audio.started)
			error("dsp not started");
		if(audio.in.dspid < 0)
			dspacquire(&audio.in);
		m = n>>2;
		ad = a;
		while(m > 0){
			i = m;
			if(i > DATASIZE/4)
				i = DATASIZE/4;
			dspraudio(&audio.in, ad, i*2);
			ad += i*4;
			m -= i;
		}
		return n;

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
	return n;
}

long
audiowrite(Chan *c, void *a, long n, ulong offset)
{
	char buf[256], *field[Ncmd];
	int i, nf, v, left, right, in, out;
	uchar *ad;
	int m, k;

	USED(offset);
	if(n <= 0)
		return 0;
	switch(c->qid.path){
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
		break;

	case Qaudio:
		if(!audio.started)
			error("dsp not started");
		if(audio.out.dspid < 0)
			dspacquire(&audio.out);
		m = n>>2;
		ad = a;
		while(m > 0){
			k = m;
			if(k > DATASIZE/4)
				k = DATASIZE/4;
			dspwaudio(&audio.out, ad, k*2);
			ad += k*4;
			m -= k;
		}
		return n;
	}
	return n;
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
	USED(c, dp);
	error(Eperm);
}

static ulong*
setvolume(ulong *pcmd, int tag, int v)
{
	*pcmd++ = tag;
	switch(tag){
	case Plvol: case Prvol:
	case Platten: case Pratten:
		v = (v*128) / 100;
		if(v > 128)
			v = 128;
		if(v < 0)
			v = 0;
		break;
	case Pispeed: case Pospeed:
		v = dsprcode(v);
		break;
	}
	*pcmd++ = v;
	return pcmd;
}

static void
setvolumes(void)
{
	ulong cmdbuf[32], *pcmd;

	if(!audio.started)
		error("dsp not started");
	pcmd = cmdbuf+1;
	*pcmd++ = Setparms;	/* cmd */
	*pcmd++ = 0;		/* id */
	pcmd = setvolume(pcmd, Plvol, audio.lovol[Vaudio]);
	pcmd = setvolume(pcmd, Prvol, audio.rovol[Vaudio]);
	pcmd = setvolume(pcmd, Pispeed, audio.livol[Vspeed]);
	pcmd = setvolume(pcmd, Pospeed, audio.lovol[Vspeed]);

	/*
	 * really cheesy way of selecting the input source
	 */
	if(audio.livol[Vline]>=audio.livol[Vmic] && audio.livol[Vline]>=audio.livol[Vaes]){
		pcmd = setvolume(pcmd, Platten, audio.livol[Vline]);
		pcmd = setvolume(pcmd, Pratten, audio.rivol[Vline]);
		pcmd = setvolume(pcmd, Psource, 0);
	}else if(audio.livol[Vmic]>=audio.livol[Vaes]){
		pcmd = setvolume(pcmd, Platten, audio.livol[Vmic]);
		pcmd = setvolume(pcmd, Pratten, audio.rivol[Vmic]);
		pcmd = setvolume(pcmd, Psource, 1);
	}else{
		pcmd = setvolume(pcmd, Platten, audio.livol[Vaes]);
		pcmd = setvolume(pcmd, Pratten, audio.rivol[Vaes]);
		pcmd = setvolume(pcmd, Psource, 2);
	}

	cmdbuf[0] = pcmd - cmdbuf;
	dsprpc(cmdbuf, 4);
	if(cmdbuf[3] != 0)
		error(Eio);
}

static void
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
isstarted(void *arg)
{
	USED(arg);
	return audio.started;
}

static void
dspinit(void)
{
	ulong *qmem;
	int k;
	uchar *p = dspprog;

	dspreset();
	for(;;){
		qmem = &MEM[(p[0]<<8)|p[1]];
		k = (p[2]<<8)|p[3];
		p += 4;
		if(k == 0)
			break;
		DPRINT("dspinit: 0x%4.4ux %d\n", qmem-MEM, k);
		while(--k >= 0){
			*qmem++ = (p[0]<<16)|(p[1]<<8)|p[2];
			p += 3;
		}
	}
	dspstart();
}

static void
dspreset(void)
{
	Dspdev *dev = DEV;

	dev->miscsr = Reset | Sram32K;
	audio.started = 0;
}

static void
dspstart(void)
{
	Dspdev *dev = DEV;
	ulong *mem = MEM;

	dev->miscsr = Reset | Sram32K;
	Xdelay(50);

	audio.started = 0;

	resetlevel();

	audio.cmd.h->head = 0;
	audio.cmd.h->tail = 0;
	audio.cmd.h->intreq = 0;
	audio.cmd.h->level = 200;
	audio.reply.h->head = 0;
	audio.reply.h->tail = 0;
	audio.reply.h->intreq = 0;
	audio.reply.h->level = 200;

	DPRINT("dspstart: cmd.h=0x%ux, reply.h=0x%ux\n", audio.cmd.h, audio.reply.h);
	mem[0] = PADDR(audio.cmd.h)&0xffff;
	mem[1] = PADDR(audio.cmd.h)>>16;
	mem[2] = audio.cmd.ndata;
	mem[3] = PADDR(audio.reply.h)&0xffff;
	mem[4] = PADDR(audio.reply.h)>>16;
	mem[5] = audio.reply.ndata;
	Xdelay(50);

	dev->miscsr = Sram32K;
	DPRINT("dspstart: sleeping...\n");
	if(waserror()){
		DPRINT("dspstart: interrupted\n");
		return;
	}
	tsleep(&audio.vous, isstarted, &audio, 500);
	poperror();
	DPRINT("dspstart: %s\n", audio.started?"OK":"timed out");
	setvolumes();
}

static int
wontintr(void *arg)
{
	return ((Rbh *)arg)->intreq == 0;
}

static void
dspacquire(Rbuf *rb)
{
	ulong cmdbuf[128], *pcmd;
	ulong pdata, edata;

	DPRINT("dspacquire: bufid=%d ndata=%d\n", rb->bufid, rb->ndata);
	qlock(rb);
	if(waserror()){
		qunlock(rb);
		nexterror();
	}
	if(rb->dspid >= 0){
		qunlock(rb);
		poperror();
		return;
	}
	rb->h->head = 0;
	rb->h->tail = 0;
	rb->h->intreq = 0;
	rb->h->level = rb->ndata/3;
	pcmd = cmdbuf+1;
	*pcmd++ = Acquire;	/* cmd */
	*pcmd++ = rb->bufid;
	*pcmd++ = rb->ndata;
	*pcmd++ = rb->type&Write;
	*pcmd++ = PADDR(rb->h)>>16;
	*pcmd++ = PADDR(rb->h)&0xffff;
	*pcmd++ = PADDR(rb->data)&0x0fff;

	pdata = PADDR(rb->data);
	edata = PADDR(rb->data + rb->ndata);
	pdata &= ~(BY2PG-1);
	while(pdata < edata){
		*pcmd++ = pdata >> PGSHIFT;
		pdata += BY2PG;
	}
	cmdbuf[0] = pcmd - cmdbuf;
	dsprpc(cmdbuf, 5);
	if(cmdbuf[3] != 0)
		error(Eio);
	rb->dspid = cmdbuf[4];
	DPRINT("dspacquire: bufid=%d dspid=%d\n", rb->bufid, rb->dspid);
	qunlock(rb);
	poperror();
}

static void
dspfree(Rbuf *rb)
{
	ulong cmdbuf[8], *pcmd;

	DPRINT("dspfree: bufid=%d dspid=%d\n", rb->bufid, rb->dspid);
	qlock(rb);
	if(waserror()){
		qunlock(rb);
		nexterror();
	}
	if(rb->dspid >= 0){
		pcmd = cmdbuf+1;
		*pcmd++ = Free;	/* cmd */
		*pcmd++ = rb->bufid;
		*pcmd++ = rb->dspid;
		cmdbuf[0] = pcmd - cmdbuf;
		dsprpc(cmdbuf, 4);
		rb->dspid = -1;
	}
	DPRINT("dspfree: bufid=%d OK\n", rb->bufid);
	qunlock(rb);
	poperror();
}

static void
dsprpc(ulong *cmdbuf, int nreply)
{
	ulong nr;
	int i;

	qlock(&audio);
	if(waserror()){
		qunlock(&audio);
		nexterror();
	}
	dspwrite(&audio.cmd, cmdbuf, cmdbuf[0]);
	for(;;){
		dspread(&audio.reply, &nr, 1);
		if(nr == 0)
			continue;
		if(nr == nreply)
			break;
		audio.reply.h->head = audio.reply.h->tail;
		error("dsprpc reply");
	}
	cmdbuf[0] = nr;
	dspread(&audio.reply, &cmdbuf[1], nr-1);
	if(dspdebug){
		kprint("dsp reply:");
		for(i=0; i<nreply; i++)
			kprint(" 0x%ux", cmdbuf[i]);
		kprint("\n");
	}
	qunlock(&audio);
	poperror();
}

static void
dspwrite(Rbuf *rb, ulong *data, int ndata)
{
	Rbh *r = rb->h;
	int tail, m;

	DPRINT("dspwrite %d: %d\n", rb->dspid, ndata);
	qlock(rb);
	if(ndata > rb->ndata)
		panic("dspwrite len");
	if(waserror()){
		qunlock(rb);
		nexterror();
	}
	r->level = rb->ndata - ndata;
	tail = r->tail;
	for(;;){
		m = r->head - tail - 1;
		if(m < 0)
			m += rb->ndata;
		DPRINT("dspwrite %d: avail=%d\n", rb->dspid, m);
		if(m >= ndata)
			break;
		r->intreq = 1;
		sleep(rb, wontintr, r);
	}
	poperror();
	while(ndata){
		m = r->head - tail - 1;
		if(m < 0)
			m = rb->ndata - tail;
		if(m == 0)
			panic("dspwrite");
		if(m > ndata)
			m = ndata;
		memmove(&rb->data[tail], data, m*sizeof(ulong));
		ndata -= m;
		data += m;
		tail += m;
		if(tail >= rb->ndata)
			tail = 0;
	}
	r->tail = tail;
	qunlock(rb);
}

static void
dspwaudio(Rbuf *rb, uchar *data, int ndata)
{
	Rbh *r = rb->h;
	int tail, m;
	ulong *up;

	DPRINT("dspwaudio %d: %d\n", rb->dspid, ndata);
	qlock(rb);
	if(ndata > rb->ndata)
		panic("dspwaudio len");
	if(waserror()){
		qunlock(rb);
		nexterror();
	}
	r->level = rb->ndata - ndata;
	tail = r->tail;
	for(;;){
		m = r->head - tail - 1;
		if(m < 0)
			m += rb->ndata;
		DPRINT("dspwaudio %d: avail=%d\n", rb->dspid, m);
		if(m >= ndata)
			break;
		r->intreq = 1;
		sleep(rb, wontintr, r);
	}
	poperror();
	while(ndata){
		m = r->head - tail - 1;
		if(m < 0)
			m = rb->ndata - tail;
		if(m == 0)
			panic("dspwaudio");
		if(m > ndata)
			m = ndata;
		up = &rb->data[tail];
		ndata -= m;
		tail += m;
		if(tail >= rb->ndata)
			tail = 0;
		while(--m >= 0){
			*up++ = (data[0] << 8) | (data[1] << 16);
			data += 2;
		}
	}
	r->tail = tail;
	qunlock(rb);
}

static int
dspread(Rbuf *rb, ulong *data, int ndata)
{
	Rbh *r = rb->h;
	int head, avail, m;

	DPRINT("dspread %d: %d\n", rb->dspid, ndata);
	qlock(rb);
	if(ndata > rb->ndata)
		panic("dspread len");
	if(waserror()){
		qunlock(rb);
		nexterror();
	}
	r->level = rb->ndata - ndata;
	head = r->head;
	for(;;){
		avail = r->tail - head;
		if(avail < 0)
			avail += rb->ndata;
		DPRINT("dspread %d: avail=%d\n", rb->dspid, avail);
		if(avail >= ndata)
			break;
		r->intreq = 1;
		sleep(rb, wontintr, r);
	}
	poperror();
	avail -= ndata;
	while(ndata){
		m = r->tail - head;
		if(m < 0)
			m = rb->ndata - head;
		if(m == 0)
			panic("dspread");
		if(m > ndata)
			m = ndata;
		memmove(data, &rb->data[head], m*sizeof(ulong));
		ndata -= m;
		data += m;
		head += m;
		if(head >= rb->ndata)
			head = 0;
	}
	r->head = head;
	qunlock(rb);
	return avail;
}

static int
dspraudio(Rbuf *rb, uchar *data, int ndata)
{
	Rbh *r = rb->h;
	int head, avail, m, adata;
	ulong *up;

	DPRINT("dspraudio %d: %d\n", rb->dspid, ndata);
	qlock(rb);
	if(ndata > rb->ndata)
		panic("dspraudio len");
	if(waserror()){
		qunlock(rb);
		nexterror();
	}
	r->level = rb->ndata - ndata;
	head = r->head;
	for(;;){
		avail = r->tail - head;
		if(avail < 0)
			avail += rb->ndata;
		DPRINT("dspraudio %d: avail=%d\n", rb->dspid, avail);
		if(avail >= ndata)
			break;
		r->intreq = 1;
		sleep(rb, wontintr, r);
	}
	poperror();
	avail -= ndata;
	while(ndata){
		m = r->tail - head;
		if(m < 0)
			m = rb->ndata - head;
		if(m == 0)
			panic("dspraudio");
		if(m > ndata)
			m = ndata;
		up = &rb->data[head];
		ndata -= m;
		head += m;
		if(head >= rb->ndata)
			head = 0;
		while(--m >= 0){
			adata = *up++;
			*data++ = adata >> 8;
			*data++ = adata >> 16;
		}
	}
	r->head = head;
	qunlock(rb);
	return avail;
}

static void
dspdrain(Rbuf *rb)
{
	Rbh *r = rb->h;
	int tail, level, m;

	DPRINT("dspdrain %d\n", rb->dspid);
	qlock(rb);
	level = r->level;
	r->level = 2;
	if(waserror()){
		r->level = level;
		qunlock(rb);
		nexterror();
	}
	tail = r->tail;
	for(;;){
		m = tail - r->head;
		if(m < 0)
			m += rb->ndata;
		DPRINT("dspdrain %d: avail=%d\n", rb->dspid, m);
		if(m <= 2)
			break;
		r->intreq = 1;
		sleep(rb, wontintr, r);
	}
	poperror();
	r->level = level;
	qunlock(rb);
}

void
audiointr(void)
{
	Dspdev *dev = DEV;
	Rbuf *rb;
	ulong token;

	if(dev->cintstat & Handtx){
		dev->cintstat = ~Handtx;	/* clear interrupt first */
		token = dev->handtx;
		DPRINT("audiointr tx %ud 0x%ux\n", MACHP(0)->ticks*MS2HZ, token);
		for(rb = rblist; rb; rb = rb->next)
			if(rb->dspid == token)
				break;
		if(rb){
			rb->h->intreq = 0;
			wakeup(rb);
		}else if(!audio.started){
			audio.started = 1;
			wakeup(&audio.vous);
		}
	}else if(dev->cintstat & Handrx){
		dev->cintstat &= ~Handrx;
		DPRINT("audiointr rx\n");
	}else if(dev->cintstat & Dmadone){
		dev->cintstat &= ~Dmadone;
		DPRINT("audiointr dma\n");
	}
}

#define	C(x)	((x)<<1)
#define	M(x)	((x)<<2)
#define	S(x)	((x)<<4)

static int rtable[] = {
	48000,	M(0)|C(0)|S(0),
	44100,	M(1)|C(0)|S(0),
	32000,	M(0)|C(1)|S(0),
	29400,	M(1)|C(1)|S(0),
	24000,	M(0)|C(0)|S(1),
	22050,	M(1)|C(0)|S(1),
	16000,	M(0)|C(0)|S(2),
	14700,	M(1)|C(0)|S(2),
	12000,	M(0)|C(0)|S(3),
	11025,	M(1)|C(0)|S(3),
	10666,	M(0)|C(1)|S(2),
	9800,	M(1)|C(1)|S(2),
	9600,	M(0)|C(0)|S(4),
	8820,	M(1)|C(0)|S(4),
	8000,	M(0)|C(0)|S(5),
	7350,	M(1)|C(0)|S(5),
	6857,	M(0)|C(0)|S(6),
	6400,	M(0)|C(1)|S(4),
	6300,	M(1)|C(0)|S(6),
	6000,	M(0)|C(0)|S(7),
	5880,	M(1)|C(1)|S(4),
	5512,	M(1)|C(0)|S(7),
	5333,	M(0)|C(1)|S(5),
	4900,	M(1)|C(1)|S(5),
	4571,	M(0)|C(1)|S(6),
	4200,	M(1)|C(1)|S(6),
	4000,	M(0)|C(1)|S(7),
	3675,	M(1)|C(1)|S(7),
	0,	0
};

static int
dsprcode(int hz)
{
	int *p;

	for(p=rtable; *p; p+=2)
		if(hz == p[0])
			return p[1];
	error(Ebadarg);
	return -1;
}
