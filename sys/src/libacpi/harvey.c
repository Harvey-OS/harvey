/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */
#include <acpi.h>

#define MiB (1<<20)

static uint32_t rsdp;
static char *name;

#if 0
static int xisprint(int c)
{
	return (c >= 32 && c <= 126);
}

static void hexdump(void *v, int length)
{
	int i;
	uint8_t *m = v;
	uintptr_t memory = (uintptr_t) v;
	int all_zero = 0;
	fprint(2,"hexdump: %p, %u\n", v, length);
	for (i = 0; i < length; i += 16) {
		int j;

		all_zero++;
		for (j = 0; (j < 16) && (i + j < length); j++) {
			if (m[i + j] != 0) {
				all_zero = 0;
				break;
			}
		}

		if (all_zero < 2) {
			fprint(2,"%p:", (void *)(memory + i));
			for (j = 0; j < 16; j++)
				fprint(2," %02x", m[i + j]);
			fprint(2,"  ");
			for (j = 0; j < 16; j++)
				fprint(2,"%c", xisprint(m[i + j]) ? m[i + j] : '.');
			fprint(2,"\n");
		} else if (all_zero == 2) {
			fprint(2,"...\n");
		}
	}
}
#endif

/* try several variants */
static int rawfd(void){
	int fd;
	if (name == nil) {
		name = smprint("#%C/raw", L'α');
		fprint(2,"Rawfd: open '%s'\n", name);
		fd = open(name, OREAD);
		if (fd > -1)
			return fd;
		name = smprint("#%C/raw", 'Z');
		fprint(2,"Rawfd: open '%s'\n", name);
		fd = open(name, OREAD);
		if (fd > -1)
			return fd;
		/* try /dev paths here later. */
	}
	return open(name, OREAD);
}

static int tbdf(ACPI_PCI_ID *p)
{
	return (p->Bus << 8) | (p->Device << 3) | (p->Function);
}

int iol = -1, iow = -1, iob = -1;

uint32_t inl(uint16_t addr)
{
	uint64_t off = addr;
	uint32_t l;
	if (pread(iol, &l, 4, off) < 4)
		print("inl(0x%x): %r\n", addr);
	return l;
}

uint16_t ins(uint16_t addr)
{
	uint64_t off = addr;
	uint16_t w;
	if (pread(iow, &w, 2, off) < 2)
		print("ins(0x%x): %r\n", addr);
	return w;
}

uint8_t inb(uint16_t addr)
{
	uint64_t off = addr;
	uint16_t b;
	if (pread(iow, &b, 1, off) < 1)
		print("inb(0x%x): %r\n", addr);
	return b;
}

void outl(uint32_t val, uint16_t addr)
{
	uint64_t off = addr;
	if (pwrite(iol, &val, 4, off) < 4)
		print("outl(0x%x): %r\n", addr);
}

void outs(uint16_t val, uint16_t addr)
{
	uint64_t off = addr;
	if (pwrite(iow, &val, 2, off) < 2)
		print("outs(0x%x): %r\n", addr);
}

void outb(uint8_t val, uint16_t addr)
{
	uint64_t off = addr;
	if (pwrite(iob, &val, 1, off) < 1)
		print("outb(0x%x): %r\n", addr);
}

#define MKBUS(t,b,d,f)	(((t)<<24)|(((b)&0xFF)<<16)|(((d)&0x1F)<<11)|(((f)&0x07)<<8))
#define BUSFNO(tbdf)	(((tbdf)>>8)&0x07)
#define BUSDNO(tbdf)	(((tbdf)>>11)&0x1F)
#define BUSBNO(tbdf)	(((tbdf)>>16)&0xFF)
#define BUSTYPE(tbdf)	((tbdf)>>24)
#define BUSBDF(tbdf)	((tbdf)&0x00FFFF00)

ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width)
{
	ACPI_STATUS ret = AE_OK;
	uint64_t val;
	int amt;
	static char path[128];
	snprint(path, sizeof(path), "/dev/pci/%d.%d.%draw", PciId->Bus, PciId->Device, PciId->Function);
	int fd = open(path, OREAD);
	if (fd < 0) {
		print("%s: open %s: %r\n", __func__, path);
		return AE_IO_ERROR;
	}

	switch(Width) {
	case 32:
	case 16:
	case 8:
		amt = pread(fd, Value, Width/8, Reg);
		if (amt < Width/8)
			ret = AE_IO_ERROR;
		break;
	default:
		sysfatal("Can't read pci: bad width %d\n", Width);
	}
	close(fd);
	if (amt > 0)
		memmove(&val, Value, amt);
	fprint(2, "pciread 0x%x 0x%x 0x%x 0x%x/%d 0x%x %d\n", PciId->Bus, PciId->Device, PciId->Function, 
	       Reg, Width/8, *Value, ret);

	return ret;

}

ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  Value,
    UINT32                  Width)
{
	ACPI_STATUS ret = AE_OK;
	int amt;
	static char path[128];
	snprint(path, sizeof(path), "/dev/pci/%d.%d.%draw", PciId->Bus, PciId->Device, PciId->Function);
	int fd = open(path, ORDWR);
	if (fd < 0) {
		print("%s: open %s: %r\n", __func__, path);
		return AE_IO_ERROR;
	}

	switch(Width) {
	case 32:
	case 16:
	case 8:
		amt = pwrite(fd, &Value, Width/8, Reg);
		if (amt < Width/8)
			ret = AE_IO_ERROR;
		break;
	default:
		sysfatal("Can't read pci: bad width %d\n", Width);
	}
	close(fd);
	fprint(2, "pciwrite 0x%x 0x%x 0x%x 0x%x/%d 0x%x %d\n", PciId->Bus, PciId->Device, PciId->Function, 
	       Reg, Width/8, Value, ret);
	return ret;
}

/*
 * Miscellaneous
 */
BOOLEAN
AcpiOsReadable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
	fprint(2,"%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}


BOOLEAN
AcpiOsWritable (
    void                    *Pointer,
    ACPI_SIZE               Length)
{
	fprint(2,"%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}


UINT64
AcpiOsGetTimer (
    void)
{
	fprint(2,"%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}


ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
	fprint(2,"%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

/* N.B. Only works for single thread! */
void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Format,
    ...)
{
	static char buf[65536];
	va_list args;

	va_start(args, Format);
	vseprint(buf, &buf[65535], (char *)Format, args);
	va_end(args);
	fprint(2,buf);
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
	fprint(2,buf);
}

void
AcpiOsFree (
    void *                  Memory)
{
	//fprint(2,"%s\n", __func__);
	free(Memory);
}

void *
AcpiOsAllocate (
    ACPI_SIZE               Size)
{
	//fprint(2,"%s\n", __func__);
	return malloc(Size);
}

void *
AcpiOsMapMemory (
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{

	int fd, amt;
	fd = rawfd();
	fprint(2,"%s %p %d\n", __func__, (void *)Where, Length);
	void *v = malloc(Length);
	if (!v){
		sysfatal("%s: %r", __func__);
		return nil;
	}
	fd = rawfd();
	if (fd < 0) {
		fprint(2,"%s: open %s: %r\n", __func__, name);
		return nil;
	}
	
	amt = pread(fd, v, Length, Where);
	close(fd);
	/* If we just do the amt < Length test, it will not work when
	 * amt is -1. Length is uint64_t. */
	if ((amt < 0) || (amt < Length)){
		free(v);
		fprint(2,"%s: read %s: %r\n", __func__, name);
		return nil;
	}
	//hexdump(v, Length);
	return v;
}

void
AcpiOsUnmapMemory (
    void                    *LogicalAddress,
    ACPI_SIZE               Size)
{
	free(LogicalAddress);
	fprint(2,"%s %p %d \n", __func__, LogicalAddress, Size);
}

ACPI_STATUS
AcpiOsGetPhysicalAddress (
    void                    *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS   *PhysicalAddress)
{
	ACPI_PHYSICAL_ADDRESS ret = (uintptr_t)LogicalAddress;
	fprint(2,"%s %p = %p", __func__, (void *)ret, LogicalAddress);
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
	//fprint(2,"%s\n", __func__);
	*OutHandle = (ACPI_SEMAPHORE) 1;
	return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_SEMAPHORE          Handle)
{
	//fprint(2,"%s\n", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units,
    UINT16                  Timeout)
{
	//fprint(2,"%s\n", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units)
{
	//fprint(2,"%s\n", __func__);
	return AE_OK;
}

/* this is the single threaded case and as minix shows there is nothing to do. */
ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{
	//fprint(2,"%s\n", __func__);
	*OutHandle = nil;
	return AE_OK;
}

void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
	//fprint(2,"%s\n", __func__);
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_SPINLOCK           Handle)
{
	//fprint(2,"%s\n", __func__);
	return 0;
}

void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
	//fprint(2,"%s\n", __func__);
}

struct handler {
	ACPI_OSD_HANDLER        ServiceRoutine;
	void                    *Context;
};

#if 0
/* The ACPI interrupt signature and the Harvey one are not compatible. So, we pass an arg to
 * intrenable that can in turn be used to this function to call the ACPI handler. */
static void acpihandler(void*_, void *arg)
{
	struct handler *h = arg;
	h->ServiceRoutine(h->Context);
}
#endif

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
	fprint(2,"NOT DOING %s %d %p %p \n", __func__, InterruptNumber, ServiceRoutine, Context);
	/* once enabled, can't be disabled; ignore the return value unless it's nil. */
	//intrenable(InterruptNumber, acpihandler, h, 0x5, "ACPI interrupt handler");
	return AE_OK;
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
	fprint(2,"%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

void
AcpiOsWaitEventsComplete (
	void)
{
	fprint(2,"%s\n", __func__);
	sysfatal("%s", __func__);
}

void
AcpiOsSleep (
    UINT64                  Milliseconds)
{
	fprint(2,"%s\n", __func__);
	sleep(Milliseconds);
}

void
AcpiOsStall(
    UINT32                  Microseconds)
{
	fprint(2,"%s\n", __func__);
	sleep(Microseconds/1000+1);
}

ACPI_THREAD_ID
AcpiOsGetThreadId (
    void)
{
	static int pid = 2525;
	if (pid < 0)
		pid = getpid();
	return pid;
}

ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
	ACPI_STATUS ret = AE_OK;
	fprint(2,"%s\n", __func__);
	Function(Context);
	
	return ret;
}

ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{
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
		sysfatal("%s, bad width %d", __func__, Width);
		break;
	}
	fprint(2,"%s 0x%x 0x%x\n", __func__, Address, *Value);
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
		sysfatal("%s, bad width %d", __func__, Width);
		break;
	}
	fprint(2,"%s 0x%x 0x%x\n", __func__, Address, Value);
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
	fprint(2,"%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width)
{
	fprint(2,"%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}
/* Just try to read the rsdp, if that fails, we're screwed anyway. */
ACPI_STATUS
AcpiOsInitialize(void)
{
	int fd, amt;
	fprint(2,"%s\n", __func__);

	fd = rawfd();
	if (fd < 0) {
		fprint(2,"%s: open %s: %r\n", __func__, name);
		return AE_ERROR;
	}
	
	amt = read(fd, &rsdp, sizeof(rsdp));
	if (amt < sizeof(rsdp)) {
		fprint(2,"%s: read %s: %r\n", __func__, name);
		return AE_ERROR;
	}
	close(fd);

	iol = open("/dev/iol", ORDWR);
	if (iol < 0)
		sysfatal("iol: %r");
	iow = open("/dev/iow", ORDWR);
	if (iow < 0)
		sysfatal("iow: %r");
	iob = open("/dev/iob", ORDWR);
	if (iob < 0)
		sysfatal("iob: %r");
	return AE_OK;
}
/*
 * ACPI Table interfaces
 */
ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
    void)
{
	fprint(2,"%s returns %p\n", __func__, rsdp);
	return (ACPI_PHYSICAL_ADDRESS) rsdp;
}

ACPI_STATUS
AcpiOsPredefinedOverride (
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{
	fprint(2,"%s\n", __func__);
	*NewVal = nil;
	return AE_OK;
}

ACPI_STATUS
AcpiOsTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{
	fprint(2,"%s\n", __func__);
	*NewTable = nil;
	return AE_OK;
}

ACPI_STATUS
AcpiOsPhysicalTableOverride (
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength)
{
	fprint(2,"%s\n", __func__);
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
	fprint(2,"%s\n", __func__);
	*BytesRead = read(0, Buffer, BufferLength);
	return AE_OK;
}

ACPI_STATUS
AcpiOsTerminate (
    void)
{
	sysfatal("%s\n", __func__);
	return AE_OK;
}

/* Another acpica failure of vision: code in libraries that depends on functions defined only
 * in the compiler or other programs. Really! What to do?
 * this is how the acpi tools do it. Save we add a print so we know the function is called, duh! */
typedef void *ACPI_PARSE_OBJECT;
typedef void *AML_RESOURCE;
/* this is from acpiexec. */

/* For AcpiExec only */
void
AeDoObjectOverrides (
    void)
{
	fprint(2,"%s\n", __func__);
}

/* Stubs for the disassembler */

void
MpSaveGpioInfo (
    ACPI_PARSE_OBJECT       *Op,
    AML_RESOURCE            *Resource,
    UINT32                  PinCount,
    UINT16                  *PinList,
    char                    *DeviceName)
{
	fprint(2,"%s\n", __func__);
}

void
MpSaveSerialInfo (
    ACPI_PARSE_OBJECT       *Op,
    AML_RESOURCE            *Resource,
    char                    *DeviceName)
{
	fprint(2,"%s\n", __func__);
}

int
AcpiOsWriteFile (
    ACPI_FILE               File,
    void                    *Buffer,
    ACPI_SIZE               Size,
    ACPI_SIZE               Count)
{
	fprint(2,"%s(%p, %p, %d, %d);\n", File, Buffer, Size, Count);
	return write(1, Buffer, Size*Count);
}
