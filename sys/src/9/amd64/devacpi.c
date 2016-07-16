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
#include "apic.h"
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


/* 
 * This is the array of eyesores.
 * An Eyesore is an Interrupt Source Over Ride, which maps from
 * what they want to what it needs to be. You are not expected
 * to understand this.
 */
static ACPI_MADT_INTERRUPT_OVERRIDE *eyesore;
static int numeyesore;

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

static void *rsd;
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
	rsd = rsdsearch("RSD PTR ");
	if (rsd == nil) {
		print("NO RSD PTR found\n");
		return;
	}
	print("Found RST PTR ta %p\n", rsd);
		
#if 0
	assert(sizeof(Sdthdr) == 36);

	DBG("acpi: RSD PTR@ %#p, physaddr %#ux length %u %#llux rev %d\n",
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
	print("%s\n", __func__);
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

/* run AML with one integer arg. */
static int
run_aml_arg(char *name, int val)
{
    ACPI_OBJECT arg1;
    ACPI_OBJECT_LIST args;
    ACPI_STATUS as;

    arg1.Type = ACPI_TYPE_INTEGER;
    arg1.Integer.Value = 1;
    args.Count = 1;
    args.Pointer = &arg1;

    /* This does not work. Just leaving it here in case someone
     * else thinks it will.
	ACPI_STATUS as;
	ACPI_OBJECT arg[] = {
		{
			.Type = ACPI_TYPE_INTEGER,
			.Integer.Value = val
		}
	};
	ACPI_OBJECT_LIST args = {
		.Count = 1,
		.Pointer = arg
	};
    */
	as = AcpiEvaluateObject(ACPI_ROOT_OBJECT, name, &args, NULL);
	print("run_aml_arg(%s, %d) returns %d\n", name, val, as);
	return ACPI_SUCCESS(as);
}

static int
set_machine_mode(void)
{
	/* we always enable the APIC. */
	return run_aml_arg("_PIC", 1);
}

void pi(int indent)
{
	int i;
	for(i = 0; i < indent; i++)
		print(" ");
}

/* walk the whatever. Limited, right now. */
static void
objwalk(ACPI_OBJECT *p)
{
	static int indent;
	int cnt;
	ACPI_OBJECT *e;
	pi(indent);
	switch(p->Type) {
	case 4: // ACPI_DESC_TYPE_STATE_PACKAGE:
		print("Package:\n");
		indent += 2;
		e = p->Package.Elements;
		for(cnt = 0; cnt < p->Package.Count; cnt++, e++){
			objwalk(e);
		}
			
		indent -= 2;
		print("\n");
		break;
	case 1:
		print("Integer:0x%llx", p->Integer.Value);
		break;
	default:
		print("Can't handle type %d\n", p->Type);
		break;
	}
			
}

static ACPI_STATUS
resource(ACPI_RESOURCE *r, void *Context)
{
	ACPI_RESOURCE_IRQ *i = &r->Data.Irq;
	print("\tACPI_RESOURCE_TYPE_%d: Length %d\n", r->Type, r->Length);
	if (r->Type != ACPI_RESOURCE_TYPE_IRQ)
		return 0;
	print("\t\tIRQ Triggering %d Polarity %d Sharable %d InterruptCount %d: ", 
	      i->Triggering, i->Polarity, i->Sharable, i->InterruptCount);
	for(int j = 0; j < i->InterruptCount; j++)
		print("%d,", i->Interrupts[j]);
	print("\n");
	/* assumptions: we assume apic 0 for now. This will need to be fixed.
	 * We also just take the first interrupt. 
	 */
	uint32_t low = Im;
	switch (i->Polarity){
	case ACPI_ACTIVE_HIGH:
		low |= IPhigh;
		break;
	case ACPI_ACTIVE_LOW:
		low |= IPlow;
		break;
	case ACPI_ACTIVE_BOTH:
		low |= IPlow | IPhigh;
		break;
	default:
		print("BOTCH! i->Polarity is 0x%x and I don't do that\n", i->Polarity);
		break;
	}

	switch (i->Triggering) {
	case ACPI_LEVEL_SENSITIVE:
		low |= TMlevel;
		break;
	case ACPI_EDGE_SENSITIVE:
		low |= TMedge;
		break;
	default:
		print("BOTCH! i->Triggering is 0x%x and I don't do that\n", i->Triggering);
		break;
	}
	print("CODE: ioapicintrinit(0xff, 0x%x, 0x%x, 0x%x, 0x%x\n", 1, i->Interrupts[0], i->Interrupts[0]<<2, low);
	return 0;
}
ACPI_STATUS
device(ACPI_HANDLE                     Object,
    UINT32                          NestingLevel,
    void                            *Context,
    void                            **ReturnValue)

{
	ACPI_STATUS as;
	ACPI_DEVICE_INFO *info;
	as = AcpiGetObjectInfo(Object, &info);
	print("as is %d\n", as);
	if (!ACPI_SUCCESS(as))
		return 0;
	ACPI_BUFFER out;
	out.Length = ACPI_ALLOCATE_BUFFER;
	out.Pointer = nil;
	char n[5];
	memmove(n, &info->Name, sizeof(info->Name));
	n[4] = 0;
	print("%s\n", n);
	as = AcpiGetIrqRoutingTable(Object, &out);
	print("get the PRT: %d\n", as);
	print("Length is %u ptr is %p\n", out.Length, out.Pointer);
	if (ACPI_SUCCESS(as)) {
		void *p = (void *)out.Pointer;
		while(((ACPI_PCI_ROUTING_TABLE*)p)->Length > 0) {
			ACPI_PCI_ROUTING_TABLE *t = p;
			print("%s: ", t->Source);
			print("Pin 0x%x, Address 0x%llx, SourceIndex 0x%x\n", 
			      t->Address, t->SourceIndex);
			p += t->Length;
		}
	}
	as = AcpiWalkResources(Object, "_CRS", resource, nil);
	print("Walk resources: as is %d\n", as);
#if 0
	out.Length = ACPI_ALLOCATE_BUFFER;
	out.Pointer = nil;
	as = AcpiGetPossibleResources(Object, &out);
	print("get the possible resources: %d\n", as);
	if (ACPI_SUCCESS(as)) {
		void *p = (void *)out.Pointer;
		hexdump(out.Pointer, out.Length);
		while(((ACPI_RESOURCE*)p)->Type != ACPI_RESOURCE_TYPE_END_TAG) {
			ACPI_RESOURCE *r = p;
			ACPI_RESOURCE_IRQ *i = p + sizeof(r->Type);
			print("\tACPI_RESOURCE_TYPE_%d: Length %d\n", r->Type, r->Length);
			p += r->Length;
			if (r->Type != ACPI_RESOURCE_TYPE_IRQ)
				continue;
			print("\t\tIRQ Triggering %d Polarity %d Sharable %d InterruptCount %d: ", 
			      i->Triggering, i->Polarity, i->Sharable, i->InterruptCount);
			for(int j = 0; j < i->InterruptCount; j++)
				print("%d,", i->Interrupts[j]);
			print("\n");
			

		}
		print("Length is %u ptr is %p\n", out.Length, out.Pointer);
		
	}
#endif
	print("hi\n");
	
	return 0;
}

int
acpiinit(void)
{
	ACPI_STATUS as;
	ACPI_TABLE_HEADER *h;
	ACPI_BUFFER out;
	int status;
	int apiccnt = 1;
	out.Length = ACPI_ALLOCATE_BUFFER;
	out.Pointer = nil;
	status = AcpiInitializeSubsystem();
        if (ACPI_FAILURE(status))
		panic("can't start acpi");


        status = AcpiInitializeTables(NULL, 16, FALSE);
        if (ACPI_FAILURE(status))
		panic("can't set up acpi tables");

        status = AcpiLoadTables();
        if (ACPI_FAILURE(status))
		panic("Can't load ACPI tables");

        status = AcpiEnableSubsystem(0);
        if (ACPI_FAILURE(status))
		panic("Can't enable ACPI subsystem");

        status = AcpiInitializeObjects(0);
        if (ACPI_FAILURE(status))
		panic("Can't Initialize ACPI objects");

	int sublen;
	uint8_t *p;
	extern uint8_t *apicbase;
	ACPI_TABLE_MADT *m;
	status = AcpiGetTable(ACPI_SIG_MADT, apiccnt, &h);
	if (ACPI_FAILURE(status))
		panic("Can't find a MADT");
	m = (ACPI_TABLE_MADT *)h;
	print("APIC %d: %p 0x%x\n", apiccnt, (void *)(uint64_t)m->Address, m->Flags);
	if(apicbase == nil){
		if((apicbase = vmap((uintptr_t)m->Address, 1024)) == nil){
			panic("%s: can't map apicbase\n", __func__);
		}
		print("%s: apicbase %#p -> %#p\n", __func__, (void *)(uint64_t)m->Address, apicbase);
	}
	if (! set_machine_mode()){
		print("Set machine mode failed\n");
		return 0;
	}

	p = (void*)&m[1];
	sublen = m->Header.Length;
	/* we only process the ones we're certain we need to. */
	while (sublen > 0) {
		switch(p[0]){
		case ACPI_MADT_TYPE_LOCAL_APIC:
		{
			ACPI_MADT_LOCAL_APIC *l = (void *)p;
			if (!l->LapicFlags)
				break;
			apicinit(l->Id, m->Address, apiccnt == 1);
print("CODE: apicinit(%d, %p, %d\n", l->Id, m->Address, apiccnt == 1);
			apiccnt++;
		}
			break;
		case ACPI_MADT_TYPE_IO_APIC:
		{
			ACPI_MADT_IO_APIC *io = (void *)p;
			print("IOapic %d @ %p\n", io->Id, io->Address);
			ioapicinit(io->Id, io->Address);
print("CODE: ioapicinit(%d, %p);\n", io->Id, (void*)(uint64_t)io->Address);
		}
			break;
		case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
		{
			ACPI_MADT_INTERRUPT_OVERRIDE *e = (void *)p;
			print("What an eyesore. Bus %d, SourceIrq %d, GlobalIrq %d, InitFlags 0x%x\n",
			      e->Bus, e->SourceIrq, e->GlobalIrq, e->IntiFlags);
			eyesore = realloc(eyesore, numeyesore+1);
			if (! eyesore)
				panic("Ran out of eyesores");
			eyesore[numeyesore] = *e;
			numeyesore++;
		}
		break;
		case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
		{
			ACPI_MADT_LOCAL_APIC_NMI *nmi = (void *)p;
			apicnmi(nmi->ProcessorId, nmi->Lint, nmi->IntiFlags);
		}
			break;
		default:
			print("%s: can't handle subtable type %d\n", __func__, p[0]);
			break;
		}
		sublen -= p[1];
		p += p[1];
	}

	/* Get the _PRT */
	as = AcpiEvaluateObject(ACPI_ROOT_OBJECT, "\\_SB.PCI0._PRT", NULL, &out);
	print("get the PRT: %d\n", as);
	print("Length is %u ptr is %p\n", out.Length, out.Pointer);
	hexdump(out.Pointer, out.Length);
	objwalk(out.Pointer);

	as = AcpiGetDevices (nil, device, nil, nil);
	print("acpigetdevices %d\n", as);

/* per device code. Not useful yet.

	as = AcpiGetIrqRoutingTable(some device, &out);
	print("get the PRT: %d\n", as);
	print("Length is %u ptr is %p\n", out.Length, out.Pointer);
	hexdump(out.Pointer, out.Length);
*/
	print("CODE: ioapicintrinit(0xff, DONE\n");
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

static int tbdf(ACPI_PCI_ID *p)
{
	return (p->Bus << 8) | (p->Device << 3) | (p->Function);
}

ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width)
{
	Pcidev p;
	p.tbdf = tbdf(PciId);
	print("%s\n", __func__);
	switch(Width) {
	case 32:
		*Value = pcicfgr32(&p, Reg);
		break;
	case 16:
		*Value = pcicfgr16(&p, Reg);
		break;
	case 8:
		*Value = pcicfgr8(&p, Reg);
		break;
	default:
		panic("Can't read pci: bad width %d\n", Width);	
	}

	return AE_OK;

}

ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  Value,
    UINT32                  Width)
{
	Pcidev p;
	p.tbdf = tbdf(PciId);
	print("%s\n", __func__);
	switch(Width) {
	case 32:
		pcicfgw32(&p, Reg, Value);
		break;
	case 16:
		pcicfgw16(&p, Reg, Value);
		break;
	case 8:
		pcicfgw8(&p, Reg, Value);
		break;
	default:
		panic("Can't read pci: bad width %d\n", Width);	
	}

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
	print("%s\n", __func__);
	panic("%s", __func__);
	return AE_OK;
}


BOOLEAN
AcpiOsWritable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
	print("%s\n", __func__);
	panic("%s", __func__);
	return AE_OK;
}


UINT64
AcpiOsGetTimer (
    void)
{
	print("%s\n", __func__);
	panic("%s", __func__);
	return AE_OK;
}


ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
	print("%s\n", __func__);
	panic("%s", __func__);
	return AE_OK;
}

void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Format,
    ...)
{
	va_list args;

	va_start(args, Format);
	print((char *)Format, args);
	va_end(args);
}

void
AcpiOsVprintf (
    const char              *Format,
    va_list                 Args)
{
	/* This is a leaf function, and this function is required to implement
	 * the va_list argument. I couldn't find any other way to do this. */
	static char buf[1024];
	vseprint(buf, &buf[1023], (char *)Format, Args);
	print(buf);
}

void
AcpiOsFree (
    void *                  Memory)
{
	//print("%s\n", __func__);
	free(Memory);
}

void *
AcpiOsAllocate (
    ACPI_SIZE               Size)
{
	//print("%s\n", __func__);
	return malloc(Size);
}

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{
	void *v = vmap(Where, Length);
	print("%s %p = vmap(%p,0x%x)\n", __func__, v, (void*)Where, Length);
	print("Val @ %p is 0x%x\n", v, *(int *)v);
	return v;
}

void
AcpiOsUnmapMemory (
    void                    *LogicalAddress,
    ACPI_SIZE               Size)
{
	print("%s %p %d \n", __func__, LogicalAddress, Size);
	vunmap(LogicalAddress, Size);
}

ACPI_STATUS
AcpiOsGetPhysicalAddress (
    void                    *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS   *PhysicalAddress)
{
	ACPI_PHYSICAL_ADDRESS ret = mmuphysaddr((uintptr_t)LogicalAddress);
	print("%s %p = mmyphysaddr(%p)", __func__, (void *)ret, LogicalAddress);
	*PhysicalAddress = ret;
	return AE_OK;
}

/* This is the single threaded version of
 * these functions. This is now NetBSD does it. */
ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32                  MaxUnits,
    UINT32                  InitialUnits,
    ACPI_SEMAPHORE          *OutHandle)
{
	//print("%s\n", __func__);
	*OutHandle = (ACPI_SEMAPHORE) 1;
	return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_SEMAPHORE          Handle)
{
	//print("%s\n", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units,
    UINT16                  Timeout)
{
	//print("%s\n", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units)
{
	//print("%s\n", __func__);
	return AE_OK;
}

/* this is the single threaded case and as minix shows there is nothing to do. */
ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{
	//print("%s\n", __func__);
	*OutHandle = nil;
	return AE_OK;
}

void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
	//print("%s\n", __func__);
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_SPINLOCK           Handle)
{
	//print("%s\n", __func__);
	return 0;
}

void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
	//print("%s\n", __func__);
}

struct handler {
	ACPI_OSD_HANDLER        ServiceRoutine;
	void                    *Context;
};

/* The ACPI interrupt signature and the Harvey one are not compatible. So, we pass an arg to
 * intrenable that can in turn be used to this function to call the ACPI handler. */
static void acpihandler(Ureg *_, void *arg)
{
	struct handler *h = arg;
	h->ServiceRoutine(h->Context);
}

ACPI_STATUS
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{
	/* minix says "don't do it". So we don't, yet. */
	return AE_OK;
	struct handler *h = malloc(sizeof(*h));
	if (! h)
		return AE_NO_MEMORY;
	h->ServiceRoutine = ServiceRoutine;
	h->Context = Context;
	print("%s %d %p %p \n", __func__, InterruptNumber, ServiceRoutine, Context);
	/* once enabled, can't be disabled; ignore the return value unless it's nil. */
	intrenable(InterruptNumber, acpihandler, h, 0x5, "ACPI interrupt handler");
	return AE_OK;
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
	print("%s\n", __func__);
	panic("%s", __func__);
	return AE_OK;
}

void
AcpiOsWaitEventsComplete (
	void)
{
	print("%s\n", __func__);
	panic("%s", __func__);
}

void
AcpiOsSleep (
    UINT64                  Milliseconds)
{
	print("%s\n", __func__);
	panic("%s", __func__);
}

void
AcpiOsStall(
    UINT32                  Microseconds)
{
	print("%s\n", __func__);
	panic("%s", __func__);
}

ACPI_THREAD_ID
AcpiOsGetThreadId (
    void)
{
	/* What to do here? ACPI won't take 0 for an answer.
	 * I guess tell it we're 1? What do we do? */
	return 1;
	//print("%s\n", __func__);
	Proc *up = externup();
	return up->pid;
}

ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
	print("%s\n", __func__);
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{
	/* Ooooooookay ... ACPI specifies the IO width in *bits*. */
	switch(Width) {
	case 4*8:
		*Value = inl(Address);
		break;
	case 2*8:
		*Value = ins(Address);
		break;
	case 1*8:
		*Value = inb(Address);
		break;
	default:
		panic("%s, bad width %d", __func__, Width);
		break;
	}
	print("%s 0x%x 0x%x\n", __func__, Address, *Value);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{
	switch(Width) {
	case 4*8:
		outl(Address, Value);
		break;
	case 2*8:
		outs(Address, Value);
		break;
	case 1*8:
		outb(Address, Value);
		break;
	default:
		panic("%s, bad width %d", __func__, Width);
		break;
	}
	print("%s 0x%x 0x%x\n", __func__, Address, Value);
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
	print("%s\n", __func__);
	panic("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width)
{
	print("%s\n", __func__);
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
	print("%s returns %p\n", __func__, rsd);
	return (ACPI_PHYSICAL_ADDRESS) PADDR(rsd);
}

ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{
	print("%s\n", __func__);
	*NewVal = nil;
	return AE_OK;
}

ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{
	print("%s\n", __func__);
	*NewTable = nil;
	return AE_OK;
}

ACPI_STATUS
AcpiOsPhysicalTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength)
{
	print("%s\n", __func__);
	*NewAddress = (ACPI_PHYSICAL_ADDRESS)nil;
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
	print("%s\n", __func__);
	panic("%s", __func__);
	return AE_OK;
}

