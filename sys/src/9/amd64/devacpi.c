/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"
#include "mp.h"
#include <acpi/acpica/acpi.h>

enum
{

	Sdthdrsz	= 36,	/* size of SDT header */
	Qdir = 0,
	Qctl,
	Qtbl,
	Qio,
};

#if 0
static Cmdtab ctls[] =
{
	{CMregion,	"region",	6},
	{CMgpe,		"gpe",		3},
};
#endif

static Dirtab acpidir[]={
	".",		{Qdir, 0, QTDIR},	0,	DMDIR|0555,
	"acpictl",	{Qctl},			0,	0666,
	"acpitbl",	{Qtbl},			0,	0444,
	"acpiregio",	{Qio},			0,	0666,
};

#if 0
static char* regnames[] = {
	"mem", "io", "pcicfg", "embed",
	"smb", "cmos", "pcibar",
};
#endif

/*
 * we use mp->machno (or index in Mach array) as the identifier,
 * but ACPI relies on the apic identifier.
 */
int
corecolor(int core)
{
	Mach *m;
	static int colors[32];

	if(core < 0 || core >= MACHMAX)
		return -1;
	m = sys->machptr[core];
	if(m == nil)
		return -1;

	if(core >= 0 && core < nelem(colors) && colors[core] != 0)
		return colors[core] - 1;

	return -1;
}


int
pickcore(int mycolor, int index)
{
	return 0;
}
static void*
rsdscan(uint8_t* addr, int len, char* signature)
{
	int sl;
	uint8_t *e, *p;

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
	uintptr_t p;
	uint8_t *bda;
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
	void *rsd;
	rsd = rsdsearch("RSD PTR ");
	if (rsd == nil) {
		print("NO RSD PTR found\n");
		return;
	}
	print("Found RST PTR ta %p\n", rsd);
		
#if 0
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
#endif
}

static int
acpigen(Chan *c, char* d, Dirtab *tab, int ntab, int i, Dir *dp)
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

ACPI_STATUS
AcpiOsInitialize(void)
{
	acpirsdptr();
	return AE_OK;
}

ACPI_STATUS
AcpiOsTerminate (
	void)
{
	print("%s\n", __func__);
	return AE_OK;
}

int
acpiinit(void)
{
	AcpiInitializeSubsystem();
	//AcpiOsInitialize();
	return 0;
}

static Chan*
acpiattach(char *spec)
{
	return nil;
//	return devattach(L'α', spec);
}

static Walkqid*
acpiwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, acpidir, nelem(acpidir), acpigen);
}

static int32_t
acpistat(Chan *c, uint8_t *dp, int32_t n)
{
	return devstat(c, dp, n, acpidir, nelem(acpidir), acpigen);
}

static Chan*
acpiopen(Chan *c, int omode)
{
	return devopen(c, omode, acpidir, nelem(acpidir), acpigen);
}

static void
acpiclose(Chan *c)
{
}

#if 0
static char*ttext;
static int tlen;
#endif

static int32_t
acpiread(Chan *c, void *a, int32_t n, int64_t off)
{
	uint64_t q;

	q = c->qid.path;
	switch(q){
	case Qdir:
		return devdirread(c, a, n, acpidir, nelem(acpidir), acpigen);
	case Qtbl:
		return -1; //readstr(off, a, n, ttext);
	case Qio:
		return -1; //regio(reg, a, n, off, 0);
	}
	error(Eperm);
	return -1;
}

static int32_t
acpiwrite(Chan *c, void *a, int32_t n, int64_t off)
{
	if(c->qid.path == Qio){
		//if(reg == nil)
		error("region not configured");
	}
	if(c->qid.path != Qctl)
		error(Eperm);

	error("NP");
#if 0
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
#endif
	return -1;
}


Dev acpidevtab = {
	.dc = L'α',
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

/* Shims. We'll leave these here for now until we find a better way. */
ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width)
{
	panic("%s", __func__);
	return AE_OK;

}

ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  Value,
    UINT32                  Width)
{
	panic("%s", __func__);
	return AE_OK;
}

/*
 * Miscellaneous
 */
BOOLEAN
AcpiOsReadable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
	panic("%s", __func__);
	return AE_OK;
}


BOOLEAN
AcpiOsWritable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
	panic("%s", __func__);
	return AE_OK;
}


UINT64
AcpiOsGetTimer (
    void)
{
	panic("%s", __func__);
	return AE_OK;
}


ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
	panic("%s", __func__);
	return AE_OK;
}

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Format,
    ...)
{
	panic("printf");
}

void
AcpiOsVprintf (
    const char              *Format,
    va_list                 Args)
{
	panic("%s", __func__);
}

void
AcpiOsFree (
    void *                  Memory)
{
	free(Memory);
}

void *
AcpiOsAllocate (
    ACPI_SIZE               Size)
{
	return malloc(Size);
}

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{
	panic("%s", __func__);
	return AE_OK;
}

void
AcpiOsUnmapMemory (
    void                    *LogicalAddress,
    ACPI_SIZE               Size)
{
	panic("%s", __func__);
}

ACPI_STATUS
AcpiOsGetPhysicalAddress (
    void                    *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS   *PhysicalAddress)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32                  MaxUnits,
    UINT32                  InitialUnits,
    ACPI_SEMAPHORE          *OutHandle)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_SEMAPHORE          Handle)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units,
    UINT16                  Timeout)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{
	ACPI_SPINLOCK l = mallocz(sizeof(ACPI_SPINLOCK), 1);
	if (l == nil)
		return AE_NO_MEMORY;
	*OutHandle = l;
	return AE_OK;
}

void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
	free(Handle);
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_SPINLOCK           Handle)
{
	panic("figure out flags");
	ilock(Handle);
	return 0;
}

void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
	panic("figure out flags");
	iunlock(Handle);
}

ACPI_STATUS
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
	panic("%s", __func__);
	return AE_OK;
}

void
AcpiOsWaitEventsComplete (
	void)
{
	panic("%s", __func__);
}

void
AcpiOsSleep (
    UINT64                  Milliseconds)
{
	panic("%s", __func__);
}

void
AcpiOsStall(
    UINT32                  Microseconds)
{
	panic("%s", __func__);
}

ACPI_THREAD_ID
AcpiOsGetThreadId (
    void)
{
	Proc *up = externup();
	return up->pid;
}

ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{
	panic("%s", __func__);
	return AE_OK;
}

/*
 * Platform and hardware-independent physical memory interfaces
 */
ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width)
{
	panic("%s", __func__);
	return AE_OK;
}

/*
 * ACPI Table interfaces
 */
ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsPhysicalTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength)
{
	panic("%s", __func__);
	return AE_OK;
}


/*
 * Memory/Object Cache
 */
ACPI_STATUS
AcpiOsCreateCache (
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_CACHE_T            **ReturnCache)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteCache (
    ACPI_CACHE_T            *Cache)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_CACHE_T            *Cache)
{
	panic("%s", __func__);
	return AE_OK;
}

void *
AcpiOsAcquireObject (
    ACPI_CACHE_T            *Cache)
{
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_CACHE_T            *Cache,
    void                    *Object)
{
	panic("%s", __func__);
	return AE_OK;
}


/*
 * Debug input
 */
ACPI_STATUS
AcpiOsGetLine (
    char                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  *BytesRead)
{
	panic("%s", __func__);
	return AE_OK;
}

