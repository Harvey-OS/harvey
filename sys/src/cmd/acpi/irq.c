/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <acpi.h>

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

void
main(int argc, char *argv[])
{
	ACPI_STATUS status;
	AcpiDbgLevel = ACPI_LV_VERBOSITY1;
	print("hi\n");
	status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status)) {
		sysfatal("Error %d\n", status);
	}
        status = AcpiInitializeTables(NULL, 2048, FALSE);
        if (ACPI_FAILURE(status))
		sysfatal("can't set up acpi tables: %d", status);

	print("initit dables\n"); 
        status = AcpiLoadTables();
        if (ACPI_FAILURE(status))
		sysfatal("Can't load ACPI tables: %d", status);

	print("LOADED TABLES. Hi the any key to continue\n"); getchar();
        status = AcpiEnableSubsystem(0);
        if (ACPI_FAILURE(status))
		sysfatal("Can't enable ACPI subsystem");

	print("enabled subsystem. Hi the any key to continue\n"); getchar();
        status = AcpiInitializeObjects(0);
        if (ACPI_FAILURE(status))
		sysfatal("Can't Initialize ACPI objects");

	print("inited objects. Hi the any key to continue\n"); getchar();
	status = AcpiInitializeDebugger();
	if (ACPI_FAILURE(status)) {
		sysfatal("Error %d\n", status);
	}
	print("OK on init.\n");
	exits(0);
}

