#include "u.h"
#include "lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "error.h"
#include "io.h"

/*
 *  Intel 82365SL PCIC controller and compatibles.
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

	/*
	 *  configuration registers - they start at an offset in attribute
	 *  memory found in the CIS.
	 */
	Rconfig=	0,
	 Creset=	 (1<<7),	/*  reset device */
	 Clevel=	 (1<<6),	/*  level sensitive interrupt line */
	 Cirq=		 (1<<2),	/*  IRQ enable */
	 Cdecode=	 (1<<1),	/*  address decode */
	 Cfunc=		 (1<<0),	/*  function enable */
	Riobase0=	5,
	Riobase1=	6,
	Riosize=	9,
};

static int pcmcia_pcmspecial(char *, ISAConf *);
static void pcmcia_pcmspecialclose(int);

#define MAP(x,o)	(Rmap + (x)*0x8 + o)

typedef struct I82365	I82365;

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
static PCMslot	*slot;
static PCMslot	*lastslot;
static nslot;

static void	i82365intr(Ureg*, void*);
static void	i82365reset(void);
static int	pcmio(int, ISAConf*);

static void i82365dump(PCMslot*);

void
devi82365link(void)
{
	static int already;
	char *p;

	if(already)
		return;
	already = 1;

	if((p=getconf("pcmcia0")) && strncmp(p, "disabled", 8)==0)
		return;

	if (_pcmspecial)
		return;
	
	_pcmspecial = pcmcia_pcmspecial;
	_pcmspecialclose = pcmcia_pcmspecialclose;
}

/*
 *  reading and writing card registers
 */
static uchar
rdreg(PCMslot *pp, int index)
{
	outb(((I82365*)pp->cp)->xreg, pp->base + index);
	return inb(((I82365*)pp->cp)->dreg);
}
static void
wrreg(PCMslot *pp, int index, uchar val)
{
	outb(((I82365*)pp->cp)->xreg, pp->base + index);
	outb(((I82365*)pp->cp)->dreg, val);
}

/*
 *  get info about card
 */
static void
slotinfo(PCMslot *pp)
{
	uchar isr;

	isr = rdreg(pp, Ris);
	pp->occupied = (isr & (3<<2)) == (3<<2);
	pp->powered = isr & (1<<6);
	pp->battery = (isr & 3) == 3;
	pp->wrprot = isr & (1<<4);
	pp->busy = isr & (1<<5);
	//pp->msec = TK2MS(MACHP(0)->ticks);
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
slotena(PCMslot *pp)
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
		pcmcisread(pp);
		pp->enabled = 1;
	} else
		wrreg(pp, Rpc, Fautopower);
}

/*
 *  disable the slot card
 */
static void
slotdis(PCMslot *pp)
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
	PCMslot *pp;

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
	PCMslot *pp;
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
	for(m = pp->mmap; m < &pp->mmap[nelem(pp->mmap)]; m++){
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
	PCMslot *pp;

	pp = slot + slotno;
	lock(&pp->mlock);
	m->ref--;
	unlock(&pp->mlock);
}

static void
increfp(PCMslot *pp)
{
	lock(pp);
	if(pp->ref++ == 0)
		slotena(pp);
	unlock(pp);
}

static void
decrefp(PCMslot *pp)
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
	PCMslot *pp;
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
			if(strstr(pp->verstr, idstr)){
				if (!enabled){
					enabled = 1;
					increfp(pp);
				}
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
	PCMslot *pp;

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
		return 0;		/* not a memory & I/O card */
	if((id & 0x0f) == 0x00)
		return 0;		/* no revision number, not possible */

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

	/* if it's not a Cirrus, it could be a Vadem... */
	if(cp->type == Ti82365){
		/* unlock the Vadem extended regs */
		outb(x, 0x0E + (dev<<7));
		outb(x, 0x37 + (dev<<7));

		/* make the id register show the Vadem id */
		outb(x, 0x3A + (dev<<7));
		c = inb(d);
		outb(d, c|0xC0);
		outb(x, Rid + (dev<<7));
		c = inb(d);
		if(c & 0x08)
			cp->type = Tvg46x;

		/* go back to Intel compatible id */
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
i82365dump(PCMslot *pp)
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
	PCMslot *pp;

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
	slot = xalloc(nslot * sizeof(PCMslot));

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
			pp->msec = ~0;
			pp->verstr[0] = 0;
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
 *  configure the PCMslot for IO.  We assume very heavily that we can read
 *  configuration info from the CIS.  If not, we won't set up correctly.
 */
static int
pcmio(int slotno, ISAConf *isa)
{
	uchar we, x, *p;
	PCMslot *pp;
	PCMconftab *ct, *et, *t;
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

	/*
	 * enable io port map 0
	 * the 'top' register value includes the last valid address
	 */
	if(isa->port == 0)
		isa->port = ct->io[0].start;
	we = rdreg(pp, Rwe);
	wrreg(pp, Riobtm0lo, isa->port);
	wrreg(pp, Riobtm0hi, isa->port>>8);
	i = isa->port+ct->io[0].len-1;
	wrreg(pp, Riotop0lo, i);
	wrreg(pp, Riotop0hi, i>>8);
	we |= 1<<6;
	if(ct->nio >= 2 && ct->io[1].start){
		wrreg(pp, Riobtm1lo, ct->io[1].start);
		wrreg(pp, Riobtm1hi, ct->io[1].start>>8);
		i = ct->io[1].start+ct->io[1].len-1;
		wrreg(pp, Riotop1lo, i);
		wrreg(pp, Riotop1hi, i>>8);
		we |= 1<<7;
	}
	wrreg(pp, Rwe, we);

	/* only touch Rconfig if it is present */
	m = pcmmap(slotno, pp->cfg[0].caddr + Rconfig, 0x20, 1);
	p = KADDR(m->isa + pp->cfg[0].caddr - m->ca);
	if(pp->cfg[0].cpresent & (1<<Rconfig)){
		/*  Reset adapter */

		/*  set configuration and interrupt type.
		 *  if level is possible on the card, use it.
		 */
		x = ct->index;
		if(ct->irqtype & 0x20)
			x |= Clevel;

		/*  enable the device, enable address decode and
		 *  irq enable.
		 */
		x |= Cfunc|Cdecode|Cirq;

		p[0] = x;
		//delay(5);
		microdelay(40);
	}

	if(pp->cfg[0].cpresent & (1<<Riobase0)){
		/* set up the iobase 0 */
		p[Riobase0 << 1] = isa->port;
		p[Riobase1 << 1] = isa->port >> 8;
	}

	if(pp->cfg[0].cpresent & (1<<Riosize))
		p[Riosize << 1] = ct->io[0].len;
	pcmunmap(slotno, m);
	return 0;
}
