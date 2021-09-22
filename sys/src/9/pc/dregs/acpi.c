/*
 * ACPI 4.0 Support.
 * Still WIP.
 *
 * This loads primary ACPI tables and also prints
 * AML code for use on a user-level interpreter.
 * When the interpreter is ready, the code must be
 * evaluated and not printed. We need a way to
 * be able to run the interpreter from user level
 * it is a VERY dangerous piece of software.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "acpi.h"

#define DBGFLG 0			/* was 4 */
#define DBG(...)	if(DBGFLG) print(__VA_ARGS__)

#define L16GET(p)	(((p)[1]<<8)|(p)[0])
#define L32GET(p)	(((u32int)L16GET(p+2)<<16)|L16GET(p))

typedef struct Parse Parse;
struct Parse
{
	char*	sig;
	Atable*	(*f)(uchar*, int);	/* return 0 to keep vmap */
};

/* return nil to vunmap */
static Atable* acpifadt(uchar*, int);
static Atable* acpiapic(uchar*, int);
static Atable* acpisrat(uchar*, int);
static Atable* acpimsct(uchar*, int);
Atable* acpidsdt(uchar*, int);

/* tables listed here are loaded from the XSDT */
static Parse ptables[] =
{
	"FACP", acpifadt,
	"APIC",	acpiapic,
	"SSDT", acpidsdt,
	"SRAT",	acpisrat,
	"MSCT",	acpimsct,
};

static Fadt	fadt;	/* To reach ACPI registers */
static Madt*	apics;	/* APIC info */
static Xsdt*	xsdt;	/* XSDT table */
static Facs*	facs;	/* Firmware ACPI control structure */
static Srat*	srat;	/* System resource affinity */
static Msct*	msct;	/* Maximum system characteristics table */
static Atable*	tables;	/* loaded SSDT tables */

static Atable*
newtable(uchar *p)
{
	Atable *t;
	Sdthdr *h;

	t = malloc(sizeof(Atable));
	if(t == nil)
		panic("no memory for more aml tables");
	t->tbl = h = (Sdthdr*)p;
	t->is64 = h->rev >= 2;
	t->data = p + Sdthdrsz;
	t->dlen = L32GET(h->length) - Sdthdrsz;
	memmove(t->sig, h->sig, sizeof(h->sig));
	t->sig[sizeof(t->sig)-1] = 0;
	memmove(t->oemid, h->oemid, sizeof(h->oemid));
	t->oemtblid[sizeof(t->oemtblid)-1] = 0;
	memmove(t->oemtblid, h->oemtblid, sizeof(h->oemtblid));
	t->oemtblid[sizeof(t->oemtblid)-1] = 0;
	t->next = tables;
	tables = t;

	return t;
}

static Atable*
findtable(char *sig, char *oemid, char *oemtblid)
{
	Atable *t;

	for(t = tables; t != nil; t = t->next){
		if(sig != nil && strcmp(t->sig, sig) != 0 ||
		   oemid != nil && strcmp(t->oemid, oemid) != 0 ||
		   oemtblid != nil && strcmp(t->oemtblid, oemtblid) != 0)
			continue;
		break;
	}
	return t;
}

static void
gasget(Gas *gas, uchar *p)
{
	gas->id = p[0];
	gas->rsz = p[1];
	gas->boff = p[2];
	gas->asz = p[3];
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

static void
dumpfacs(Facs *facs)
{
	if(DBGFLG == 0)
		return;

	DBG("acpi: facs: hwsig: %#ux\n", facs->hwsig);
	DBG("acpi: facs: wakingv: %#ux\n", facs->wakingv);
	DBG("acpi: facs: flags: %#ux\n", facs->flags);
	DBG("acpi: facs: glock: %#ux\n", facs->glock);
	DBG("acpi: facs: xwakingv: %#llux\n", facs->xwakingv);
	DBG("acpi: facs: vers: %#ux\n", facs->vers);
	DBG("acpi: facs: ospmflags: %#ux\n", facs->ospmflags);
}

static void
dumpmadt(Madt *apics)
{
	Apicst *st;

	if(DBGFLG == 0)
		return;
	DBG("acpi: madt lapic addr %llux pcat %d:\n", apics->laddr, apics->pcat);
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

static void
dumpsrat(Srat *st)
{
	if(DBGFLG == 0)
		return;
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

static void
dumpmsct(Msct *msct)
{
	Mdom *st;

	if(DBGFLG == 0)
		return;
	DBG("acpi: msct: %d doms %d clkdoms %#ullx maxpa\n",
		msct->ndoms, msct->nclkdoms, msct->maxpa);
	for(st = msct->dom; st != nil; st = st->next)
		DBG("\t[%d:%d] %d maxproc %#ullx maxmmem\n",
			st->start, st->end, st->maxproc, st->maxmem);
	DBG("\n");
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

/*
 * Both DSDT and SSDT.
 * NB: I have one machine were attempts to dump SSDTs panic.
 * This should return a table handle and evaluate its aml.
 */
Atable*
acpidsdt(uchar *p, int l)
{
	int n, i;
	char sig[5];
	Atable *t;

	if(l < Sdthdrsz)
		return nil;
	memmove(sig, p, 4);
	sig[4] = 0;
	if(strcmp(sig, "DSDT") != 0 && strcmp(sig, "SSDT") != 0)
		return nil;

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
	t = newtable(p);

	/* should evaluate AML here. */

	return t;
}

static void
loaddsdt(uintptr pa)
{
	int n;
	uchar *dsdtp;

	dsdtp = sdtmap(pa, &n, 1);
	if(dsdtp == nil)
		return;
	if(acpidsdt(dsdtp, n) != 0)
		vunmap(dsdtp, n);
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
	return 0;
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
		st = malloc(sizeof(Apicst));
		if(st == nil){
			print("acpiapic: no memory\n");
			break;
		}
		st->type = p[0];
		st->next = nil;
		stlen = p[1];
		switch(st->type){
		case SRlapic:
			st->lapic.dom = p[2] | p[9]<<24| p[10]<<16 | p[11]<<8;
			st->lapic.apic = p[3];
			st->lapic.sapic = p[8];
			st->lapic.clkdom = L32GET(p+12);
			if(L32GET(p+4) == 0){
				free(st);
				st = nil;
			}
			break;
		case SRmem:
			st->mem.dom = L32GET(p+2);
			st->mem.addr = l64get(p+8);
			st->mem.len = l64get(p+16);
			flags = L32GET(p+28);
			if((flags&1) == 0){	/* not enabled */
				free(st);
				st = nil;
			}else{
				st->mem.hplug = flags & 2;
				st->mem.nvram = flags & 4;
			}
			break;
		case SRlx2apic:
			st->lx2apic.dom = L32GET(p+4);
			st->lx2apic.apic = L32GET(p+8);
			st->lx2apic.clkdom = L32GET(p+16);
			if(L32GET(p+12) == 0){
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
	return nil;
}

static Atable*
acpimsct(uchar *p, int len)
{
	uchar *pe;
	Mdom **stl, *st;
	int off;

	msct = malloc(sizeof(Msct));
	if(msct == nil){
		print("acpimsct: no memory\n");
		return nil;
	}
	msct->ndoms = L32GET(p+40) + 1;
	msct->nclkdoms = L32GET(p+44) + 1;
	msct->maxpa = l64get(p+48);
	msct->dom = nil;
	stl = &msct->dom;
	pe = p + len;
	off = L32GET(p+36);
	for(p += off; p < pe; p += 22){
		st = malloc(sizeof(Mdom));
		if (st == nil){
			print("acpimsct: no memory\n");
			break;
		}
		st->next = nil;
		st->start = L32GET(p+2);
		st->end = L32GET(p+6);
		st->maxproc = L32GET(p+10);
		st->maxmem = l64get(p+14);
		*stl = st;
		stl = &st->next;
	}

	dumpmsct(msct);
	return nil;
}

static Atable*
acpiapic(uchar *p, int len)
{
	uchar *pe;
	Apicst *st, *l, **stl;
	int stlen, id;

	apics = malloc(sizeof(Madt));
	if(apics == nil){
		print("acpiapic: no memory\n");
		return nil;
	}
	apics->laddr = L32GET(p+36);
	apics->pcat = L32GET(p+40);
	apics->st = nil;
	stl = &apics->st;
	pe = p + len;
	for(p += 44; p < pe; p += stlen){
		st = malloc(sizeof(Apicst));
		if(st == nil){
			print("acpiapic: no memory\n");
			break;
		}
		st->type = p[0];
		st->next = nil;
		stlen = p[1];
		switch(st->type){
		case ASlapic:
			st->lapic.pid = p[2];
			st->lapic.id = p[3];
			if(L32GET(p+4) == 0){
				free(st);
				st = nil;
			}
			break;
		case ASioapic:
			st->ioapic.id = p[2];
			st->ioapic.addr = L32GET(p+4);
			st->ioapic.ibase = L32GET(p+8);
			break;
		case ASintovr:
			st->intovr.irq = p[3];
			st->intovr.intr = L32GET(p+4);
			st->intovr.flags = L16GET(p+8);
			break;
		case ASnmi:
			st->nmi.flags = L16GET(p+2);
			st->nmi.intr = L32GET(p+4);
			break;
		case ASlnmi:
			st->lnmi.pid = p[2];
			st->lnmi.flags = L16GET(p+3);
			st->lnmi.lint = p[5];
			break;
		case ASladdr:
			apics->laddr = l64get(p+8);
			break;
		case ASiosapic:
			id = st->iosapic.id = p[2];
			st->iosapic.ibase = L32GET(p+4);
			st->iosapic.addr = l64get(p+8);
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
			st->lsapic.puid = L32GET(p+12);
			if(L32GET(p+8) == 0){
				free(st);
				st = nil;
			}else
				kstrdup(&st->lsapic.puids, (char*)p+16);
			break;
		case ASintsrc:
			st->intsrc.flags = L16GET(p+2);
			st->type = p[4];
			st->intsrc.pid = p[5];
			st->intsrc.peid = p[6];
			st->intsrc.iosv = p[7];
			st->intsrc.intr = L32GET(p+8);
			st->intsrc.any = L32GET(p+12);
			break;
		case ASlx2apic:
			st->lx2apic.id = L32GET(p+4);
			st->lx2apic.puid = L32GET(p+12);
			if(L32GET(p+8) == 0){
				free(st);
				st = nil;
			}
			break;
		case ASlx2nmi:
			st->lx2nmi.flags = L16GET(p+2);
			st->lx2nmi.puid = L32GET(p+4);
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
	return nil;
}

/*
 * process xsdt table and load tables with sig, or all if nil.
 * (XXX: should be able to search for sig, oemid, oemtblid)
 */
static int
acpixsdt(char *sig)
{
	int i, l, t, mapped, found;
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
		mapped = 1;
		memmove(tsig, sdt, 4);
		tsig[4] = 0;
		if(sig == nil || strcmp(sig, tsig) == 0){
			DBG("acpi: %s addr %#p\n", tsig, sdt);
			for(t = 0; t < nelem(ptables); t++)
				if(strcmp(tsig, ptables[t].sig) == 0){
					mapped = ptables[t].f(sdt, l) != nil;
					found = 1;
					break;
				}
		}
		if(mapped)
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
	acpixsdt(nil);
	/* xsdt is kept and not unmapped */

}

static int
gasfmt(Fmt* fmt)
{
	static char* rnames[] = {
			"mem", "io", "pcicfg", "embed",
			"smb", "cmos", "pcibar", "ipmi"};
	Gas *g;
	char *s, *e;
	char buf[80];

	g = va_arg(fmt->args, Gas*);
	s = buf;
	e = buf + sizeof(buf);
	switch(g->id){
	case Rsysmem:
	case Rsysio:
	case Rembed:
	case Rsmbus:
	case Rcmos:
	case Rpcibar:
	case Ripmi:
		s = seprint(s, e, "[%s ", rnames[g->id]);
		break;
	case Rpcicfg:
		s = seprint(s, e, "[pci ");
		s = seprint(s, e, "dev %#ulx ", (ulong)(g->addr >> 32) & 0xFFFF);
		s = seprint(s, e, "fn %#ulx ", (ulong)(g->addr & 0xFFFF0000) >> 16);
		s = seprint(s, e, "off %#ulx ", (ulong)(g->addr &0xFFFF));
		break;
	case Rfixedhw:
		s = seprint(s, e, "[hw ");
		break;
	default:
		s = seprint(s, e, "[%#ux ", g->id);
	}
	seprint(s, e, "%d rsz %d boff %d addr %#ullx]", g->rsz, g->boff, g->asz, g->addr);
	return fmtstrcpy(fmt, buf);
}

void
acpilink(void)
{
	fmtinstall('G', gasfmt);
	acpirsdptr();
}
