#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include "mp.h"
#include "acpi.h"

/*
 * ACPI 4.0 Support.
 * Still WIP.
 *
 * This driver locates tables and parses only the FADT
 * and the XSDT. All other tables are mapped and kept there
 * for a user-level interpreter.
 */

#define l16get(p)	(((p)[1]<<8)|(p)[0])
#define l32get(p)	(((u32int)l16get(p+2)<<16)|l16get(p))
static Atable* acpifadt(uchar*, int);
static Atable* acpitable(uchar*, int);
static Atable* acpimadt(uchar*, int);
static Atable* acpimsct(uchar*, int);
static Atable* acpisrat(uchar*, int);
static Atable* acpislit(uchar*, int);
static Atable* acpihpet(uchar*, int);

#pragma	varargck	type	"G"	Gas*

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

/*
 * The DSDT is always given to the user interpreter.
 * Tables listed here are also loaded from the XSDT:
 * MSCT, MADT, and FADT are processed by us, because they are
 * required to do early initialization before we have user processes.
 * Other tables are given to the user level interpreter for
 * execution.
 */
static Parse ptables[] =
{
	"FACP", acpifadt,
	"APIC",	acpimadt,
	"SRAT",	acpisrat,
	"SLIT",	acpislit,
	"MSCT",	acpimsct,
	"HPET", acpihpet,
	"SSDT", acpitable,
};

static Facs*	facs;	/* Firmware ACPI control structure */
static Fadt	fadt;	/* Fixed ACPI description. To reach ACPI registers */
static Xsdt*	xsdt;	/* XSDT table */
static Atable*	tfirst;	/* loaded DSDT/SSDT/... tables */
static Atable*	tlast;	/* pointer to last table */
	Madt*	apics;	/* APIC info */
static Srat*	srat;	/* System resource affinity, used by physalloc */
static Slit*	slit;	/* System locality information table used by the scheduler */
static Msct*	msct;	/* Maximum system characteristics table */
static Hpet*	hpet;
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

static u64int
l64get(u8int* p)
{
	/*
	 * Doing this as a define
	 * #define l64get(p)	(((u64int)l32get(p+4)<<32)|l32get(p))
	 * causes 8c to abort with "out of fixed registers" in
	 * rsdlink() below.
	 */
	return (((u64int)l32get(p+4)<<32)|l32get(p));
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

	DBG("regcpy %#ullx %#ullx %#ulx %#ux\n", da, sa, len, align);
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

	DBG("reg%s %s %#p %#ullx %#lx sz=%d\n",
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
			error("regio: vmap failed");
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
	t->dlen = l32get(h->length) - Sdthdrsz;
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
sdtmap(uintmem pa, int *n, int cksum)
{
	Sdthdr* sdt;

	sdt = vmap(pa, sizeof(Sdthdr));
	if(sdt == nil){
		DBG("acpi: vmap1: nil\n");
		return nil;
	}
	*n = l32get(sdt->length);
	vunmap(sdt, sizeof(Sdthdr));
	if((sdt = vmap(pa, *n)) == nil){
		DBG("acpi: nil vmap\n");
		return nil;
	}
	DBG("acpi: sdtmap %#P -> %#p, length %d\n", pa, sdt, *n);
	if(cksum != 0 && sdtchecksum(sdt, *n) == nil){
		DBG("acpi: SDT: bad checksum\n");
		vunmap(sdt, sizeof(Sdthdr));
		return nil;
	}
	return sdt;
}

static int
loadfacs(uintmem pa)
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
loaddsdt(uintmem pa)
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
dumpfadt(Fadt *fp, int revision)
{
	USED(fp);
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

	if(revision < 3)
		return;

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
	int revision;

	fp = &fadt;
	revision = p[8];
	DBG("acpifadt %#p length %ud revision %ud\n", p, l32get(p+4), revision);
	fp->facs = l32get(p + 36);
	fp->dsdt = l32get(p + 40);
	fp->pmprofile = p[45];
	fp->sciint = l16get(p+46);
	fp->smicmd = l32get(p+48);
	fp->acpienable = p[52];
	fp->acpidisable = p[53];
	fp->s4biosreq = p[54];
	fp->pstatecnt = p[55];
	fp->pm1aevtblk = l32get(p+56);
	fp->pm1bevtblk = l32get(p+60);
	fp->pm1acntblk = l32get(p+64);
	fp->pm1bcntblk = l32get(p+68);
	fp->pm2cntblk = l32get(p+72);
	fp->pmtmrblk = l32get(p+76);
	fp->gpe0blk = l32get(p+80);
	fp->gpe1blk = l32get(p+84);
	fp->pm1evtlen = p[88];
	fp->pm1cntlen = p[89];
	fp->pm2cntlen = p[90];
	fp->pmtmrlen = p[91];
	fp->gpe0blklen = p[92];
	fp->gpe1blklen = p[93];
	fp->gp1base = p[94];
	fp->cstcnt = p[95];
	fp->plvl2lat = l16get(p+96);
	fp->plvl3lat = l16get(p+98);
	fp->flushsz = l16get(p+100);
	fp->flushstride = l16get(p+102);
	fp->dutyoff = p[104];
	fp->dutywidth = p[105];
	fp->dayalrm = p[106];
	fp->monalrm = p[107];
	fp->century = p[108];
	fp->iapcbootarch = l16get(p+109);
	fp->flags = l32get(p+112);

	/*
	 * Revision 1 of this table stops here, and is described
	 * in The ACPI Specification 1.0, 22-Dec-1996.
	 * The ACPI Specification 2.0, 27-Jul-2000, bumped the revision
	 * number up to 3 and added the extra fields on the end.
	 * Thank you, QEMU.
	 */
	if(revision >= 3){
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
	}

	dumpfadt(fp, revision);

	/* If xfacs or xdsdt are either nil
	 * or different from their 32-bit
	 * counter-parts, then go with
	 * the 32-bit addresses (as the
	 * ACPICA does), since those are
	 * tested by BIOS manufacturers.
	 */

	if(fp->xfacs != 0 && fp->xfacs == fp->facs)
		loadfacs(fp->xfacs);
	else
		loadfacs(fp->facs);

	if(fp->xdsdt != 0 && fp->xdsdt == fp->dsdt)
		loadfacs(fp->xdsdt);
	else
		loadfacs(fp->dsdt);

	return nil;			/* can be unmapped once parsed */
}

static void
dumpmsct(Msct *msct)
{
	Mdom *st;

	DBG("acpi: msct: %d doms %d clkdoms %#ullx maxpa\n",
		msct->ndoms, msct->nclkdoms, msct->maxpa);
	for(st = msct->dom; st != nil; st = st->next)
		DBG("\t[%d:%d] %d maxproc %#ullx maxmmem\n",
			st->start, st->end, st->maxproc, st->maxmem);
	DBG("\n");
}

/*
 * XXX: should perhaps update our idea of available memory.
 * Else we should remove this code.
 */
static Atable*
acpimsct(uchar *p, int len)
{
	uchar *pe;
	Mdom **stl, *st;
	int off;

	msct = mallocz(sizeof(Msct), 1);
	msct->ndoms = l32get(p+40) + 1;
	msct->nclkdoms = l32get(p+44) + 1;
	msct->maxpa = l64get(p+48);
	msct->dom = nil;
	stl = &msct->dom;
	pe = p + len;
	off = l32get(p+36);
	for(p += off; p < pe; p += 22){
		st = mallocz(sizeof(Mdom), 1);
		st->next = nil;
		st->start = l32get(p+2);
		st->end = l32get(p+6);
		st->maxproc = l32get(p+10);
		st->maxmem = l64get(p+14);
		*stl = st;
		stl = &st->next;
	}

	dumpmsct(msct);
	return nil;	/* can be unmapped once parsed */
}

static Atable*
acpihpet(uchar *p, int)
{
	hpet = mallocz(sizeof(Hpet), 1);
	p += 36;

	hpet->id = l32get(p);
	p += 4;

	p += 4; /* it's a Gas */
	hpet->addr = l64get(p);
	p += 8;

	hpet->seqno = *p;
	p++;

	hpet->minticks = l16get(p);
	p += 2;
	hpet->attr = *p;

	DBG("hpet: id %#ux addr %#p seqno %d ticks %d attr %#ux\n",
		hpet->id, hpet->addr, hpet->seqno, hpet->minticks, hpet->attr);
//	hpetinit(hpet->seqno, hpet->addr, hpet->minticks)
	return nil;	/* can be unmapped once parsed */
}


static void
dumpsrat(Srat *st)
{
	DBG("acpi: srat:\n");
	for(; st != nil; st = st->next)
		switch(st->type){
		case SRlapic:
			DBG("\tlapic: dom %d apic %d sapic %d clk %d\n",
				st->lapic.dom, st->lapic.apic,
				st->lapic.sapic, st->lapic.clkdom);
			break;
		case SRmem:
			DBG("\tmem: dom %d %#ullx %#ullx %c%c\n",
				st->mem.dom, st->mem.addr, st->mem.len,
				st->mem.hplug?'h':'-',
				st->mem.nvram?'n':'-');
			break;
		case SRlx2apic:
			DBG("\tlx2apic: dom %d apic %d clk %d\n",
				st->lx2apic.dom, st->lx2apic.apic,
				st->lx2apic.clkdom);
			break;
		default:
			DBG("\t<unknown srat entry>\n");
		}
	DBG("\n");
}

static Atable*
acpisrat(uchar *p, int len)
{
	Srat **stl, *st;
	uchar *pe;
	int stlen, flags;

	if(srat != nil){
		print("acpi: two SRATs?\n");
		return nil;
	}

	stl = &srat;
	pe = p + len;
	for(p += 48; p < pe; p += stlen){
		st = mallocz(sizeof(Srat), 1);
		st->type = p[0];
		st->next = nil;
		stlen = p[1];
		switch(st->type){
		case SRlapic:
			st->lapic.dom = p[2] | p[9]<<24| p[10]<<16 | p[11]<<8;
			st->lapic.apic = p[3];
			st->lapic.sapic = p[8];
			st->lapic.clkdom = l32get(p+12);
			if(l32get(p+4) == 0){
				free(st);
				st = nil;
			}
			break;
		case SRmem:
			st->mem.dom = l32get(p+2);
			st->mem.addr = l64get(p+8);
			st->mem.len = l64get(p+16);
			flags = l32get(p+28);
			if((flags&1) == 0){	/* not enabled */
				free(st);
				st = nil;
			}else{
				st->mem.hplug = flags & 2;
				st->mem.nvram = flags & 4;
			}
			break;
		case SRlx2apic:
			st->lx2apic.dom = l32get(p+4);
			st->lx2apic.apic = l32get(p+8);
			st->lx2apic.clkdom = l32get(p+16);
			if(l32get(p+12) == 0){
				free(st);
				st = nil;
			}
			break;
		default:
			print("unknown SRAT structure\n");
			free(st);
			st = nil;
		}
		if(st != nil){
			*stl = st;
			stl = &st->next;
		}
	}

	dumpsrat(srat);
	return nil;	/* can be unmapped once parsed */
}

static void
dumpslit(Slit *sl)
{
	int i;

	DBG("acpi slit:\n");
	for(i = 0; i < sl->rowlen*sl->rowlen; i++){
		DBG(" %ux", sl->e[i/sl->rowlen][i%sl->rowlen].dist);
	}
	DBG("\n");
}

static int
cmpslitent(void* v1, void* v2)
{
	SlEntry *se1, *se2;

	se1 = v1;
	se2 = v2;
	return se1->dist - se2->dist;
}

static Atable*
acpislit(uchar *p, int len)
{
	uchar *pe;
	int i, j, k;
	SlEntry *se;

	pe = p + len;
	slit = malloc(sizeof(*slit));
	slit->rowlen = l64get(p+36);
	slit->e = malloc(slit->rowlen*sizeof(SlEntry*));
	for(i = 0; i < slit->rowlen; i++)
		slit->e[i] = malloc(sizeof(SlEntry)*slit->rowlen);

	i = 0;
	for(p += 44; p < pe; p++, i++){
		j = i/slit->rowlen;
		k = i%slit->rowlen;
		se = &slit->e[j][k];
		se->dom = k;
		se->dist = *p;
	}
	dumpslit(slit);
	for(i = 0; i < slit->rowlen; i++)
		qsort(slit->e[i], slit->rowlen, sizeof(slit->e[0][0]), cmpslitent);

	dumpslit(slit);
	return nil;	/* can be unmapped once parsed */
}

uintmem
acpimblocksize(uintmem addr, int *dom)
{
	Srat *sl;

	for(sl = srat; sl != nil; sl = sl->next)
		if(sl->type == SRmem)
		if(sl->mem.addr <= addr && sl->mem.addr + sl->mem.len > addr){
			*dom = sl->mem.dom;
			return sl->mem.len - (addr - sl->mem.addr);
		}
	return 0;
}

/*
 * Return a number identifying a color for the memory at
 * the given address (color identifies locality) and fill *sizep
 * with the size for memory of the same color starting at addr.
 */
int
memcolor(uintmem addr, uintmem *sizep)
{
	Srat *sl;

	for(sl = srat; sl != nil; sl = sl->next)
		if(sl->type == SRmem)
		if(sl->mem.addr <= addr && addr-sl->mem.addr < sl->mem.len){
			if(sizep != nil)
				*sizep = sl->mem.len - (addr - sl->mem.addr);
			if(sl->mem.dom >= NCOLOR)
				print("memcolor: NCOLOR too small");
			return sl->mem.dom%NCOLOR;
		}
	return -1;
}

/*
 * Being machno an index in sys->machptr, return the color
 * for that mach (color identifies locality).
 */
int
corecolor(int machno)
{
	Srat *sl;
	Mach *m;
	static int colors[32];

	if(machno < 0 || machno >= MACHMAX)
		return -1;
	m = sys->machptr[machno];
	if(m == nil)
		return -1;

	if(machno >= 0 && machno < nelem(colors) && colors[machno] != 0)
		return colors[machno] - 1;

	for(sl = srat; sl != nil; sl = sl->next)
		if(sl->type == SRlapic && sl->lapic.apic == m->apicno){
			if(machno >= 0 && machno < nelem(colors))
				colors[machno] = 1 + sl->lapic.dom;
			if(sl->lapic.dom >= NCOLOR)
				print("memcolor: NCOLOR too small");
			return sl->lapic.dom%NCOLOR;
		}
	return -1;
}


int
pickcore(int mycolor, int index)
{
	int color;
	int ncorepercol;

	if(slit == nil)
		return index;
	ncorepercol = MACHMAX/slit->rowlen;
	color = slit->e[mycolor][index/ncorepercol].dom;
	return color * ncorepercol + index % ncorepercol;
}


static void
dumpmadt(Madt *apics)
{
	Apicst *st;

	DBG("acpi: madt lapic paddr %llux pcat %d:\n", apics->lapicpa, apics->pcat);
	for(st = apics->st; st != nil; st = st->next)
		switch(st->type){
		case ASlapic:
			DBG("\tlapic pid %d id %d\n", st->lapic.pid, st->lapic.id);
			break;
		case ASioapic:
		case ASiosapic:
			DBG("\tioapic id %d addr %#llux ibase %d\n",
				st->ioapic.id, st->ioapic.addr, st->ioapic.ibase);
			break;
		case ASintovr:
			DBG("\tintovr irq %d intr %d flags %#ux\n",
				st->intovr.irq, st->intovr.intr,st->intovr.flags);
			break;
		case ASnmi:
			DBG("\tnmi intr %d flags %#ux\n",
				st->nmi.intr, st->nmi.flags);
			break;
		case ASlnmi:
			DBG("\tlnmi pid %d lint %d flags %#ux\n",
				st->lnmi.pid, st->lnmi.lint, st->lnmi.flags);
			break;
		case ASlsapic:
			DBG("\tlsapic pid %d id %d eid %d puid %d puids %s\n",
				st->lsapic.pid, st->lsapic.id,
				st->lsapic.eid, st->lsapic.puid,
				st->lsapic.puids);
			break;
		case ASintsrc:
			DBG("\tintr type %d pid %d peid %d iosv %d intr %d %#x\n",
				st->type, st->intsrc.pid,
				st->intsrc.peid, st->intsrc.iosv,
				st->intsrc.intr, st->intsrc.flags);
			break;
		case ASlx2apic:
			DBG("\tlx2apic puid %d id %d\n", st->lx2apic.puid, st->lx2apic.id);
			break;
		case ASlx2nmi:
			DBG("\tlx2nmi puid %d intr %d flags %#ux\n",
				st->lx2nmi.puid, st->lx2nmi.intr, st->lx2nmi.flags);
			break;
		default:
			DBG("\t<unknown madt entry>\n");
		}
	DBG("\n");
}

static Atable*
acpimadt(uchar *p, int len)
{
	uchar *pe;
	Apicst *st, *l, **stl;
	int stlen, id;

	apics = mallocz(sizeof(Madt), 1);
	apics->lapicpa = l32get(p+36);
	apics->pcat = l32get(p+40);
	apics->st = nil;
	stl = &apics->st;
	pe = p + len;
	for(p += 44; p < pe; p += stlen){
		st = mallocz(sizeof(Apicst), 1);
		st->type = p[0];
		st->next = nil;
		stlen = p[1];
		switch(st->type){
		case ASlapic:
			st->lapic.pid = p[2];
			st->lapic.id = p[3];
			if(l32get(p+4) == 0){
				free(st);
				st = nil;
			}
			break;
		case ASioapic:
			st->ioapic.id = id = p[2];
			st->ioapic.addr = l32get(p+4);
			st->ioapic.ibase = l32get(p+8);
			/* iosapic overrides any ioapic entry for the same id */
			for(l = apics->st; l != nil; l = l->next)
				if(l->type == ASiosapic && l->iosapic.id == id){
					st->ioapic = l->iosapic;
					/* we leave it linked; could be removed */
					break;
				}
			break;
		case ASintovr:
			st->intovr.irq = p[3];
			st->intovr.intr = l32get(p+4);
			st->intovr.flags = l16get(p+8);
			break;
		case ASnmi:
			st->nmi.flags = l16get(p+2);
			st->nmi.intr = l32get(p+4);
			break;
		case ASlnmi:
			st->lnmi.pid = p[2];
			st->lnmi.flags = l16get(p+3);
			st->lnmi.lint = p[5];
			break;
		case ASladdr:
			if(sizeof(apics->lapicpa) >= 8)
				apics->lapicpa = l64get(p+8);
			break;
		case ASiosapic:
			id = st->iosapic.id = p[2];
			st->iosapic.ibase = l32get(p+4);
			st->iosapic.addr = l64get(p+8);
			/* iosapic overrides any ioapic entry for the same id */
			for(l = apics->st; l != nil; l = l->next)
				if(l->type == ASioapic && l->ioapic.id == id){
					l->ioapic = st->iosapic;
					free(st);
					st = nil;
					break;
				}
			break;
		case ASlsapic:
			st->lsapic.pid = p[2];
			st->lsapic.id = p[3];
			st->lsapic.eid = p[4];
			st->lsapic.puid = l32get(p+12);
			if(l32get(p+8) == 0){
				free(st);
				st = nil;
			}else
				kstrdup(&st->lsapic.puids, (char*)p+16);
			break;
		case ASintsrc:
			st->intsrc.flags = l16get(p+2);
			st->type = p[4];
			st->intsrc.pid = p[5];
			st->intsrc.peid = p[6];
			st->intsrc.iosv = p[7];
			st->intsrc.intr = l32get(p+8);
			st->intsrc.any = l32get(p+12);
			break;
		case ASlx2apic:
			st->lx2apic.id = l32get(p+4);
			st->lx2apic.puid = l32get(p+12);
			if(l32get(p+8) == 0){
				free(st);
				st = nil;
			}
			break;
		case ASlx2nmi:
			st->lx2nmi.flags = l16get(p+2);
			st->lx2nmi.puid = l32get(p+4);
			st->lx2nmi.intr = p[8];
			break;
		default:
			print("unknown APIC structure\n");
			free(st);
			st = nil;
		}
		if(st != nil){
			*stl = st;
			stl = &st->next;
		}
	}

	dumpmadt(apics);
	return nil;	/* can be unmapped once parsed */
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

	USED(sig);
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
	uintmem dhpa;
	uchar *sdt;
	char tsig[5];

	found = 0;
	for(i = 0; i < xsdt->len; i += xsdt->asize){
		if(xsdt->asize == 8)
			dhpa = l64get(xsdt->p+i);
		else
			dhpa = l32get(xsdt->p+i);
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

static void*
rsdscan(u8int* addr, int len, char* signature)
{
	int sl;
	u8int *e, *p;

	e = addr+len;
	sl = strlen(signature);
	for(p = addr; p+sl < e; p += 16){
		if(memcmp(p, signature, sl))
			continue;
		return p;
	}

	return nil;
}

static void*
rsdsearch(char* signature)
{
	uintmem p;
	u8int *bda;
	void *rsd;

	/*
	 * Search for the data structure signature:
	 * 1) in the first KB of the EBDA;
	 * 2) in the BIOS ROM between 0xE0000 and 0xFFFFF.
	 */
	if(strncmp((char*)KADDR(0xFFFD9), "EISA", 4) == 0){
		bda = BIOSSEG(0x40);
		if((p = (bda[0x0F]<<8)|bda[0x0E])){
			if(rsd = rsdscan(KADDR(p), 1024, signature))
				return rsd;
		}
	}
	return rsdscan(BIOSSEG(0xE000), 0x20000, signature);
}

static void
acpirsdptr(void)
{
	Rsdp *rsd;
	int asize;
	uintmem sdtpa;

	if((rsd = rsdsearch("RSD PTR ")) == nil)
		return;

	assert(sizeof(Sdthdr) == 36);

	DBG("acpi: RSD PTR@ %#p, physaddr %#ux length %ud %#llux rev %d\n",
		rsd, l32get(rsd->raddr), l32get(rsd->length),
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
		sdtpa = l32get(rsd->raddr);
		asize = 4;
	}

	/*
	 * process the RSDT or XSDT table.
	 */
	xsdt = malloc(sizeof(Xsdt));
	if(xsdt == nil){
		DBG("acpi: malloc failed\n");
		return;
	}
	if((xsdt->p = sdtmap(sdtpa, &xsdt->len, 1)) == nil){
		DBG("acpi: sdtmap failed\n");
		return;
	}
	if((xsdt->p[0] != 'R' && xsdt->p[0] != 'X') || memcmp(xsdt->p+1, "SDT", 3) != 0){
		DBG("acpi: xsdt sig: %c%c%c%c\n",
			xsdt->p[0], xsdt->p[1], xsdt->p[2], xsdt->p[3]);
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
		fmtprint(f, "[%s ", acpiregstr(g->spc));
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
getbanked(int ra, int rb, int sz)
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
setbanked(int ra, int rb, int sz, int v)
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
	DBG("acpi: setpm1sts %#ux\n", v);
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

	DBG("acpi: setgpe %d %d\n", n, v);
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
	gpes = mallocz(sizeof(Gpe) * ngpes, 1);
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

int
acpiinit(void)
{
	if(fadt.smicmd == 0){
		fmtinstall('G', Gfmt);
		acpirsdptr();
		if(fadt.smicmd == 0)
			return -1;
	}
	return 0;
}

static Chan*
acpiattach(char *spec)
{
	int i;

	/*
	 * This was written for the stock kernel.
	 * This code must use 64bit registers to be acpi ready in nix.
	 * Besides, we mostly want the locality and affinity tables.
	 */
	if(1 || acpiinit() < 0)
		error("acpi not enabled");

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
		error("acpi: failed to enable\n");
	if(fadt.sciint != 0)
		intrenable(fadt.sciint, acpiintr, 0, BUSUNKNOWN, "acpi");
	return devattach(L'α', spec);
}

static Walkqid*
acpiwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, acpidir, nelem(acpidir), acpigen);
}

static long
acpistat(Chan *c, uchar *dp, long n)
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
		panic("acpi: unknown ctl");
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
