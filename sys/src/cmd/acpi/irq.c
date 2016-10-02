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

void
main(int argc, char *argv[])
{
	int set = -1, enable = -1;
	int seg = 0, bus = 0, dev = 2, fn = 0, pin = 0;
	ACPI_STATUS status;
	int verbose = 0;
	AcpiDbgLevel = 0;
	ARGBEGIN{
	case 'v':
		AcpiDbgLevel = ACPI_LV_VERBOSITY1;
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

	while (1) {
		if (scanf("%x %x", &bus, &dev) < 0)
			break;
		ACPI_STATUS RouteIRQ(ACPI_PCI_ID* device, int pin, int* irq);
		AcpiDbgLevel = 0;
		ACPI_PCI_ID id = (ACPI_PCI_ID){seg, bus, dev, fn};
		status = AcpiOsReadPciConfiguration (&id, 0x3d, &pin, 8);
		if (!ACPI_SUCCESS(status)){
			printf("Can't read pin for bus %d dev %d\n", bus, dev);
			continue;
		}
		print("ROUTE {%d, %d, %d, %d}, pin %d\n", seg, bus, dev, fn, pin);
		int irq;
		status = RouteIRQ(&id, pin, &irq);
		print("status %d, irq %d\n", status, irq);
		AcpiDbgLevel = 0;
		print("echo %d %d %d %d 0x%x > /dev/irqmap", seg, bus, dev, fn, irq);
		//ACPI_STATUS PrintDevices(void);
		//status = PrintDevices();
		if (set > -1)
			fprint(set, "%d %d %d %d 0x%x", seg, bus, dev, fn, irq);
	}

	/* we're done. Enable the IRQs we might have set up. */
	enable = open("/dev/irqenable", OWRITE);
	if (enable < 0)
		sysfatal("/dev/irqenable: %r");
	if (write(enable, "", 1) < 1)
		sysfatal("Write irqenable: %r");

	exits(0);
}

