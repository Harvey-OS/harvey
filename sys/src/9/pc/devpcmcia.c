#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

enum{
	Qdir,
	Qmem,
	Qattr,
	Qctl,
};
Dirtab pcmciatab[]={
	"pcmmem",		{ Qmem, 0 },		0,	0600,
	"pcmattr",		{ Qattr, 0 },		0,	0600,
	"pcmctl",		{ Qctl, 0 },		0,	0600,
};
#define Npcmciatab (sizeof(pcmciatab)/sizeof(Dirtab))

/*
 *  Intel 82365SL PCIC controller for the PCMCIA
 */
enum
{
	/* ports */
	Pindex=		0x3E0,
	Pdata=		0x3E1,

	/*
	 *  registers indices
	 */
	Rid=		0x0,		/* identification and revision */
	Ris=		0x1,		/* interface status */
	Rpc=	 	0x2,		/* power control */
	 Foutena=	 (1<<7),	/*  output enable */
	 Fautopower=	 (1<<5),	/*  automatic power switching */
	 Fcardena=	 (1<<4),	/*  PC card enable */
	Rigc= 		0x3,		/* interrupt and general control */
	 Fiocard=	 (1<<5),	/*  I/O card (vs memory) */
	 Fnotreset=	 (1<<6),	/*  reset if not set */	
	 FSMIena=	 (1<<4),	/*  enable change interrupt on SMI */ 
	Rcsc= 		0x4,		/* card status change */
	Rcscic= 	0x5,		/* card status change interrupt config */
	 Fchangeena=	 (1<<3),	/*  card changed */
	 Fbwarnena=	 (1<<1),	/*  card battery warning */
	 Fbdeadena=	 (1<<0),	/*  card battery dead */
	Rwe= 		0x6,		/* address window enable */
	 Fmem16=	 (1<<5),	/*  use A23-A12 to decode address */
	Rio= 		0x7,		/* I/O control */
	Riobtm0lo=	0x8,		/* I/O address 0 start low byte */
	Riobtm0hi=	0x9,		/* I/O address 0 start high byte */
	Riotop0loi=	0xa,		/* I/O address 0 stop low byte */
	Riotop0hi=	0xb,		/* I/O address 0 stop high byte */
	Riobtm1lo=	0xc,		/* I/O address 1 start low byte */
	Riobtm1hi=	0xd,		/* I/O address 1 start high byte */
	Riotop1lo=	0xe,		/* I/O address 1 stop low byte */
	Riotop1hi=	0xf,		/* I/O address 1 stop high byte */
	Rmap=		0x10,		/* map 0 */

	/*
	 *  offsets into the system memory address maps
	 */
	Mbtmlo=		0x0,		/* System mem addr mapping start low byte */
	Mbtmhi=		0x1,		/* System mem addr mapping start high byte */
	Mtoplo=		0x2,		/* System mem addr mapping stop low byte */
	Mtophi=		0x3,		/* System mem addr mapping stop high byte */
	 F16bit=	 (1<<7),	/*  16-bit wide data path */
	Mofflo=		0x4,		/* Card memory offset address low byte */
	Moffhi=		0x5,		/* Card memory offset address high byte */
	 Fregactive=	 (1<<6),	/*  attribute meory */


	Mbits=		18,		/* msb of Mchunk */
	Mchunk=		1<<Mbits,	/* logical mapping granularity */
	Nmap=		4,		/* max number of maps to use */
};

#define MAP(x,o)	(Rmap + (x)*0x8 + o)

typedef struct PCMCIA	PCMCIA;
typedef struct PCMmap	PCMmap;

/* maps between ISA memory space and the card memory space */
struct PCMmap
{
	ulong	ca;		/* card address */
	ulong	cea;		/* card end address */
	ulong	isa;		/* ISA address */
	int	attr;		/* attribute memory */
	int	time;
};

struct PCMCIA
{
	QLock;
	int	ref;

	/* status */
	uchar	occupied;
	uchar	battery;
	uchar	wrprot;
	uchar	powered;
	uchar	configed;
	uchar	enabled;

	/* memory maps */
	int	time;
	PCMmap	mmap[Nmap];
};
PCMCIA	pcmcia;

/*
 *  reading and writing card registers
 */
static uchar
rdreg(int index)
{
	outb(Pindex, index);
	return inb(Pdata);
}
static void
wrreg(int index, uchar val)
{
	outb(Pindex, index);
	outb(Pdata, val);
}

/*
 *  get info about card
 */
static void
pcmciainfo(void)
{
	uchar isr;

	isr = rdreg(Ris);
	pcmcia.occupied = (isr & (3<<2)) == (3<<2);
	pcmcia.powered = isr & (1<<6);
	pcmcia.battery = (isr & 3) == 3;
	pcmcia.wrprot = isr & (1<<4);
}

/*
 *  disable the pcmcia card
 */
static void
pcmciadis(void)
{
	/* disable the windows into the card */
	wrreg(Rwe, 0);

	/* disable the card */
	wrreg(Rpc, 5|Fautopower);
	pcmcia.enabled = 0;
}

/*
 *  card detect status change interrupt
 */
void
pcmciaintr(Ureg *ur)
{
	uchar csc;

	USED(ur);
	csc = rdreg(Rcsc);
	pcmciainfo();
	if(csc & 1)
		print("cia card battery dead\n");
	if(csc & (1<<1))
		print("cia card battery warning\n");
	if(csc & (1<<3)){
		if(pcmcia.occupied)
			print("cia card inserted\n");
		else
			print("cia card removed\n");
		pcmciadis();
	}
}

/*
 *  enable the pcmcia card
 */
static void
pcmciaena(void)
{
	static int already;

	if(pcmcia.enabled)
		return;

	if(already == 0){
		already = 1;

		/* interrupt on card status change */
		wrreg(Rigc, (PCMCIAvec-Int0vec) | Fnotreset);
		wrreg(Rcscic, ((PCMCIAvec-Int0vec)<<4) | Fchangeena
			| Fbwarnena | Fbdeadena);
		setvec(PCMCIAvec, pcmciaintr);
	}

	/* display status */
	pcmciainfo();
	if(pcmcia.occupied){
		/* enable the card */
		wrreg(Rpc, 5|Fautopower|Foutena|Fcardena);
		pcmcia.enabled = 1;
	} else
		print("cia unoccupied\n");
}

/*
 *  get a map for pc card region, return corrected len
 */
static PCMmap*
getmap(ulong offset, int attr)
{
	uchar we, bit;
	PCMmap *m, *lru;
	int i;
	static int already;

	if(already == 0){
		already = 1;

		/* grab ISA address space for memory maps */
		for(i = 0; i < Nmap; i++)
			pcmcia.mmap[i].isa = isamem(Mchunk);
	}

	/* look for a map that starts in the right place */
	we = rdreg(Rwe);
	bit = 1;
	lru = pcmcia.mmap;
	for(m = pcmcia.mmap; m < &pcmcia.mmap[Nmap]; m++){
		if((we & bit) && m->attr == attr && offset >= m->ca && offset < m->cea){
			m->time = pcmcia.time++;
			return m;
		}
		bit <<= 1;
		if(lru->time > m->time)
			lru = m;
	}

	/* use the least recently used */
	m = lru;
	offset &= ~(Mchunk - 1);
	m->ca = offset;
	m->cea = m->ca + Mchunk;
	m->attr = attr;
	m->time = pcmcia.time++;
	i = m - pcmcia.mmap;
	bit = 1<<i;
	wrreg(Rwe, we & ~bit);		/* disable map before changing it */
	wrreg(MAP(i, Mbtmlo), m->isa>>12);
	wrreg(MAP(i, Mbtmhi), (m->isa>>(12+8)) | F16bit);
	wrreg(MAP(i, Mtoplo), (m->isa+Mchunk-1)>>12);
	wrreg(MAP(i, Mtophi), (m->isa+Mchunk-1)>>(12+8));
	offset -= m->isa;
	offset &= (1<<25)-1;
	offset >>= 12;
	wrreg(MAP(i, Mofflo), offset);
	wrreg(MAP(i, Moffhi), (offset>>8) | (attr ? Fregactive : 0));
	wrreg(Rwe, we | bit);		/* enable map */
	return m;
}

/*
 *  set up for pcmcia cards
 */
void
pcmciareset(void)
{
	/* max possible length */
	pcmciatab[0].length = 64*MB;
	pcmciatab[1].length = 64*MB;

	/* turn on 5V power if the card is there to keep it's battery alive */
	wrreg(Rpc, 5|Fautopower);
}

void
pcmciainit(void)
{
}

Chan *
pcmciaattach(char *spec)
{
	qlock(&pcmcia);
	pcmcia.ref++;
	qunlock(&pcmcia);
	return devattach('y', spec);
}

Chan *
pcmciaclone(Chan *c, Chan *nc)
{
	qlock(&pcmcia);
	pcmcia.ref++;
	qunlock(&pcmcia);
	return devclone(c, nc);
}

int
pcmciawalk(Chan *c, char *name)
{
	return devwalk(c, name, pcmciatab, (long)Npcmciatab, devgen);
}

void
pcmciastat(Chan *c, char *db)
{
	devstat(c, db, pcmciatab, (long)Npcmciatab, devgen);
}

Chan *
pcmciaopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	}
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

void
pcmciacreate(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
pcmciaremove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
pcmciawstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
pcmciaclose(Chan *c)
{
	USED(c);
	qlock(&pcmcia);
	if(--pcmcia.ref == 0)
		pcmciadis();
	qunlock(&pcmcia);
}

long
pcmciaread(Chan *c, void *a, long n, ulong offset)
{
	int i, len;
	PCMmap *m;
	ulong ka;
	ulong p;
	uchar *ac;

	ac = a;
	p = (int)(c->qid.path&~CHDIR);
	switch(p){
	case Qdir:
		return devdirread(c, a, n, pcmciatab, Npcmciatab, devgen);
	case Qmem:
	case Qattr:
		if(pcmciatab[0].length < offset)
			return 0;
		if(pcmciatab[0].length < offset + n)
			n = pcmciatab[0].length - offset;
		if(waserror()){
			qunlock(&pcmcia);
			nexterror();
		}
		qlock(&pcmcia);
		pcmciaena();
		for(len = n; len > 0; len -= i){
			if(pcmcia.occupied == 0 || pcmcia.enabled == 0)
				error(Eio);
			m = getmap(offset, p == Qattr);
			if(offset + len > m->cea)
				i = m->cea - offset;
			else
				i = len;
			ka = KZERO|(m->isa + (offset&(Mchunk-1)));
			memmove(ac, (void*)ka, i);
			offset += i;
			ac += i;
		}
		qunlock(&pcmcia);
		poperror();
		break;
	default:
		n=0;
		break;
	}
	return n;
}

long
pcmciawrite(Chan *c, void *a, long n, ulong offset)
{
	int i, len;
	PCMmap *m;
	ulong ka;
	ulong p;
	uchar *ac;
	char *bp;
	char buf[32];

	ac = a;
	p = (int)(c->qid.path&~CHDIR);
	switch(p){
	case Qmem:
	case Qattr:
		if(pcmciatab[0].length < offset)
			return 0;
		if(pcmciatab[0].length < offset + n)
			n = pcmciatab[0].length - offset;
		if(waserror()){
			qunlock(&pcmcia);
			nexterror();
		}
		qlock(&pcmcia);
		pcmciaena();
		for(len = n; len > 0; len -= i){
			if(pcmcia.wrprot)
				error(Eperm);
			if(pcmcia.occupied == 0 || pcmcia.enabled == 0)
				error(Eio);
			m = getmap(offset, p == Qattr);
			if(offset + len > m->cea)
				i = m->cea - offset;
			else
				i = len;
			ka = KZERO|(m->isa + (offset&(Mchunk-1)));
			memmove((void*)ka, ac, i);
			offset += i;
			ac += i;
		}
		qunlock(&pcmcia);
		poperror();
		break;
	case Qctl:
		qlock(&pcmcia);
		if(n > sizeof(buf)-1)
			n = sizeof(buf)-1;
		memmove(buf, a, n);
		buf[n] = 0;
		if(strncmp(buf, "length", 6)==0){
			bp = buf+6;
			while(*bp == ' ')
				bp++;
			pcmciatab[0].length = atoi(bp);
		}
		qunlock(&pcmcia);
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}
