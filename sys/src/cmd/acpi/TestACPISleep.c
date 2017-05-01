#include <acpi.h>
#include <accommon.h>
#include <acnamesp.h>


void
fail(char *msg)
{
	fprint(2, "%s\n", msg);
	exits("FAIL");
}

UINT32
shutdownhandler(void *ctxt)
{
	ACPI_STATUS status;

	syslog(1, "acpi", "acpi event: power button");
	status = AcpiEnterSleepStatePrep(5);
	if (ACPI_FAILURE(status))
		fail("failed to prep for sleep");
	status = AcpiEnterSleepState(5);
	if (ACPI_FAILURE(status))
		fail("failed to go to sleep");
	return 0;
}

UINT32
timerhandler(void *ctxt)
{
	syslog(1, "acpi", "acpi event: pm timer");
	return 0;
}

unsigned int
AcpiIntrWait(int afd, unsigned int *info);

ACPI_STATUS AcpiRunInterrupt(void);

/* The order of initialization comes from the ACPICA Programmer's Reference */
void main(int argc, char *argv[])
{
	ACPI_STATUS status;
	extern UINT32 AcpiDbgLevel;
	unsigned int info;

	int fd = open("/dev/acpiintr", OWRITE);
	if (fd < 0)
		exits("can't open /dev/acpiintr");

	status = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(status))
		fail("failed to init subsystem");
	status = AcpiInitializeTables(NULL, 4, 0);
	if (ACPI_FAILURE(status))
		fail("failed to init tables");
	status = AcpiLoadTables();
	if (ACPI_FAILURE(status))
		fail("failed to load tables");
	status = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status))
		fail("failed to enable subsystem");
	/* ACPI Handlers should be installed at this time */
	status = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(status))
		fail("failed to initialize objects");
	/* Other ACPI initialization can occur here including:
	 * 	Enumerating devices using _HID method
	 * 	Loading, configuring, installing device drivers
	 *	Device Driver install handlers for other address spaces
	 *	Enumerating PCI devices and loading PCIConfig handlers
	 */
	//AcpiDbgLevel |= ACPI_LV_VERBOSITY1 | ACPI_LV_FUNCTIONS;
	//status = AcpiInstallGlobalEventHandler(shutdownhandler, nil);
	status = AcpiInstallFixedEventHandler(ACPI_EVENT_POWER_BUTTON, shutdownhandler, nil);
	status = AcpiInstallFixedEventHandler(ACPI_EVENT_PMTIMER, timerhandler, nil);
	if (ACPI_FAILURE(status))
		fail("failed to install fixed event handler");
	print("Fixed Event Handler installed!\n");
	status = AcpiEnableEvent(ACPI_EVENT_POWER_BUTTON, 0);
	if (ACPI_FAILURE(status))
		fail("failed to enable power button");
	status = AcpiEnableEvent(ACPI_EVENT_PMTIMER, 0);
	if (ACPI_FAILURE(status))
		fail("failed to enable pmtimer");

	while (AcpiIntrWait(fd, &info) > sizeof(info)) {
		AcpiRunInterrupt();

	}

	status = AcpiTerminate();
        if (ACPI_FAILURE(status))
                fail("failed to terminate acpi");
        exits(nil);
}
