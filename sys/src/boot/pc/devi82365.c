#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "io.h"

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
	 Fzerows=	 (1<<2),	/*  zero wait state */
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

static int pcmcia_pcmspecial(char *, ISAConf *);
static void pcmcia_pcmspecialclose(int);

#define MAP(x,o)	(Rmap + (x)*0x8 + o)

typedef struct I82365	I82365;
typedef struct Slot	Slot;
typedef struct Conftab	Conftab;
typedef struct Cisdat	Cisdat;
/* a controller */
enum
{
	Ti82365,
	Tpd6710,
	Tpd6720,
	Tvg46x,
};
struct I82365
{
	int	type;
	int	dev;
	int	nslot;
	int	xreg;		/* index register address */
	int	dreg;		/* data register address */
	int	irq;
};
static I82365 *controller[4];
static int ncontroller;

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

/* cis memory walking */
struct Cisdat
{
	uchar	*cisbase;
	int	cispos;
	int	cisskip;
	int	cislen;
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
	Cisdat;

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
static void	i82365reset(void);
static int	pcmio(int, ISAConf*);
static long	pcmread(int, int, void*, long, vlong);
static long	pcmwrite(int, int, void*, long, vlong);

static void i82365dump(Slot*);

void
devi82365link(void)
{
	static int already;

	if(already)
		return;
	already = 1;

	if (_pcmspecial)
		return;
	
	_pcmspecial = pcmcia_pcmspecial;
	_pcmspecialclose = pcmcia_pcmspecialclose;
}

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
i82365intr(Ureg *, void *)
{
	uchar csc, was;
	Slot *pp;

	if(slot == 0)
		return;

	for(pp = slot; pp < lastslot; pp++){
		csc = rdreg(pp, Rcsc);
		was = pp->occupied;
		slotinfo(pp);
		if(csc & (1<<3) && was != pp->occupied){
			if(!pp->occupied)
				slotdis(pp);
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
			umbfree(m->isa, m->len);
			m->len = 0;
		}
		m->isa = PADDR(umbmalloc(0, len, Mgran));
		if(m->isa == 0){
			print("pcmmap %d: out of isa space\n", len);
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
static int
pcmcia_pcmspecial(char *idstr, ISAConf *isa)
{
	Slot *pp;
	extern char *strstr(char*, char*);
	int enabled;

	i82365reset();
	for(pp = slot; pp < lastslot; pp++){
		if(pp->special)
			continue;	/* already taken */
		enabled = 0;
		/* make sure we don't power on cards when we already know what's
		 * in them.  We'll reread every two minutes if necessary
		 */
		if (pp->verstr[0] == '\0') {
			increfp(pp);
			enabled++;
		}

		if(pp->occupied) {
			if(strstr(pp->verstr, idstr)) {
				if (!enabled)
					increfp(pp);
				if(isa == 0 || pcmio(pp->slotno, isa) == 0){
					pp->special = 1;
					return pp->slotno;
				}
			}
		} else
			pp->special = 1;
		if (enabled)
			decrefp(pp);
	}
	return -1;
}

static void
pcmcia_pcmspecialclose(int slotno)
{
	Slot *pp;

	print("pcmspecialclose called\n");
	if(slotno >= nslot)
		panic("pcmspecialclose");
	pp = slot + slotno;
	pp->special = 0;
	decrefp(pp);
}

static char *chipname[] =
{
[Ti82365]	"Intel 82365SL",
[Tpd6710]	"Cirrus Logic PD6710",
[Tpd6720]	"Cirrus Logic PD6720",
[Tvg46x]	"Vadem VG-46x",
};

static I82365*
i82365probe(int x, int d, int dev)
{
	uchar c, id;
	I82365 *cp;
	ISAConf isa;
	int i, nslot;

	outb(x, Rid + (dev<<7));
	id = inb(d);
	if((id & 0xf0) != 0x80)
		return 0;		/* not this family */

	cp = xalloc(sizeof(I82365));
	cp->xreg = x;
	cp->dreg = d;
	cp->dev = dev;
	cp->type = Ti82365;
	cp->nslot = 2;

	switch(id){
	case 0x82:
	case 0x83:
	case 0x84:
		/* could be a cirrus */
		outb(x, Rchipinfo + (dev<<7));
		outb(d, 0);
		c = inb(d);
		if((c & 0xc0) != 0xc0)
			break;
		c = inb(d);
		if((c & 0xc0) != 0x00)
			break;
		if(c & 0x20){
			cp->type = Tpd6720;
		} else {
			cp->type = Tpd6710;
			cp->nslot = 1;
		}

		/* low power mode */
		outb(x, Rmisc2 + (dev<<7));
		c = inb(d);
		outb(d, c & ~Flowpow);
		break;
	}

	if(cp->type == Ti82365){
		outb(x, 0x0E + (dev<<7));
		outb(x, 0x37 + (dev<<7));
		outb(x, 0x3A + (dev<<7));
		c = inb(d);
		outb(d, c|0xC0);
		outb(x, Rid + (dev<<7));
		c = inb(d);
		if(c != id && !(c & 0x08))
			print("#y%d: id %uX changed to %uX\n", ncontroller, id, c);
		if(c & 0x08)
			cp->type = Tvg46x;
		outb(x, 0x3A + (dev<<7));
		c = inb(d);
		outb(d, c & ~0xC0);
	}

	memset(&isa, 0, sizeof(ISAConf));
	if(isaconfig("pcmcia", ncontroller, &isa) && isa.irq)
		cp->irq = isa.irq;
	else
		cp->irq = VectorPCMCIA - VectorPIC;

	for(i = 0; i < isa.nopt; i++){
		if(cistrncmp(isa.opt[i], "nslot=", 6))
			continue;
		nslot = strtol(&isa.opt[i][6], nil, 0);
		if(nslot > 0 && nslot <= 2)
			cp->nslot = nslot;
	}

	controller[ncontroller++] = cp;
	return cp;
}

static void
i82365dump(Slot *pp)
{
	int i;

	for(i = 0; i < 0x40; i++){
		if((i&0x0F) == 0)
			print("\n%2.2uX:	", i);
		if(((i+1) & 0x0F) == 0x08)
			print(" - ");
		print("%2.2uX ", rdreg(pp, i));
	}
	print("\n");
}

/*
 *  set up for slot cards
 */
static void
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
	i82365probe(0x3E0, 0x3E1, 0);
	i82365probe(0x3E0, 0x3E1, 1);
	i82365probe(0x3E2, 0x3E3, 0);
	i82365probe(0x3E2, 0x3E3, 1);

	for(i = 0; i < ncontroller; i++)
		nslot += controller[i]->nslot;
	slot = xalloc(nslot * sizeof(Slot));

	/* if the card is there turn on 5V power to keep its battery alive */
	lastslot = slot;
	for(i = 0; i < ncontroller; i++){
		cp = controller[i];
		print("#y%d: %d slot %s: port 0x%uX irq %d\n",
			i, cp->nslot, chipname[cp->type], cp->xreg, cp->irq);
		for(j = 0; j < cp->nslot; j++){
			pp = lastslot++;
			pp->slotno = pp - slot;
			pp->memlen = 64*MB;
			pp->base = (cp->dev<<7) | (j<<6);
			pp->cp = cp;
			slotdis(pp);

			/* interrupt on status change */
			wrreg(pp, Rcscic, (cp->irq<<4) | Fchangeena);
			rdreg(pp, Rcsc);
		}

		/* for card management interrupts */
		setvec(cp->irq+VectorPIC, i82365intr, 0);
	}
}

/*
 *  configure the Slot for IO.  We assume very heavily that we can read
 *  configuration info from the CIS.  If not, we won't set up correctly.
 */
static int
pcmio(int slotno, ISAConf *isa)
{
	uchar we, x, *p;
	Slot *pp;
	Conftab *ct, *et, *t;
	PCMmap *m;
	int i, index, irq;
	char *cp;

	irq = isa->irq;
	if(irq == 2)
		irq = 9;

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
	if(isa->port == 0 && ct->io[0].start == 0)
		return -1;

	/* route interrupts */
	isa->irq = irq;
	wrreg(pp, Rigc, irq | Fnotreset | Fiocard);
	
	/* set power and enable device */
	x = vcode(ct->vpp1);
	wrreg(pp, Rpc, x|Fautopower|Foutena|Fcardena);

	/* 16-bit data path */
	if(ct->bit16)
		x = Ftiming|Fiocs16|Fwidth16;
	else
		x = Ftiming;
	if(ct->nio == 2 && ct->io[1].start)
		x |= x<<4;
	wrreg(pp, Rio, x);

	/* enable io port map 0 */
	if(isa->port == 0)
		isa->port = ct->io[0].start;
	we = rdreg(pp, Rwe);
	wrreg(pp, Riobtm0lo, isa->port);
	wrreg(pp, Riobtm0hi, isa->port>>8);
	i = isa->port+ct->io[0].len-1;
	wrreg(pp, Riotop0lo, i);
	wrreg(pp, Riotop0hi, i>>8);
	we |= 1<<6;
	if(ct->nio == 2 && ct->io[1].start){
		wrreg(pp, Riobtm1lo, ct->io[1].start);
		wrreg(pp, Riobtm1hi, ct->io[1].start>>8);
		i = ct->io[1].start+ct->io[1].len-1;
		wrreg(pp, Riotop1lo, i);
		wrreg(pp, Riotop1hi, i>>8);
		we |= 1<<7;
	}
	wrreg(pp, Rwe, we);

	/* only touch Rconfig if it is present */
	if(pp->cpresent & (1<<Rconfig)){
		/*  Reset adapter */
		m = pcmmap(slotno, pp->caddr + Rconfig, 1, 1);
		p = KADDR(m->isa + pp->caddr + Rconfig - m->ca);

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
static void	tcfig(Slot*, Cisdat*, int);
static void	tentry(Slot*, Cisdat*, int);
static void	tvers1(Slot*, Cisdat*, int);

struct {
	int n;
	void (*parse)(Slot*, Cisdat*, int);
} cistab[] = {
	0x15, tvers1,
	0x1A, tcfig,
	0x1B, tentry,
};

static int
readc(Cisdat *pp, uchar *x)
{
	if(pp->cispos >= pp->cislen)
		return 0;
	*x = pp->cisbase[pp->cisskip*pp->cispos];
	pp->cispos++;
	return 1;
}

static int
xcistuple(int slotno, int tuple, void *v, int nv, int attr)
{
	PCMmap *m;
	Cisdat cis;
	int i, l;
	uchar *p;
	uchar type, link;
	int this;

	m = pcmmap(slotno, 0, 0, attr);
	if(m == 0) {
if(debug) print("could not map\n");
		return -1;
	}

	cis.cisbase = KADDR(m->isa);
	cis.cispos = 0;
	cis.cisskip = attr ? 2 : 1;
	cis.cislen = Mchunk;

if(debug) print("cis %d %d #%lux srch %x...", attr, cis.cisskip, cis.cisbase, tuple);
	/* loop through all the tuples */
	for(i = 0; i < 1000; i++){
		this = cis.cispos;
		if(readc(&cis, &type) != 1)
			break;
if(debug) print("%2ux...", type);
		if(type == 0xFF)
			break;
		if(readc(&cis, &link) != 1)
			break;
		if(link == 0xFF)
			break;
		if(type == tuple) {
			p = v;
			for(l=0; l<nv && l<link; l++)
				if(readc(&cis, p++) != 1)
					break;
			pcmunmap(slotno, m);
if(debug) print("pcm find %2.2ux %d %d\n", type, link, l);
			return l;
		}
		cis.cispos = this + (2+link);
	}
	pcmunmap(slotno, m);
	return -1;
}

int
pcmcistuple(int slotno, int tuple, void *v, int nv)
{
	int n;

	/* try attribute space, then memory */
	if((n = xcistuple(slotno, tuple, v, nv, 1)) >= 0)
		return n;
	return xcistuple(slotno, tuple, v, nv, 0);
}

static void
cisread(Slot *pp)
{
	uchar v[256];
	int i, nv;
	Cisdat cis;

	memset(pp->ctab, 0, sizeof(pp->ctab));
	pp->caddr = 0;
	pp->cpresent = 0;
	pp->configed = 0;
	pp->nctab = 0;

	for(i = 0; i < nelem(cistab); i++) {
		if((nv = pcmcistuple(pp->slotno, cistab[i].n, v, sizeof(v))) >= 0) {
			cis.cisbase = v;
			cis.cispos = 0;
			cis.cisskip = 1;
			cis.cislen = nv;
			
			(*cistab[i].parse)(pp, &cis, cistab[i].n);
		}
	}
}

static ulong
getlong(Cisdat *cis, int size)
{
	uchar c;
	int i;
	ulong x;

	x = 0;
	for(i = 0; i < size; i++){
		if(readc(cis, &c) != 1)
			break;
		x |= c<<(i*8);
	}
	return x;
}

static void
tcfig(Slot *pp, Cisdat *cis, int )
{
	uchar size, rasize, rmsize;
	uchar last;

	if(readc(cis, &size) != 1)
		return;
	rasize = (size&0x3) + 1;
	rmsize = ((size>>2)&0xf) + 1;
	if(readc(cis, &last) != 1)
		return;
	pp->caddr = getlong(cis, rasize);
	pp->cpresent = getlong(cis, rmsize);
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
microvolt(Cisdat *cis)
{
	uchar c;
	ulong microvolts;
	ulong exp;

	if(readc(cis, &c) != 1)
		return 0;
	exp = vexp[c&0x7];
	microvolts = vmant[(c>>3)&0xf]*exp;
	while(c & 0x80){
		if(readc(cis, &c) != 1)
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
nanoamps(Cisdat *cis)
{
	uchar c;
	ulong nanoamps;

	if(readc(cis, &c) != 1)
		return 0;
	nanoamps = vexp[c&0x7]*vmant[(c>>3)&0xf];
	while(c & 0x80){
		if(readc(cis, &c) != 1)
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
power(Cisdat *cis)
{
	uchar feature;
	ulong mv;

	mv = 0;
	if(readc(cis, &feature) != 1)
		return 0;
	if(feature & 1)
		mv = microvolt(cis);
	if(feature & 2)
		microvolt(cis);
	if(feature & 4)
		microvolt(cis);
	if(feature & 8)
		nanoamps(cis);
	if(feature & 0x10)
		nanoamps(cis);
	if(feature & 0x20)
		nanoamps(cis);
	if(feature & 0x40)
		nanoamps(cis);
	return mv/1000000;
}

static ulong mantissa[16] =
{ 0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80, };

static ulong exponent[8] =
{ 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, };

static ulong
ttiming(Cisdat *cis, int scale)
{
	uchar unscaled;
	ulong nanosecs;

	if(readc(cis, &unscaled) != 1)
		return 0;
	nanosecs = (mantissa[(unscaled>>3)&0xf]*exponent[unscaled&7])/10;
	nanosecs = nanosecs * vexp[scale];
	return nanosecs;
}

static void
timing(Cisdat *cis, Conftab *ct)
{
	uchar c, i;

	if(readc(cis, &c) != 1)
		return;
	i = c&0x3;
	if(i != 3)
		ct->maxwait = ttiming(cis, i);		/* max wait */
	i = (c>>2)&0x7;
	if(i != 7)
		ct->readywait = ttiming(cis, i);	/* max ready/busy wait */
	i = (c>>5)&0x7;
	if(i != 7)
		ct->otherwait = ttiming(cis, i);	/* reserved wait */
}

static void
iospaces(Cisdat *cis, Conftab *ct)
{
	uchar c;
	int i, nio;

	ct->nio = 0;
	if(readc(cis, &c) != 1)
		return;

	ct->bit16 = ((c>>5)&3) >= 2;
	if(!(c & 0x80)){
		ct->io[0].start = 0;
		ct->io[0].len = 1<<(c&0x1f);
		ct->nio = 1;
		return;
	}

	if(readc(cis, &c) != 1)
		return;

	nio = (c&0xf)+1;
	for(i = 0; i < nio; i++){
		ct->io[i].start = getlong(cis, (c>>4)&0x3);
		ct->io[i].len = getlong(cis, (c>>6)&0x3)+1;
	}
	ct->nio = nio;
}

static void
irq(Cisdat *cis, Conftab *ct)
{
	uchar c;

	if(readc(cis, &c) != 1)
		return;
	ct->irqtype = c & 0xe0;
	if(c & 0x10)
		ct->irqs = getlong(cis, 2);
	else
		ct->irqs = 1<<(c&0xf);
	ct->irqs &= 0xDEB8;		/* levels available to card */
}

static void
memspace(Cisdat *cis, int asize, int lsize, int host)
{
	ulong haddress, address, len;

	len = getlong(cis, lsize)*256;
	address = getlong(cis, asize)*256;
	USED(len, address);
	if(host){
		haddress = getlong(cis, asize)*256;
		USED(haddress);
	}
}

static void
tentry(Slot *pp, Cisdat *cis, int )
{
	uchar c, i, feature;
	Conftab *ct;

	if(pp->nctab >= Maxctab)
		return;
	if(readc(cis, &c) != 1)
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
		if(readc(cis, &i) != 1)
			return;
		if(i&0x80)
			ct->memwait = 1;
	}

	if(readc(cis, &feature) != 1)
		return;
	switch(feature&0x3){
	case 1:
		ct->vpp1 = ct->vpp2 = power(cis);
		break;
	case 2:
		power(cis);
		ct->vpp1 = ct->vpp2 = power(cis);
		break;
	case 3:
		power(cis);
		ct->vpp1 = power(cis);
		ct->vpp2 = power(cis);
		break;
	default:
		break;
	}
	if(feature&0x4)
		timing(cis, ct);
	if(feature&0x8)
		iospaces(cis, ct);
	if(feature&0x10)
		irq(cis, ct);
	switch((feature>>5)&0x3){
	case 1:
		memspace(cis, 0, 2, 0);
		break;
	case 2:
		memspace(cis, 2, 2, 0);
		break;
	case 3:
		if(readc(cis, &c) != 1)
			return;
		for(i = 0; i <= (c&0x7); i++)
			memspace(cis, (c>>5)&0x3, (c>>3)&0x3, c&0x80);
		break;
	}
	pp->configed++;
}

static void
tvers1(Slot *pp, Cisdat *cis, int )
{
	uchar c, major, minor;
	int  i;

	if(readc(cis, &major) != 1)
		return;
	if(readc(cis, &minor) != 1)
		return;
	for(i = 0; i < sizeof(pp->verstr)-1; i++){
		if(readc(cis, &c) != 1)
			return;
		if(c == 0)
			c = '\n';
		if(c == 0xff)
			break;
		pp->verstr[i] = c;
	}
	pp->verstr[i] = 0;
}
