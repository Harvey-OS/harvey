#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"

/*
 *  MPC8xx PCMCIA driver 
 *
 */

typedef struct Slot	Slot;
typedef struct Conftab	Conftab;

enum
{
	Maxctlr=	1,

	/* pipr */
	Cvs1=	1<<15,		// voltage sense 1
	Cvs2=	1<<14,		// voltage sense 2
	Cwp=	1<<13,		// write protect
	Ccd2=	1<<12,		// card detect 2
	Ccd1=	1<<11,		// card detect 1
	Cbvd2=	1<<10,		// battery voltage 2
	Cbvd1=	1<<9,		// battery voltage 1
	Crdy=	1<<8,		// ready

	/* pscr */
	Cvs1_c=	1<<15,		// voltage sense 1 changed
	Cvs2_c=	1<<14,		// voltage sense 2 changed
	Cwp_c=	1<<13,		// write protect changed
	Ccd2_c=	1<<12,		// card detect 2 changed
	Ccd1_c=	1<<11,		// card detect 1 changed
	Cbvd2_c=	1<<10,	// battery voltage 2 changed
	Cbvd1_c=	1<<9,	// battery voltage 1 changed
	Crdy_l=	1<<7,		// ready is low
	Crdy_h=	1<<6,		// ready is hi
	Crdy_r=	1<<5,		// ready raising edge
	Crdy_f=	1<<4,		// ready falling edge

	/* pgcrx */
	Creset = IBIT(25),	// card reset
	Coe = IBIT(24),		// output enable

	/* porN */
	Rport8=		0<<6,
	Rport16=	1<<6,
	Rmem=	0<<3,	/* common memory space */
	Rattrib=	2<<3,	/* attribute space */
	Rio=		3<<3,
	Rdma=	4<<3,	/* normal DMA */
	Rdmalx=	5<<3,	/* DMA, last transaction */
	RA22_23= 6<<3,	/* ``drive A22 and A23 signals on CE2 and CE1'' */
	RslotB=	1<<2,	/* select slot B (always, on MPC823) */
	Rwp=	1<<1,	/* write protect */
	Rvalid=	1<<0,	/* region valid */

	/*
	 *  configuration registers - they start at an offset in attribute
	 *  memory found in the CIS.
	 */
	Rconfig=	0,
	 Clevel=	 (1<<6),	/*  level sensitive interrupt line */

	Maxctab=	8,		/* maximum configuration table entries */
	Nmap= 4,		// map entries per slot
	Mshift=	12,
	Mgran=	(1<<Mshift),	/* granularity of maps */
	Mmask=	~(Mgran-1),	/* mask for address bits important to the chip */
};

/* configuration table entry */
struct Conftab
{
	int	index;
	ushort	irqs;		/* legal irqs */
	uchar	irqtype;
	uchar	bit16;		/* true for 16 bit access */
	struct {
		ulong	start;
		ulong	len;
	} io[16];
	int	nio;
	uchar	vpp1;
	uchar	vpp2;
	uchar	memwait;
	ulong	maxwait;
	ulong	readywait;
	ulong	otherwait;
};

/* a card slot */
struct Slot
{
	Lock;
	int	ref;

	long	memlen;		/* memory length */
	uchar	base;		/* index register base */
	uchar	slotno;		/* slot number */

	/* status */
	uchar	special;	/* in use for a special device */
	uchar	already;	/* already inited */
	uchar	occupied;
	uchar	voltage;
	uchar	battery;
	uchar	wrprot;
	uchar	powered;
	uchar	configed;
	uchar	enabled;
	uchar	busy;

	/* cis info */
	char	verstr[512];	/* version string */
	uchar	cpresent;	/* config registers present */
	ulong	caddr;		/* relative address of config registers */
	int	nctab;		/* number of config table entries */
	Conftab	ctab[Maxctab];
	Conftab	*def;		/* default conftab */

	/* for walking through cis */
	int	cispos;		/* current position scanning cis */
	uchar	*cisbase;

	/* memory maps */
	Lock	mlock;		/* lock down the maps */
	int	time;
	PCMmap	mmap[Nmap];	/* maps, last is always for the kernel */
};

enum
{
	Qdir,
	Qmem,
	Qattr,
	Qctl,
};

#define SLOTNO(c)	((c->qid.path>>8)&0xff)
#define TYPE(c)		(c->qid.path&0xff)
#define QID(s,t)	(((s)<<8)|(t))

static void pcmciaintr(Ureg *, void *);
static void increfp(Slot *pp);
static void decrefp(Slot *pp);

static Slot	*slot;
static nslot;

static int
pcmgen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	int slotno;
	Qid qid;
	long len;
	Slot *pp;
	char name[NAMELEN];

	USED(tab, ntab);

	if(i == DEVDOTDOT){
		devdir(c, c->qid, "#y", 0, eve, 0550, dp);
		return 1;
	}
	if(i>=3*nslot)
		return -1;
	slotno = i/3;
	pp = slot + slotno;
	len = 0;
	switch(i%3){
	case 0:
		qid.path = QID(slotno, Qmem);
		sprint(name, "pcm%dmem", slotno);
		len = pp->memlen;
		break;
	case 1:
		qid.path = QID(slotno, Qattr);
		sprint(name, "pcm%dattr", slotno);
		len = pp->memlen;
		break;
	case 2:
		qid.path = QID(slotno, Qctl);
		sprint(name, "pcm%dctl", slotno);
		break;
	}
	qid.vers = 0;
	devdir(c, qid, name, len, eve, 0660, dp);
	return 1;
}

/*
 *  set up for slot cards
 */
static void
pcmciareset(void)
{
	static int already;
	int i;
	IMM *io;
	Slot *pp;

	if(already)
		return;
	already = 1;
	
	nslot = 2;
	slot = xalloc(nslot * sizeof(Slot));

	io = m->iomem;

	for(i=0; i<8; i++){
		io->pcmr[i].option = 0;
		io->pcmr[i].base = 0;
	}

	for(i=0; i<2; i++)
		io->pgcr[i] = (1<<(31-PCMCIAlevel)) | (1<<(23-PCMCIAlevel));

	for(i = 0; i < nslot; i++){
		pp = slot + i;
		pp->slotno = i;
		pp->memlen = 512*1024;
	}

	/* 512K - slow - attr */
	io->pcmr[0].base = ISAMEM + 512*1024;
	io->pcmr[0].option = (0x1a<<27) | (8<<16) | (6<<12) | (24<<7)
		| Rport16 | Rattrib | Rvalid;

//	intrenable(VectorPIC+PCMCIAlevel, pcmciaintr, 0, BUSUNKNOWN);
	io->pscr = 0xFEF0FEF0;	/* reset status */
	io->per = 0x00800000;	/* enable interrupts */

	print("pcmcia reset\n");
}

static Chan*
pcmciaattach(char *spec)
{
	return devattach('y', spec);
}

static int
pcmciawalk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, pcmgen);
}

static void
pcmciastat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, pcmgen);
}

static Chan*
pcmciaopen(Chan *c, int omode)
{
	if(c->qid.path == CHDIR){
		if(omode != OREAD)
			error(Eperm);
	} else
		increfp(slot + SLOTNO(c));
	c->mode = openmode(omode);
	c->flag |= COPEN;
	c->offset = 0;
	return c;
}

static void
pcmciaclose(Chan *c)
{
	if(c->flag & COPEN)
		if(c->qid.path != CHDIR)
			decrefp(slot+SLOTNO(c));
}

/* a memmove using only bytes */
static void
memmoveb(uchar *to, uchar *from, int n)
{
	while(n-- > 0)
		*to++ = *from++;
}

static long
pcmread(int slotno, int attr, void *a, long n, vlong offset)
{
	int i, len;
	PCMmap *m;
	ulong ka;
	uchar *ac;
	Slot *pp;

	pp = slot + slotno;
	if(pp->memlen < offset)
		return 0;
	if(pp->memlen < offset + n)
		n = pp->memlen - offset;

	m = 0;
	if(waserror()){
		if(m)
			pcmunmap(pp->slotno, m);
		nexterror();
	}

	ac = a;
	for(len = n; len > 0; len -= i){
		m = pcmmap(pp->slotno, offset, 0, attr);
		if(m == 0)
			error("can't map PCMCIA card");
		if(offset + len > m->cea)
			i = m->cea - offset;
		else
			i = len;
		ka = KZERO|(m->isa + offset - m->ca);
		memmoveb(ac, (void*)ka, i);
		pcmunmap(pp->slotno, m);
		offset += i;
		ac += i;
	}
	poperror();
	return n;
}

static long
pcmciaread(Chan *c, void *a, long n, vlong offset)
{
	char *cp, *buf;
	ulong p;
	Slot *pp;

	p = TYPE(c);
	switch(p){
	case Qdir:
		return devdirread(c, a, n, 0, 0, pcmgen);
	case Qmem:
	case Qattr:
		return pcmread(SLOTNO(c), p==Qattr, a, n, offset);
	case Qctl:
		buf = malloc(2048);
		if(buf == nil)
			error(Enomem);
		if(waserror()){
			free(buf);
			nexterror();
		}
		cp = buf;
		pp = slot + SLOTNO(c);
		if(pp->occupied)
			cp += sprint(cp, "occupied\n");
		if(pp->enabled)
			cp += sprint(cp, "enabled\n");
		if(pp->powered)
			cp += sprint(cp, "powered\n");
		if(pp->configed)
			cp += sprint(cp, "configed\n");
		if(pp->wrprot)
			cp += sprint(cp, "write protected\n");
		if(pp->busy)
			cp += sprint(cp, "busy\n");
		cp += sprint(cp, "battery lvl %d\n", pp->battery);
		cp += sprint(cp, "voltage select %d\n", pp->voltage);
		cp += sprint(cp, "pipr %ulx\n", m->iomem->pipr);
		cp += sprint(cp, "pcsr %ulx\n", m->iomem->pscr);
		*cp = 0;
		n = readstr(offset, a, n, buf);
		poperror();
		free(buf);
		break;
	default:
		n=0;
		break;
	}
	return n;
}

static long
pcmwrite(int dev, int attr, void *a, long n, vlong offset)
{
	int i, len;
	PCMmap *m;
	Slot *pp;
	uchar *ac;
	ulong ka;

	pp = slot + dev;
	if(pp->memlen < offset)
		return 0;
	if(pp->memlen < offset + n)
		n = pp->memlen - offset;

	m = 0;
	if(waserror()){
		if(m)
			pcmunmap(pp->slotno, m);
		nexterror();
	}

	ac = a;
	for(len = n; len > 0; len -= i){
		m = pcmmap(pp->slotno, offset, 0, attr);
		if(m == 0)
			error("can't map PCMCIA card");
		if(offset + len > m->cea)
			i = m->cea - offset;
		else
			i = len;
		ka = KZERO|(m->isa + offset - m->ca);
		memmoveb((void*)ka, ac, i);
		pcmunmap(pp->slotno, m);
		offset += i;
		ac += i;
	}

	poperror();

	return n;
}

static long
pcmciawrite(Chan *c, void *a, long n, vlong offset)
{
	ulong p;
	Slot *pp;
	char buf[32];

	p = TYPE(c);
	switch(p){
	case Qctl:
		if(n >= sizeof(buf))
			n = sizeof(buf) - 1;
		strncpy(buf, a, n);
		buf[n] = 0;
		pp = slot + SLOTNO(c);
		if(!pp->occupied)
			error(Eio);

		break;
	case Qmem:
	case Qattr:
		pp = slot + SLOTNO(c);
		if(pp->occupied == 0 || pp->enabled == 0)
			error(Eio);
		n = pcmwrite(pp->slotno, p == Qattr, a, n, offset);
		if(n < 0)
			error(Eio);
		break;
	default:
		error(Ebadusefd);
	}
	return n;
}

Dev pcmciadevtab = {
	'y',
	"pcmcia",

	pcmciareset,
	devinit,
	pcmciaattach,
	devclone,
	pcmciawalk,
	pcmciastat,
	pcmciaopen,
	devcreate,
	pcmciaclose,
	pcmciaread,
	devbread,
	pcmciawrite,
	devbwrite,
	devremove,
	devwstat,
};

static void
pcmciaintr(Ureg *, void *)
{
	print("pcmciaintr\n");
}

/*
 *  get a map for pc card region, return corrected len
 */
PCMmap*
pcmmap(int slotno, ulong offset, int len, int attr)
{
	PCMmap *m;
	ulong e;
	Slot *pp;

	pp = slot + slotno;
	if(attr == 0)
		panic("pcmmap");

	/* convert offset to granularity */
	if(len <= 0)
		len = 1;
	e = ROUND(offset+len, Mgran);
	offset &= Mmask;
	len = e - offset;
	USED(len);

	m = pp->mmap;

	m->ca = 0;
	m->cea = 512*1024;
	m->isa = ISAMEM + 512*1024;
	m->len = 512*1024;
	m->attr = attr;
	m->ref = 1;

	return m;
}

void
pcmunmap(int slotno, PCMmap *m)
{
	USED(slotno, m);
}

static int
pcmio(int slotno, ISAConf *isa)
{
	uchar *p;
	Slot *pp;
	Conftab *ct, *et, *t;
	PCMmap *mm;
	int i, index, irq;
	char *cp;
	IMM *io;
	ulong x;

	io = m->iomem;

	isa->irq = VectorPIC+PCMCIAlevel;
	irq = isa->irq;

	if(slotno > nslot)
		return -1;
	pp = slot + slotno;

	if(!pp->occupied)
		return -1;

	et = &pp->ctab[pp->nctab];

	ct = 0;
	for(i = 0; i < isa->nopt; i++){
		if(strncmp(isa->opt[i], "index=", 6))
			continue;
		index = strtol(&isa->opt[i][6], &cp, 0);
		if(cp == &isa->opt[i][6] || index >= pp->nctab)
			return -1;
		ct = &pp->ctab[index];
	}

	if(ct == 0){
	
		/* assume default is right */
		if(pp->def)
			ct = pp->def;
		else
			ct = pp->ctab;
	
		/* try for best match */
		if(ct->nio == 0
		|| ct->io[0].start != isa->port || ((1<<irq) & ct->irqs) == 0){
			for(t = pp->ctab; t < et; t++)
				if(t->nio
				&& t->io[0].start == isa->port
				&& ((1<<irq) & t->irqs)){
					ct = t;
					break;
				}
		}
		if(ct->nio == 0 || ((1<<irq) & ct->irqs) == 0){
			for(t = pp->ctab; t < et; t++)
				if(t->nio && ((1<<irq) & t->irqs)){
					ct = t;
					break;
				}
		}
		if(ct->nio == 0){
			for(t = pp->ctab; t < et; t++)
				if(t->nio){
					ct = t;
					break;
				}
		}
	}

	if(ct == et || ct->nio == 0)
		return -1;

	/* enable io port map 0 */
	if(isa->port == 0)
		isa->port = ct->io[0].start;
	if(isa->port == 0 && ct->io[0].start == 0)
		return -1;

	// setup io space - this could be better
	io->pcmr[1].base = ISAMEM+isa->port;
	x = (0x6<<27) | (2<<16) | (4<<12) | (8<<7) | Rio | Rvalid;
//	x = (0x6<<27) | (8<<16) | (6<<12) | (24<<7) | Rio | Rvalid;
	/* 16-bit data path */
	if(ct->bit16)
		x |= Rport16;
	else
		x |= Rport8;
	io->pcmr[1].option = x;

	if(pp->cpresent & (1<<Rconfig)){
		/*  Reset adapter */
		mm = pcmmap(pp->slotno, pp->caddr + Rconfig, 1, 1);
		p = KADDR(mm->isa + pp->caddr + Rconfig - mm->ca);

		/* set configuration and interrupt type */
		x = 1;
		x |= ct->index<<1;
		if((ct->irqtype & 0x20) && ((ct->irqtype & 0x40)==0 || isa->irq>7)) {
			x |= Clevel;
		}
		*p = x;
		delay(5);
		pcmunmap(pp->slotno, mm);
	}
	return 0;
}

/*
 *  look for a card whose version contains 'idstr'
 */
int
pcmspecial(char *idstr, ISAConf *isa)
{
	Slot *pp;
	extern char *strstr(char*, char*);

	for(pp = slot; pp < slot+nslot; pp++){
		if(pp->special)
			continue;	/* already taken */
		increfp(pp);

		if(pp->occupied)
		if(strstr(pp->verstr, idstr))
		if(isa == 0 || pcmio(pp->slotno, isa) == 0){
			pp->special = 1;
			return pp->slotno;
		}

		decrefp(pp);
	}
	return -1;
}

void
pcmspecialclose(int slotno)
{
	Slot *pp;

	if(slotno >= nslot)
		panic("pcmspecialclose");
	pp = slot + slotno;
	pp->special = 0;
	decrefp(pp);
}

static void
slotinfo(Slot *pp)
{
	ulong pipr;

	pipr = m->iomem->pipr;
	if(pp->slotno == 0)
		pipr >>= 16;
	pp->occupied = (pipr&(Ccd1|Ccd2))==0;
	pp->battery = (pipr & (Cbvd1|Cbvd2)) != 0;
	pp->voltage = ((pipr & Cvs1) != 0) | (((pipr & Cvs2) != 0)<<1);
	pp->wrprot = (pipr&Cwp)==0;
	pp->busy = (pipr&Crdy)==0;
	pp->powered = 1;	// assume power is always on for the momment
}

/*
 *  read and crack the card information structure enough to set
 *  important parameters like power
 */
static void	tcfig(Slot*, int);
static void	tentry(Slot*, int);
static void	tvers1(Slot*, int);

static void (*parse[256])(Slot*, int) =
{
[0x15]	tvers1,
[0x1A]	tcfig,
[0x1B]	tentry,
};

static int
readc(Slot *pp, uchar *x)
{
	if(pp->cispos >= Mgran)
		return 0;
	*x = pp->cisbase[2*pp->cispos];
	pp->cispos++;
	return 1;
}

static void
cisread(Slot *pp)
{
	uchar link;
	uchar type;
	int this, i;
	PCMmap *m;

	memset(pp->ctab, 0, sizeof(pp->ctab));
	pp->caddr = 0;
	pp->cpresent = 0;
	pp->configed = 0;
	pp->nctab = 0;

	m = pcmmap(pp->slotno, 0, 0, 1);
	if(m == 0)
		return;
	pp->cisbase = KADDR(m->isa);
	pp->cispos = 0;

	/* loop through all the tuples */
	for(i = 0; i < 1000; i++){
		this = pp->cispos;
		if(readc(pp, &type) != 1)
			break;
		if(type == 0xFF)
			break;
		if(readc(pp, &link) != 1)
			break;
		if(parse[type])
			(*parse[type])(pp, type);
		if(link == 0xff)
			break;
		pp->cispos = this + (2+link);
	}
	pcmunmap(pp->slotno, m);
}

static ulong
getlong(Slot *pp, int size)
{
	uchar c;
	int i;
	ulong x;

	x = 0;
	for(i = 0; i < size; i++){
		if(readc(pp, &c) != 1)
			break;
		x |= c<<(i*8);
	}
	return x;
}

static void
tcfig(Slot *pp, int )
{
	uchar size, rasize, rmsize;
	uchar last;

	if(readc(pp, &size) != 1)
		return;
	rasize = (size&0x3) + 1;
	rmsize = ((size>>2)&0xf) + 1;
	if(readc(pp, &last) != 1)
		return;
	pp->caddr = getlong(pp, rasize);
	pp->cpresent = getlong(pp, rmsize);
}

/*
 *  enable the slot card
 */
static void
slotena(Slot *pp)
{
	IMM *io;

	if(pp->enabled)
		return;

	io = m->iomem;
//	io->pgcr[pp->slotno] |= Coe;
	io->pgcr[pp->slotno] &= ~Coe;	// active low
	delay(300);
	io->pgcr[pp->slotno] |= Creset;
	delay(100);
	io->pgcr[pp->slotno] &= ~Creset;
	delay(500);
	/* get configuration */
	slotinfo(pp);
	if(pp->occupied){
		cisread(pp);
		pp->enabled = 1;
	}
}

/*
 *  disable the slot card
 */
static void
slotdis(Slot *pp)
{
	pp->enabled = 0;
}

static void
increfp(Slot *pp)
{
	lock(pp);
	if(pp->ref++ == 0)
		slotena(pp);
	unlock(pp);
}

static void
decrefp(Slot *pp)
{
	lock(pp);
	if(pp->ref-- == 1)
		slotdis(pp);
	unlock(pp);
}

static ulong vexp[8] =
{
	1, 10, 100, 1000, 10000, 100000, 1000000, 10000000
};
static ulong vmant[16] =
{
	10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, 90,
};

static ulong
microvolt(Slot *pp)
{
	uchar c;
	ulong microvolts;
	ulong exp;

	if(readc(pp, &c) != 1)
		return 0;
	exp = vexp[c&0x7];
	microvolts = vmant[(c>>3)&0xf]*exp;
	while(c & 0x80){
		if(readc(pp, &c) != 1)
			return 0;
		switch(c){
		case 0x7d:
			break;		/* high impedence when sleeping */
		case 0x7e:
		case 0x7f:
			microvolts = 0;	/* no connection */
			break;
		default:
			exp /= 10;
			microvolts += exp*(c&0x7f);
		}
	}
	return microvolts;
}

static ulong
nanoamps(Slot *pp)
{
	uchar c;
	ulong nanoamps;

	if(readc(pp, &c) != 1)
		return 0;
	nanoamps = vexp[c&0x7]*vmant[(c>>3)&0xf];
	while(c & 0x80){
		if(readc(pp, &c) != 1)
			return 0;
		if(c == 0x7d || c == 0x7e || c == 0x7f)
			nanoamps = 0;
	}
	return nanoamps;
}

/*
 *  only nominal voltage is important for config
 */
static ulong
power(Slot *pp)
{
	uchar feature;
	ulong mv;

	mv = 0;
	if(readc(pp, &feature) != 1)
		return 0;
	if(feature & 1)
		mv = microvolt(pp);
	if(feature & 2)
		microvolt(pp);
	if(feature & 4)
		microvolt(pp);
	if(feature & 8)
		nanoamps(pp);
	if(feature & 0x10)
		nanoamps(pp);
	if(feature & 0x20)
		nanoamps(pp);
	if(feature & 0x40)
		nanoamps(pp);
	return mv/1000000;
}

static ulong mantissa[16] =
{ 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, };

static ulong exponent[8] =
{ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, };

static ulong
ttiming(Slot *pp, int scale)
{
	uchar unscaled;
	ulong nanosecs;

	if(readc(pp, &unscaled) != 1)
		return 0;
	nanosecs = (mantissa[(unscaled>>3)&0xf]*exponent[unscaled&7])/10;
	nanosecs = nanosecs * vexp[scale];
	return nanosecs;
}

static void
timing(Slot *pp, Conftab *ct)
{
	uchar c, i;

	if(readc(pp, &c) != 1)
		return;
	i = c&0x3;
	if(i != 3)
		ct->maxwait = ttiming(pp, i);		/* max wait */
	i = (c>>2)&0x7;
	if(i != 7)
		ct->readywait = ttiming(pp, i);		/* max ready/busy wait */
	i = (c>>5)&0x7;
	if(i != 7)
		ct->otherwait = ttiming(pp, i);		/* reserved wait */
}

static void
iospaces(Slot *pp, Conftab *ct)
{
	uchar c;
	int i, nio;

	ct->nio = 0;
	if(readc(pp, &c) != 1)
		return;

	ct->bit16 = ((c>>5)&3) >= 2;
	if(!(c & 0x80)){
		ct->io[0].start = 0;
		ct->io[0].len = 1<<(c&0x1f);
		ct->nio = 1;
		return;
	}

	if(readc(pp, &c) != 1)
		return;

	nio = (c&0xf)+1;
	for(i = 0; i < nio; i++){
		ct->io[i].start = getlong(pp, (c>>4)&0x3);
		ct->io[0].len = getlong(pp, (c>>6)&0x3);
	}
	ct->nio = nio;
}

static void
irq(Slot *pp, Conftab *ct)
{
	uchar c;

	if(readc(pp, &c) != 1)
		return;
	ct->irqtype = c & 0xe0;
	if(c & 0x10)
		ct->irqs = getlong(pp, 2);
	else
		ct->irqs = 1<<(c&0xf);
	ct->irqs &= 0xDEB8;		/* levels available to card */
}

static void
memspace(Slot *pp, int asize, int lsize, int host)
{
	ulong haddress, address, len;

	len = getlong(pp, lsize)*256;
	address = getlong(pp, asize)*256;
	USED(len, address);
	if(host){
		haddress = getlong(pp, asize)*256;
		USED(haddress);
	}
}

static void
tentry(Slot *pp, int )
{
	uchar c, i, feature;
	Conftab *ct;

	if(pp->nctab >= Maxctab)
		return;
	if(readc(pp, &c) != 1)
		return;
	ct = &pp->ctab[pp->nctab++];

	/* copy from last default config */
	if(pp->def)
		*ct = *pp->def;

	ct->index = c & 0x3f;

	/* is this the new default? */
	if(c & 0x40)
		pp->def = ct;

	/* memory wait specified? */
	if(c & 0x80){
		if(readc(pp, &i) != 1)
			return;
		if(i&0x80)
			ct->memwait = 1;
	}

	if(readc(pp, &feature) != 1)
		return;
	switch(feature&0x3){
	case 1:
		ct->vpp1 = ct->vpp2 = power(pp);
		break;
	case 2:
		power(pp);
		ct->vpp1 = ct->vpp2 = power(pp);
		break;
	case 3:
		power(pp);
		ct->vpp1 = power(pp);
		ct->vpp2 = power(pp);
		break;
	default:
		break;
	}
	if(feature&0x4)
		timing(pp, ct);
	if(feature&0x8)
		iospaces(pp, ct);
	if(feature&0x10)
		irq(pp, ct);
	switch((feature>>5)&0x3){
	case 1:
		memspace(pp, 0, 2, 0);
		break;
	case 2:
		memspace(pp, 2, 2, 0);
		break;
	case 3:
		if(readc(pp, &c) != 1)
			return;
		for(i = 0; i <= (c&0x7); i++)
			memspace(pp, (c>>5)&0x3, (c>>3)&0x3, c&0x80);
		break;
	}
	pp->configed++;
}

static void
tvers1(Slot *pp, int )
{
	uchar c, major, minor;
	int  i;

	if(readc(pp, &major) != 1)
		return;
	if(readc(pp, &minor) != 1)
		return;
	for(i = 0; i < sizeof(pp->verstr)-1; i++){
		if(readc(pp, &c) != 1)
			return;
		if(c == 0)
			c = '\n';
		if(c == 0xff)
			break;
		pp->verstr[i] = c;
	}
	pp->verstr[i] = 0;
}
