/*
 * minimal acpi support for multiprocessors.
 *
 * avoids AML but that's only enough to discover
 * the processors, not the interrupt routing details.
 */
#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "mp.h"
#include "mpacpi.h"

/* 8c says: out of fixed registers */
#define L64GET(p)	((uvlong)L32GET((p)+4) << 32 | L32GET(p))

enum {
	/* apic types */
	Apiclproc,
	Apicio,
	Apicintrsrcoverride,
	Apicnmisrc,
	Apiclnmi,
	Apicladdroverride,
	Apicios,
	Apicls,
	Apicintrsrc,
	Apiclx2,
	Apiclx2nmi,

	PcmpUsed = 1ul<<31,		/* Apic->flags addition */

	Lapicbase = 0x1b,		/* msr */

	Lapicae	= 1<<11,		/* apic enable in Lapicbase */
};

#define dprint(...)	if(mpdebug) print(__VA_ARGS__); else USED(mpdebug)

/* from mp.c */
int	mpdebug;
int	mpmachno;
Apic	mpapic[MaxAPICNO+1];
int	machno2apicno[MaxAPICNO+1];	/* inverse map: machno -> APIC ID */

Apic	*bootapic;

static int nprocid;

static uvlong
l64get(uchar *p)
{
	return L64GET(p);
}

int
apicset(Apic *apic, int type, int apicno, int f)
{
	if(apicno > MaxAPICNO)
		return -1;
	apic->type = type;
	apic->apicno = apicno;
	apic->flags = f | PcmpEN | PcmpUsed;
	return 0;
}

int
mpnewproc(Apic *apic, int apicno, int f)
{
	if(apic->flags & PcmpUsed) {
		print("mpnewproc: apic already enabled\n");
		return -1;
	}
	if (apicset(apic, PcmpPROCESSOR, apicno, f) < 0)
		return -1;
	apic->lintr[1] = apic->lintr[0] = ApicIMASK;
	/* botch! just enumerate */
	if(apic->flags & PcmpBP)
		apic->machno = 0;
	else
		apic->machno = ++mpmachno;
	machno2apicno[apic->machno] = apicno;
	return 0;
}

static int
mpacpiproc(uchar *p, ulong laddr)
{
	int id, f;
	ulong *vladdr;
	vlong base;
	char *already;
	Apic *apic;

	/* p bytes: type (0), len (8), cpuid, cpu_lapic id, flags[4] */
	id = p[3];
	/* cpu unusable flag or id out of range? */
	if((L32GET(p+4) & 1) == 0 || id > MaxAPICNO)
		return -1;

	vladdr = nil;
	already = "";
	f = 0;
	apic = &mpapic[id];
	dprint("\tmpacpiproc: apic %#p\n", apic);
	apic->paddr = laddr;
	if (nprocid++ == 0) {
		f = PcmpBP;
		vladdr = vmap(apic->paddr, 1024);
		if(apic->addr == nil){
			print("proc apic %d: failed to map %#p\n", id,
				apic->paddr);
			already = "(fail)";
		}
		bootapic = apic;
	}
	apic->addr = vladdr;

	if(apic->flags & PcmpUsed)
		already = "(on)";
	else
		mpnewproc(apic, id, f);

	if (0)
		dprint("\tapic proc %d/%d apicid %d flags%s%s %s\n", nprocid-1,
			apic->machno, id, f & PcmpBP? " boot": "",
			f & PcmpEN? " enabled": "", already);
	USED(already);

	rdmsr(Lapicbase, &base);
	if (!(base & Lapicae)) {
		dprint("mpacpiproc: enabling lapic\n");
		wrmsr(Lapicbase, base | Lapicae);
	}
	return 0;
}

static void
mpacpicpus(Madt *madt)
{
	int i, n;
	ulong laddr;
	uchar *p;

	laddr = L32GET(madt->addr);
	dprint("APIC mpacpicpus(%#p) lapic addr %#lux, flags %#ux\n",
		madt, laddr, L32GET(madt->flags));

	n = L32GET(&madt->sdthdr[4]);
	p = madt->structures;
	dprint("\t%d structures at %#p\n",n, p);
	/* byte 0 is assumed to be type, 1 is assumed to be length */
	for(i = offsetof(Madt, structures[0]); i < n; i += p[1], p += p[1])
		switch(p[0]){
		case Apiclproc:
			mpacpiproc(p, laddr);
			break;
		}
}

/* returns nil iff checksum is bad */
static void *
mpacpirsdchecksum(void* addr, int length)
{
	uchar *p, sum;

	sum = 0;
	for(p = addr; length-- > 0; p++)
		sum += *p;
	return sum == 0? addr: nil;
}

/* call func for each acpi table found */
static void
mpacpiscan(void (*func)(uchar *))
{
	int asize, i, tbllen, sdtlen;
	uintptr dhpa, sdtpa;
	uchar *tbl, *sdt;
	Rsd *rsd;

	dprint("ACPI...");
	if((rsd = sigsearch("RSD PTR ")) == nil) {
		dprint("none\n");
		return;
	}

	dprint("rsd %#p physaddr %#ux length %ud %#llux rev %d oem %.6s\n",
		rsd, L32GET(rsd->raddr), L32GET(rsd->length),
		l64get(rsd->xaddr), rsd->revision, (char*)rsd->oemid);

	if(rsd->revision == 2){
		if(mpacpirsdchecksum(rsd, 36) == nil)
			return;
		asize = 8;
		sdtpa = l64get(rsd->xaddr);
	} else {
		if(mpacpirsdchecksum(rsd, 20) == nil)
			return;
		asize = 4;
		sdtpa = L32GET(rsd->raddr);
	}

	if((sdt = vmap(sdtpa, 8)) == nil)
		return;
	if((sdt[0] != 'R' && sdt[0] != 'X') || memcmp(sdt+1, "SDT", 3) != 0){
		vunmap(sdt, 8);
		return;
	}
	sdtlen = L32GET(sdt + 4);
	vunmap(sdt, 8);

	if((sdt = vmap(sdtpa, sdtlen)) == nil)
		return;
	if(mpacpirsdchecksum(sdt, sdtlen) != nil)
		for(i = 36; i < sdtlen; i += asize){
			if(asize == 8)
				dhpa = l64get(sdt+i);
			else
				dhpa = L32GET(sdt+i);
	
			if((tbl = vmap(dhpa, 8)) == nil)
				continue;
			tbllen = L32GET(tbl + 4);
			vunmap(tbl, 8);
	
			if((tbl = vmap(dhpa, tbllen)) == nil)
				continue;
			if(mpacpirsdchecksum(tbl, tbllen) != nil)
				(*func)(tbl);
			vunmap(tbl, tbllen);
		}
	vunmap(sdt, sdtlen);
}

static void
mpacpitbl(uchar *p)
{
	/* for now, just activate any idle cpus */
	if (memcmp(p, "APIC", 4) == 0)
		mpacpicpus((Madt *)p);
}

static void
mpacpi(void)
{
	mpdebug = getconf("*debugmp") != nil;
	mpacpiscan(mpacpitbl);
}

void	(*mpacpifunc)(void) = mpacpi;
