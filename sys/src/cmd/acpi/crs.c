/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <acpi.h>

#define CHECK_STATUS(fmt, ...) do { if (ACPI_FAILURE(status)) { \
	printf("ACPI failed (%d): " fmt "\n", status, ## __VA_ARGS__); \
	goto failed; \
	} } while(0)

extern void *ACPIRootPointer;
extern int ACPITableSize;
extern UINT32 AcpiDbgLevel;

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

/* these go somewhere else, someday. */
ACPI_STATUS FindIOAPICs(int *pic_mode);

static void DumpResource(ACPI_RESOURCE* resource)
{
	while (resource->Type != ACPI_RESOURCE_TYPE_END_TAG) {
		printf("Got resource type %d\n", resource->Type);
		ACPI_RESOURCE_ADDRESS64 addr64;
		ACPI_STATUS status = AcpiResourceToAddress64(resource, &addr64);
		printf("Processed and got type 0x%x\n",  addr64.ResourceType);

		if (status == AE_OK && addr64.ResourceType == ACPI_BUS_NUMBER_RANGE)
		{
			printf("RouteIRQ: Root bridge bus range %#x..%#x\n",
			       addr64.Address.Minimum,
					addr64.Address.Maximum);
			break;
		}
		resource = ACPI_NEXT_RESOURCE(resource);
	}
}

static ACPI_STATUS PrintAcpiDevice(ACPI_HANDLE Device)
{
	ACPI_BUFFER buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	ACPI_DEVICE_INFO* info = NULL;
	ACPI_STATUS status = AcpiGetObjectInfo(Device, &info);
	ACPI_STATUS crs;
	if (!ACPI_SUCCESS(status))
		return AE_OK;
	//ResetBuffer(&buffer);
	status = AcpiGetCurrentResources(Device, &buffer);
	if (!ACPI_SUCCESS(status))
		return AE_OK;
	//ACPI_FREE(info);
	//return_ACPI_STATUS(status);
	printf("Device %p type %#x\n", Device, info->Type);
	DumpResource((ACPI_RESOURCE*)buffer.Pointer);
	AcpiRsDumpResourceList((ACPI_RESOURCE*)buffer.Pointer);
	return AE_OK;
}


static ACPI_STATUS PrintDeviceCallback(ACPI_HANDLE Device, UINT32 Depth, void *Context, void** ReturnValue)
{
	return PrintAcpiDevice(Device);
}

// PNP0C0F = PCI Interrupt Link Device
// PNP0A03 = PCI Root Bridge
static ACPI_STATUS PrintCRS(void) {
	ACPI_STATUS status = AE_OK;

	printf("Searching for PNP0A03\n");
	status = AcpiGetDevices(NULL/*"PNP0A03"*/, PrintDeviceCallback, NULL, NULL);
#if 0
	CHECK_STATUS("AcpiGetDevices PNP0A03");
	if (DBGFLG) printf("Searching for PNP0C0F\n");
	status = AcpiGetDevices("PNP0C0F", PrintDeviceCallback, NULL, NULL);
	CHECK_STATUS("AcpiGetDevices PNP0C0F");
#endif

	return status;
}

void
main(int argc, char *argv[])
{
	int set = -1, enable = -1;
	int seg = 0, bus = 0, dev = 2, fn = 0, pin = 0;
	ACPI_STATUS status;
	int verbose = 0;
	AcpiDbgLevel = ACPI_LV_VERBOSE | ACPI_LV_RESOURCES | ACPI_LV_VERBOSITY2;
	AcpiDbgLayer = ACPI_RESOURCES | ACPI_EXECUTER;
	if (0) AcpiDbgLayer |= ACPI_ALL_COMPONENTS;

	ARGBEGIN{
	case 'v':
		AcpiDbgLevel |= ACPI_LV_VERBOSITY1 | ACPI_LV_FUNCTIONS;
		verbose++;
		break;
	case 's':
		set = open("#P/irqmap", OWRITE);
		if (set < 0)
			sysfatal("%r");
		break;
	default:
		sysfatal("usage: acpi/irq [-v] [-s]");
		break;
	}ARGEND;

	status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status)) {
		sysfatal("Error %d\n", status);
	}
        status = AcpiInitializeTables(NULL, 0, FALSE);
        if (ACPI_FAILURE(status))
		sysfatal("can't set up acpi tables: %d", status);

	if (verbose)
		print("init dables\n");
        status = AcpiLoadTables();
        if (ACPI_FAILURE(status))
		sysfatal("Can't load ACPI tables: %d", status);

	/* from acpi: */
    	/* If the Hardware Reduced flag is set, machine is always in acpi mode */
	AcpiGbl_ReducedHardware = 1;
	if (verbose)
		print("LOADED TABLES.\n");
        status = AcpiEnableSubsystem(0);
        if (ACPI_FAILURE(status))
		print("Probably does not matter: Can't enable ACPI subsystem");

	if (verbose)
		print("enabled subsystem.\n");
        status = AcpiInitializeObjects(0);
        if (ACPI_FAILURE(status))
		sysfatal("Can't Initialize ACPI objects");

	int picmode;
	status = FindIOAPICs(&picmode);

	if (picmode == 0)
		sysfatal("PANIC: Can't handle picmode 0!");
	ACPI_STATUS ExecuteOSI(int pic_mode);
	if (verbose)
		print("FindIOAPICs returns status %d picmode %d\n", status, picmode);
	status = ExecuteOSI(picmode);
	CHECK_STATUS("ExecuteOSI");
failed:
	if (verbose)
		print("inited objects.\n");
	if (verbose)
		AcpiDbgLevel |= ACPI_LV_VERBOSITY1 | ACPI_LV_FUNCTIONS;

	status = AcpiInitializeDebugger();
	if (ACPI_FAILURE(status)) {
		sysfatal("Error %d\n", status);
	}
	int GetPRT();
	status = GetPRT();
	if (ACPI_FAILURE(status)) {
		sysfatal("Error %d\n", status);
	}

	printf("Searching for PNP0A03\n");
	status = PrintCRS(); //AcpiGetDevices(NULL/*"PNP0A03"*/, CRSCallBack, NULL, NULL);
	CHECK_STATUS("AcpiGetDevices PNP0A03");
	exits(0);
}
