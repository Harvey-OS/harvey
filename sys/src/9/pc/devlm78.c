#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "ureg.h"
#include "../port/error.h"

/* this driver doesn't implement the management interrupts.  we
 * leave the LM78 interrupts set to whatever the BIOS did.  we do
 * allow reading and writing the the readouts and alarm values.
 * Read(2)ing or write(2)ing at offset 0x0-0x1f, is
 * equivalent to reading or writing lm78 registers 0x20-0x3f.
 */
enum
{
	/*  address of chip on serial interface */
	Serialaddr=	0x2d,

	/*  parallel access registers */
	Rpaddr=		0x5,
	Bbusy=		 (1<<7),
	Rpdata=		0x6,

	/*  internal register addresses */
	Rconfig=	0x40,
	Bstart=		 (1<<0),
	Bsmiena=	 (1<<1),
	Birqena=	 (1<<2),
	Bintclr=	 (1<<3),
	Breset=		 (1<<4),
	Bnmi=		 (1<<5),	/*  if set, use nmi, else irq */
	Bpowbypass=	 (1<<6),
	Binit=		 (1<<7),
	Ristat1=	0x41,
	Ristat2=	0x42,
	Rsmimask1=	0x43,
	Rsmimask2=	0x44,
	Rnmimask1=	0x45,
	Rnmimask2=	0x46,
	Rvidfan=	0x47,		/*  set fan counter, and read voltage level */
	Mvid=		 0x0f,
	Mfan=		 0xf0,
	Raddr=		0x48,		/*  address used on serial bus */
	Rresetid=	0x49,		/*  chip reset and ID register */
	Rpost=		0x00,		/*  start of post ram */
	Rvalue=		0x20,		/*  start of value ram */

	VRsize=		0x20,		/*  size of value ram */
};

enum
{
	Qdir,
	Qlm78vram,
};

static Dirtab lm78dir[] = {
	".",			{ Qdir, 0, QTDIR},	0,	0555,
	"lm78vram",	{ Qlm78vram, 0 },	0,	0444,
};

/*  interface type */
enum
{
	None=	0,
	Smbus,
	Parallel,
};

static struct {
	QLock;
	int	probed;
	int 	ifc;	/*  which interface is connected */
	SMBus	*smbus;	/*  serial interface */
	int	port;	/*  parallel interface */
} lm78;

extern SMBus*	piix4smbus(void);

/*  wait for device to become quiescent and then set the */
/*  register address */
static void
setreg(int reg)
{
	int tries;

	for(tries = 0; tries < 1000000; tries++)
		if((inb(lm78.port+Rpaddr) & Bbusy) == 0){
			outb(lm78.port+Rpaddr, reg);
			return;
		}
	error("lm78 broken");
}

/*  routines that actually touch the device */
static void
lm78wrreg(int reg, uchar val)
{
	if(waserror()){
		qunlock(&lm78);
		nexterror();
	}
	qlock(&lm78);

	switch(lm78.ifc){
	case Smbus:
		lm78.smbus->transact(lm78.smbus, SMBbytewrite, Serialaddr, reg, &val);
		break;
	case Parallel:
		setreg(reg);
		outb(lm78.port+Rpdata, val);
		break;
	default:
		error(Enodev);
		break;
	}

	qunlock(&lm78);
	poperror();
}

static int
lm78rdreg(int reg)
{
	uchar val;

	if(waserror()){
		qunlock(&lm78);
		nexterror();
	}
	qlock(&lm78);

	switch(lm78.ifc){
	case Smbus:
		lm78.smbus->transact(lm78.smbus, SMBsend, Serialaddr, reg, nil);
		lm78.smbus->transact(lm78.smbus, SMBrecv, Serialaddr, 0, &val);
		break;
	case Parallel:
		setreg(reg);
		val = inb(lm78.port+Rpdata);
		break;
	default:
		error(Enodev);
		break;
	}

	qunlock(&lm78);
	poperror();
	return val;
}

/*  start the chip monitoring but don't change any smi 
 *  interrupts and/or alarms that the BIOS may have set up. 
 *  this isn't locked because it's thought to be idempotent 
 */
static void
lm78enable(void)
{
	uchar config;

	if(lm78.ifc == None)
		error(Enodev);

	if(lm78.probed == 0){
		/*  make sure its really there */
		if(lm78rdreg(Raddr) != Serialaddr){
			lm78.ifc = None;
			error(Enodev);
		} else {
			/*  start the sampling */
			config = lm78rdreg(Rconfig);
			config = (config | Bstart) & ~(Bintclr|Binit);
			lm78wrreg(Rconfig, config);
pprint("Rvidfan %2.2ux\n", lm78rdreg(Rconfig), lm78rdreg(Rvidfan));
		}
		lm78.probed = 1;
	}
}

enum
{
	IntelVendID=	0x8086,
	PiixID=		0x122E,
	Piix3ID=	0x7000,

	Piix4PMID=	0x7113,		/*  PIIX4 power management function */

	PCSC=		0x78,		/*  programmable chip select control register */
	PCSC8bytes=	0x01,
};

/*  figure out what kind of interface we could have */
void
lm78reset(void)
{
	int pcs;
	Pcidev *p;

	lm78.ifc = None;
	p = nil;
	while((p = pcimatch(p, IntelVendID, 0)) != nil){
		switch(p->did){
		/*  these bridges use the PCSC to map the lm78 into port space. */
		/*  for this case the lm78's CS# select is connected to the PIIX's */
		/*  PCS# output and the bottom 3 bits of address are passed to the */
		/*  LM78's A0-A2 inputs. */
		case PiixID:
		case Piix3ID:
			pcs = pcicfgr16(p, PCSC);
			if(pcs & 3) {
				/* already enabled */
				lm78.port = pcs & ~3;
				lm78.ifc = Parallel;
				return;	
			}

			/*  enable the chip, use default address 0x50 */
			pcicfgw16(p, PCSC, 0x50|PCSC8bytes);
			pcs = pcicfgr16(p, PCSC);
			lm78.port = pcs & ~3;
			lm78.ifc = Parallel;
			return;

		/*  this bridge puts the lm78's serial interface on the smbus */
		case Piix4PMID:
			lm78.smbus = piix4smbus();
			if(lm78.smbus == nil)
				continue;
			print("found piix4 smbus, base %lud\n", lm78.smbus->base);
			lm78.ifc = Smbus;
			return;
		}
	}
}

Walkqid *
lm78walk(Chan* c, Chan *nc, char** name, int nname)
{
	return devwalk(c, nc, name, nname, lm78dir, nelem(lm78dir), devgen);
}

static int
lm78stat(Chan* c, uchar* dp, int n)
{
	return devstat(c, dp, n, lm78dir, nelem(lm78dir), devgen);
}

static Chan*
lm78open(Chan* c, int omode)
{
	return devopen(c, omode, lm78dir, nelem(lm78dir), devgen);
}

static void
lm78close(Chan*)
{
}

enum
{
	Linelen= 25,
};

static long
lm78read(Chan *c, void *a, long n, vlong offset)
{
	uchar *va = a;
	int off, e;

	off = offset;

	switch((ulong)c->qid.path){
	case Qdir:
		return devdirread(c, a, n, lm78dir, nelem(lm78dir), devgen);

	case Qlm78vram:
		if(off >=  VRsize)
			return 0;
		e = off + n;
		if(e > VRsize)
			e = VRsize;
		for(; off < e; off++)
			*va++ = lm78rdreg(Rvalue+off);
		return (int)(va - (uchar*)a);
	}
	return 0;
}

static long
lm78write(Chan *c, void *a, long n, vlong offset)
{
	uchar *va = a;
	int off, e;

	off = offset;

	switch((ulong)c->qid.path){
	default:
		error(Eperm);

	case Qlm78vram:
		if(off >=  VRsize)
			return 0;
		e = off + n;
		if(e > VRsize)
			e = VRsize;
		for(; off < e; off++)
			lm78wrreg(Rvalue+off, *va++);
		return va - (uchar*)a;
	}
	return 0;
}

extern Dev lm78devtab;

static Chan*
lm78attach(char* spec)
{
	lm78enable();

	return devattach(lm78devtab.dc, spec);
}

Dev lm78devtab = {
	'T',
	"lm78",

	lm78reset,
	devinit,
	devshutdown,
	lm78attach,
	lm78walk,
	lm78stat,
	lm78open,
	devcreate,
	lm78close,
	lm78read,
	devbread,
	lm78write,
	devbwrite,
	devremove,
	devwstat,
};

