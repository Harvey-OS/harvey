#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "../port/error.h"
#include "io.h"
#include "devtab.h"

/*
 *  Support for up to 4 Slot card slots.  Generalizing above that is hard
 *  since addressing is not obvious. - presotto
 *
 *  WARNING: This has never been tried with more than one card slot.
 */

/*
 *  Intel 82365SL PCIC controller for the PCMCIA or
 *  Cirrus Logic PD6710/PD6720 which is mostly register compatible
 */
enum
{
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
	 Fwidth16=	 (1<<0),	/*  16 bit data width */
	 Fiocs16=	 (1<<1),	/*  IOCS16 determines data width */
	 Ftiming=	 (1<<3),	/*  timing register to use */
	Riobtm0lo=	0x8,		/* I/O address 0 start low byte */
	Riobtm0hi=	0x9,		/* I/O address 0 start high byte */
	Riotop0lo=	0xa,		/* I/O address 0 stop low byte */
	Riotop0hi=	0xb,		/* I/O address 0 stop high byte */
	Riobtm1lo=	0xc,		/* I/O address 1 start low byte */
	Riobtm1hi=	0xd,		/* I/O address 1 start high byte */
	Riotop1lo=	0xe,		/* I/O address 1 stop low byte */
	Riotop1hi=	0xf,		/* I/O address 1 stop high byte */
	Rmap=		0x10,		/* map 0 */

	/*
	 *  CL-PD67xx extension registers
	 */
	Rmisc1=		0x16,		/* misc control 1 */
	 F5Vdetect=	 (1<<0),
	 Fvcc3V=	 (1<<1),
	 Fpmint=	 (1<<2),
	 Fpsirq=	 (1<<3),
	 Fspeaker=	 (1<<4),
	 Finpack=	 (1<<7),
	Rfifo=		0x17,		/* fifo control */
	 Fflush=	 (1<<7),	/*  flush fifo */
	Rmisc2=		0x1E,		/* misc control 2 */
	 Flowpow=	 (1<<1),	/*  low power mode */
	Rchipinfo=	0x1F,		/* chip information */
	Ratactl=	0x26,		/* ATA control */

	/*
	 *  offsets into the system memory address maps
	 */
	Mbtmlo=		0x0,		/* System mem addr mapping start low byte */
	Mbtmhi=		0x1,		/* System mem addr mapping start high byte */
	 F16bit=	 (1<<7),	/*  16-bit wide data path */
	Mtoplo=		0x2,		/* System mem addr mapping stop low byte */
	Mtophi=		0x3,		/* System mem addr mapping stop high byte */
	 Ftimer1=	 (1<<6),	/*  timer set 1 */
	Mofflo=		0x4,		/* Card memory offset address low byte */
	Moffhi=		0x5,		/* Card memory offset address high byte */
	 Fregactive=	 (1<<6),	/*  attribute memory */

	Mbits=		13,		/* msb of Mchunk */
	Mchunk=		1<<Mbits,	/* logical mapping granularity */
	Nmap=		4,		/* max number of maps to use */

	/*
	 *  configuration registers - they start at an offset in attribute
	 *  memory found in the CIS.
	 */
	Rconfig=	0,
	 Creset=	 (1<<7),	/*  reset device */
	 Clevel=	 (1<<6),	/*  level sensitive interrupt line */

	Maxctab=	8,		/* maximum configuration table entries */
};

#define MAP(x,o)	(Rmap + (x)*0x8 + o)

typedef struct I82365	I82365;
typedef struct Slot	Slot;
typedef struct Conftab	Conftab;

/* a controller */
enum
{
	Ti82365,
	Tpd6710,
	Tpd6720,
};
struct I82365
{
	int	type;
	int	dev;
	int	nslot;
	int	xreg;		/* index register address */
	int	dreg;		/* data register address */
};
static I82365 *controller[4];
static int ncontroller;

/* configuration table entry */
struct Conftab
{
	int	index;
	ushort	irqs;		/* legal irqs */
	ushort	port;		/* port address */
	uchar	irqtype;
	uchar	nioregs;	/* number of io registers */
	uchar	bit16;		/* true for 16 bit access */
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

	I82365	*cp;		/* controller for this slot */
	long	memlen;		/* memory length */
	uchar	base;		/* index register base */
	uchar	slotno;		/* slot number */

	/* status */
	uchar	special;	/* in use for a special device */
	uchar	already;	/* already inited */
	uchar	occupied;
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
static Slot	*slot;
static Slot	*lastslot;
static nslot;

static void	cisread(Slot*);
static void	i82365intr(Ureg*, void*);
static int	pcmio(int, ISAConf*);
static long	pcmread(int, int, void*, long, ulong);
static long	pcmwrite(int, int, void*, long, ulong);

/*
 *  reading and writing card registers
 */
static uchar
rdreg(Slot *pp, int index)
{
	outb(pp->cp->xreg, pp->base + index);
	return inb(pp->cp->dreg);
}
static void
wrreg(Slot *pp, int index, uchar val)
{
	outb(pp->cp->xreg, pp->base + index);
	outb(pp->cp->dreg, val);
}

/*
 *  get info about card
 */
static void
slotinfo(Slot *pp)
{
	uchar isr;

	isr = rdreg(pp, Ris);
	pp->occupied = (isr & (3<<2)) == (3<<2);
	pp->powered = isr & (1<<6);
	pp->battery = (isr & 3) == 3;
	pp->wrprot = isr & (1<<4);
	pp->busy = isr & (1<<5);
}

static int
vcode(int volt)
{
	switch(volt){
	case 5:
		return 1;
	case 12:
		return 2;
	default:
		return 0;
	}
}

/*
 *  enable the slot card
 */
static void
slotena(Slot *pp)
{
	if(pp->enabled)
		return;

	/* power up and unreset, wait's are empirical (???) */
	wrreg(pp, Rpc, Fautopower|Foutena|Fcardena);
	delay(300);
	wrreg(pp, Rigc, 0);
	delay(100);
	wrreg(pp, Rigc, Fnotreset);
	delay(500);

	/* get configuration */
	slotinfo(pp);
	if(pp->occupied){
		cisread(pp);
		pp->enabled = 1;
	} else
		wrreg(pp, Rpc, Fautopower);
}

/*
 *  disable the slot card
 */
static void
slotdis(Slot *pp)
{
	wrreg(pp, Rpc, 0);	/* turn off card power */
	wrreg(pp, Rwe, 0);	/* no windows */
	pp->enabled = 0;
}

/*
 *  status change interrupt
 */
static void
i82365intr(Ureg *ur, void *a)
{
	uchar csc, was;
	Slot *pp;

	USED(ur,a);
	if(slot == 0)
		return;

	for(pp = slot; pp < lastslot; pp++){
		csc = rdreg(pp, Rcsc);
		was = pp->occupied;
		slotinfo(pp);
		if(csc & (1<<3) && was != pp->occupied){
			if(pp->occupied)
				print("slot%d card inserted\n", pp->slotno);
			else {
				print("slot%d card removed\n", pp->slotno);
				slotdis(pp);
			}
		}
	}
}

enum
{
	Mshift=	12,
	Mgran=	(1<<Mshift),	/* granularity of maps */
	Mmask=	~(Mgran-1),	/* mask for address bits important to the chip */
};

/*
 *  get a map for pc card region, return corrected len
 */
PCMmap*
pcmmap(int slotno, ulong offset, int len, int attr)
{
	Slot *pp;
	uchar we, bit;
	PCMmap *m, *nm;
	int i;
	ulong e;

	pp = slot + slotno;
	lock(&pp->mlock);

	/* convert offset to granularity */
	if(len <= 0)
		len = 1;
	e = ROUND(offset+len, Mgran);
	offset &= Mmask;
	len = e - offset;

	/* look for a map that covers the right area */
	we = rdreg(pp, Rwe);
	bit = 1;
	nm = 0;
	for(m = pp->mmap; m < &pp->mmap[Nmap]; m++){
		if((we & bit))
		if(m->attr == attr)
		if(offset >= m->ca && e <= m->cea){

			m->ref++;
			unlock(&pp->mlock);
			return m;
		}
		bit <<= 1;
		if(nm == 0 && m->ref == 0)
			nm = m;
	}
	m = nm;
	if(m == 0){
		unlock(&pp->mlock);
		return 0;
	}

	/* if isa space isn't big enough, free it and get more */
	if(m->len < len){
		if(m->isa){
			putisa(m->isa, m->len);
			m->len = 0;
		}
		m->isa = getisa(0, len, Mgran)&~KZERO;
		if(m->isa == 0){
			print("pcmmap: out of isa space\n");
			unlock(&pp->mlock);
			return 0;
		}
		m->len = len;
	}

	/* set up new map */
	m->ca = offset;
	m->cea = m->ca + m->len;
	m->attr = attr;
	i = m-pp->mmap;
	bit = 1<<i;
	wrreg(pp, Rwe, we & ~bit);		/* disable map before changing it */
	wrreg(pp, MAP(i, Mbtmlo), m->isa>>Mshift);
	wrreg(pp, MAP(i, Mbtmhi), (m->isa>>(Mshift+8)) | F16bit);
	wrreg(pp, MAP(i, Mtoplo), (m->isa+m->len-1)>>Mshift);
	wrreg(pp, MAP(i, Mtophi), ((m->isa+m->len-1)>>(Mshift+8)));
	offset -= m->isa;
	offset &= (1<<25)-1;
	offset >>= Mshift;
	wrreg(pp, MAP(i, Mofflo), offset);
	wrreg(pp, MAP(i, Moffhi), (offset>>8) | (attr ? Fregactive : 0));
	wrreg(pp, Rwe, we | bit);		/* enable map */
	m->ref = 1;

	unlock(&pp->mlock);
	return m;
}

void
pcmunmap(int slotno, PCMmap* m)
{
	Slot *pp;

	pp = slot + slotno;
	lock(&pp->mlock);
	m->ref--;
	unlock(&pp->mlock);
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

/*
 *  look for a card whose version contains 'idstr'
 */
int
pcmspecial(char *idstr, ISAConf *isa)
{
	Slot *pp;
	extern char *strstr(char*, char*);

	i82365reset();
	for(pp = slot; pp < lastslot; pp++){
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

static int
pcmgen(Chan *c, Dirtab *tab, int ntab, int i, Dir *dp)
{
	int slotno;
	Qid qid;
	long len;
	Slot *pp;
	char name[NAMELEN];

	USED(tab, ntab);
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

static char *chipname[] =
{
[Ti82365]	"Intel 82365SL",
[Tpd6710]	"Cirrus Logic PD6710",
[Tpd6720]	"Cirrus Logic PD6720",
};

static I82365*
i82386probe(int x, int d, int dev)
{
	uchar c;
	I82365 *cp;

	outb(x, Rid + (dev<<7));
	c = inb(d);
	if((c & 0xf0) != 0x80)
		return 0;		/* not this family */

	cp = xalloc(sizeof(I82365));
	cp->xreg = x;
	cp->dreg = d;
	cp->dev = dev;
	cp->type = Ti82365;
	cp->nslot = 2;

	switch(c){
	case 0x82:
	case 0x83:
		/* could be a cirrus */
		outb(x, Rchipinfo + (dev<<7));
		outb(d, 0);
		c = inb(d);
		if((c & 0xdf) == 0xdc){
			c = inb(d);
			if((c & 0xdf) != 0x0c)
				break;
		}
		if(c & 0x40){
			cp->type = Tpd6720;
		} else {
			cp->type = Tpd6710;
			cp->nslot = 1;
		}
		break;
	}

	print("pcmcia controller%d is a %d slot %s\n", ncontroller, cp->nslot,
		chipname[cp->type]);

	/* low power mode */
	outb(x, Rmisc2 + (dev<<7));
	outb(d, Flowpow);

	controller[ncontroller++] = cp;
	return cp;
}

static void
i82365dump(Slot *pp)
{
	int i;

	for(i = 0; i < 0x40; i++){
		if((i&0x7) == 0)
			print("\n%ux:	", i);
		print("%ux ", rdreg(pp, i));
	}
	print("\n");
}

/*
 *  set up for slot cards
 */
void
i82365reset(void)
{
	static int already;
	int i, j;
	I82365 *cp;
	Slot *pp;

	if(already)
		return;
	already = 1;

	/* look for controllers */
	i82386probe(0x3E0, 0x3E1, 0);
	i82386probe(0x3E0, 0x3E1, 1);
	i82386probe(0x3E2, 0x3E3, 0);
	i82386probe(0x3E2, 0x3E3, 1);
	for(i = 0; i < ncontroller; i++)
		nslot += controller[i]->nslot;
	slot = xalloc(nslot * sizeof(Slot));

	/* if the card is there turn on 5V power to keep its battery alive */
	lastslot = slot;
	for(i = 0; i < ncontroller; i++){
		cp = controller[i];
		for(j = 0; j < cp->nslot; j++){
			pp = lastslot++;
			pp->slotno = pp - slot;
			pp->memlen = 64*MB;
			pp->base = (cp->dev<<7) | (j<<6);
			pp->cp = cp;
			slotdis(pp);

			/* interrupt on status change */
			wrreg(pp, Rcscic, ((PCMCIAvec-Int0vec)<<4) | Fchangeena);
		}
	}

	/* for card management interrupts */
	setvec(PCMCIAvec, i82365intr, 0);
}

void
i82365init(void)
{
}

Chan *
i82365attach(char *spec)
{
	return devattach('y', spec);
}

Chan *
i82365clone(Chan *c, Chan *nc)
{
	return devclone(c, nc);
}

int
i82365walk(Chan *c, char *name)
{
	return devwalk(c, name, 0, 0, pcmgen);
}

void
i82365stat(Chan *c, char *db)
{
	devstat(c, db, 0, 0, pcmgen);
}

Chan *
i82365open(Chan *c, int omode)
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

void
i82365create(Chan *c, char *name, int omode, ulong perm)
{
	USED(c, name, omode, perm);
	error(Eperm);
}

void
i82365remove(Chan *c)
{
	USED(c);
	error(Eperm);
}

void
i82365wstat(Chan *c, char *dp)
{
	USED(c, dp);
	error(Eperm);
}

void
i82365close(Chan *c)
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

/* a memmove using only shorts & bytes */
static void
memmoves(uchar *to, uchar *from, int n)
{
	ushort *t, *f;

	if((((ulong)to) & 1) || (((ulong)from) & 1) || (n & 1)){
		while(n-- > 0)
			*to++ = *from++;
	} else {
		n = n/2;
		t = (ushort*)to;
		f = (ushort*)from;
		while(n-- > 0)
			*t++ = *f++;
	}
}

static long
pcmread(int slotno, int attr, void *a, long n, ulong offset)
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

long
i82365read(Chan *c, void *a, long n, ulong offset)
{
	char *cp, buf[2048];
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
{
	int i;

	for(i = 0; i < 0x40; i++){
		if((i&0x7) == 0)
			cp += sprint(cp, "\n%ux:	", i);
		cp += sprint(cp, "%ux ", rdreg(pp, i));
	}
	cp += sprint(cp, "\n");
}
		*cp = 0;
		return readstr(offset, a, n, buf);
	default:
		n=0;
		break;
	}
	return n;
}

static long
pcmwrite(int dev, int attr, void *a, long n, ulong offset)
{
	int i, len;
	PCMmap *m;
	ulong ka;
	uchar *ac;
	Slot *pp;

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

long
i82365write(Chan *c, void *a, long n, ulong offset)
{
	ulong p;
	Slot *pp;

	p = TYPE(c);
	switch(p){
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

/*
 *  configure the Slot for IO.  We assume very heavily that we can read
 *  cofiguration info from the CIS.  If not, we won't set up correctly.
 */
static int
pcmio(int slotno, ISAConf *isa)
{
	uchar we, x, *p;
	Slot *pp;
	Conftab *ct;
	PCMmap *m;
	int irq;

	irq = isa->irq;
	if(irq == 2)
		irq = 9;

	if(slotno > nslot)
		return -1;
	pp = slot + slotno;

	if(!pp->occupied)
		return -1;

	/* find a configuration with the right port */
	for(ct = pp->ctab; ct < &pp->ctab[pp->nctab]; ct++){
		if(ct->nioregs && ct->port == isa->port && ((1<<irq) & ct->irqs))
			break;
	}

	/* if non found, settle for one with the some ioregs */
	if(ct == &pp->ctab[pp->nctab])
		for(ct = pp->ctab; ct < &pp->ctab[pp->nctab]; ct++)
			if(ct->nioregs && ((1<<irq) & ct->irqs))
				break;

	if(ct == &pp->ctab[pp->nctab])
		return -1;

	/* route interrupts */
	isa->irq = irq;
	wrreg(pp, Rigc, irq | Fnotreset | Fiocard);
	
	/* set power and enable device */
	x = vcode(ct->vpp1);
	wrreg(pp, Rpc, x|Fautopower|Foutena|Fcardena);

	/* 16-bit data path */
	if(ct->bit16)
		x = Fiocs16|Fwidth16;
	else
		x = 0;
	wrreg(pp, Rio, Ftiming|x);

	/* enable io port map 0 */
	if(isa->port == 0)
		isa->port = ct->port;
	we = rdreg(pp, Rwe);
	wrreg(pp, Riobtm0lo, isa->port);
	wrreg(pp, Riobtm0hi, isa->port>>8);
	wrreg(pp, Riotop0lo, (isa->port+ct->nioregs-1));
	wrreg(pp, Riotop0hi, (isa->port+ct->nioregs-1)>>8);
	wrreg(pp, Rwe, we | (1<<6));

	/* only touch Rconfig if it is present */
	if(pp->cpresent & (1<<Rconfig)){
		/*  Reset adapter */
		m = pcmmap(slotno, pp->caddr + Rconfig, 1, 1);
		p = (uchar*)(KZERO|(m->isa + pp->caddr + Rconfig - m->ca));

		/* set configuration and interrupt type */
		x = ct->index;
		if((ct->irqtype & 0x20) && ((ct->irqtype & 0x40)==0 || isa->irq>7))
			x |= Clevel;
		*p = x;
		delay(5);

		pcmunmap(slotno, m);
	}
	return 0;
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
	if(pp->cispos >= Mchunk)
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
	pp->cisbase = (uchar*)(KZERO|m->isa);
	pp->cispos = 0;

	/* loop through all the tuples */
	for(i = 0; i < 1000; i++){
		this = pp->cispos;
		if(readc(pp, &type) != 1)
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
tcfig(Slot *pp, int ttype)
{
	uchar size, rasize, rmsize;
	uchar last;

	USED(ttype);
	if(readc(pp, &size) != 1)
		return;
	rasize = (size&0x3) + 1;
	rmsize = ((size>>2)&0xf) + 1;
	if(readc(pp, &last) != 1)
		return;
	pp->caddr = getlong(pp, rasize);
	pp->cpresent = getlong(pp, rmsize);
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

void
iospaces(Slot *pp, Conftab *ct)
{
	uchar c;
	int i;
	ulong len;

	if(readc(pp, &c) != 1)
		return;

	ct->nioregs = 1<<(c&0x1f);
	ct->bit16 = ((c>>5)&3) >= 2;
	if((c & 0x80) == 0)
		return;

	if(readc(pp, &c) != 1)
		return;

	for(i = (c&0xf)+1; i; i--){
		ct->port = getlong(pp, (c>>4)&0x3);
		len = getlong(pp, (c>>6)&0x3);
		USED(len);
	}
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

void
tentry(Slot *pp, int ttype)
{
	uchar c, i, feature;
	Conftab *ct;

	USED(ttype);

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

void
tvers1(Slot *pp, int ttype)
{
	uchar c, major, minor;
	int  i;

	USED(ttype);
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

