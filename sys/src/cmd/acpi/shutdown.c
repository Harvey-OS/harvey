#include <acpi.h>
#include <accommon.h>
#include <acnamesp.h>

// Immediately shutdown via ACPI

void
main(void)
{
	ACPI_STATUS status;
	ACPI_BUFFER outBuffer = {ACPI_ALLOCATE_BUFFER, nil};
	extern UINT32 AcpiDbgLevel;
	unsigned int info;

        // ACPI init
	status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status))
		sysfatal("acpi: failed to init subsystem");
	status = AcpiInitializeTables(NULL, 4, 0);
	if (ACPI_FAILURE(status))
		sysfatal("acpi: failed to init tables");
	status = AcpiLoadTables();
	if (ACPI_FAILURE(status))
		sysfatal("acpi: failed to load tables");
	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status))
		sysfatal("acpi: failed to enable subsystem");
	status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status))
		sysfatal("acpi: failed to initialize objects");

        // Sleep
        status = AcpiEnterSleepStatePrep(5);
	if (ACPI_FAILURE(status))
		sysfatal("acpi: failed to enter sleep state prep");
        status = AcpiEnterSleepState(5);
	if (ACPI_FAILURE(status))
		sysfatal("acpi: failed to enter sleep state");

	exits(0);
}
