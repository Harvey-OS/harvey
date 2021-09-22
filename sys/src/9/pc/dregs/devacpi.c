/*
 * ACPI 4.0 Support.
 * Still WIP.
 *
 * This driver locates tables and parses only the FADT
 * and the XSDT.  All other tables are mapped and kept there
 * for the user-level interpreter.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#pragma	varargck	type	"G"	Gas*

#define DBGFLG 0			/* was 4 */
#define DBG(...)	if(DBGFLG) print(__VA_ARGS__)

typedef struct Atable Atable;
typedef struct Facs Facs;
typedef struct Fadt Fadt;
typedef struct Gas Gas;
typedef struct Gpe Gpe;
typedef struct Rsdp Rsdp;
typedef struct Sdthdr Sdthdr;
typedef struct Parse Parse;
typedef struct Xsdt Xsdt;
typedef struct Regio Regio;
typedef struct Reg Reg;

enum {
	Qdir = 0,
	Qctl,
	Qtbl,
	Qio,

	Sdthdrsz	= 36,	/* size of SDT header */

	/* ACPI regions. Gas ids */
	Rsysmem	= 0,
	Rsysio,
	Rpcicfg,
	Rembed,
	Rsmbus,
	Rcmos,
	Rpcibar,
	Ripmi,
	Rfixedhw	= 0x7f,

	/* ACPI PM1 control */
	Pm1SciEn		= 0x1,		/* Generate SCI and not SMI */

	/* ACPI tbdf as encoded in acpi region base addresses */
	Rpciregshift	= 0,
	Rpciregmask	= 0xFFFF,
	Rpcifunshift	= 16,
	Rpcifunmask	= 0xFFFF,
	Rpcidevshift	= 32,
	Rpcidevmask	= 0xFFFF,
	Rpcibusshift	= 48,
	Rpcibusmask	= 0xFFFF,

	CMregion = 0,			/* regio name spc base len accsz*/
	CMgpe,				/* gpe name id */
};

struct Gpe
{
	uintptr	stsio;		/* port used for status */
	int	stsbit;		/* bit number */
	uintptr	enio;		/* port used for enable */
	int	enbit;		/* bit number */
	int	nb;		/* event number */
	char*	obj;		/* handler object  */
	int	id;		/* id as supplied by user */
};

struct Parse
{
	char*	sig;
	Atable*	(*f)(uchar*, int);	/* return nil to keep vmap */
};

struct Regio {
	void	*arg;
	u8int	(*get8)(uintptr, void*);
	void	(*set8)(uintptr, u8int, void*);
	u16int	(*get16)(uintptr, void*);
	void	(*set16)(uintptr, u16int, void*);
	u32int	(*get32)(uintptr, void*);
	void	(*set32)(uintptr, u32int, void*);
	u64int	(*get64)(uintptr, void*);
	void	(*set64)(uintptr, u64int, void*);
};

struct Reg
{
	char*	name;
	int	spc;		/* io space */
	u64int	base;		/* address, physical */
	uchar*	p;		/* address, kmapped */
	u64int	len;
	int	tbdf;
	int	accsz;		/* access size */
};

/*
 * Generic address structure.
 */
struct Gas
{
	u8int	spc;	/* address space id */
	u8int	len;	/* register size in bits */
	u8int	off;	/* bit offset */
	u8int	accsz;	/* 1: byte; 2: word; 3: dword; 4: qword */
	u64int	addr;	/* address (or acpi encoded tbdf + reg) */
};

/*
 * Root system description table pointer.
 * Used to locate the root system description table RSDT
 * (or the extended system description table from version 2) XSDT.
 * The XDST contains (after the DST header) a list of pointers to tables:
 *	- FADT	fixed acpi description table.
 *		It points to the DSDT, AML code making the acpi namespace.
 *	- SSDTs	tables with AML code to add to the acpi namespace.
 *	- pointers to other tables for apics, etc.
 */
struct Rsdp
{
	u8int	signature[8];			/* "RSD PTR " */
	u8int	rchecksum;
	u8int	oemid[6];
	u8int	revision;
	u8int	raddr[4];			/* RSDT */
	u8int	length[4];
	u8int	xaddr[8];			/* XSDT */
	u8int	xchecksum;			/* XSDT */
	u8int	_33_[3];			/* reserved */
};

/*
 * Header for ACPI description tables
 */
struct Sdthdr
{
	u8int	sig[4];			/* "FACP" or whatever */
	u8int	length[4];
	u8int	rev;
	u8int	csum;
	u8int	oemid[6];
	u8int	oemtblid[8];
	u8int	oemrev[4];
	u8int	creatorid[4];
	u8int	creatorrev[4];
};

/* Firmware control structure
 */
struct Facs
{
	u32int	hwsig;
	u32int	wakingv;
	u32int	glock;
	u32int	flags;
	u64int	xwakingv;
	u8int	vers;
	u32int	ospmflags;
};

/* Fixed ACPI description table.
 * Describes implementation and hardware registers.
 * PM* blocks are low level functions.
 * GPE* blocks refer to general purpose events.
 * P_* blocks are for processor features.
 * Has address for the DSDT.
 */
struct Fadt
{
	u32int	facs;
	u32int	dsdt;
	/* 1 reserved */
	u8int	pmprofile;
	u16int	sciint;
	u32int	smicmd;
	u8int	acpienable;
	u8int	acpidisable;
	u8int	s4biosreq;
	u8int	pstatecnt;
	u32int	pm1aevtblk;
	u32int	pm1bevtblk;
	u32int	pm1acntblk;
	u32int	pm1bcntblk;
	u32int	pm2cntblk;
	u32int	pmtmrblk;
	u32int	gpe0blk;
	u32int	gpe1blk;
	u8int	pm1evtlen;
	u8int	pm1cntlen;
	u8int	pm2cntlen;
	u8int	pmtmrlen;
	u8int	gpe0blklen;
	u8int	gpe1blklen;
	u8int	gp1base;
	u8int	cstcnt;
	u16int	plvl2lat;
	u16int	plvl3lat;
	u16int	flushsz;
	u16int	flushstride;
	u8int	dutyoff;
	u8int	dutywidth;
	u8int	dayalrm;
	u8int	monalrm;
	u8int	century;
	u16int	iapcbootarch;
	/* 1 reserved */
	u32int	flags;
	Gas	resetreg;
	u8int	resetval;
	/* 3 reserved */
	u64int	xfacs;
	u64int	xdsdt;
	Gas	xpm1aevtblk;
	Gas	xpm1bevtblk;
	Gas	xpm1acntblk;
	Gas	xpm1bcntblk;
	Gas	xpm2cntblk;
	Gas	xpmtmrblk;
	Gas	xgpe0blk;
	Gas	xgpe1blk;
};

/*
 * ACPI table
 */
struct Atable
{
	Atable*	next;		/* next table in list */
	int	is64;		/* uses 64bits */
	char	sig[5];		/* signature */
	char	oemid[7];	/* oem id str. */
	char	oemtblid[9];	/* oem tbl. id str. */
	uchar* tbl;		/* pointer to table in memory */
	long	dlen;		/* size of data in table, after Stdhdr */
};

/* XSDT/RSDT. 4/8 byte addresses starting at p.
 */
struct Xsdt
{
	int	len;
	int	asize;
	u8int*	p;
};

static Atable* acpifadt(uchar*, int);
static Atable* acpitable(uchar*, int);

static Cmdtab ctls[] =
{
	{CMregion,	"region",	6},
	{CMgpe,		"gpe",		3},
};

static Dirtab acpidir[]={
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"acpictl",	{Qctl},			0,	0666,
	"acpitbl",	{Qtbl},			0,	0444,
	"acpiregio",	{Qio},			0,	0666,
};

/* tables listed here are loaded from the XSDT */
static Parse ptables[] =
{
	"FACP", acpifadt,
	"APIC",	acpitable,
	"SSDT", acpitable,
	"SRAT",	acpitable,
	"MSCT",	acpitable,
};

static Facs*	facs;	/* Firmware ACPI control structure */
static Fadt	fadt;	/* Fixed ACPI description. To reach ACPI registers */
static Xsdt*	xsdt;	/* XSDT table */
static Atable*	tfirst;	/* loaded DSDT/SSDT/... tables */
static Atable*	tlast;	/* pointer to last table */
static Reg*	reg;	/* region used for I/O */
static Gpe*	gpes;	/* General purpose events */
static int	ngpes;

static char* regnames[] = {
	"mem", "io", "pcicfg", "embed",
	"smb", "cmos", "pcibar",
};

static char*
acpiregstr(int id)
{
	static char buf[20];	/* BUG */

	if(id >= 0 && id < nelem(regnames))
		return regnames[id];
	seprint(buf, buf+sizeof(buf), "spc:%#x", id);
	return buf;
}

static int
acpiregid(char *s)
{
	int i;

	for(i = 0; i < nelem(regnames); i++)
		if(strcmp(regnames[i], s) == 0)
			return i;
	return -1;
}

static u8int
mget8(uintptr p, void*)
{
	u8int *cp = (u8int*)p;

	return *cp;
}

static void
mset8(uintptr p, u8int v, void*)
{
	u8int *cp = (u8int*)p;

	*cp = v;
}

static u16int
mget16(uintptr p, void*)
{
	u16int *cp = (u16int*)p;

	return *cp;
}

static void
mset16(uintptr p, u16int v, void*)
{
	u16int *cp = (u16int*)p;

	*cp = v;
}

static u32int
mget32(uintptr p, void*)
{
	u32int *cp = (u32int*)p;

	return *cp;
}

static void
mset32(uintptr p, u32int v, void*)
{
	u32int *cp = (u32int*)p;

	*cp = v;
}

static u64int
mget64(uintptr p, void*)
{
	u64int *cp = (u64int*)p;

	return *cp;
}

static void
mset64(uintptr p, u64int v, void*)
{
	u64int *cp = (u64int*)p;

	*cp = v;
}

static u8int
ioget8(uintptr p, void*)
{
	return inb(p);
}

static void
ioset8(uintptr p, u8int v, void*)
{
	outb(p, v);
}

static u16int
ioget16(uintptr p, void*)
{
	return ins(p);
}

static void
ioset16(uintptr p, u16int v, void*)
{
	outs(p, v);
}

static u32int
ioget32(uintptr p, void*)
{
	return inl(p);
}

static void
ioset32(uintptr p, u32int v, void*)
{
	outl(p, v);
}

static u8int
cfgget8(uintptr p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr8(&d, p);
}

static void
cfgset8(uintptr p, u8int v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw8(&d, p, v);
}

static u16int
cfgget16(uintptr p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr16(&d, p);
}

static void
cfgset16(uintptr p, u16int v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw16(&d, p, v);
}

static u32int
cfgget32(uintptr p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr32(&d, p);
}

static void
cfgset32(uintptr p, u32int v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw32(&d, p, v);
}

static Regio memio =
{
	nil,
	mget8, mset8, mget16, mset16,
	mget32, mset32, mget64, mset64
};

static Regio ioio =
{
	nil,
	ioget8, ioset8, ioget16, ioset16,
	ioget32, ioset32, nil, nil
};

static Regio cfgio =
{
	nil,
	cfgget8, cfgset8, cfgget16, cfgset16,
	cfgget32, cfgset32, nil, nil
};

/*
 * Copy memory, 1/2/4/8-bytes at a time, to/from a region.
 */
static long
regcpy(Regio *dio, uintptr da, Regio *sio, uintptr sa, long len, int align)
{
	int n, i;

	DBG("regcpy %#ulx %#ulx %#ulx %#ux\n", da, sa, len, align);
	if((len%align) != 0)
		print("regcpy: bug: copy not aligned. truncated\n");
	n = len/align;
	for(i = 0; i < n; i++){
		switch(align){
		case 1:
			DBG("cpy8 %#p %#p\n", da, sa);
			dio->set8(da, sio->get8(sa, sio->arg), dio->arg);
			break;
		case 2:
			DBG("cpy16 %#p %#p\n", da, sa);
			dio->set16(da, sio->get16(sa, sio->arg), dio->arg);
			break;
		case 4:
			DBG("cpy32 %#p %#p\n", da, sa);
			dio->set32(da, sio->get32(sa, sio->arg), dio->arg);
			break;
		case 8:
			DBG("cpy64 %#p %#p\n", da, sa);
		//	dio->set64(da, sio->get64(sa, sio->arg), dio->arg);
			break;
		default:
			panic("regcpy: align bug");
		}
		da += align;
		sa += align;
	}
	return n*align;
}

/*
 * Perform I/O within region in access units of accsz bytes.
 * All units in bytes.
 */
static long
regio(Reg *r, void *p, ulong len, uintptr off, int iswr)
{
	Regio rio;
	uintptr rp;

	DBG("reg%s %s %#p %#ulx %#lx sz=%d\n",
		iswr ? "out" : "in", r->name, p, off, len, r->accsz);
	rp = 0;
	if(off + len > r->len){
		print("regio: access outside limits");
		len = r->len - off;
	}
	if(len <= 0){
		print("regio: zero len\n");
		return 0;
	}
	switch(r->spc){
	case Rsysmem:
		// XXX should map only what we are going to use
		// A region might be too large.
		if(r->p == nil)
			r->p = vmap(r->base, len);
		if(r->p == nil)
			error("acpi: regio: vmap failed");
		rp = (uintptr)r->p + off;
		rio = memio;
		break;
	case Rsysio:
		rp = r->base + off;
		rio = ioio;
		break;
	case Rpcicfg:
		rp = r->base + off;
		rio = cfgio;
		rio.arg = r;
		break;
	case Rpcibar:
	case Rembed:
	case Rsmbus:
	case Rcmos:
	case Ripmi:
	case Rfixedhw:
		print("regio: reg %s not supported\n", acpiregstr(r->spc));
		error("region not supported");
	}
	if(iswr)
		regcpy(&rio, rp, &memio, (uintptr)p, len, r->accsz);
	else
		regcpy(&memio, (uintptr)p, &rio, rp, len, r->accsz);
	return len;
}

static Atable*
newtable(uchar *p)
{
	Atable *t;
	Sdthdr *h;

	t = malloc(sizeof(Atable));
	if(t == nil)
		panic("no memory for more aml tables");
	t->tbl = p;
	h = (Sdthdr*)t->tbl;
	t->is64 = h->rev >= 2;
	t->dlen = L32GET(h->length) - Sdthdrsz;
	memmove(t->sig, h->sig, sizeof(h->sig));
	t->sig[sizeof(t->sig)-1] = 0;
	memmove(t->oemid, h->oemid, sizeof(h->oemid));
	t->oemtblid[sizeof(t->oemtblid)-1] = 0;
	memmove(t->oemtblid, h->oemtblid, sizeof(h->oemtblid));
	t->oemtblid[sizeof(t->oemtblid)-1] = 0;
	t->next = nil;
	if(tfirst == nil)
		tfirst = tlast = t;
	else{
		tlast->next = t;
		tlast = t;
	}
	return t;
}

static void*
sdtchecksum(void* addr, int len)
{
	u8int *p, sum;

	sum = 0;
	for(p = addr; len-- > 0; p++)
		sum += *p;
	if(sum == 0)
		return addr;

	return nil;
}

static void *
sdtmap(uintptr pa, int *n, int cksum)
{
	Sdthdr* sdt;

	sdt = vmap(pa, sizeof(Sdthdr));
	if(sdt == nil)
		return nil;
	*n = L32GET(sdt->length);
	vunmap(sdt, sizeof(Sdthdr));
	if((sdt = vmap(pa, *n)) == nil)
		return nil;
	if(cksum != 0 && sdtchecksum(sdt, *n) == nil){
		DBG("acpi: SDT: bad checksum\n");
		vunmap(sdt, sizeof(Sdthdr));
		return nil;
	}
	return sdt;
}

static int
loadfacs(uintptr pa)
{
	int n;

	facs = sdtmap(pa, &n, 0);
	if(facs == nil)
		return -1;
	if(memcmp(facs, "FACS", 4) != 0){
		vunmap(facs, n);
		facs = nil;
		return -1;
	}
	/* no unmap */

	DBG("acpi: facs: hwsig: %#ux\n", facs->hwsig);
	DBG("acpi: facs: wakingv: %#ux\n", facs->wakingv);
	DBG("acpi: facs: flags: %#ux\n", facs->flags);
	DBG("acpi: facs: glock: %#ux\n", facs->glock);
	DBG("acpi: facs: xwakingv: %#llux\n", facs->xwakingv);
	DBG("acpi: facs: vers: %#ux\n", facs->vers);
	DBG("acpi: facs: ospmflags: %#ux\n", facs->ospmflags);
	return 0;
}

static void
loaddsdt(uintptr pa)
{
	int n;
	uchar *dsdtp;

	dsdtp = sdtmap(pa, &n, 1);
	if(dsdtp == nil)
		return;
	if(acpitable(dsdtp, n) == nil)
		vunmap(dsdtp, n);
}

static void
gasget(Gas *gas, uchar *p)
{
	gas->spc = p[0];
	gas->len = p[1];
	gas->off = p[2];
	gas->accsz = p[3];
	gas->addr = l64get(p+4);
}

static void
dumpfadt(Fadt *fp)
{
	if(DBGFLG == 0)
		return;

	DBG("acpi: fadt: facs: %#ux\n", fp->facs);
	DBG("acpi: fadt: dsdt: %#ux\n", fp->dsdt);
	DBG("acpi: fadt: pmprofile: %#ux\n", fp->pmprofile);
	DBG("acpi: fadt: sciint: %#ux\n", fp->sciint);
	DBG("acpi: fadt: smicmd: %#ux\n", fp->smicmd);
	DBG("acpi: fadt: acpienable: %#ux\n", fp->acpienable);
	DBG("acpi: fadt: acpidisable: %#ux\n", fp->acpidisable);
	DBG("acpi: fadt: s4biosreq: %#ux\n", fp->s4biosreq);
	DBG("acpi: fadt: pstatecnt: %#ux\n", fp->pstatecnt);
	DBG("acpi: fadt: pm1aevtblk: %#ux\n", fp->pm1aevtblk);
	DBG("acpi: fadt: pm1bevtblk: %#ux\n", fp->pm1bevtblk);
	DBG("acpi: fadt: pm1acntblk: %#ux\n", fp->pm1acntblk);
	DBG("acpi: fadt: pm1bcntblk: %#ux\n", fp->pm1bcntblk);
	DBG("acpi: fadt: pm2cntblk: %#ux\n", fp->pm2cntblk);
	DBG("acpi: fadt: pmtmrblk: %#ux\n", fp->pmtmrblk);
	DBG("acpi: fadt: gpe0blk: %#ux\n", fp->gpe0blk);
	DBG("acpi: fadt: gpe1blk: %#ux\n", fp->gpe1blk);
	DBG("acpi: fadt: pm1evtlen: %#ux\n", fp->pm1evtlen);
	DBG("acpi: fadt: pm1cntlen: %#ux\n", fp->pm1cntlen);
	DBG("acpi: fadt: pm2cntlen: %#ux\n", fp->pm2cntlen);
	DBG("acpi: fadt: pmtmrlen: %#ux\n", fp->pmtmrlen);
	DBG("acpi: fadt: gpe0blklen: %#ux\n", fp->gpe0blklen);
	DBG("acpi: fadt: gpe1blklen: %#ux\n", fp->gpe1blklen);
	DBG("acpi: fadt: gp1base: %#ux\n", fp->gp1base);
	DBG("acpi: fadt: cstcnt: %#ux\n", fp->cstcnt);
	DBG("acpi: fadt: plvl2lat: %#ux\n", fp->plvl2lat);
	DBG("acpi: fadt: plvl3lat: %#ux\n", fp->plvl3lat);
	DBG("acpi: fadt: flushsz: %#ux\n", fp->flushsz);
	DBG("acpi: fadt: flushstride: %#ux\n", fp->flushstride);
	DBG("acpi: fadt: dutyoff: %#ux\n", fp->dutyoff);
	DBG("acpi: fadt: dutywidth: %#ux\n", fp->dutywidth);
	DBG("acpi: fadt: dayalrm: %#ux\n", fp->dayalrm);
	DBG("acpi: fadt: monalrm: %#ux\n", fp->monalrm);
	DBG("acpi: fadt: century: %#ux\n", fp->century);
	DBG("acpi: fadt: iapcbootarch: %#ux\n", fp->iapcbootarch);
	DBG("acpi: fadt: flags: %#ux\n", fp->flags);
	DBG("acpi: fadt: resetreg: %G\n", &fp->resetreg);
	DBG("acpi: fadt: resetval: %#ux\n", fp->resetval);
	DBG("acpi: fadt: xfacs: %#llux\n", fp->xfacs);
	DBG("acpi: fadt: xdsdt: %#llux\n", fp->xdsdt);
	DBG("acpi: fadt: xpm1aevtblk: %G\n", &fp->xpm1aevtblk);
	DBG("acpi: fadt: xpm1bevtblk: %G\n", &fp->xpm1bevtblk);
	DBG("acpi: fadt: xpm1acntblk: %G\n", &fp->xpm1acntblk);
	DBG("acpi: fadt: xpm1bcntblk: %G\n", &fp->xpm1bcntblk);
	DBG("acpi: fadt: xpm2cntblk: %G\n", &fp->xpm2cntblk);
	DBG("acpi: fadt: xpmtmrblk: %G\n", &fp->xpmtmrblk);
	DBG("acpi: fadt: xgpe0blk: %G\n", &fp->xgpe0blk);
	DBG("acpi: fadt: xgpe1blk: %G\n", &fp->xgpe1blk);
}

static Atable*
acpifadt(uchar *p, int)
{
	Fadt *fp;

	fp = &fadt;
	fp->facs = L32GET(p + 36);
	fp->dsdt = L32GET(p + 40);
	fp->pmprofile = p[45];
	fp->sciint = L16GET(p+46);
	fp->smicmd = L32GET(p+48);
	fp->acpienable = p[52];
	fp->acpidisable = p[53];
	fp->s4biosreq = p[54];
	fp->pstatecnt = p[55];
	fp->pm1aevtblk = L32GET(p+56);
	fp->pm1bevtblk = L32GET(p+60);
	fp->pm1acntblk = L32GET(p+64);
	fp->pm1bcntblk = L32GET(p+68);
	fp->pm2cntblk = L32GET(p+72);
	fp->pmtmrblk = L32GET(p+76);
	fp->gpe0blk = L32GET(p+80);
	fp->gpe1blk = L32GET(p+84);
	fp->pm1evtlen = p[88];
	fp->pm1cntlen = p[89];
	fp->pm2cntlen = p[90];
	fp->pmtmrlen = p[91];
	fp->gpe0blklen = p[92];
	fp->gpe1blklen = p[93];
	fp->gp1base = p[94];
	fp->cstcnt = p[95];
	fp->plvl2lat = L16GET(p+96);
	fp->plvl3lat = L16GET(p+98);
	fp->flushsz = L16GET(p+100);
	fp->flushstride = L16GET(p+102);
	fp->dutyoff = p[104];
	fp->dutywidth = p[105];
	fp->dayalrm = p[106];
	fp->monalrm = p[107];
	fp->century = p[108];
	fp->iapcbootarch = L16GET(p+109);
	fp->flags = L32GET(p+112);
	gasget(&fp->resetreg, p+116);
	fp->resetval = p[128];
	fp->xfacs = l64get(p+132);
	fp->xdsdt = l64get(p+140);
	gasget(&fp->xpm1aevtblk, p+148);
	gasget(&fp->xpm1bevtblk, p+160);
	gasget(&fp->xpm1acntblk, p+172);
	gasget(&fp->xpm1bcntblk, p+184);
	gasget(&fp->xpm2cntblk, p+196);
	gasget(&fp->xpmtmrblk, p+208);
	gasget(&fp->xgpe0blk, p+220);
	gasget(&fp->xgpe1blk, p+232);

	dumpfadt(fp);
	if(fp->xfacs != 0)
		loadfacs(fp->xfacs);
	else
		loadfacs(fp->facs);
	if(fp->xdsdt != 0)
		loaddsdt(fp->xdsdt);
	else
		loaddsdt(fp->dsdt);

	return nil;
}

/*
 * Map the table and keep it there.
 */
static Atable*
acpitable(uchar *p, int len)
{
	if(len < Sdthdrsz)
		return nil;
	return newtable(p);
}

static void
dumptable(char *sig, uchar *p, int l)
{
	int n, i;

	if(DBGFLG > 1){
		DBG("%s @ %#p\n", sig, p);
		if(DBGFLG > 2)
			n = l;
		else
			n = 256;
		for(i = 0; i < n; i++){
			if((i % 16) == 0)
				DBG("%x: ", i);
			DBG(" %2.2ux", p[i]);
			if((i % 16) == 15)
				DBG("\n");
		}
		DBG("\n");
		DBG("\n");
	}
}

static char*
seprinttable(char *s, char *e, Atable *t)
{
	uchar *p;
	int i, n;

	p = (uchar*)t->tbl;	/* include header */
	n = Sdthdrsz + t->dlen;
	s = seprint(s, e, "%s @ %#p\n", t->sig, p);
	for(i = 0; i < n; i++){
		if((i % 16) == 0)
			s = seprint(s, e, "%x: ", i);
		s = seprint(s, e, " %2.2ux", p[i]);
		if((i % 16) == 15)
			s = seprint(s, e, "\n");
	}
	return seprint(s, e, "\n\n");
}

/*
 * process xsdt table and load tables with sig, or all if nil.
 * (XXX: should be able to search for sig, oemid, oemtblid)
 */
static int
acpixsdtload(char *sig)
{
	int i, l, t, unmap, found;
	uintptr dhpa;
	uchar *sdt;
	char tsig[5];

	found = 0;
	for(i = 0; i < xsdt->len; i += xsdt->asize){
		if(xsdt->asize == 8)
			dhpa = l64get(xsdt->p+i);
		else
			dhpa = L32GET(xsdt->p+i);
		if((sdt = sdtmap(dhpa, &l, 1)) == nil)
			continue;
		unmap = 1;
		memmove(tsig, sdt, 4);
		tsig[4] = 0;
		if(sig == nil || strcmp(sig, tsig) == 0){
			DBG("acpi: %s addr %#p\n", tsig, sdt);
			for(t = 0; t < nelem(ptables); t++)
				if(strcmp(tsig, ptables[t].sig) == 0){
					dumptable(tsig, sdt, l);
					unmap = ptables[t].f(sdt, l) == nil;
					found = 1;
					break;
				}
		}
		if(unmap)
			vunmap(sdt, l);
	}
	return found;
}

static void
acpirsdptr(void)
{
	Rsdp *rsd;
	int asize;
	uintptr sdtpa;

	if((rsd = sigsearch("RSD PTR ")) == nil)
		return;

	assert(sizeof(Sdthdr) == 36);

	DBG("acpi: RSD PTR @ %#p, physaddr %#ux length %ud %#llux rev %d\n",
		rsd, L32GET(rsd->raddr), L32GET(rsd->length),
		l64get(rsd->xaddr), rsd->revision);

	if(rsd->revision >= 2){
		if(sdtchecksum(rsd, 36) == nil){
			DBG("acpi: RSD: bad checksum\n");
			return;
		}
		sdtpa = l64get(rsd->xaddr);
		asize = 8;
	}
	else{
		if(sdtchecksum(rsd, 20) == nil){
			DBG("acpi: RSD: bad checksum\n");
			return;
		}
		sdtpa = L32GET(rsd->raddr);
		asize = 4;
	}

	/*
	 * process the RSDT or XSDT table.
	 */
	xsdt = malloc(sizeof(Xsdt));
	if(xsdt == nil)
		return;
	if((xsdt->p = sdtmap(sdtpa, &xsdt->len, 1)) == nil)
		return;
	if((xsdt->p[0] != 'R' && xsdt->p[0] != 'X') || memcmp(xsdt->p+1, "SDT", 3) != 0){
		free(xsdt);
		xsdt = nil;
		vunmap(xsdt, xsdt->len);
		return;
	}
	xsdt->p += sizeof(Sdthdr);
	xsdt->len -= sizeof(Sdthdr);
	xsdt->asize = asize;
	DBG("acpi: XSDT %#p\n", xsdt);
	acpixsdtload(nil);
	/* xsdt is kept and not unmapped */

}

static int
acpigen(Chan *c, char*, Dirtab *tab, int ntab, int i, Dir *dp)
{
	Qid qid;

	if(i == DEVDOTDOT){
		mkqid(&qid, Qdir, 0, QTDIR);
		devdir(c, qid, ".", 0, eve, 0555, dp);
		return 1;
	}
	i++; /* skip first element for . itself */
	if(tab==0 || i>=ntab)
		return -1;
	tab += i;
	qid = tab->qid;
	qid.path &= ~Qdir;
	qid.vers = 0;
	devdir(c, qid, tab->name, tab->length, eve, tab->perm, dp);
	return 1;
}

static int
Gfmt(Fmt* f)
{
	static char* rnames[] = {
			"mem", "io", "pcicfg", "embed",
			"smb", "cmos", "pcibar", "ipmi"};
	Gas *g;

	g = va_arg(f->args, Gas*);
	switch(g->spc){
	case Rsysmem:
	case Rsysio:
	case Rembed:
	case Rsmbus:
	case Rcmos:
	case Rpcibar:
	case Ripmi:
		fmtprint(f, "[%s ", rnames[g->spc]);
		break;
	case Rpcicfg:
		fmtprint(f, "[pci ");
		fmtprint(f, "dev %#ulx ", (ulong)(g->addr >> 32) & 0xFFFF);
		fmtprint(f, "fn %#ulx ", (ulong)(g->addr & 0xFFFF0000) >> 16);
		fmtprint(f, "adr %#ulx ", (ulong)(g->addr &0xFFFF));
		break;
	case Rfixedhw:
		fmtprint(f, "[hw ");
		break;
	default:
		fmtprint(f, "[spc=%#ux ", g->spc);
	}
	return fmtprint(f, "off %d len %d addr %#ullx sz%d]",
		g->off, g->len, g->addr, g->accsz);
}

static uint
getbanked(uintptr ra, uintptr rb, int sz)
{
	uint r;

	r = 0;
	switch(sz){
	case 1:
		if(ra != 0)
			r |= inb(ra);
		if(rb != 0)
			r |= inb(rb);
		break;
	case 2:
		if(ra != 0)
			r |= ins(ra);
		if(rb != 0)
			r |= ins(rb);
		break;
	case 4:
		if(ra != 0)
			r |= inl(ra);
		if(rb != 0)
			r |= inl(rb);
		break;
	default:
		print("getbanked: wrong size\n");
	}
	return r;
}

static uint
setbanked(uintptr ra, uintptr rb, int sz, int v)
{
	uint r;

	r = -1;
	switch(sz){
	case 1:
		if(ra != 0)
			outb(ra, v);
		if(rb != 0)
			outb(rb, v);
		break;
	case 2:
		if(ra != 0)
			outs(ra, v);
		if(rb != 0)
			outs(rb, v);
		break;
	case 4:
		if(ra != 0)
			outl(ra, v);
		if(rb != 0)
			outl(rb, v);
		break;
	default:
		print("setbanked: wrong size\n");
	}
	return r;
}

static uint
getpm1ctl(void)
{
	return getbanked(fadt.pm1acntblk, fadt.pm1bcntblk, fadt.pm1cntlen);
}

static void
setpm1sts(uint v)
{
	setbanked(fadt.pm1aevtblk, fadt.pm1bevtblk, fadt.pm1evtlen/2, v);
}

static uint
getpm1sts(void)
{
	return getbanked(fadt.pm1aevtblk, fadt.pm1bevtblk, fadt.pm1evtlen/2);
}

static uint
getpm1en(void)
{
	int sz;

	sz = fadt.pm1evtlen/2;
	return getbanked(fadt.pm1aevtblk+sz, fadt.pm1bevtblk+sz, sz);
}

static int
getgpeen(int n)
{
	return inb(gpes[n].enio) & 1<<gpes[n].enbit;
}

static void
setgpeen(int n, uint v)
{
	int old;

	DBG("acpi: gpe %d %d\n", n, v);
	old = inb(gpes[n].enio);
	if(v)
		outb(gpes[n].enio, old | 1<<gpes[n].enbit);
	else
		outb(gpes[n].enio, old & ~(1<<gpes[n].enbit));
}

static void
clrgpests(int n)
{
	outb(gpes[n].stsio, 1<<gpes[n].stsbit);
}

static uint
getgpests(int n)
{
	return inb(gpes[n].stsio) & 1<<gpes[n].stsbit;
}

static void
acpiintr(Ureg*, void*)
{
	int i;
	uint sts, en;

	print("acpi: intr\n");

	for(i = 0; i < ngpes; i++)
		if(getgpests(i)){
			print("gpe %d on\n", i);
 			en = getgpeen(i);
			setgpeen(i, 0);
			clrgpests(i);
			if(en != 0)
				print("acpiitr: calling gpe %d\n", i);
		//	queue gpe for calling gpe->ho in the
		//	aml process.
		//	enable it again when it returns.
		}
	sts = getpm1sts();
	en = getpm1en();
	print("acpiitr: pm1sts %#ux pm1en %#ux\n", sts, en);
	if(sts&en)
		print("have enabled events\n");
	if(sts&1)
		print("power button\n");
	// XXX serve other interrupts here.
	setpm1sts(sts);
}

static void
initgpes(void)
{
	int i, n0, n1;

	n0 = fadt.gpe0blklen/2;
	n1 = fadt.gpe1blklen/2;
	ngpes = n0 + n1;
	gpes = smalloc(sizeof(Gpe) * ngpes);
	for(i = 0; i < n0; i++){
		gpes[i].nb = i;
		gpes[i].stsbit = i&7;
		gpes[i].stsio = fadt.gpe0blk + (i>>3);
		gpes[i].enbit = (n0 + i)&7;
		gpes[i].enio = fadt.gpe0blk + ((n0 + i)>>3);
	}
	for(i = 0; i + n0 < ngpes; i++){
		gpes[i + n0].nb = fadt.gp1base + i;
		gpes[i + n0].stsbit = i&7;
		gpes[i + n0].stsio = fadt.gpe1blk + (i>>3);
		gpes[i + n0].enbit = (n1 + i)&7;
		gpes[i + n0].enio = fadt.gpe1blk + ((n1 + i)>>3);
	}
	for(i = 0; i < ngpes; i++){
		setgpeen(i, 0);
		clrgpests(i);
	}
}

static void
acpiioalloc(uint addr, int len)
{
	if(addr != 0)
		ioalloc(addr, len, 0, "acpi");
}

static Chan*
acpiattach(char *spec)
{
	int i;

	fmtinstall('G', Gfmt);
	acpirsdptr();
	if(fadt.smicmd == 0)
		error("no acpi tables");
	/*
	 * should use fadt->xpm* and fadt->xgpe* registers for 64 bits.
	 * We are not ready in this kernel for that.
	 */
	DBG("acpi io alloc\n");
	acpiioalloc(fadt.smicmd, 1);
	acpiioalloc(fadt.pm1aevtblk, fadt.pm1evtlen);
	acpiioalloc(fadt.pm1bevtblk, fadt.pm1evtlen );
	acpiioalloc(fadt.pm1acntblk, fadt.pm1cntlen);
	acpiioalloc(fadt.pm1bcntblk, fadt.pm1cntlen);
	acpiioalloc(fadt.pm2cntblk, fadt.pm2cntlen);
	acpiioalloc(fadt.pmtmrblk, fadt.pmtmrlen);
	acpiioalloc(fadt.gpe0blk, fadt.gpe0blklen);
	acpiioalloc(fadt.gpe1blk, fadt.gpe1blklen);

	DBG("acpi init gpes\n");
	initgpes();

	/*
	 * This starts ACPI, which may require we handle
	 * power mgmt events ourselves. Use with care.
	 */
	DBG("acpi starting\n");
	outb(fadt.smicmd, fadt.acpienable);
	for(i = 0; i < 10; i++)
		if(getpm1ctl() & Pm1SciEn)
			break;
	if(i == 10)
		error("acpi failed to enable");
	if(fadt.sciint != 0)
		intrenable(fadt.sciint, acpiintr, 0, BUSUNKNOWN, "acpi");
	return devattach(L'α', spec);
}

static Walkqid*
acpiwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, acpidir, nelem(acpidir), acpigen);
}

static int
acpistat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, acpidir, nelem(acpidir), acpigen);
}

static Chan*
acpiopen(Chan *c, int omode)
{
	return devopen(c, omode, acpidir, nelem(acpidir), acpigen);
}

static void
acpiclose(Chan *)
{
}

static char*ttext;
static int tlen;

static long
acpiread(Chan *c, void *a, long n, vlong off)
{
	long q;
	Atable *t;
	char *ns, *s, *e, *ntext;

	q = c->qid.path;
	switch(q){
	case Qdir:
		return devdirread(c, a, n, acpidir, nelem(acpidir), acpigen);
	case Qtbl:
		if(ttext == nil){
			tlen = 1024;
			ttext = malloc(tlen);
			if(ttext == nil){
				print("acpi: no memory\n");
				return 0;
			}
			s = ttext;
			e = ttext + tlen;
			strcpy(s, "no tables\n");
			for(t = tfirst; t != nil; t = t->next){
				ns = seprinttable(s, e, t);
				while(ns == e - 1){
					DBG("acpiread: allocated %d\n", tlen*2);
					ntext = realloc(ttext, tlen*2);
					if(ntext == nil)
						panic("acpi: no memory\n");
					s = ntext + (ttext - s);
					ttext = ntext;
					tlen *= 2;
					e = ttext + tlen;
					ns = seprinttable(s, e, t);
				}
				s = ns;
			}

		}
		return readstr(off, a, n, ttext);
	case Qio:
		if(reg == nil)
			error("region not configured");
		return regio(reg, a, n, off, 0);
	}
	error(Eperm);
	return -1;
}

static long
acpiwrite(Chan *c, void *a, long n, vlong off)
{
	Cmdtab *ct;
	Cmdbuf *cb;
	Reg *r;
	uint rno, fun, dev, bus, i;

	if(c->qid.path == Qio){
		if(reg == nil)
			error("region not configured");
		return regio(reg, a, n, off, 1);
	}
	if(c->qid.path != Qctl)
		error(Eperm);

	cb = parsecmd(a, n);
	if(waserror()){
		free(cb);
		nexterror();
	}
	/* lookupcmd can throw an error (e.g., if it doesn't know the cmd) */
	ct = lookupcmd(cb, ctls, nelem(ctls));
	DBG("acpi ctl %s\n", cb->f[0]);
	switch(ct->index){
	case CMregion:
		r = reg;
		if(r == nil){
			r = smalloc(sizeof(Reg));
			r->name = nil;
		}
		kstrdup(&r->name, cb->f[1]);
		r->spc = acpiregid(cb->f[2]);
		if(r->spc < 0){
			free(r);
			reg = nil;
			error("bad region type");
		}
		if(r->spc == Rpcicfg || r->spc == Rpcibar){
			rno = r->base>>Rpciregshift & Rpciregmask;
			fun = r->base>>Rpcifunshift & Rpcifunmask;
			dev = r->base>>Rpcidevshift & Rpcidevmask;
			bus = r->base>>Rpcibusshift & Rpcibusmask;
			r->tbdf = MKBUS(BusPCI, bus, dev, fun);
			r->base = rno;	/* register ~ our base addr */
		}
		r->base = strtoull(cb->f[3], nil, 0);
		r->len = strtoull(cb->f[4], nil, 0);
		r->accsz = strtoul(cb->f[5], nil, 0);
		if(r->accsz < 1 || r->accsz > 4){
			free(r);
			reg = nil;
			error("bad region access size");
		}
		reg = r;
		DBG("region %s %s %llux %llux sz%d",
			r->name, acpiregstr(r->spc), r->base, r->len, r->accsz);
		break;
	case CMgpe:
		i = strtoul(cb->f[1], nil, 0);
		if(i >= ngpes)
			error("gpe out of range");
		kstrdup(&gpes[i].obj, cb->f[2]);
		DBG("gpe %d %s\n", i, gpes[i].obj);
		setgpeen(i, 1);
		break;
	default:
		panic("acpi: unknown control message");
	}
	poperror();
	free(cb);
	return n;
}

Dev acpidevtab = {
	L'α',
	"acpi",

	devreset,
	devinit,
	devshutdown,
	acpiattach,
	acpiwalk,
	acpistat,
	acpiopen,
	devcreate,
	acpiclose,
	acpiread,
	devbread,
	acpiwrite,
	devbwrite,
	devremove,
	devwstat,
};
