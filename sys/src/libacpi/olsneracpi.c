/*
Copyright (c) 2014 Simon Brenner

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include "acpi.h"
#include "accommon.h"
#include "acapps.h"
#include "actables.h"
#include "acutils.h"
#include <errno.h>

#define _COMPONENT          ACPI_UTILITIES
ACPI_MODULE_NAME    ("harvey")

/******************************************************************************
 *
 * Example ACPICA handler and handler installation
 *
 *****************************************************************************/
#if 0
static void NotifyHandler (
    ACPI_HANDLE                 Device,
    UINT32                      Value,
    void                        *Context)
{

    ACPI_INFO ((AE_INFO, "Received a notify 0x%x (device %p, context %p)", Value, Device, Context));
}

static ACPI_STATUS InstallHandlers (void)
{
    ACPI_STATUS             Status;


    /* Install global notify handler */

    Status = AcpiInstallNotifyHandler (ACPI_ROOT_OBJECT, ACPI_SYSTEM_NOTIFY,
                                        NotifyHandler, NULL);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing Notify handler"));
        return (Status);
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * Example ACPICA initialization code. This shows a full initialization with
 * no early ACPI table access.
 *
 *****************************************************************************/

static ACPI_STATUS InitializeFullAcpi (void)
{
    ACPI_STATUS             Status;


    /* Initialize the ACPICA subsystem */

    Status = AcpiInitializeSubsystem ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing ACPICA"));
        return (Status);
    }

    /* Initialize the ACPICA Table Manager and get all ACPI tables */

    Status = AcpiInitializeTables (NULL, 0, FALSE);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing Table Manager"));
        return (Status);
    }

    /* Create the ACPI namespace from ACPI tables */

    Status = AcpiLoadTables ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While loading ACPI tables"));
        return (Status);
    }

    /* Install local handlers */

    Status = InstallHandlers ();
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While installing handlers"));
        return (Status);
    }

    /* Initialize the ACPI hardware */

    Status = AcpiEnableSubsystem (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While enabling ACPICA"));
        return (Status);
    }

    /* Complete the ACPI namespace object initialization */

    Status = AcpiInitializeObjects (ACPI_FULL_INITIALIZATION);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While initializing ACPICA objects"));
        return (Status);
    }

    return (AE_OK);
}


#endif
ACPI_STATUS ExecuteOSI(int pic_mode)
{
    ACPI_STATUS             Status;
    ACPI_OBJECT_LIST        ArgList;
    ACPI_OBJECT             Arg[1];
    ACPI_BUFFER             ReturnValue;


    /* Setup input argument */

    ArgList.Count = 1;
    ArgList.Pointer = Arg;

    Arg[0].Type = ACPI_TYPE_INTEGER;
    Arg[0].Integer.Value = pic_mode;

    ACPI_INFO ((AE_INFO, "Executing _PIC(%ld)", Arg[0].Integer.Value));

    /* Ask ACPICA to allocate space for the return object */

    ReturnValue.Length = ACPI_ALLOCATE_BUFFER;

    Status = AcpiEvaluateObject (NULL, "\\_PIC", &ArgList, &ReturnValue);
	////ACPI_FREE_BUFFER(ReturnValue);
	if (Status == AE_NOT_FOUND)
	{
		printf("\\_PIC was not found. Assuming that's ok.\n");
		return AE_OK;
	}
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status, "While executing _PIC"));
        return Status;
    }

    printf("_PIC returned.\n");
    return Status;
}

#define CHECK_STATUS(fmt, ...) do { if (ACPI_FAILURE(status)) { \
	printf("ACPI failed (%d): " fmt "\n", status, ## __VA_ARGS__); \
	goto failed; \
	} } while(0)

#pragma pack(1)
typedef union acpi_apic_struct
{
	struct {
		UINT8 Type;
		UINT8 Length;
	};
	ACPI_MADT_LOCAL_APIC LocalApic;
	ACPI_MADT_IO_APIC IOApic;
	ACPI_MADT_INTERRUPT_OVERRIDE InterruptOverride;
	ACPI_MADT_LOCAL_APIC_NMI LocalApicNMI;
} ACPI_APIC_STRUCT;
#pragma pack()

typedef struct {
	int irqs[8];
} PRT;

PRT prts[256];

ACPI_STATUS FindIOAPICs(int *pic_mode) {
	ACPI_TABLE_MADT* table = NULL;
	ACPI_STATUS status = AcpiGetTable("APIC", 0, (ACPI_TABLE_HEADER**)&table);
	CHECK_STATUS("AcpiGetTable");

	char* endOfTable = (char*)table + table->Header.Length;
	char* p = (char*)(table + 1);
	int n = 0;
	while (p < endOfTable) {
		ACPI_APIC_STRUCT* apic = (ACPI_APIC_STRUCT*)p;
		p += apic->Length;
		n++;
		switch (apic->Type)
		{
		case ACPI_MADT_TYPE_IO_APIC:
			printf("Found I/O APIC. ID %#x Addr %#x GSI base %#x.\n",
				(int)apic->IOApic.Id,
				apic->IOApic.Address,
				apic->IOApic.GlobalIrqBase);
			//AddIOAPIC(&apic->IOApic);
			*pic_mode = 1;
			break;
		}
	}
	if (*pic_mode)
	{
		printf("I/O APICs found, setting APIC mode\n");
	}
	else
	{
		printf("I/O APICs NOT found, setting PIC mode\n");
		//AddPIC();
	}
failed:
	return AE_OK;
}

static ACPI_STATUS PrintAPICTable(void) {
	static const char *polarities[] = {
		"Bus-Conformant",
		"Active-High",
		"Reserved",
		"Active-Low"
	};
	static const char *triggerings[] = {
		"Bus-Conformant",
		"Edge-Triggered",
		"Reserved",
		"Level-Triggered"
	};

	ACPI_TABLE_MADT* table = NULL;
	ACPI_STATUS status = AcpiGetTable("APIC", 0, (ACPI_TABLE_HEADER**)&table);
	CHECK_STATUS("AcpiGetTable");

	printf("Found APIC table: %p\n", table);
	printf("Address of Local APIC: %#x\n", table->Address);
	printf("Flags: %#x\n", table->Flags);
	char* endOfTable = (char*)table + table->Header.Length;
	char* p = (char*)(table + 1);
	int n = 0;
	while (p < endOfTable) {
		ACPI_APIC_STRUCT* apic = (ACPI_APIC_STRUCT*)p;
		p += apic->Length;
		n++;
		switch (apic->Type)
		{
		case ACPI_MADT_TYPE_LOCAL_APIC:
			printf("%d: Local APIC. Processor ID %#x APIC ID %#x En=%d (%#x)\n", n,
				(int)apic->LocalApic.ProcessorId,
				(int)apic->LocalApic.Id,
				apic->LocalApic.LapicFlags & 1,
				apic->LocalApic.LapicFlags);
			break;
		case ACPI_MADT_TYPE_IO_APIC:
			printf("%d: I/O APIC. ID %#x Addr %#x GSI base %#x\n", n,
				(int)apic->IOApic.Id,
				apic->IOApic.Address,
				apic->IOApic.GlobalIrqBase);
			break;
		case ACPI_MADT_TYPE_INTERRUPT_OVERRIDE:
		{
			UINT32 flags = apic->InterruptOverride.IntiFlags;
			printf("%d: Interrupt Override. Source %#x GSI %#x Pol=%s Trigger=%s\n", n,
				apic->InterruptOverride.SourceIrq,
				apic->InterruptOverride.GlobalIrq,
				polarities[flags & 3], triggerings[(flags >> 2) & 3]);
			break;
		}
		case ACPI_MADT_TYPE_LOCAL_APIC_NMI:
		{
			UINT32 flags = apic->InterruptOverride.IntiFlags;
			printf("%d: Local APIC NMI. Processor ID %#x Pol=%s Trigger=%s LINT# %#x\n", n,
				apic->LocalApicNMI.ProcessorId,
				polarities[flags & 3], triggerings[(flags >> 2) & 3],
				apic->LocalApicNMI.Lint);
			break;
		}
		default:
			printf("%d: Unknown APIC type %d\n", n, apic->Type);
			break;
		}
	}

failed:
	return status;
}

ACPI_STATUS PrintAcpiDevice(ACPI_HANDLE Device)
{
	ACPI_DEVICE_INFO* info = NULL;
	ACPI_STATUS status = AcpiGetObjectInfo(Device, &info);
	if (ACPI_SUCCESS(status)) {
		printf("Device %p type %#x\n", Device, info->Type);
	}

	//ACPI_FREE(info);
	return_ACPI_STATUS(status);
}


static ACPI_STATUS PrintDeviceCallback(ACPI_HANDLE Device, UINT32 Depth, void *Context, void** ReturnValue)
{
	return PrintAcpiDevice(Device);
}

// PNP0C0F = PCI Interrupt Link Device
// PNP0A03 = PCI Root Bridge
ACPI_STATUS PrintDevices(void) {
	ACPI_STATUS status = AE_OK;

	printf("Searching for PNP0A03\n");
	status = AcpiGetDevices("PNP0A03", PrintDeviceCallback, NULL, NULL);
	CHECK_STATUS("AcpiGetDevices PNP0A03");

	printf("Searching for PNP0C0F\n");
	status = AcpiGetDevices("PNP0C0F", PrintDeviceCallback, NULL, NULL);
	CHECK_STATUS("AcpiGetDevices PNP0C0F");

failed:
	return_ACPI_STATUS(status);
}

typedef struct IRQRouteData
{
	ACPI_PCI_ID pci;
	unsigned pin;
	int8_t gsi;
	// triggering: 1 = edge triggered, 0 = level
	int8_t triggering;
	// polarity: 1 = active-low, 0 = active-high
	int8_t polarity;
	BOOLEAN found;
} IRQRouteData;

static void ResetBuffer(ACPI_BUFFER* buffer) {
	////ACPI_FREE_BUFFER((*buffer));
	buffer->Pointer = 0;
	buffer->Length = ACPI_ALLOCATE_BUFFER;
}

static ACPI_STATUS RouteIRQLinkDevice(ACPI_HANDLE Device, ACPI_PCI_ROUTING_TABLE* found, IRQRouteData* data) {
	ACPI_STATUS status = AE_OK;
	ACPI_HANDLE LinkDevice = NULL;
	ACPI_BUFFER buffer = {0, NULL};

	printf("Routing IRQ Link device %s\n", found->Source);
	status = AcpiGetHandle(Device, found->Source, &LinkDevice);
	CHECK_STATUS("AcpiGetHandle %s", found->Source);

	ResetBuffer(&buffer);
	status = AcpiGetCurrentResources(LinkDevice, &buffer);
	CHECK_STATUS("AcpiGetCurrentResources");
	printf("Got %lu bytes of current resources\n", buffer.Length);
	ACPI_RESOURCE* resource = (ACPI_RESOURCE*)buffer.Pointer;
	switch (resource->Type) {
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
		// The interrupt count must be 1 when returned from _CRS, supposedly.
		// I think the "possible resource setting" may list several.
		printf("Extended IRQ: %d interrupts, first one %#x. %s triggered, Active-%s.\n",
			resource->Data.ExtendedIrq.InterruptCount,
			resource->Data.ExtendedIrq.Interrupts[0],
			resource->Data.ExtendedIrq.Triggering ? "Edge" : "Level",
			resource->Data.ExtendedIrq.Polarity ? "Low" : "High");
		data->gsi = resource->Data.ExtendedIrq.Interrupts[0];
		data->triggering = resource->Data.ExtendedIrq.Triggering;
		data->polarity = resource->Data.ExtendedIrq.Polarity;
		break;
	case ACPI_RESOURCE_TYPE_IRQ:
		printf("IRQ: %d interrupts, first one %#x.\n",
			resource->Data.Irq.InterruptCount,
			resource->Data.Irq.Interrupts[0]);
		data->gsi = resource->Data.Irq.Interrupts[0];
		// PIC interrupts can't be set up for specific polarity and triggering,
		// I think.
		break;
	default:
		printf("RouteIRQLinkDevice: unknown resource type %d\n", resource->Type);
		status = AE_BAD_DATA;
		goto failed;
	}
	status = AcpiSetCurrentResources(LinkDevice, &buffer);
	CHECK_STATUS("AcpiSetCurrentResources");

failed:
	//ACPI_FREE_BUFFER(buffer);
	return_ACPI_STATUS(status);
}

static int readfile(char *path, char *buf, size_t buflen)
{
	int fd = open(path, OREAD);
	int amt;
	if (fd < 0)
		return fd;
	amt = read(fd, buf, buflen);
	(void)close(fd);
	return amt;
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

	print("apic %d, pin 0x%x\n", 1, i->Interrupts[0]);
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
			      t->Pin, t->Address, t->SourceIndex);
			int adr = t->Address>>16;
			prts[adr].irqs[t->Pin] = t->SourceIndex;
			p += t->Length;
		}
	}
	as = AcpiWalkResources(Object, "_CRS", resource, nil);
	print("Walk resources: as is %d\n", as);
	return 0;
}

int GetPRT(void)
{
	ACPI_BUFFER out;
	static char path[128];
	static char buf[1024];
	ACPI_STATUS as;
	/* There are, potentially, 256 levels. Unlikely but ... */
	char *f[255];
	int nf;
	char *bridges;
	out.Length = ACPI_ALLOCATE_BUFFER;
	out.Pointer = nil;

	/* this is the part I don't know well. for now, let's try
	 * getting the _PRT for everything in the path from ROOT.
	 */
	snprint(path, sizeof(path), "\\_SB.PCI0._PRT");
	print("OK, try to evaluate %s\n", path);
	as = AcpiEvaluateObject(ACPI_ROOT_OBJECT, path, NULL, &out);
	print("returns %d\n", as);
	if (!ACPI_SUCCESS(as))
		return as;
	print("------>GOT the PRT: for 0\n");
	print("Length is %u ptr is %p\n", out.Length, out.Pointer);

	/* now get all PRTs for all devices. */
	as = AcpiGetDevices (nil, device, nil, nil);
	print("acpigetdevices %d\n", as);
	return AE_OK;
}
static int mapit(ACPI_HANDLE dev, IRQRouteData*d, int r)
{
	static char path[128];
	static char buf[1024];
	int BridgeDevice;
	int gsi;
	ACPI_STATUS as;
	ACPI_STATUS status;

	/* There are, potentially, 256 levels. Unlikely but ... */
	char *f[255];
	int nf;
	char *bridges;
	/* try namespace, then device. */
	snprint(path, sizeof(path), "/dev/pci/%d.%d.0ctl", d->pci.Bus, d->pci.Device);
	if (readfile(path, buf, sizeof(buf)) < 0) {
		snprint(path, sizeof(path), "#$/pci/%d.%d.0ctl", d->pci.Bus, d->pci.Device);
		if (readfile(path, buf, sizeof(buf)) < 0)
			return -1;
	}
	nf = tokenize(buf, f, 5);
	if (nf < 5)
		return -1;
	print("Path is %s\n", f[3]);
	bridges = f[3];
	nf = gettokens(bridges, f, nelem(f), "/");
	print("Path as %d componenents\n", nf);
	if (nf < 2)
		return 0;
	/* OK, FOR NOW, we're just doing one level. */
	if (nf > 2)
		sysfatal("PANIC: Can't do more than one level of bridge");
	/* and, further, we take the root bridge as kind of a given. So we only care about
	 * the bridge in f[1].
	 */
	char *bridgeno = f[1];
	nf = gettokens(bridgeno, f, 3, ".");
	if (nf < 3)
		sysfatal("BOTCH! the bridge device requires 3 fields, only had %d\n", nf);
	BridgeDevice = strtoul(f[1], 0, 0);
	print("BridgeDevice is 0x%x, pin is %d\n", BridgeDevice, d->pin);

	/* and the swizzling is fixed, per the PCI standard. take the low 2 bits (for now)
	 * of device #, add pin, mod3, that's it. */
	/* confusing: pin numbers are 1-relative. But for this to work they need to be
	 * zero-relative. Sorry. */
	int pin = (d->pin - 1 + d->pci.Device) % 3;
	gsi = prts[BridgeDevice].irqs[pin];
	print("GSI is 0x%x\n", gsi);
	print("echo -n %d %d %d %d 0x%x > /dev/irqmap\n", 0, d->pci.Bus, d->pci.Device, 0, gsi);
	
	return 0;

}
static ACPI_STATUS RouteIRQCallback(ACPI_HANDLE Device, UINT32 Depth, void *Context, void** ReturnValue)
{
	IRQRouteData* data = (IRQRouteData*)Context;
	ACPI_STATUS status = AE_OK;
	ACPI_RESOURCE* resource = NULL;
	ACPI_BUFFER buffer = {0, NULL};
	buffer.Length = ACPI_ALLOCATE_BUFFER;
	ACPI_PCI_ROUTING_TABLE* found = NULL;

	ACPI_DEVICE_INFO* info = NULL;
	status = AcpiGetObjectInfo(Device, &info);
	CHECK_STATUS("AcpiGetObjectInfo");

	if (!(info->Flags & ACPI_PCI_ROOT_BRIDGE)) {
		printf("RouteIRQCallback: not a root bridge.\n");
		goto failed;
	}

	printf("RouteIRQ: Root bridge with address %#x:\n", info->Address);
	int rootBus = -1;

	// Get _CRS, parse, check if the bus number range includes the one in
	// data->pci.Bus - then we've found the right *root* PCI bridge.
	// Though this might actually be a lot more complicated if we allow for
	// multiple root pci bridges.
	status = AcpiGetCurrentResources(Device, &buffer);
	CHECK_STATUS("AcpiGetCurrentResources");
	printf("Got %lu bytes of current resources\n", buffer.Length);
	status = AcpiBufferToResource(buffer.Pointer, buffer.Length, &resource);
	resource = (ACPI_RESOURCE*)buffer.Pointer;
	printf("Got resources %p (status %#x)\n", resource, status);
	//CHECK_STATUS();
	while (resource->Type != ACPI_RESOURCE_TYPE_END_TAG) {
		printf("Got resource type %d\n", resource->Type);
		ACPI_RESOURCE_ADDRESS64 addr64;
		ACPI_STATUS status = AcpiResourceToAddress64(resource, &addr64);
		printf("Processed and got type\n",  addr64.ResourceType);
		if (status == AE_OK && addr64.ResourceType == ACPI_BUS_NUMBER_RANGE)
		{
			printf("RouteIRQ: Root bridge bus range %#x..%#x\n",
			       addr64.Address.Minimum,
					addr64.Address.Maximum);
			if (data->pci.Bus < addr64.Address.Minimum ||
			    data->pci.Bus > addr64.Address.Maximum)
			{
				// This is not the root bridge we're looking for...
				goto failed;
			}
			rootBus = addr64.Address.Minimum;
			break;
		}
		resource = ACPI_NEXT_RESOURCE(resource);
	}
	// dunno!
	if (rootBus == -1)
	{
		printf("Couldn't figure out the bus number for root bridge %#x\n",
				info->Address);
		goto failed;
	}

	/* this is the easy case, and the most common: it's all on Bus 0
	 * and it Just Works. */
	if (rootBus == data->pci.Bus) {
		ResetBuffer(&buffer);
		status = AcpiGetIrqRoutingTable(Device, &buffer);
		CHECK_STATUS("AcpiGetIrqRoutingTable");
		printf("Got %u bytes of IRQ routing table\n", buffer.Length);
		ACPI_PCI_ROUTING_TABLE* route = buffer.Pointer;
		ACPI_PCI_ROUTING_TABLE* const end = buffer.Pointer + buffer.Length;
		printf("Routing table: %p..%p\n", route, end);
		UINT64 pciAddr = data->pci.Device;
		print("pciAddr: 0x%x\n", pciAddr);
		while (route < end && route->Length) {
			print("Route: %p, Address: 0x%08x, Pin: %d\n", route, route->Address, route->Pin);
			if ((route->Address >> 16) == pciAddr && route->Pin == data->pin) {
				print("FOUND!\n");
				found = route;
				break;
			}
			print("Route Length 0x%x\n", route->Length);
			route = (ACPI_PCI_ROUTING_TABLE*)((char*)route + route->Length);
		}
		if (!found) {
			print("NOT FOUND! FAIL!\n");
			goto failed;
		}
		
		printf("RouteIRQ: %02x:%02x.%d pin %d -> %c%c%c%c:%d\n",
		       data->pci.Bus, data->pci.Device, data->pci.Function,
		       found->Pin,
		       found->Source[0],
		       found->Source[1],
		       found->Source[2],
		       found->Source[3],
		       found->SourceIndex);
		
		if (found->Source[0]) {
			status = RouteIRQLinkDevice(Device, found, data);
			printf("status %#x irq %#x\n", status, data->gsi);
			CHECK_STATUS("RouteIRQLinkDevice");
		} else {
			printf("found->Source[0] is zero since it's PIC 1\n");
			data->gsi = found->SourceIndex;
		}
		data->found = TRUE;
		status = AE_CTRL_TERMINATE;
		status = AE_OK;
		return status;
	}
	// This requires us to walk the chain of pci-pci bridges between the
	// root bridge and the device. Unimplemented.
	printf("Unimplemented! Device on bus %#x, but root is %#x\n",
	       data->pci.Bus, rootBus);
	if (mapit(Device, data, rootBus))
		goto failed;



failed:
	//ACPI_FREE_BUFFER(buffer);
	ACPI_FREE(info);
	print("MAPIT: status %d\n", status);
	return_ACPI_STATUS(status);
}

ACPI_STATUS RouteIRQ(ACPI_PCI_ID* device, int pin, int* irq) {
	IRQRouteData data = { *device, pin, 0, 0, 0, FALSE };
	ACPI_STATUS status = AE_OK;

	status = AcpiGetDevices("PNP0A03", RouteIRQCallback, &data, NULL);
	if (status == AE_OK)
	{
printf("Data f %d 0x%x\n", data.found, data.pci);
		if (data.found)
		{
			*irq = data.gsi
				| (data.triggering ? 0x100 : 0)
				| (data.polarity ? 0x200 : 0);
		}
		else
		{
			status = AE_NOT_FOUND;
		}
	}
	return_ACPI_STATUS(status);
}
#if 0
// reserve some virtual memory space (never touched) to keep track pci device
// handles.
static const char pci_device_handles[65536] PLACEHOLDER_SECTION;

static void MsgFindPci(uintptr_t rcpt, uintptr_t arg)
{
	ACPI_PCI_ID temp = { 0, 0, 0, 0 };
	u16 vendor = arg >> 16;
	u16 device = arg;
	uintptr_t addr = -1;
	printf("acpica: find pci %#x:%#x.\n", vendor, device);
	ACPI_STATUS status = FindPCIDevByVendor(vendor, device, &temp);
	if (ACPI_SUCCESS(status)) {
		addr = temp.Bus << 16 | temp.Device << 3 | temp.Function;
	}
	send1(MSG_ACPI_FIND_PCI, rcpt, addr);
}

static void MsgClaimPci(uintptr_t rcpt, uintptr_t addr, uintptr_t pins)
{
	addr &= 0xffff;
	ACPI_PCI_ID id = { 0, (addr >> 8) & 0xff, (addr >> 3) & 31, addr & 7 };
	printf("acpica: claim pci %02x:%02x.%x\n", id.Bus, id.Device, id.Function);

	// Set up whatever stuff to track PCI device drivers in general

	int irqs[4] = {0};
	for (int pin = 0; pin < 4; pin++) {
		if (!(pins & (1 << pin))) continue;

		ACPI_STATUS status = RouteIRQ(&id, 0, &irqs[pin]);
		CHECK_STATUS("RouteIRQ");
		printf("acpica: %02x:%02x.%x pin %d routed to IRQ %#x\n",
			id.Bus, id.Device, id.Function,
			pin, irqs[pin]);
	}

	if (pins & ACPI_PCI_CLAIM_MASTER) {
		u64 value;
		AcpiOsReadPciConfiguration(&id, PCI_COMMAND, &value, 16);
		if (!(value & PCI_COMMAND_MASTER)) {
			value |= PCI_COMMAND_MASTER;
			AcpiOsWritePciConfiguration(&id, PCI_COMMAND, value, 16);
		}
	}

	pins = (u64)irqs[3] << 48 | (u64)irqs[2] << 32 | irqs[1] << 16 | irqs[0];

	send2(MSG_ACPI_CLAIM_PCI, rcpt, addr, pins);
	hmod(rcpt, (uintptr_t)pci_device_handles + addr, 0);
	return;

failed:
	send2(MSG_ACPI_CLAIM_PCI, rcpt, 0, 0);
}

static size_t debugger_buffer_pos = 0;

static void debugger_pre_cmd(void) {
	debugger_buffer_pos = 0;
	AcpiGbl_MethodExecuting = FALSE;
	AcpiGbl_StepToNextCall = FALSE;
	AcpiDbSetOutputDestination(ACPI_DB_CONSOLE_OUTPUT);
}

void GlobalEventHandler(UINT32 EventType, ACPI_HANDLE Device,
		UINT32 EventNumber, void *Context) {
	if (EventType == ACPI_EVENT_TYPE_FIXED &&
		EventNumber == ACPI_EVENT_POWER_BUTTON) {

		printf("POWER BUTTON! Shutting down.\n");

		AcpiEnterSleepStatePrep(ACPI_STATE_S5);
		AcpiEnterSleepState(ACPI_STATE_S5);
	}
}
#endif
#if 0
void start() {
	ACPI_STATUS status = AE_OK;

	printf("ACPICA: start\n");

	// NB! Must be at least as large as physical memory - the ACPI tables could
	// be anywhere. (Could be handled by AcpiOsMapMemory though.)
	map(0, MAP_PHYS | PROT_READ | PROT_WRITE | PROT_NO_CACHE,
		(void*)ACPI_PHYS_BASE, 0, USER_MAP_MAX - ACPI_PHYS_BASE);
	char* p = ((char*)ACPI_PHYS_BASE) + 0x100000;
	printf("Testing physical memory access: %p (0x100000): %x\n", p, *(u32*)p);

	__default_section_init();
	init_heap();

    ACPI_DEBUG_INITIALIZE (); /* For debug version only */
	status = InitializeFullAcpi ();
	CHECK_STATUS("InitializeFullAcpi");

    /* Enable debug output, example debug print */

    AcpiDbgLayer = ACPI_EXAMPLE; //ACPI_ALL_COMPONENTS;
    AcpiDbgLevel = ACPI_LV_ALL_EXCEPTIONS | ACPI_LV_INTERRUPTS;

	int pic_mode = 0; // Default is PIC mode if something fails
	status = FindIOAPICs(&pic_mode);
	CHECK_STATUS("Find IOAPIC");
	status = ExecuteOSI(pic_mode);
	CHECK_STATUS("ExecuteOSI");
	// Tables we get in Bochs:
	// * DSDT: All the AML code
	// * FACS
	// * FACP
	// * APIC (= MADT)
	// * SSDT: Secondary System Description Table
	//   Contains more AML code loaded automatically by ACPICA
	// More tables on qemu:
	// * Another SSDT (Loaded by ACPICA)
	// * HPET table
//	PrintFACSTable();
//	PrintFACPTable();
	PrintAPICTable();
	CHECK_STATUS("PrintAPICTable");
	// TODO Do something like PrintDevices to disable all pci interrupt link
	// devices (call _DIS). Then we'll enable them as we go along.
	PrintDevices();
	EnumeratePCI();

	AcpiWriteBitRegister(ACPI_BITREG_SCI_ENABLE, 1);
	//AcpiWriteBitRegister(ACPI_BITREG_POWER_BUTTON_ENABLE, 1);
	AcpiInstallGlobalEventHandler(GlobalEventHandler, NULL);
	AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);

	printf("Waiting for SCI interrupts...\n");
	for (;;) {
		uintptr_t rcpt = 0x100;
		uintptr_t arg = 0;
		uintptr_t arg2 = 0;
		uintptr_t msg = recv2(&rcpt, &arg, &arg2);
		//printf("acpica: Received %#lx from %#lx: %#lx %#lx\n", msg, rcpt, arg, arg2);
		if (msg == MSG_PULSE) {
			if (AcpiOsCheckInterrupt(rcpt, arg)) {
				continue;
			} else {
				printf("acpica: Unhandled pulse: %#x from %#lx\n", arg, rcpt);
			}
		}
		switch (msg & 0xff)
		{
		case MSG_ACPI_FIND_PCI:
			MsgFindPci(rcpt, arg);
			break;
		case MSG_ACPI_CLAIM_PCI:
			MsgClaimPci(rcpt, arg, arg2);
			break;
		// This feels a bit wrong, but as long as we use PIO access to PCI
		// configuration space, we need to serialize all accesses.
		case MSG_ACPI_READ_PCI:
			arg = PciReadWord((arg & 0x7ffffffc) | 0x80000000);
			send1(MSG_ACPI_READ_PCI, rcpt, arg);
			break;
		case MSG_ACPI_DEBUGGER_INIT:
			debugger_pre_cmd();
			send0(MSG_ACPI_DEBUGGER_INIT, rcpt);
			break;
		case MSG_ACPI_DEBUGGER_BUFFER:
			assert(debugger_buffer_pos < ACPI_DB_LINE_BUFFER_SIZE);
			AcpiGbl_DbLineBuf[debugger_buffer_pos++] = arg;
			send0(MSG_ACPI_DEBUGGER_BUFFER, rcpt);
			break;
		case MSG_ACPI_DEBUGGER_CMD:
			assert(debugger_buffer_pos < ACPI_DB_LINE_BUFFER_SIZE);
			AcpiGbl_DbLineBuf[debugger_buffer_pos++] = 0;
			AcpiDbCommandDispatch(AcpiGbl_DbLineBuf, NULL, NULL);
			debugger_pre_cmd();
			send0(MSG_ACPI_DEBUGGER_CMD, rcpt);
			break;
		case MSG_ACPI_DEBUGGER_CLR_BUFFER:
			debugger_pre_cmd();
			send0(MSG_ACPI_DEBUGGER_CLR_BUFFER, rcpt);
			break;
		case MSG_REG_IRQ:
			RegIRQ(rcpt, arg);
			continue;
		case MSG_IRQ_ACK:
			AckIRQ(rcpt);
			continue;
		}
		// TODO Handle other stuff.
		if (rcpt == 0x100)
		{
			hmod(rcpt, 0, 0);
		}
	}
	status = AcpiTerminate();
	CHECK_STATUS("AcpiTerminate");
	printf("Acpi terminated... Halting.\n");

failed:
	printf("ACPI failed :( (status %x)\n", status);
	abort();
}


#endif
