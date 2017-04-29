/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"

#include "apic.h"
#include "acpi.h"

/* -----------------------------------------------------------------------------
 * Basic ACPI device.
 *
 * The qid.Path will be made unique by incrementing lastpath. lastpath starts
 * at Qroot.
 *
 * Qtbl will return a pointer to the Atable, which includes the signature, OEM
 * data, and so on.
 *
 * Raw, at any level, dumps the raw table at that level, which by the ACPI
 * flattened tree layout will include all descendents.
 *
 * Qpretty, at any level, will print the pretty form for that level and all
 * descendants.
 */
enum {
	Qroot = 0,

	// The type is the qid.path mod NQtypes.
	Qdir = 0,
	Qpretty,
	Qraw,
	Qtbl,
	NQtypes,

	QIndexShift = 8,
	QIndexMask = (1 << QIndexShift) - 1,
};

/* what do we need to round up to? */
#define ATABLEBUFSZ	ROUNDUP(sizeof(Atable), 128)

static uint64_t lastpath;
static PSlice emptyslice;
static Atable **atableindex;
static Rsdp *rsd;
static Queue *acpiev;
Dev acpidevtab;

static uint16_t pm1status;
/* Table of ACPI ports we own. We just burn 64k of bss here rather
 * than look them up in a function each time they're used. */
static uint8_t acpiport[1<<16];
static int32_t acpiioread(Chan *c, void *a, int32_t n, int64_t off);
static int32_t acpiiowrite(Chan *c, void *a, int32_t n, int64_t off);
static int32_t acpimemread(Chan *c, void *a, int32_t n, int64_t off);
static int32_t acpiintrread(Chan *c, void *a, int32_t n, int64_t off);

static void acpiintr(Ureg *, void *);

static char *devname(void)
{
	return acpidevtab.name;
}

static int devdc(void)
{
	return acpidevtab.dc;
}

void outofyourelement(void);

#if 0
/*
 * ACPI 4.0 Support.
 * Still WIP.
 *
 * This driver locates tables and parses only a small subset
 * of tables. All other tables are mapped and kept for the user-level
 * interpreter.
 */
enum {
	CMirq,
};

static Cmdtab ctls[] = {
//	{CMregion, "region", 6},
//	{CMgpe, "gpe", 3},
	{CMirq, "irq", 2},
};
#endif
static Facs *facs;	/* Firmware ACPI control structure */
static Fadt *fadt;	/* Fixed ACPI description to reach ACPI regs */
static Atable *root;
static Xsdt *xsdt;		/* XSDT table */
static Atable *tfirst;	/* loaded DSDT/SSDT/... tables */
//static Atable *tlast;	/* pointer to last table */
Atable *apics; 		/* APIC info */
Atable *srat;		/* System resource affinity used by physalloc */
Atable *dmar;
static Slit *slit;	/* Sys locality info table used by scheduler */
static Atable *mscttbl;	/* Maximum system characteristics table */
//static Reg *reg;	/* region used for I/O */
static Gpe *gpes;	/* General purpose events */
static int ngpes;

static char *regnames[] = {
	"mem", "io", "pcicfg", "embed",
	"smb", "cmos", "pcibar", "ipmi",
};

/*
 * Lists to store RAM that we copy ACPI tables into. When we map a new
 * ACPI list into the kernel, we copy it into a specifically RAM buffer
 * (to make sure it's not coming from e.g. slow device memory). We store
 * pointers to those buffers on these lists. We maintain information
 * about base and size to support Qraw and, hence, the ACPICA library.
 */
struct Acpilist {
	struct Acpilist *next;
	uintptr_t base;
	size_t size;
	int8_t raw[];
};
typedef struct Acpilist Acpilist;
static Acpilist *acpilists;

/*
 * Given a base address, bind the list that contains it.
 * It's possible and allowed for ACPICA to ask for a bigger region,
 * so size matters.
 */
static Acpilist *findlist(uintptr_t base, uint size)
{
	Acpilist *a = acpilists;
	//print("findlist: find %p\n", (void *)base);
	for(; a; a = a->next){
		if ((base >= a->base) && ((base + size) < (a->base + a->size))){
			return a;
		}
	}
	//print("Can't find list for %p\n", (void *)base);
	return nil;
}
/*
 * Produces an Atable at some level in the tree. Note that Atables are
 * isomorphic to directories in the file system namespace; this code
 * ensures that invariant.
 */
Atable *mkatable(Atable *parent,
                        int type, char *name, uint8_t *raw,
                        size_t rawsize, size_t addsize)
{
	void *m;
	Atable *t;

	m = mallocz(ATABLEBUFSZ + addsize, 1);
	if (m == nil)
		panic("no memory for more aml tables");
	t = m;
	t->parent = parent;
	t->tbl = nil;
	if (addsize != 0)
		t->tbl = m + ATABLEBUFSZ;
	t->rawsize = rawsize;
	t->raw = raw;
	strlcpy(t->name, name, sizeof(t->name));
	mkqid(&t->qid,  (lastpath << QIndexShift) + Qdir, 0, QTDIR);
	mkqid(&t->rqid, (lastpath << QIndexShift) + Qraw, 0, 0);
	mkqid(&t->pqid, (lastpath << QIndexShift) + Qpretty, 0, 0);
	mkqid(&t->tqid, (lastpath << QIndexShift) + Qtbl, 0, 0);
	lastpath++;

	return t;
}

Atable *finatable(Atable *t, PSlice *slice)
{
	size_t n;
	Atable *tail;
	Dirtab *dirs;

	n = pslicelen(slice);
	t->nchildren = n;
	t->children = (Atable **)pslicefinalize(slice);
	dirs = reallocarray(nil, n + NQtypes, sizeof(Dirtab));
	assert(dirs != nil);
	dirs[0] = (Dirtab){ ".",      t->qid,   0, 0555 };
	dirs[1] = (Dirtab){ "pretty", t->pqid,  0, 0444 };
	dirs[2] = (Dirtab){ "raw",    t->rqid,  t->rawsize, 0444 };
	dirs[3] = (Dirtab){ "table",  t->tqid,  0, 0444 };
	dirs[4] = (Dirtab){ "ctl",  t->tqid,  0, 0666 };
	for (size_t i = 0; i < n; i++) {
		strlcpy(dirs[i + NQtypes].name, t->children[i]->name, KNAMELEN);
		dirs[i + NQtypes].qid = t->children[i]->qid;
		dirs[i + NQtypes].length = 0;
		dirs[i + NQtypes].perm = DMDIR | 0555;
	}
	t->cdirs = dirs;
	tail = nil;
	while (n-- > 0) {
		t->children[n]->next = tail;
		tail = t->children[n];
	}

	return t;
}

Atable *finatable_nochildren(Atable *t)
{
	return finatable(t, &emptyslice);
}

static char *dumpGas(char *start, char *end, char *prefix, Gas *g);
static void dumpxsdt(void);

static char *acpiregstr(int id)
{
	static char buf[20];		/* BUG */

	if (id >= 0 && id < nelem(regnames))
		return regnames[id];
	seprint(buf, buf + sizeof(buf), "spc:%#x", id);
	return buf;
}

static int acpiregid(char *s)
{
	for (int i = 0; i < nelem(regnames); i++)
		if (strcmp(regnames[i], s) == 0)
			return i;
	return -1;
}

/*
 * TODO(rminnich): Fix these if we're ever on a different-endian machine.
 * They are specific to little-endian processors and are not portable.
 */
static uint8_t mget8(uintptr_t p, void *unused)
{
	uint8_t *cp = (uint8_t *) p;
	return *cp;
}

static void mset8(uintptr_t p, uint8_t v, void *unused)
{
	uint8_t *cp = (uint8_t *) p;
	*cp = v;
}

static uint16_t mget16(uintptr_t p, void *unused)
{
	uint16_t *cp = (uint16_t *) p;
	return *cp;
}

static void mset16(uintptr_t p, uint16_t v, void *unused)
{
	uint16_t *cp = (uint16_t *) p;
	*cp = v;
}

static uint32_t mget32(uintptr_t p, void *unused)
{
	uint32_t *cp = (uint32_t *) p;
	return *cp;
}

static void mset32(uintptr_t p, uint32_t v, void *unused)
{
	uint32_t *cp = (uint32_t *) p;
	*cp = v;
}

static uint64_t mget64(uintptr_t p, void *unused)
{
	uint64_t *cp = (uint64_t *) p;
	return *cp;
}

static void mset64(uintptr_t p, uint64_t v, void *unused)
{
	uint64_t *cp = (uint64_t *) p;
	*cp = v;
}

static uint8_t ioget8(uintptr_t p, void *unused)
{
	return inb(p);
}

static void ioset8(uintptr_t p, uint8_t v, void *unused)
{
	outb(p, v);
}

static uint16_t ioget16(uintptr_t p, void *unused)
{
	return ins(p);
}

static void ioset16(uintptr_t p, uint16_t v, void *unused)
{
	outs(p, v);
}

static uint32_t ioget32(uintptr_t p, void *unused)
{
	return inl(p);
}

static void ioset32(uintptr_t p, uint32_t v, void *unused)
{
	outl(p, v);
}

static uint8_t
cfgget8(uintptr_t p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr8(&d, p);
}

static void
cfgset8(uintptr_t p, uint8_t v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw8(&d, p, v);
}

static uint16_t
cfgget16(uintptr_t p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr16(&d, p);
}

static void
cfgset16(uintptr_t p, uint16_t v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw16(&d, p, v);
}

static uint32_t
cfgget32(uintptr_t p, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	return pcicfgr32(&d, p);
}

static void
cfgset32(uintptr_t p, uint32_t v, void* r)
{
	Reg *ro = r;
	Pcidev d;

	d.tbdf = ro->tbdf;
	pcicfgw32(&d, p, v);
}

static struct Regio memio = {
	nil,
	mget8, mset8, mget16, mset16,
	mget32, mset32, mget64, mset64
};

static struct Regio ioio = {
	nil,
	ioget8, ioset8, ioget16, ioset16,
	ioget32, ioset32, nil, nil
};

static struct Regio cfgio = {
	nil,
	cfgget8, cfgset8, cfgget16, cfgset16,
	cfgget32, cfgset32, nil, nil
};

/*
 * Copy memory, 1/2/4/8-bytes at a time, to/from a region.
 */
static long
regcpy(Regio *dio, uintptr_t da, Regio *sio,
	   uintptr_t sa, long len, int align)
{
	int n, i;

	print("regcpy %#p %#p %#p %#p\n", da, sa, len, align);
	if ((len % align) != 0)
		print("regcpy: bug: copy not aligned. truncated\n");
	n = len / align;
	for (i = 0; i < n; i++) {
		switch (align) {
			case 1:
				print("cpy8 %#p %#p\n", da, sa);
				dio->set8(da, sio->get8(sa, sio->arg), dio->arg);
				break;
			case 2:
				print("cpy16 %#p %#p\n", da, sa);
				dio->set16(da, sio->get16(sa, sio->arg), dio->arg);
				break;
			case 4:
				print("cpy32 %#p %#p\n", da, sa);
				dio->set32(da, sio->get32(sa, sio->arg), dio->arg);
				break;
			case 8:
				print("cpy64 %#p %#p\n", da, sa);
				print("Not doing set64 for some reason, fix me!");
				//  dio->set64(da, sio->get64(sa, sio->arg), dio->arg);
				break;
			default:
				panic("regcpy: align bug");
		}
		da += align;
		sa += align;
	}
	return n * align;
}

/*
 * Perform I/O within region in access units of accsz bytes.
 * All units in bytes.
 */
static long regio(Reg *r, void *p, uint32_t len, uintptr_t off, int iswr)
{
	Regio rio;
	uintptr_t rp;

	print("reg%s %s %#p %#p %#lx sz=%d\n",
		   iswr ? "out" : "in", r->name, p, off, len, r->accsz);
	rp = 0;
	if (off + len > r->len) {
		print("regio: access outside limits");
		len = r->len - off;
	}
	if (len <= 0) {
		print("regio: zero len\n");
		return 0;
	}
	switch (r->spc) {
		case Rsysmem:
			if (r->p == nil)
				r->p = vmap(r->base, len);
			if (r->p == nil)
				error("regio: vmap/KADDR failed");
			rp = (uintptr_t) r->p + off;
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
	if (iswr)
		regcpy(&rio, rp, &memio, (uintptr_t) p, len, r->accsz);
	else
		regcpy(&memio, (uintptr_t) p, &rio, rp, len, r->accsz);
	return len;
}

/*
 * Compute and return SDT checksum: '0' is a correct sum.
 */
static uint8_t sdtchecksum(void *addr, int len)
{
	uint8_t *p, sum;

	sum = 0;
	//print("check %p %d\n", addr, len);
	for (p = addr; len-- > 0; p++)
		sum += *p;
	//print("sum is 0x%x\n", sum);
	return sum;
}

static void *sdtmap(uintptr_t pa, size_t want, size_t *n, int cksum)
{
	Sdthdr *sdt;
	Acpilist *p;
	//print("sdtmap %p\n", (void *)pa);
	if (!pa) {
		print("sdtmap: nil pa\n");
		return nil;
	}
	if (want) {
		sdt = vmap(pa, want);
		if (sdt == nil) {
			print("acpi: vmap full table @%p/0x%x: nil\n", (void *)pa, want);
			return nil;
		}
		/* realistically, we get a full page, and acpica seems to know that somehow. */
		uintptr_t endaddress = (uintptr_t) sdt;
		endaddress += want + 0xfff;
		endaddress &= ~0xfff;
		want = endaddress - (uintptr_t)sdt;
		*n = want;
	} else {

	sdt = vmap(pa, sizeof(Sdthdr));
	if (sdt == nil) {
		print("acpi: vmap header@%p/%d: nil\n", (void *)pa, sizeof(Sdthdr));
		return nil;
	}
	//hexdump(sdt, sizeof(Sdthdr));
	//print("sdt %p\n", sdt);
	//print("get it\n");
	*n = l32get(sdt->length);
	//print("*n is %d\n", *n);
	if (*n == 0) {
		print("sdt has zero length: pa = %p, sig = %.4s\n", pa, sdt->sig);
		return nil;
	}

	sdt = vmap(pa, *n);
	if (sdt == nil) {
		print("acpi: vmap full table @%p/0x%x: nil\n", (void *)pa, *n);
		return nil;
	}
	//print("check it\n");
	if (cksum != 0 && sdtchecksum(sdt, *n) != 0) {
		print("acpi: SDT: bad checksum. pa = %p, len = %lu\n", pa, *n);
		return nil;
	}
	}
	//print("now mallocz\n");
	p = mallocz(sizeof(Acpilist) + *n, 1);
	//print("malloc'ed %p\n", p);
	if (p == nil)
		panic("sdtmap: memory allocation failed for %lu bytes", *n);
	//print("move (%p, %p, %d)\n", p->raw, (void *)sdt, *n);
	memmove(p->raw, (void *)sdt, *n);
	p->base = pa;
	p->size = *n;
	p->next = acpilists;
	acpilists = p;
	//print("sdtmap: size %d\n", *n);
	return p->raw;
}

static int loadfacs(uintptr_t pa)
{
	size_t n;

	facs = sdtmap(pa, 0, &n, 0);
	if (facs == nil)
		return -1;
	if (memcmp(facs->sig, "FACS", 4) != 0) {
		facs = nil;
		return -1;
	}

	/* no unmap */
	if (0) {
		print("acpi: facs: hwsig: %#p\n", facs->hwsig);
		print("acpi: facs: wakingv: %#p\n", facs->wakingv);
		print("acpi: facs: flags: %#p\n", facs->flags);
		print("acpi: facs: glock: %#p\n", facs->glock);
		print("acpi: facs: xwakingv: %#p\n", facs->xwakingv);
		print("acpi: facs: vers: %#p\n", facs->vers);
		print("acpi: facs: ospmflags: %#p\n", facs->ospmflags);
	}

	return 0;
}

static void loaddsdt(uintptr_t pa)
{
	size_t n;
	uint8_t *dsdtp;

	dsdtp = sdtmap(pa, 0, &n, 1);
	//print("Loaded it\n");
	if (dsdtp == nil) {
		print("acpi: Failed to map dsdtp.\n");
		return;
	}
}

static void gasget(Gas *gas, uint8_t *p)
{
	gas->spc = p[0];
	gas->len = p[1];
	gas->off = p[2];
	gas->accsz = p[3];
	gas->addr = l64get(p + 4);
}

static char *dumpfadt(char *start, char *end, Fadt *fp)
{
	if (fp == nil)
		return start;

	start = seprint(start, end, "acpi: FADT@%p\n", fp);
	start = seprint(start, end, "acpi: fadt: facs: $%p\n", fp->facs);
	start = seprint(start, end, "acpi: fadt: dsdt: $%p\n", fp->dsdt);
	start = seprint(start, end, "acpi: fadt: pmprofile: $%p\n", fp->pmprofile);
	start = seprint(start, end, "acpi: fadt: sciint: $%p\n", fp->sciint);
	start = seprint(start, end, "acpi: fadt: smicmd: $%p\n", fp->smicmd);
	start =
		seprint(start, end, "acpi: fadt: acpienable: $%p\n", fp->acpienable);
	start =
		seprint(start, end, "acpi: fadt: acpidisable: $%p\n", fp->acpidisable);
	start = seprint(start, end, "acpi: fadt: s4biosreq: $%p\n", fp->s4biosreq);
	start = seprint(start, end, "acpi: fadt: pstatecnt: $%p\n", fp->pstatecnt);
	start =
		seprint(start, end, "acpi: fadt: pm1aevtblk: $%p\n", fp->pm1aevtblk);
	start =
		seprint(start, end, "acpi: fadt: pm1bevtblk: $%p\n", fp->pm1bevtblk);
	start =
		seprint(start, end, "acpi: fadt: pm1acntblk: $%p\n", fp->pm1acntblk);
	start =
		seprint(start, end, "acpi: fadt: pm1bcntblk: $%p\n", fp->pm1bcntblk);
	start = seprint(start, end, "acpi: fadt: pm2cntblk: $%p\n", fp->pm2cntblk);
	start = seprint(start, end, "acpi: fadt: pmtmrblk: $%p\n", fp->pmtmrblk);
	start = seprint(start, end, "acpi: fadt: gpe0blk: $%p\n", fp->gpe0blk);
	start = seprint(start, end, "acpi: fadt: gpe1blk: $%p\n", fp->gpe1blk);
	start = seprint(start, end, "acpi: fadt: pm1evtlen: $%p\n", fp->pm1evtlen);
	start = seprint(start, end, "acpi: fadt: pm1cntlen: $%p\n", fp->pm1cntlen);
	start = seprint(start, end, "acpi: fadt: pm2cntlen: $%p\n", fp->pm2cntlen);
	start = seprint(start, end, "acpi: fadt: pmtmrlen: $%p\n", fp->pmtmrlen);
	start =
		seprint(start, end, "acpi: fadt: gpe0blklen: $%p\n", fp->gpe0blklen);
	start =
		seprint(start, end, "acpi: fadt: gpe1blklen: $%p\n", fp->gpe1blklen);
	start = seprint(start, end, "acpi: fadt: gp1base: $%p\n", fp->gp1base);
	start = seprint(start, end, "acpi: fadt: cstcnt: $%p\n", fp->cstcnt);
	start = seprint(start, end, "acpi: fadt: plvl2lat: $%p\n", fp->plvl2lat);
	start = seprint(start, end, "acpi: fadt: plvl3lat: $%p\n", fp->plvl3lat);
	start = seprint(start, end, "acpi: fadt: flushsz: $%p\n", fp->flushsz);
	start =
		seprint(start, end, "acpi: fadt: flushstride: $%p\n", fp->flushstride);
	start = seprint(start, end, "acpi: fadt: dutyoff: $%p\n", fp->dutyoff);
	start = seprint(start, end, "acpi: fadt: dutywidth: $%p\n", fp->dutywidth);
	start = seprint(start, end, "acpi: fadt: dayalrm: $%p\n", fp->dayalrm);
	start = seprint(start, end, "acpi: fadt: monalrm: $%p\n", fp->monalrm);
	start = seprint(start, end, "acpi: fadt: century: $%p\n", fp->century);
	start =
		seprint(start, end, "acpi: fadt: iapcbootarch: $%p\n",
				 fp->iapcbootarch);
	start = seprint(start, end, "acpi: fadt: flags: $%p\n", fp->flags);
	start = dumpGas(start, end, "acpi: fadt: resetreg: ", &fp->resetreg);
	start = seprint(start, end, "acpi: fadt: resetval: $%p\n", fp->resetval);
	start = seprint(start, end, "acpi: fadt: xfacs: %p\n", fp->xfacs);
	start = seprint(start, end, "acpi: fadt: xdsdt: %p\n", fp->xdsdt);
	start = dumpGas(start, end, "acpi: fadt: xpm1aevtblk:", &fp->xpm1aevtblk);
	start = dumpGas(start, end, "acpi: fadt: xpm1bevtblk:", &fp->xpm1bevtblk);
	start = dumpGas(start, end, "acpi: fadt: xpm1acntblk:", &fp->xpm1acntblk);
	start = dumpGas(start, end, "acpi: fadt: xpm1bcntblk:", &fp->xpm1bcntblk);
	start = dumpGas(start, end, "acpi: fadt: xpm2cntblk:", &fp->xpm2cntblk);
	start = dumpGas(start, end, "acpi: fadt: xpmtmrblk:", &fp->xpmtmrblk);
	start = dumpGas(start, end, "acpi: fadt: xgpe0blk:", &fp->xgpe0blk);
	start = dumpGas(start, end, "acpi: fadt: xgpe1blk:", &fp->xgpe1blk);
	return start;
}

static Atable *parsefadt(Atable *parent,
								char *name, uint8_t *p, size_t rawsize)
{
	Atable *t;
	Fadt *fp;

	t = mkatable(parent, FADT, name, p, rawsize, sizeof(Fadt));

	if (rawsize < 116) {
		print("ACPI: unusually short FADT, aborting!\n");
		return t;
	}
	/* for now, keep the globals. We'll get rid of them later. */
	fp = t->tbl;
	fadt = fp;
	fp->facs = l32get(p + 36);
	fp->dsdt = l32get(p + 40);
	fp->pmprofile = p[45];
	fp->sciint = l16get(p + 46);
	fp->smicmd = l32get(p + 48);
	fp->acpienable = p[52];
	fp->acpidisable = p[53];
	fp->s4biosreq = p[54];
	fp->pstatecnt = p[55];
	fp->pm1aevtblk = l32get(p + 56);
	fp->pm1bevtblk = l32get(p + 60);
	fp->pm1acntblk = l32get(p + 64);
	fp->pm1bcntblk = l32get(p + 68);
	fp->pm2cntblk = l32get(p + 72);
	fp->pmtmrblk = l32get(p + 76);
	fp->gpe0blk = l32get(p + 80);
	fp->gpe1blk = l32get(p + 84);
	fp->pm1evtlen = p[88];
	fp->pm1cntlen = p[89];
	fp->pm2cntlen = p[90];
	fp->pmtmrlen = p[91];
	fp->gpe0blklen = p[92];
	fp->gpe1blklen = p[93];
	fp->gp1base = p[94];
	fp->cstcnt = p[95];
	fp->plvl2lat = l16get(p + 96);
	fp->plvl3lat = l16get(p + 98);
	fp->flushsz = l16get(p + 100);
	fp->flushstride = l16get(p + 102);
	fp->dutyoff = p[104];
	fp->dutywidth = p[105];
	fp->dayalrm = p[106];
	fp->monalrm = p[107];
	fp->century = p[108];
	fp->iapcbootarch = l16get(p + 109);
	fp->flags = l32get(p + 112);

	/*
	 * qemu gives us a 116 byte fadt, though i haven't seen any HW do that.
	 * The right way to do this is to realloc the table and fake it out.
	 */
	if (rawsize < 244)
		return finatable_nochildren(t);

	gasget(&fp->resetreg, p + 116);
	fp->resetval = p[128];
	fp->xfacs = l64get(p + 132);
	fp->xdsdt = l64get(p + 140);
	gasget(&fp->xpm1aevtblk, p + 148);
	gasget(&fp->xpm1bevtblk, p + 160);
	gasget(&fp->xpm1acntblk, p + 172);
	gasget(&fp->xpm1bcntblk, p + 184);
	gasget(&fp->xpm2cntblk, p + 196);
	gasget(&fp->xpmtmrblk, p + 208);
	gasget(&fp->xgpe0blk, p + 220);
	gasget(&fp->xgpe1blk, p + 232);

	if (fp->xfacs != 0)
		loadfacs(fp->xfacs);
	else
		loadfacs(fp->facs);

	//print("x %p %p %p \n", fp, (void *)fp->xdsdt, (void *)(uint64_t)fp->dsdt);

	if (fp->xdsdt == (uint64_t)fp->dsdt)	/* acpica */
		loaddsdt(fp->xdsdt);
	else
		loaddsdt(fp->dsdt);
	//print("y\n");

	return finatable_nochildren(t);
}

static char *dumpmsct(char *start, char *end, Atable *table)
{
	Msct *msct;

	if (!table)
		return start;

	msct = table->tbl;
	if (!msct)
		return start;

	start = seprint(start, end, "acpi: msct: %d doms %d clkdoms %#p maxpa\n",
					 msct->ndoms, msct->nclkdoms, msct->maxpa);
	for (int i = 0; i < table->nchildren; i++) {
		Atable *domtbl = table->children[i]->tbl;
		Mdom *st = domtbl->tbl;

		start = seprint(start, end, "\t[%d:%d] %d maxproc %#p maxmmem\n",
						 st->start, st->end, st->maxproc, st->maxmem);
	}
	start = seprint(start, end, "\n");

	return start;
}

/*
 * XXX: should perhaps update our idea of available memory.
 * Else we should remove this code.
 */
static Atable *parsemsct(Atable *parent,
                                char *name, uint8_t *raw, size_t rawsize)
{
	Atable *t;
	uint8_t *r, *re;
	Msct *msct;
	size_t off, nmdom;
	int i;

	re = raw + rawsize;
	off = l32get(raw + 36);
	nmdom = 0;
	for (r = raw + off, re = raw + rawsize; r < re; r += 22)
		nmdom++;
	t = mkatable(parent, MSCT, name, raw, rawsize,
	             sizeof(Msct) + nmdom * sizeof(Mdom));
	msct = t->tbl;
	msct->ndoms = l32get(raw + 40) + 1;
	msct->nclkdoms = l32get(raw + 44) + 1;
	msct->maxpa = l64get(raw + 48);
	msct->ndoms = nmdom;
	msct->dom = nil;
	if (nmdom != 0)
		msct->dom = (void *)msct + sizeof(Msct);
	for (i = 0, r = raw; i < nmdom; i++, r += 22) {
		msct->dom[i].start = l32get(r + 2);
		msct->dom[i].end = l32get(r + 6);
		msct->dom[i].maxproc = l32get(r + 10);
		msct->dom[i].maxmem = l64get(r + 14);
	}
	mscttbl = finatable_nochildren(t);

	return mscttbl;
}

/* TODO(rminnich): only handles on IOMMU for now. */
static char *dumpdmar(char *start, char *end, Atable *dmar)
{
	Dmar *dt;

	if (dmar == nil)
		return start;

	dt = dmar->tbl;
	start = seprint(start, end, "acpi: DMAR addr %p:\n", dt);
	start = seprint(start, end, "\tdmar: intr_remap %d haw %d\n",
	                 dt->intr_remap, dt->haw);
	for (int i = 0; i < dmar->nchildren; i++) {
		Atable *at = dmar->children[i];
		Drhd *drhd = at->tbl;

		start = seprint(start, end, "\tDRHD: ");
		start = seprint(start, end, "%s 0x%02x 0x%016x\n",
		                 drhd->all & 1 ? "INCLUDE_PCI_ALL" : "Scoped",
		                 drhd->segment, drhd->rba);
	}

	return start;
}

static char *dumpsrat(char *start, char *end, Atable *table)
{
	if (table == nil)
		return seprint(start, end, "NO SRAT\n");
	start = seprint(start, end, "acpi: SRAT@%p:\n", table->tbl);
	for (; table != nil; table = table->next) {
		Srat *st = table->tbl;

		if (st == nil)
			continue;
		switch (st->type) {
			case SRlapic:
				start =
					seprint(start, end,
							 "\tlapic: dom %d apic %d sapic %d clk %d\n",
							 st->lapic.dom, st->lapic.apic, st->lapic.sapic,
							 st->lapic.clkdom);
				break;
			case SRmem:
				start = seprint(start, end, "\tmem: dom %d %#p %#p %c%c\n",
								 st->mem.dom, st->mem.addr, st->mem.len,
								 st->mem.hplug ? 'h' : '-',
								 st->mem.nvram ? 'n' : '-');
				break;
			case SRlx2apic:
				start =
					seprint(start, end, "\tlx2apic: dom %d apic %d clk %d\n",
							 st->lx2apic.dom, st->lx2apic.apic,
							 st->lx2apic.clkdom);
				break;
			default:
				start = seprint(start, end, "\t<unknown srat entry>\n");
		}
	}
	start = seprint(start, end, "\n");
	return start;
}

static Atable *parsesrat(Atable *parent,
                                char *name, uint8_t *p, size_t rawsize)
{

	Atable *t, *tt;
	uint8_t *pe;
	int stlen, flags;
	PSlice slice;
	char buf[16];
	int i;
	Srat *st;

	/* TODO: Parse the second SRAT */
	if (srat != nil) {
		print("Multiple SRATs detected and ignored!");
		return nil;
	}

	t = mkatable(parent, SRAT, name, p, rawsize, 0);
	psliceinit(&slice);
	pe = p + rawsize;
	for (p += 48, i = 0; p < pe; p += stlen, i++) {
		snprint(buf, sizeof(buf), "%d", i);
		stlen = p[1];
		tt = mkatable(t, SRAT, buf, p, stlen, sizeof(Srat));
		st = tt->tbl;
		st->type = p[0];
		switch (st->type) {
			case SRlapic:
				st->lapic.dom = p[2] | p[9] << 24 | p[10] << 16 | p[11] << 8;
				st->lapic.apic = p[3];
				st->lapic.sapic = p[8];
				st->lapic.clkdom = l32get(p + 12);
				if (l32get(p + 4) == 0) {
					free(tt);
					tt = nil;
				}
				break;
			case SRmem:
				st->mem.dom = l32get(p + 2);
				st->mem.addr = l64get(p + 8);
				st->mem.len = l64get(p + 16);
				flags = l32get(p + 28);
				if ((flags & 1) == 0) {	/* not enabled */
					free(tt);
					tt = nil;
				} else {
					st->mem.hplug = flags & 2;
					st->mem.nvram = flags & 4;
				}
				break;
			case SRlx2apic:
				st->lx2apic.dom = l32get(p + 4);
				st->lx2apic.apic = l32get(p + 8);
				st->lx2apic.clkdom = l32get(p + 16);
				if (l32get(p + 12) == 0) {
					free(tt);
					tt = nil;
				}
				break;
			default:
				print("unknown SRAT structure\n");
				free(tt);
				tt = nil;
				break;
		}
		if (tt != nil) {
			finatable_nochildren(tt);
			psliceappend(&slice, tt);
		}
	}
	srat = finatable(t, &slice);

	return srat;
}

static char *dumpslit(char *start, char *end, Slit *sl)
{
	int i;

	if (sl == nil)
		return start;
	start = seprint(start, end, "acpi slit:\n");
	for (i = 0; i < sl->rowlen * sl->rowlen; i++) {
		start = seprint(start, end,
						 "slit: %x\n",
						 sl->e[i / sl->rowlen][i % sl->rowlen].dist);
	}
	start = seprint(start, end, "\n");
	return start;
}

static int cmpslitent(void *v1, void *v2)
{
	SlEntry *se1, *se2;

	se1 = v1;
	se2 = v2;
	return se1->dist - se2->dist;
}

static Atable *parseslit(Atable *parent,
                                char *name, uint8_t *raw, size_t rawsize)
{
	Atable *t;
	uint8_t *r, *re;
	int i;
	SlEntry *se;
	size_t addsize, rowlen;
	void *p;

	addsize = sizeof(*slit);
	rowlen = l64get(raw + 36);
	addsize += rowlen * sizeof(SlEntry *);
	addsize += sizeof(SlEntry) * rowlen * rowlen;

	t = mkatable(parent, SLIT, name, raw, rawsize, addsize);
	slit = t->tbl;
	slit->rowlen = rowlen;
	p = (void *)slit + sizeof(*slit);
	slit->e = p;
	p += rowlen * sizeof(SlEntry *);
	for (i = 0; i < rowlen; i++) {
		slit->e[i] = p;
		p += sizeof(SlEntry) * rowlen;
	}
	for (i = 0, r = raw + 44, re = raw + rawsize; r < re; r++, i++) {
		int j = i / rowlen;
		int k = i % rowlen;

		se = &slit->e[j][k];
		se->dom = k;
		se->dist = *r;
	}

#if 0
	/* TODO: might need to sort this shit */
	for (i = 0; i < slit->rowlen; i++)
		qsort(slit->e[i], slit->rowlen, sizeof(slit->e[0][0]), cmpslitent);
#endif

	return finatable_nochildren(t);
}

/*
 * we use mp->machno (or index in Mach array) as the identifier,
 * but ACPI relies on the apic identifier.
 */
int
corecolor(int core)
{
	/* FIXME */
	return -1;
#if 0
	Mach *m;
	Srat *sl;
	static int colors[32];

	if(core < 0 || core >= MACHMAX)
		return -1;
	m = sys->machptr[core];
	if(m == nil)
		return -1;

	if(core >= 0 && core < nelem(colors) && colors[core] != 0)
		return colors[core] - 1;

	for(sl = srat; sl != nil; sl = sl->next)
		if(sl->type == SRlapic && sl->lapic.apic == m->apicno){
			if(core >= 0 && core < nelem(colors))
				colors[core] = 1 + sl->lapic.dom;
			return sl->lapic.dom;
		}
	return -1;
#endif
}

int pickcore(int mycolor, int index)
{

	if (slit == nil)
		return 0;
	return 0;
#if 0
	int color;
	int ncorepercol;
	ncorepercol = num_cores / slit->rowlen;
	color = slit->e[mycolor][index / ncorepercol].dom;
	return color * ncorepercol + index % ncorepercol;
#endif
}

static char *polarity[4] = {
	"polarity/trigger like in ISA",
	"active high",
	"BOGUS POLARITY",
	"active low"
};

static char *trigger[] = {
	"BOGUS TRIGGER",
	"edge",
	"BOGUS TRIGGER",
	"level"
};

static char *printiflags(char *start, char *end, int flags)
{

	return seprint(start, end, "[%s,%s]",
					polarity[flags & AFpmask], trigger[(flags & AFtmask) >> 2]);
}

static char *dumpmadt(char *start, char *end, Atable *apics)
{
	Madt *mt;

	if (apics == nil)
		return start;

	mt = apics->tbl;
	if (mt == nil)
		return seprint(start, end, "acpi: no MADT");
	start = seprint(start, end, "acpi: MADT@%p: lapic paddr %p pcat %d:\n",
	                 mt, mt->lapicpa, mt->pcat);
	for (int i = 0; i < apics->nchildren; i++) {
		Atable *apic = apics->children[i];
		Apicst *st = apic->tbl;

		switch (st->type) {
			case ASlapic:
				start =
					seprint(start, end, "\tlapic pid %d id %d\n",
							 st->lapic.pid, st->lapic.id);
				break;
			case ASioapic:
			case ASiosapic:
				start =
					seprint(start, end,
							 "\tioapic id %d addr %p ibase %d\n",
							 st->ioapic.id, st->ioapic.addr, st->ioapic.ibase);
				break;
			case ASintovr:
				start =
					seprint(start, end, "\tintovr irq %d intr %d flags $%p",
							 st->intovr.irq, st->intovr.intr, st->intovr.flags);
				start = printiflags(start, end, st->intovr.flags);
				start = seprint(start, end, "\n");
				break;
			case ASnmi:
				start = seprint(start, end, "\tnmi intr %d flags $%p\n",
								 st->nmi.intr, st->nmi.flags);
				break;
			case ASlnmi:
				start =
					seprint(start, end, "\tlnmi pid %d lint %d flags $%p\n",
							 st->lnmi.pid, st->lnmi.lint, st->lnmi.flags);
				break;
			case ASlsapic:
				start =
					seprint(start, end,
							 "\tlsapic pid %d id %d eid %d puid %d puids %s\n",
							 st->lsapic.pid, st->lsapic.id, st->lsapic.eid,
							 st->lsapic.puid, st->lsapic.puids);
				break;
			case ASintsrc:
				start =
					seprint(start, end,
							 "\tintr type %d pid %d peid %d iosv %d intr %d %#x\n",
							 st->type, st->intsrc.pid, st->intsrc.peid,
							 st->intsrc.iosv, st->intsrc.intr,
							 st->intsrc.flags);
				start = printiflags(start, end, st->intsrc.flags);
				start = seprint(start, end, "\n");
				break;
			case ASlx2apic:
				start =
					seprint(start, end, "\tlx2apic puid %d id %d\n",
							 st->lx2apic.puid, st->lx2apic.id);
				break;
			case ASlx2nmi:
				start =
					seprint(start, end, "\tlx2nmi puid %d intr %d flags $%p\n",
							 st->lx2nmi.puid, st->lx2nmi.intr,
							 st->lx2nmi.flags);
				break;
			default:
				start = seprint(start, end, "\t<unknown madt entry>\n");
		}
	}
	start = seprint(start, end, "\n");
	return start;
}

static Atable *parsemadt(Atable *parent,
                                char *name, uint8_t *p, size_t size)
{
	Atable *t, *tt;
	uint8_t *pe;
	Madt *mt;
	Apicst *st, *l;
	int id;
	size_t stlen;
	char buf[16];
	int i;
	PSlice slice;

	psliceinit(&slice);
	t = mkatable(parent, MADT, name, p, size, sizeof(Madt));
	mt = t->tbl;
	mt->lapicpa = l32get(p + 36);
	mt->pcat = l32get(p + 40);
	pe = p + size;
	for (p += 44, i = 0; p < pe; p += stlen, i++) {
		snprint(buf, sizeof(buf), "%d", i);
		stlen = p[1];
		tt = mkatable(t, APIC, buf, p, stlen, sizeof(Apicst));
		st = tt->tbl;
		st->type = p[0];
		switch (st->type) {
			case ASlapic:
				st->lapic.pid = p[2];
				st->lapic.id = p[3];
				if (l32get(p + 4) == 0) {
					free(tt);
					tt = nil;
				}
				break;
			case ASioapic:
				st->ioapic.id = id = p[2];
				st->ioapic.addr = l32get(p + 4);
				st->ioapic.ibase = l32get(p + 8);
				/* ioapic overrides any ioapic entry for the same id */
				for (int i = 0; i < pslicelen(&slice); i++) {
					l = ((Atable *)psliceget(&slice, i))->tbl;
					if (l->type == ASiosapic && l->iosapic.id == id) {
						st->ioapic = l->iosapic;
						/* we leave it linked; could be removed */
						break;
					}
				}
				break;
			case ASintovr:
				st->intovr.irq = p[3];
				st->intovr.intr = l32get(p + 4);
				st->intovr.flags = l16get(p + 8);
				break;
			case ASnmi:
				st->nmi.flags = l16get(p + 2);
				st->nmi.intr = l32get(p + 4);
				break;
			case ASlnmi:
				st->lnmi.pid = p[2];
				st->lnmi.flags = l16get(p + 3);
				st->lnmi.lint = p[5];
				break;
			case ASladdr:
				/* This is for 64 bits, perhaps we should not
				 * honor it on 32 bits.
				 */
				mt->lapicpa = l64get(p + 8);
				break;
			case ASiosapic:
				id = st->iosapic.id = p[2];
				st->iosapic.ibase = l32get(p + 4);
				st->iosapic.addr = l64get(p + 8);
				/* iosapic overrides any ioapic entry for the same id */
				for (int i = 0; i < pslicelen(&slice); i++) {
					l = ((Atable*)psliceget(&slice, i))->tbl;
					if (l->type == ASioapic && l->ioapic.id == id) {
						l->ioapic = st->iosapic;
						free(tt);
						tt = nil;
						break;
					}
				}
				break;
			case ASlsapic:
				st->lsapic.pid = p[2];
				st->lsapic.id = p[3];
				st->lsapic.eid = p[4];
				st->lsapic.puid = l32get(p + 12);
				if (l32get(p + 8) == 0) {
					free(tt);
					tt = nil;
				} else
					kstrdup(&st->lsapic.puids, (char *)p + 16);
				break;
			case ASintsrc:
				st->intsrc.flags = l16get(p + 2);
				st->type = p[4];
				st->intsrc.pid = p[5];
				st->intsrc.peid = p[6];
				st->intsrc.iosv = p[7];
				st->intsrc.intr = l32get(p + 8);
				st->intsrc.any = l32get(p + 12);
				break;
			case ASlx2apic:
				st->lx2apic.id = l32get(p + 4);
				st->lx2apic.puid = l32get(p + 12);
				if (l32get(p + 8) == 0) {
					free(tt);
					tt = nil;
				}
				break;
			case ASlx2nmi:
				st->lx2nmi.flags = l16get(p + 2);
				st->lx2nmi.puid = l32get(p + 4);
				st->lx2nmi.intr = p[8];
				break;
			default:
				print("unknown APIC structure\n");
				free(tt);
				tt = nil;
		}
		if (tt != nil) {
			finatable_nochildren(tt);
			psliceappend(&slice, tt);
		}
	}
	apics = finatable(t, &slice);

	return apics;
}

static Atable *parsedmar(Atable *parent,
                                char *name, uint8_t *raw, size_t rawsize)
{
	Atable *t, *tt;
	int i;
	int baselen = MIN(rawsize, 38);
	int nentry, nscope, npath, off, dslen, dhlen, type, flags;
	void *pathp;
	char buf[16];
	PSlice drhds;
	Drhd *drhd;
	Dmar *dt;

	/* count the entries */
	for (nentry = 0, off = 48; off < rawsize; nentry++) {
		dslen = l16get(raw + off + 2);
		print("acpi DMAR: entry %d is addr %p (0x%x/0x%x)\n",
		       nentry, raw + off, l16get(raw + off), dslen);
		off = off + dslen;
	}
	print("DMAR: %d entries\n", nentry);

	t = mkatable(parent, DMAR, name, raw, rawsize, sizeof(*dmar));
	dt = t->tbl;
	/* The table can be only partly filled. */
	if (baselen >= 38 && raw[37] & 1)
		dt->intr_remap = 1;
	if (baselen >= 37)
		dt->haw = raw[36] + 1;

	/* Now we walk all the DMAR entries. */
	psliceinit(&drhds);
	for (off = 48, i = 0; i < nentry; i++, off += dslen) {
		snprint(buf, sizeof(buf), "%d", i);
		dslen = l16get(raw + off + 2);
		type = l16get(raw + off);
		// TODO(dcross): Introduce sensible symbolic constants
		// for DMAR entry types. For right now, type 0 => DRHD.
		// We skip everything else.
		if (type != 0)
			continue;
		npath = 0;
		nscope = 0;
		for (int o = off + 16; o < (off + dslen); o += dhlen) {
			nscope++;
			dhlen = *(raw + o + 1);	// Single byte length.
			npath += ((dhlen - 6) / 2);
		}
		tt = mkatable(t, DRHD, buf, raw + off, dslen,
		              sizeof(Drhd) + 2 * npath +
		              nscope * sizeof(DevScope));
		flags = *(raw + off + 4);
		drhd = tt->tbl;
		drhd->all = flags & 1;
		drhd->segment = l16get(raw + off + 6);
		drhd->rba = l64get(raw + off + 8);
		drhd->nscope = nscope;
		drhd->scopes = (void *)drhd + sizeof(Drhd);
		pathp = (void *)drhd +
		    sizeof(Drhd) + nscope * sizeof(DevScope);
		for (int i = 0, o = off + 16; i < nscope; i++) {
			DevScope *ds = &drhd->scopes[i];

			dhlen = *(raw + o + 1);
			ds->enumeration_id = *(raw + o + 4);
			ds->start_bus_number = *(raw + o + 5);
			ds->npath = (dhlen - 6) / 2;
			ds->paths = pathp;
			for (int j = 0; j < ds->npath; j++)
				ds->paths[j] = l16get(raw + o + 6 + 2*j);
			pathp += 2*ds->npath;
			o += dhlen;
		}
		/*
		 * NOTE: if all is set, there should be no scopes of type
		 * This being ACPI, where vendors randomly copy tables
		 * from one system to another, and creating breakage,
		 * anything is possible. But we'll warn them.
		 */
		finatable_nochildren(tt);
		psliceappend(&drhds, tt);
	}
	dmar = finatable(t, &drhds);

	return dmar;
}

/*
 * Map the table and keep it there.
 */
static Atable *parsessdt(Atable *parent,
                                char *name, uint8_t *raw, size_t size)
{
	Atable *t;
	Sdthdr *h;

	/*
	 * We found it and it is too small.
	 * Simply return with no side effect.
	 */
	if (size < Sdthdrsz)
		return nil;
	t = mkatable(parent, SSDT, name, raw, size, 0);
	h = (Sdthdr *)raw;
	memmove(t->name, h->sig, sizeof(h->sig));
	t->name[sizeof(h->sig)] = '\0';

	return finatable_nochildren(t);
}

static char *dumptable(char *start, char *end, char *sig, uint8_t *p, int l)
{
	int n, i;

	if (2 > 1) {
		start = seprint(start, end, "%s @ %#p\n", sig, p);
		if (2 > 2)
			n = l;
		else
			n = 256;
		for (i = 0; i < n; i++) {
			if ((i % 16) == 0)
				start = seprint(start, end, "%x: ", i);
			start = seprint(start, end, " %2.2x", p[i]);
			if ((i % 16) == 15)
				start = seprint(start, end, "\n");
		}
		start = seprint(start, end, "\n");
		start = seprint(start, end, "\n");
	}
	return start;
}

static char *seprinttable(char *s, char *e, Atable *t)
{
	uint8_t *p;
	int i, n;

	p = (uint8_t *)t->tbl;	/* include header */
	n = t->rawsize;
	s = seprint(s, e, "%s @ %#p\n", t->name, p);
	for (i = 0; i < n; i++) {
		if ((i % 16) == 0)
			s = seprint(s, e, "%x: ", i);
		s = seprint(s, e, " %2.2x", p[i]);
		if ((i % 16) == 15)
			s = seprint(s, e, "\n");
	}
	return seprint(s, e, "\n\n");
}

static void *rsdsearch(char *signature)
{
//	uintptr_t p;
//	uint8_t *bda;
//	void *rsd;

	/*
	 * Search for the data structure signature:
	 * 1) in the BIOS ROM between 0xE0000 and 0xFFFFF.
	 */
	return sigscan(KADDR(0xE0000), 0x20000, signature);
}

/*
 * Note: some of this comment is from the unfinished user interpreter.
 *
 * The DSDT is always given to the user interpreter.
 * Tables listed here are also loaded from the XSDT:
 * MSCT, MADT, and FADT are processed by us, because they are
 * required to do early initialization before we have user processes.
 * Other tables are given to the user level interpreter for
 * execution.
 *
 * These historically returned a value to tell acpi whether or not it was okay
 * to unmap the table.  (return 0 means there was no table, meaning it was okay
 * to unmap).  We just use the kernbase mapping, so it's irrelevant.
 *
 * N.B. The intel source code defines the constants for ACPI in a
 * non-endian-independent manner. Rather than bring in the huge wad o' code
 * that represents, we just the names.
 */
typedef struct Parser {
	char *sig;
	Atable *(*parse)(Atable *parent,
	                        char *name, uint8_t *raw, size_t rawsize);
} Parser;


static Parser ptable[] = {
	{"FACP", parsefadt},
	{"APIC", parsemadt},
	{"DMAR", parsedmar},
	{"SRAT", parsesrat},
	{"SLIT", parseslit},
	{"MSCT", parsemsct},
	{"SSDT", parsessdt},
//	{"HPET", parsehpet},
};

/*
 * process xsdt table and load tables with sig, or all if nil.
 * (XXX: should be able to search for sig, oemid, oemtblid)
 */
static void parsexsdt(Atable *root)
{
	Sdthdr *sdt;
	Atable *table;
	PSlice slice;
	size_t l, end;
	uintptr_t dhpa;
	//Atable *n;
	uint8_t *tbl;
	//print("1\n");
	psliceinit(&slice);
	//print("2\n");
	//print("xsdt %p\n", xsdt);
	tbl = xsdt->p + sizeof(Sdthdr);
	end = xsdt->len - sizeof(Sdthdr);
	print("%s: tbl %p, end %d\n", __func__, tbl, end);
	for (int i = 0; i < end; i += xsdt->asize) {
		dhpa = (xsdt->asize == 8) ? l64get(tbl + i) : l32get(tbl + i);
		sdt = sdtmap(dhpa, 0, &l, 1);
		kmprint("sdt for map of %p, %d, 1 is %p\n", (void *)dhpa, l, sdt);
		if (sdt == nil)
			continue;
		kmprint("acpi: %s: addr %#p\n", __func__, sdt);
		for (int j = 0; j < nelem(ptable); j++) {
			kmprint("tb sig %s\n", ptable[j].sig);
			if (memcmp(sdt->sig, ptable[j].sig, sizeof(sdt->sig)) == 0) {
				table = ptable[j].parse(root, ptable[j].sig, (void *)sdt, l);
				if (table != nil)
					psliceappend(&slice, table);
				break;
			}
		}
	}
	kmprint("FINATABLE\n\n\n\n"); delay(1000);
	finatable(root, &slice);
}

void makeindex(Atable *root)
{
	uint64_t index;

	if (root == nil)
		return;
	index = root->qid.path >> QIndexShift;
	atableindex[index] = root;
	for (int k = 0; k < root->nchildren; k++)
		makeindex(root->children[k]);
}

static void parsersdptr(void)
{
	int asize, cksum;
	uintptr_t sdtpa;

//	static_assert(sizeof(Sdthdr) == 36);

	/* Find the root pointer. */
	rsd = rsdsearch("RSD PTR ");
	if (rsd == nil) {
		print("NO RSDP\n");
		return;
	}

	/*
	 * Initialize the root of ACPI parse tree.
	 */
	lastpath = Qroot;
	root = mkatable(nil, XSDT, devname(), nil, 0, sizeof(Xsdt));
	root->parent = root;

	kmprint("/* RSDP */ Rsdp = {%08c, %x, %06c, %x, %p, %d, %p, %x}\n",
		   rsd->signature, rsd->rchecksum, rsd->oemid, rsd->revision,
		   *(uint32_t *)rsd->raddr, *(uint32_t *)rsd->length,
		   *(uint32_t *)rsd->xaddr, rsd->xchecksum);

	kmprint("acpi: RSD PTR@ %#p, physaddr $%p length %ud %#llx rev %d\n",
		   rsd, l32get(rsd->raddr), l32get(rsd->length),
		   l64get(rsd->xaddr), rsd->revision);

	if (rsd->revision >= 2) {
		cksum = sdtchecksum(rsd, 36);
		if (cksum != 0) {
			print("acpi: bad RSD checksum %d, 64 bit parser aborted\n", cksum);
			return;
		}
		sdtpa = l64get(rsd->xaddr);
		asize = 8;
	} else {
		cksum = sdtchecksum(rsd, 20);
		if (cksum != 0) {
			print("acpi: bad RSD checksum %d, 32 bit parser aborted\n", cksum);
			return;
		}
		sdtpa = l32get(rsd->raddr);
		asize = 4;
	}

	/*
	 * process the RSDT or XSDT table.
	 */
	xsdt = root->tbl;
	xsdt->p = sdtmap(sdtpa, 0, &xsdt->len, 1);
	if (xsdt->p == nil) {
		print("acpi: sdtmap failed\n");
		return;
	}
	if ((xsdt->p[0] != 'R' && xsdt->p[0] != 'X')
		|| memcmp(xsdt->p + 1, "SDT", 3) != 0) {
		kmprint("acpi: xsdt sig: %c%c%c%c\n",
		       xsdt->p[0], xsdt->p[1], xsdt->p[2], xsdt->p[3]);
		xsdt = nil;
		return;
	}
	xsdt->asize = asize;
	root->raw = xsdt->p;
	root->rawsize = xsdt->len;
	kmprint("acpi: XSDT %#p\n", xsdt);
	parsexsdt(root);
	kmprint("POST PARSE XSDT raw is %p len is 0x%x\n", xsdt->p, xsdt->len);
	kmprint("parsexdt done: lastpath %d\n", lastpath);
	atableindex = reallocarray(nil, lastpath, sizeof(Atable *));
	assert(atableindex != nil);
	makeindex(root);
}

/*
 * The invariant that each level in the tree has an associated
 * Atable implies that each chan can be mapped to an Atable.
 * The assertions here enforce that invariant.
 */
static Atable *genatable(Chan *c)
{
	Atable *a;
	uint64_t ai;

	ai = c->qid.path >> QIndexShift;
	assert(ai < lastpath);
	a = atableindex[ai];
	assert(a != nil);

	return a;
}

static int acpigen(Chan *c, char *name, Dirtab *tab, int ntab,
		   int i, Dir *dp)
{
	Atable *a = genatable(c);

	if (i == DEVDOTDOT) {
		assert((c->qid.path & QIndexMask) == Qdir);
		devdir(c, a->parent->qid, a->parent->name, 0, eve, DMDIR|0555, dp);
		return 1;
	}
	return devgen(c, name, a->cdirs, a->nchildren + NQtypes, i, dp);
}

/*
 * Print the contents of the XSDT.
 */
static void dumpxsdt(void)
{
	kmprint("xsdt: len = %lu, asize = %lu, p = %p\n",
	       xsdt->len, xsdt->asize, xsdt->p);
}

static char *dumpGas(char *start, char *end, char *prefix, Gas *g)
{
	start = seprint(start, end, "%s", prefix);

	switch (g->spc) {
		case Rsysmem:
		case Rsysio:
		case Rembed:
		case Rsmbus:
		case Rcmos:
		case Rpcibar:
		case Ripmi:
			start = seprint(start, end, "[%s ", regnames[g->spc]);
			break;
		case Rpcicfg:
			start = seprint(start, end, "[pci ");
			start =
				seprint(start, end, "dev %#p ",
						 (uint32_t)(g->addr >> 32) & 0xFFFF);
			start =
				seprint(start, end, "fn %#p ",
						 (uint32_t)(g->addr & 0xFFFF0000) >> 16);
			start =
				seprint(start, end, "adr %#p ", (uint32_t)(g->addr & 0xFFFF));
			break;
		case Rfixedhw:
			start = seprint(start, end, "[hw ");
			break;
		default:
			start = seprint(start, end, "[spc=%#p ", g->spc);
	}
	start = seprint(start, end, "off %d len %d addr %#p sz%d]",
					 g->off, g->len, g->addr, g->accsz);
	start = seprint(start, end, "\n");
	return start;
}

static unsigned int getbanked(uintptr_t ra, uintptr_t rb, int sz)
{
	unsigned int r;

	r = 0;
	switch (sz) {
		case 1:
			if (ra != 0)
				r |= inb(ra);
			if (rb != 0)
				r |= inb(rb);
			break;
		case 2:
			if (ra != 0)
				r |= ins(ra);
			if (rb != 0)
				r |= ins(rb);
			break;
		case 4:
			if (ra != 0)
				r |= inl(ra);
			if (rb != 0)
				r |= inl(rb);
			break;
		default:
			print("getbanked: wrong size\n");
	}
	return r;
}

static unsigned int setbanked(uintptr_t ra, uintptr_t rb, int sz, int v)
{
	unsigned int r;
	print("setbanked: ra %#x rb %#x sz %d value %#x\n", ra, rb, sz, v);

	r = -1;
	switch (sz) {
		case 1:
			if (ra != 0)
				outb(ra, v);
			if (rb != 0)
				outb(rb, v);
			break;
		case 2:
			if (ra != 0)
				outs(ra, v);
			if (rb != 0)
				outs(rb, v);
			break;
		case 4:
			if (ra != 0)
				outl(ra, v);
			if (rb != 0)
				outl(rb, v);
			break;
		default:
			print("setbanked: wrong size\n");
	}
	return r;
}

static unsigned int getpm1ctl(void)
{
	assert(fadt != nil);
	return getbanked(fadt->pm1acntblk, fadt->pm1bcntblk, fadt->pm1cntlen);
}

static void setpm1sts(unsigned int v)
{
	assert(fadt != nil);
	setbanked(fadt->pm1aevtblk, fadt->pm1bevtblk, fadt->pm1evtlen / 2, v);
}

static unsigned int getpm1sts(void)
{
	assert(fadt != nil);
	return getbanked(fadt->pm1aevtblk, fadt->pm1bevtblk, fadt->pm1evtlen / 2);
}

static unsigned int getpm1en(void)
{
	int sz;

	assert(fadt != nil);
	sz = fadt->pm1evtlen / 2;
	return getbanked(fadt->pm1aevtblk + sz, fadt->pm1bevtblk + sz, sz);
}

static int getgpeen(int n)
{
	return inb(gpes[n].enio) & 1 << gpes[n].enbit;
}

static void setgpeen(int n, unsigned int v)
{
	int old;

	old = inb(gpes[n].enio);
	if (v)
		outb(gpes[n].enio, old | 1 << gpes[n].enbit);
	else
		outb(gpes[n].enio, old & ~(1 << gpes[n].enbit));
}

static void clrgpests(int n)
{
	outb(gpes[n].stsio, 1 << gpes[n].stsbit);
}

static unsigned int getgpests(int n)
{
	return inb(gpes[n].stsio) & 1 << gpes[n].stsbit;
}

static void acpiintr(Ureg *u, void *v)
{
	int i;
	unsigned int sts, en;

	print("acpi: intr\n");

	for (i = 0; i < ngpes; i++)
		if (getgpests(i)) {
			print("gpe %d on\n", i);
			en = getgpeen(i);
			setgpeen(i, 0);
			clrgpests(i);
			if (en != 0)
				print("acpiitr: calling gpe %d\n", i);
			//  queue gpe for calling gpe->ho in the
			//  aml process.
			//  enable it again when it returns.
		}
	sts = getpm1sts();
	en = getpm1en();
	print("acpiitr: pm1sts %#p pm1en %#p\n", sts, en);
	if (sts & en)
		print("have enabled events\n");
	if (sts & 1)
		print("power button\n");
	// XXX serve other interrupts here.
	setpm1sts(sts);
}

static void initgpes(void)
{
	int i, n0, n1;

	assert(fadt != nil);
	n0 = fadt->gpe0blklen / 2;
	n1 = fadt->gpe1blklen / 2;
	ngpes = n0 + n1;
	gpes = mallocz(sizeof(Gpe) * ngpes, 1);
	for (i = 0; i < n0; i++) {
		gpes[i].nb = i;
		gpes[i].stsbit = i & 7;
		gpes[i].stsio = fadt->gpe0blk + (i >> 3);
		gpes[i].enbit = (n0 + i) & 7;
		gpes[i].enio = fadt->gpe0blk + ((n0 + i) >> 3);
	}
	for (i = 0; i + n0 < ngpes; i++) {
		gpes[i + n0].nb = fadt->gp1base + i;
		gpes[i + n0].stsbit = i & 7;
		gpes[i + n0].stsio = fadt->gpe1blk + (i >> 3);
		gpes[i + n0].enbit = (n1 + i) & 7;
		gpes[i + n0].enio = fadt->gpe1blk + ((n1 + i) >> 3);
	}
}

static void startgpes(void)
{
	int i;
	for (i = 0; i < ngpes; i++) {
		setgpeen(i, 0);
		clrgpests(i);
	}
}

static void acpiioalloc(uint16_t addr, int len)
{
	if (ioalloc(addr, len, 1, "ACPI")  < 0)
		return;
	for (int io = addr; io < addr + len; io++)
		acpiport[io] = 1;
}

static void acpiinitonce(void)
{
	parsersdptr();
	if (root != nil)
		print("ACPI initialized\n");
	addarchfile("acpimem", 0444, acpimemread, nil);
	addarchfile("acpiio", 0666|DMEXCL, acpiioread, acpiiowrite);
	addarchfile("acpiintr", 0444|DMEXCL, acpiintrread, nil);
	/*
	 * should use fadt->xpm* and fadt->xgpe* registers for 64 bits.
	 * We are not ready in this kernel for that.
	 */
	assert(fadt != nil);
	acpiioalloc(fadt->smicmd, 1);
	acpiioalloc(fadt->pm1aevtblk, fadt->pm1evtlen);
	acpiioalloc(fadt->pm1bevtblk, fadt->pm1evtlen);
	acpiioalloc(fadt->pm1acntblk, fadt->pm1cntlen);
	acpiioalloc(fadt->pm1bcntblk, fadt->pm1cntlen);
	acpiioalloc(fadt->pm2cntblk, fadt->pm2cntlen);
	acpiioalloc(fadt->pmtmrblk, fadt->pmtmrlen);
	acpiioalloc(fadt->gpe0blk, fadt->gpe0blklen);
	acpiioalloc(fadt->gpe1blk, fadt->gpe1blklen);

	initgpes();

	outofyourelement();
}

static int acpienable(void)
{
	int i;

	/* This starts ACPI, which may require we handle
	 * power mgmt events ourselves. Use with care.
	 */
	outb(fadt->smicmd, fadt->acpienable);
	for (i = 0; i < 10; i++)
		if (getpm1ctl() & Pm1SciEn)
			break;
	if (i == 10) {
		print("acpi: failed to enable\n");
		return -1;
	}
	return 0;
}

static int acpidisable(void)
{
	int i;

	/* This starts ACPI, which may require we handle
	 * power mgmt events ourselves. Use with care.
	 */
	outb(fadt->smicmd, fadt->acpidisable);
	for (i = 0; i < 10; i++)
		if ((getpm1ctl() & Pm1SciEn) == 0)
			break;
	if (i == 10) {
		print("acpi: failed to disable\n");
		return -1;
	}
	return 0;
}

void acpistart(void)
{
	acpiev = qopen(1024, Qmsg, nil, nil);
	if (!acpiev) {
		print("acpistart: qopen failed; not enabling interrupts");
		return;
	}

	if (fadt->sciint != 0)
		intrenable(fadt->sciint, acpiintr, 0, BUSUNKNOWN, "acpi");
}

int acpiinit(void)
{
	static int once = 0;
	//die("acpiinit");
	if (! once)
		acpiinitonce();
	once++;
	return (root == nil) ? -1 : 0;
}

static Chan *acpiattach(char *spec)
{
	Chan *c;
	/*
	 * This was written for the stock kernel.
	 * This code must use 64 registers to be acpi ready in nix.
	 */
	if (acpiinit() < 0)
		error("no acpi");

	c = devattach(devdc(), spec);

	return c;
}

static Walkqid*acpiwalk(Chan *c, Chan *nc, char **name,
								int nname)
{
	/*
	 * Note that devwalk hard-codes a test against the location of 'devgen',
	 * so we pretty much have to not pass it here.
	 */
	return devwalk(c, nc, name, nname, nil, 0, acpigen);
}

static int acpistat(Chan *c, uint8_t *dp, int n)
{
	Atable *a = genatable(c);

	if (c->qid.type == QTDIR)
		a = a->parent;
	assert(a != nil);

	/* TODO(dcross): make acpigen work here. */
	return devstat(c, dp, n, a->cdirs, a->nchildren + NQtypes, devgen);
}

static Chan *acpiopen(Chan *c, int omode)
{
	return devopen(c, omode, nil, 0, acpigen);
}

static void acpiclose(Chan *unused)
{
}

static char *ttext;
static int tlen;

/* acpimemread allows processes to read acpi tables, using the offset as the
 * physical address. It hence enforces limits on what is visible.
 * This is NOT the same as Qraw; Qraw is the area associated with one device, and offsets
 * start at 0 in Qraw. We need this special read so we can make sense of pointers in tables,
 * which are physical addresses.
 */
static int32_t
acpimemread(Chan *c, void *a, int32_t n, int64_t off)
{
	Proc *up = externup();
	Acpilist *l;
	int ret;

	/* due to vmap limitations, you have to be on core 0. Make
	 * it easy for them. */
	procwired(up, 0);

	/* This is horribly insecure but, for now,
	 * focus on getting it to work.
	 * The only read allowed at 0 is sizeof(*rsd).
	 * Later on, we'll need to track the things we
	 * map with sdtmap and only allow reads of those
	 * areas. But let's see if this idea even works, first.
	 */
	//print("ACPI Qraw: rsd %p %p %d %p\n", rsd, a, n, (void *)off);
	if (off == 0){
		uint32_t pa = (uint32_t)PADDR(rsd);
		print("FIND RSD");
		print("PA OF rsd is %lx, \n", pa);
		return readmem(0, a, n, &pa, sizeof(pa));
	}
	if (off == PADDR(rsd)) {
		//print("READ RSD");
		//print("returning for rsd\n");
		//hexdump(rsd, sizeof(*rsd));
		return readmem(0, a, n, rsd, sizeof(*rsd));
	}

	l = findlist(off, n);
	/* we don't load all the lists, so this may be a new one. */
	if (! l) {
		size_t _;
		if (sdtmap(off, n, &_, 0) == nil){
			static char msg[256];
			snprint(msg, sizeof(msg), "unable to map acpi@%p/%d", off, n);
			error(msg);
		}
		l = findlist(off, n);
	}
	/* we really need to improve on plan 9 error message handling. */
	if (! l){
		static char msg[256];
		snprint(msg, sizeof(msg), "unable to map acpi@%p/%d", off, n);
		error(msg);
	}
	//hexdump(l->raw, l->size);
	ret = readmem(off-l->base, a, n, l->raw, l->size);
	//print("%d = readmem(0x%lx, %p, %d, %p, %d\n", ret, off-l->base, a, n, l->raw, l->size);
	return ret;
}

/*
 * acpiintrread waits on the acpiev Queue.
 */
static int32_t
acpiintrread(Chan *c, void *a, int32_t n, int64_t off)
{
	if (acpienable())
		error("Can't enable ACPI");
	startgpes();
	n = qread(acpiev, a, n);
	if (acpidisable())
		error("Can't disable ACPI");
	return n;
}

/* acpiioread only lets you read one of each type for now.
 * i.e. one byte, word, or long.
 */
static int32_t
acpiioread(Chan *c, void *a, int32_t n, int64_t off)
{
	int port, pm1aevtblk;
	uint8_t *p;
	uint16_t *sp;
	uint32_t *lp;

	pm1aevtblk = (int)fadt->pm1aevtblk;
	port = off;
	if (!acpiport[port]) {
		error("Bad port in acpiiread");
		return -1;
	}
	switch (n) {
	case 1:
		p = a;
		if (port == pm1aevtblk)
			*p = pm1status & 0xff;
		else if (port == (pm1aevtblk + 1))
			*p = pm1status >> 8;
		else
			*p = inb(port);
		return n;
	case 2:
		if (n & 1) {
			error("Odd address for word IO");
			break;
		}
		sp = a;
		if (port == pm1aevtblk)
			*sp = pm1status;
		else
			*sp = ins(port);
		return n;
	case 4:
		if (n & 3) {
			error("bad alignment for word io");
			break;
		}
		lp = a;
		*lp = inl(port);
		if (port == pm1aevtblk)
			*lp |= pm1status;
		return n;
	default:
		error("Bad size for IO in acpiioread");
	}
	return -1;
}

static int32_t
acpiiowrite(Chan *c, void *a, int32_t n, int64_t off)
{
	int port, pm1aevtblk;
	uint8_t *p;
	uint16_t *sp;
	uint32_t *lp;

	port = off;
	pm1aevtblk = (int)fadt->pm1aevtblk;
	if (!acpiport[port]) {
		error("invalid port in acpiiowrite");
		return -1;
	}
	switch (n) {
	case 1:
		p = a;
		if (port == pm1aevtblk)
			pm1status &= ~*p;
		else if (port == (pm1aevtblk + 1))
			pm1status &= ~(*p << 8);
		else
			outb(port, *p);
		return n;
	case 2:
		if (n & 1)
			error("Odd alignment for word io");
		sp = a;
		if (port == pm1aevtblk)
			pm1status &= ~*sp;
		outs(port, *sp);
		return n;
	case 4:
		if (n & 3)
			error("bad alignment for 32 bit IO");
		lp = a;
		if (port == pm1aevtblk)
			pm1status &= ~*lp;
		outl(port, (*lp & ~0xffff));
		return n;
	default:
		error("bad size for word io");
	}
	return -1;
}

// Get the table from the qid.
// Read that one table using the pointers.
// Actually, this function doesn't do this right now for pretty or table.
// It dumps the same table at every level. Working on it.
// It does the right thing for raw.
static int32_t acpiread(Chan *c, void *a, int32_t n, int64_t off)
{
	long q;
	Atable *t;
	char *ns, *s, *e, *ntext;
	int ret;

	if (ttext == nil) {
		tlen = 32768;
		ttext = mallocz(tlen, 1);
	}
	if (ttext == nil)
		error("acpiread: no memory");
	q = c->qid.path & QIndexMask;
	switch (q) {
	case Qdir:
		return devdirread(c, a, n, nil, 0, acpigen);
	case Qraw:
		t = genatable(c);
		ret = readmem(off, a, n, t->raw, t->rawsize);
		//print("%d = readmem(0x%lx, %p, %d, %p, %d\n", ret, off-l->base, a, n, l->raw, l->size);
		return ret;
	case Qtbl:
		s = ttext;
		e = ttext + tlen;
		strlcpy(s, "no tables\n", tlen);
		for (t = tfirst; t != nil; t = t->next) {
			ns = seprinttable(s, e, t);
			while (ns == e - 1) {
				ntext = realloc(ttext, tlen * 2);
				if (ntext == nil)
					panic("acpi: no memory\n");
				s = ntext + (ttext - s);
				ttext = ntext;
				tlen *= 2;
				e = ttext + tlen;
				ns = seprinttable(s, e, t);
			}
			s = ns;
		}
		return readstr(off, a, n, ttext);
	case Qpretty:
		s = ttext;
		e = ttext + tlen;
		s = dumpfadt(s, e, fadt);
		s = dumpmadt(s, e, apics);
		s = dumpslit(s, e, slit);
		s = dumpsrat(s, e, srat);
		s = dumpdmar(s, e, dmar);
		dumpmsct(s, e, mscttbl);
		return readstr(off, a, n, ttext);
	default:
		error("acpiread: bad path");
	}
	error("Permission denied");

	return -1;
}

static int32_t acpiwrite(Chan *c, void *a, int32_t n, int64_t off)
{
	error("not yet");
	return -1;
#if 0
	int acpiirq(uint32_t tbdf, int irq);
	Proc *up = externup();
	Cmdtab *ct;
	Cmdbuf *cb;

	uint32_t tbdf;
	int irq;

	if (c->qid.path == Qio) {
		if (reg == nil)
			error("region not configured");
		return regio(reg, a, n, off, 1);
	}
	if (c->qid.path != Qctl)
		error("Can only write Qctl");

	cb = parsecmd(a, n);
	if (waserror()) {
		free(cb);
		nexterror();
	}
	ct = lookupcmd(cb, ctls, nelem(ctls));
	switch (ct->index) {
	Reg *r;

		unsigned int rno, fun, dev, bus, i;
		case CMregion:
			/* TODO: this block is racy on reg (global) */
			r = reg;
			if (r == nil) {
				r = mallocz(sizeof(Reg), 1);
				r->name = nil;
			}
			kstrdup(&r->name, cb->f[1]);
			r->spc = acpiregid(cb->f[2]);
			if (r->spc < 0) {
				free(r);
				reg = nil;
				error("bad region type");
			}
			if (r->spc == Rpcicfg || r->spc == Rpcibar) {
				rno = r->base >> Rpciregshift & Rpciregmask;
				fun = r->base >> Rpcifunshift & Rpcifunmask;
				dev = r->base >> Rpcidevshift & Rpcidevmask;
				bus = r->base >> Rpcibusshift & Rpcibusmask;
				r->tbdf = MKBUS(BusPCI, bus, dev, fun);
				r->base = rno;	/* register ~ our base addr */
			}
			r->base = strtoul(cb->f[3], nil, 0);
			r->len = strtoul(cb->f[4], nil, 0);
			r->accsz = strtoul(cb->f[5], nil, 0);
			if (r->accsz < 1 || r->accsz > 4) {
				free(r);
				reg = nil;
				error("bad region access size");
			}
			reg = r;
			print("region %s %s %p %p sz%d",
				   r->name, acpiregstr(r->spc), r->base, r->len, r->accsz);
			break;
		case CMgpe:
			i = strtoul(cb->f[1], nil, 0);
			if (i >= ngpes)
				error(ERANGE, "gpe out of range");
			kstrdup(&gpes[i].obj, cb->f[2]);
			setgpeen(i, 1);
			break;
		case CMirq:
			tbdf = strtoul(cb->f[1], 0, 0);
			irq = strtoul(cb->f[2], 0, 0);
			acpiirq(tbdf, irq);
			break;
		default:
			panic("acpi: unknown ctl");
	}
	poperror();
	free(cb);
	return n;
#endif
}

struct {
	char *(*pretty)(Atable *atbl, char *start, char *end, void *arg);
} acpisw[NACPITBLS] = {
};

static char *pretty(Atable *atbl, char *start, char *end, void *arg)
{
	int type;

	type = atbl->type;
	if (type < 0 || NACPITBLS < type)
		return start;
	if (acpisw[type].pretty == nil)
		return seprint(start, end, "\"\"\n");
	return acpisw[type].pretty(atbl, start, end, arg);
}

static char *raw(Atable *atbl, char *start, char *end, void *unused_arg)
{
	size_t len = MIN(end - start, atbl->rawsize);

	memmove(start, atbl->raw, len);

	return start + len;
}

void outofyourelement(void)
{
	/*
	 * These just shut the compiler up.
	 */
	if(0)pretty(nil, nil, nil, nil);
	if(0)raw(nil, nil, nil, nil);
	if(0)dumpxsdt();
	if(0)dumptable(nil, nil, nil, nil, 0);
	if(0)cmpslitent(nil, nil);
	if(0)regio(nil, nil, 0, 0, 0);
	if(0)acpiregid(nil);
}

Dev acpidevtab = {
	//.dc = L'α',
	.dc = 'Z',
	.name = "acpi",

	.reset = devreset,
	.init = devinit,
	.shutdown = devshutdown,
	.attach = acpiattach,
	.walk = acpiwalk,
	.stat = acpistat,
	.open = acpiopen,
	.create = devcreate,
	.close = acpiclose,
	.read = acpiread,
	.bread = devbread,
	.write = acpiwrite,
	.bwrite = devbwrite,
	.remove = devremove,
	.wstat = devwstat,
};
