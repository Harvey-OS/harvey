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

BOOLEAN                        AcpiGbl_DebugTimeout = FALSE;
static uint32_t rsdp;
static char *name;
/* debug prints for this file. You can set this in gdb or at compile time. */
static int debug;

#if 0
static int
xisprint(int c)
{
	return (c >= 32 && c <= 126);
}

static void
hexdump(void *v, int length)
{
	int i;
	uint8_t *m = v;
	uintptr_t memory = (uintptr_t) v;
	int all_zero = 0;
	if (debug)
		fprint(2, "hexdump: %p, %u\n", v, length);
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
			if (debug)
				fprint(2, "%p:", (void *)(memory + i));
			for (j = 0; j < 16; j++)
				if (debug)
					fprint(2, " %02x", m[i + j]);
			if (debug)
				fprint(2, "  ");
			for (j = 0; j < 16; j++)
				if (debug)
					fprint(2, "%c", xisprint(m[i + j]) ? m[i + j] : '.');
			if (debug)
				fprint(2, "\n");
		} else if (all_zero == 2) {
			if (debug)
				fprint(2, "...\n");
		}
	}
}
#endif

/* try several variants */
static int
rawfd(void)
{
	int fd;
	if (name == nil) {
		name = "/dev/acpimem";
		if (debug)
			fprint(2, "Rawfd: open '%s'\n", name);
		fd = open(name, OREAD);
		if (fd > -1)
			return fd;
		name = "#P/acpimem";
		if (debug)
			fprint(2, "Rawfd: open '%s'\n", name);
		fd = open(name, OREAD);
		if (fd > -1)
			return fd;
	}
	return open(name, OREAD);
}

static int
tbdf(ACPI_PCI_ID * p)
{
	return (p->Bus << 8) | (p->Device << 3) | (p->Function);
}

int acpiio = -1;

uint32_t
inl(uint16_t addr)
{
	uint64_t off = addr;
	uint32_t l;
	if (pread(acpiio, &l, 4, off) < 4)
		print("inl(0x%x): %r\n", addr);
	return l;
}

uint16_t
ins(uint16_t addr)
{
	uint64_t off = addr;
	uint16_t w;
	if (pread(acpiio, &w, 2, off) < 2)
		print("ins(0x%x): %r\n", addr);
	return w;
}

uint8_t
inb(uint16_t addr)
{
	uint64_t off = addr;
	uint16_t b;
	if (pread(acpiio, &b, 1, off) < 1)
		print("inb(0x%x): %r\n", addr);
	return b;
}

void
outl(uint16_t addr, uint32_t val)
{
	uint64_t off = addr;
	if (pwrite(acpiio, &val, 4, off) < 4)
		print("outl(0x%x): %r\n", addr);
}

void
outs(uint16_t addr, uint16_t val)
{
	uint64_t off = addr;
	if (pwrite(acpiio, &val, 2, off) < 2)
		print("outs(0x%x): %r\n", addr);
}

void
outb(uint16_t addr, uint8_t val)
{
	uint64_t off = addr;
	if (pwrite(acpiio, &val, 1, off) < 1)
		print("outb(0x%x): %r\n", addr);
}

#define MKBUS(t,b,d,f)	(((t)<<24)|(((b)&0xFF)<<16)|(((d)&0x1F)<<11)|(((f)&0x07)<<8))
#define BUSFNO(tbdf)	(((tbdf)>>8)&0x07)
#define BUSDNO(tbdf)	(((tbdf)>>11)&0x1F)
#define BUSBNO(tbdf)	(((tbdf)>>16)&0xFF)
#define BUSTYPE(tbdf)	((tbdf)>>24)
#define BUSBDF(tbdf)	((tbdf)&0x00FFFF00)

ACPI_STATUS
AcpiOsReadPciConfiguration(ACPI_PCI_ID * PciId, UINT32 Reg, UINT64 * Value, UINT32 Width)
{
	ACPI_STATUS ret = AE_OK;
	uint64_t val;
	int amt = 0;
	static char path[128];
	snprint(path, sizeof(path), "/dev/pci/%d.%d.%draw", PciId->Bus,
			PciId->Device, PciId->Function);
	int fd = open(path, OREAD);
	if (fd < 0) {
		if (debug)
			print("%s: open %s: %r\n", __func__, path);
		return AE_IO_ERROR;
	}

	switch (Width) {
		case 32:
		case 16:
		case 8:
			amt = pread(fd, Value, Width / 8, Reg);
			if (amt < Width / 8)
				ret = AE_IO_ERROR;
			break;
		default:
			sysfatal("Can't read pci: bad width %d\n", Width);
	}
	close(fd);

	if (amt > 0)
		memmove(&val, Value, amt);
	if (debug)
		fprint(2, "pciread 0x%x 0x%x 0x%x 0x%x/%d 0x%x %d\n", PciId->Bus,
			   PciId->Device, PciId->Function, Reg, Width / 8, *Value, ret);

	return ret;

}

ACPI_STATUS
AcpiOsWritePciConfiguration(ACPI_PCI_ID * PciId, UINT32 Reg, UINT64 Value, UINT32 Width)
{
	ACPI_STATUS ret = AE_OK;
	static char path[128];
	snprint(path, sizeof(path), "/dev/pci/%d.%d.%draw", PciId->Bus,
			PciId->Device, PciId->Function);
	int fd = open(path, ORDWR);
	if (fd < 0) {
		print("%s: open %s: %r\n", __func__, path);
		return AE_IO_ERROR;
	}

	switch (Width) {
		case 32:
		case 16:
		case 8:
		{
			int amt = pwrite(fd, &Value, Width / 8, Reg);
			if (amt < Width / 8)
				ret = AE_IO_ERROR;
			break;
		}
		default:
			sysfatal("Can't read pci: bad width %d\n", Width);
	}
	close(fd);
	if (debug)
		fprint(2, "pciwrite 0x%x 0x%x 0x%x 0x%x/%d 0x%x %d\n", PciId->Bus,
			   PciId->Device, PciId->Function, Reg, Width / 8, Value, ret);
	return ret;
}

/*
 * Miscellaneous
 */
BOOLEAN
AcpiOsReadable(void *Pointer, ACPI_SIZE Length)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

BOOLEAN
AcpiOsWritable(void *Pointer, ACPI_SIZE Length)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

UINT64
AcpiOsGetTimer(void)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsSignal(UINT32 Function, void *Info)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

/* N.B. Only works for single thread! */
void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf(const char *Format, ...)
{
	static char buf[65536];
	va_list args;

	va_start(args, Format);
	vseprint(buf, &buf[65535], (char *)Format, args);
	va_end(args);
	if (debug)
		fprint(2, buf);
}

void
AcpiOsVprintf(const char *Format, va_list Args)
{
	/* This is a leaf function, and this function is required to implement
	 * the va_list argument. I couldn't find any other way to do this. */
	static char buf[1024];
	vseprint(buf, &buf[1023], (char *)Format, Args);
	if (debug)
		fprint(2, buf);
}

void
AcpiOsFree(void *Memory)
{
	//fprint(2,"%s\n", __func__);
	free(Memory);
}

void *
AcpiOsAllocate(ACPI_SIZE Size)
{
	//fprint(2,"%s\n", __func__);
	return malloc(Size);
}

void hexdump(void *v, int length);
void *
AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS Where, ACPI_SIZE Length)
{

	int fd, amt;
	fd = rawfd();
	if (debug)
		fprint(2, "%s %p %d\n", __func__, (void *)Where, Length);
	void *v = malloc(Length);
	if (!v) {
		sysfatal("%s: %r", __func__);
		return nil;
	}
	fd = rawfd();
	if (fd < 0) {
		if (debug)
			fprint(2, "%s: open %s: %r\n", __func__, name);
		return nil;
	}

	amt = pread(fd, v, Length, Where);
	close(fd);
	/* If we just do the amt < Length test, it will not work when
	 * amt is -1. Length is uint64_t. */
	if ((amt < 0) || (amt < Length)) {
		free(v);
		if (debug)
			fprint(2, "%s: read %s: %r\n", __func__, name);
		return nil;
	}
	//hexdump(v, Length);
	//hexdump(v, 36);
	return v;
}

void
AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Size)
{
	free(LogicalAddress);
	if (debug)
		fprint(2, "%s %p %d \n", __func__, LogicalAddress, Size);
}

ACPI_STATUS
AcpiOsGetPhysicalAddress(void *LogicalAddress,
						 ACPI_PHYSICAL_ADDRESS * PhysicalAddress)
{
	ACPI_PHYSICAL_ADDRESS ret = (uintptr_t) LogicalAddress;
	if (debug)
		fprint(2, "%s %p = %p", __func__, (void *)ret, LogicalAddress);
	*PhysicalAddress = ret;
	return AE_OK;
}

/* This is the single threaded version of
 * these functions. This is now NetBSD does it. */
ACPI_STATUS
AcpiOsCreateSemaphore(UINT32 MaxUnits,
					  UINT32 InitialUnits, ACPI_SEMAPHORE * OutHandle)
{
	//fprint(2,"%s\n", __func__);
	*OutHandle = (ACPI_SEMAPHORE) 1;
	return AE_OK;
}

ACPI_STATUS
AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
	//fprint(2,"%s\n", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
	//fprint(2,"%s\n", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
	//fprint(2,"%s\n", __func__);
	return AE_OK;
}

/* this is the single threaded case and as minix shows there is nothing to do. */
ACPI_STATUS
AcpiOsCreateLock(ACPI_SPINLOCK * OutHandle)
{
	//fprint(2,"%s\n", __func__);
	*OutHandle = nil;
	return AE_OK;
}

void
AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
	//fprint(2,"%s\n", __func__);
}

ACPI_CPU_FLAGS
AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
	//fprint(2,"%s\n", __func__);
	return 0;
}

void
AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
	//fprint(2,"%s\n", __func__);
}

struct handler {
	ACPI_OSD_HANDLER ServiceRoutine;
	void *Context;
};

static struct handler ihandler;

unsigned int
AcpiIntrWait(int afd, unsigned int *info)
{
	int amt;

	amt = read(afd, info, sizeof(*info));
	if (amt < 1) {
		print("ACPI intrwait: eof: %r");
	}
	if (amt < sizeof(info)) {
		print("ACPI intrwait: short read: %r");
	}
	return amt;
}

ACPI_STATUS AcpiRunInterrupt(void)
{
	return ihandler.ServiceRoutine(ihandler.Context);
}

ACPI_STATUS
AcpiOsInstallInterruptHandler(UINT32 InterruptNumber,
							  ACPI_OSD_HANDLER ServiceRoutine, void *Context)
{
	ihandler.ServiceRoutine = ServiceRoutine;
	ihandler.Context = Context;

	return AE_OK;
}

ACPI_STATUS
AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber,
							 ACPI_OSD_HANDLER ServiceRoutine)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	/* No need to even do this call. Since we're user mode, we exit. */
	return AE_OK;
}

void
AcpiOsWaitEventsComplete(void)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sysfatal("%s", __func__);
}

void
AcpiOsSleep(UINT64 Milliseconds)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sleep(Milliseconds);
}

void
AcpiOsStall(UINT32 Microseconds)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sleep(Microseconds / 1000 + 1);
}

ACPI_THREAD_ID
AcpiOsGetThreadId(void)
{
	static int pid = 2525;
	if (pid < 0)
		pid = getpid();
	return pid;
}

ACPI_STATUS
AcpiOsExecute(ACPI_EXECUTE_TYPE Type,
			  ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
	ACPI_STATUS ret = AE_OK;
	if (debug)
		fprint(2, "%s\n", __func__);
	Function(Context);

	return ret;
}

ACPI_STATUS
AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 * Value, UINT32 Width)
{
	switch (Width) {
		case 4 * 8:
			*Value = inl(Address);
			break;
		case 2 * 8:
			*Value = ins(Address);
			break;
		case 1 * 8:
			*Value = inb(Address);
			break;
		default:
			sysfatal("%s, bad width %d", __func__, Width);
			break;
	}
	if (debug)
		fprint(2, "%s 0x%x 0x%x\n", __func__, Address, *Value);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
	switch (Width) {
		case 4 * 8:
			outl(Address, Value);
			break;
		case 2 * 8:
			outs(Address, Value);
			break;
		case 1 * 8:
			outb(Address, Value);
			break;
		default:
			sysfatal("%s, bad width %d", __func__, Width);
			break;
	}
	if (debug)
		fprint(2, "%s 0x%x 0x%x\n", __func__, Address, Value);
	return AE_OK;
}

/*
 * Platform and hardware-independent physical memory interfaces
 */
ACPI_STATUS
AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 * Value, UINT32 Width)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

ACPI_STATUS
AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	sysfatal("%s", __func__);
	return AE_OK;
}

/* Just try to read the rsdp, if that fails, we're screwed anyway. */
ACPI_STATUS
AcpiOsInitialize(void)
{
	int fd, amt;
	if (debug)
		fprint(2, "%s\n", __func__);

	fd = rawfd();
	if (fd < 0) {
		if (debug)
			fprint(2, "%s: open %s: %r\n", __func__, name);
		return AE_ERROR;
	}

	amt = read(fd, &rsdp, sizeof(rsdp));
	if (amt < sizeof(rsdp)) {
		if (debug)
			fprint(2, "%s: read %s: %r\n", __func__, name);
		return AE_ERROR;
	}
	close(fd);

	acpiio = open("/dev/acpiio", ORDWR);
	if (acpiio < 0)
		sysfatal("acpiio: %r");
	return AE_OK;
}

/*
 * ACPI Table interfaces
 */
__attribute__ ((weak))ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer(void)
{
	if (debug)
		fprint(2, "%s returns %p\n", __func__, rsdp);
	return (ACPI_PHYSICAL_ADDRESS) rsdp;
}

ACPI_STATUS
AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES * InitVal,
						 ACPI_STRING * NewVal)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	*NewVal = nil;
	return AE_OK;
}

ACPI_STATUS
AcpiOsTableOverride(ACPI_TABLE_HEADER * ExistingTable,
					ACPI_TABLE_HEADER ** NewTable)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	*NewTable = nil;
	return AE_OK;
}

ACPI_STATUS
AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER * ExistingTable,
							ACPI_PHYSICAL_ADDRESS * NewAddress,
							UINT32 * NewTableLength)
{
	if (debug)
		fprint(2, "%s\n", __func__);
	*NewAddress = (ACPI_PHYSICAL_ADDRESS) nil;
	return AE_OK;
}

/*
 * Debug input
 */
ACPI_STATUS
AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 * BytesRead)
{
	int amt;
	if (debug)
		fprint(2, "%s\n", __func__);
	amt = read(0, Buffer, BufferLength);
	if (BytesRead)
		*BytesRead = amt;
	return AE_OK;
}

ACPI_STATUS
AcpiOsTerminate(void)
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
__attribute__ ((weak))void
AeDoObjectOverrides(void)
{
	if (debug)
		fprint(2, "%s\n", __func__);
}

/* Stubs for the disassembler */
 __attribute__ ((weak))
void
MpSaveGpioInfo(ACPI_PARSE_OBJECT * Op,
			   AML_RESOURCE * Resource,
			   UINT32 PinCount, UINT16 * PinList, char *DeviceName)
{
	if (debug)
		fprint(2, "%s\n", __func__);
}

__attribute__ ((weak))
void
MpSaveSerialInfo(ACPI_PARSE_OBJECT * Op,
				 AML_RESOURCE * Resource, char *DeviceName)
{
	if (debug)
		fprint(2, "%s\n", __func__);
}

int
AcpiOsWriteFile(ACPI_FILE File, void *Buffer, ACPI_SIZE Size, ACPI_SIZE Count)
{
	if (debug)
		fprint(2, "%s(%p, %p, %d, %d);\n", File, Buffer, Size, Count);
	return write(1, Buffer, Size * Count);
}


__attribute__ ((weak))
void hexdump(void *v, int length)
{
	int i;
	uint8_t *m = v;
	uintptr_t memory = (uintptr_t) v;
	int all_zero = 0;
	print("hexdump: %p, %u\n", v, length);
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
			print("%p:", (void *)(memory + i));
			for (j = 0; j < 16; j++)
				print(" %02x", m[i + j]);
			print("  ");
			for (j = 0; j < 16; j++)
				print("%c", isprint(m[i + j]) ? m[i + j] : '.');
			print("\n");
		} else if (all_zero == 2) {
			print("...\n");
		}
	}
}

