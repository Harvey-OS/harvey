/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include <u.h>
#include <libc.h>
#include <acpi.h>
#include <accommon.h>
#include <actables.h>
#include <acutils.h>

extern void *ACPIRootPointer;
extern int ACPITableSize;
extern UINT32 AcpiDbgLevel;


/******************************************************************************
 *
 * FUNCTION:    ApGetTableLength
 *
 * PARAMETERS:  Table               - Pointer to the table
 *
 * RETURN:      Table length
 *
 * DESCRIPTION: Obtain table length according to table signature.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * FUNCTION:    ApIsValidHeader
 *
 * PARAMETERS:  Table               - Pointer to table to be validated
 *
 * RETURN:      TRUE if the header appears to be valid. FALSE otherwise
 *
 * DESCRIPTION: Check for a valid ACPI table header
 *
 ******************************************************************************/

BOOLEAN
ApIsValidHeader (
    ACPI_TABLE_HEADER       *Table)
{

    if (!ACPI_VALIDATE_RSDP_SIG (Table->Signature))
    {
        /* Make sure signature is all ASCII and a valid ACPI name */

        if (!AcpiUtValidNameseg (Table->Signature))
        {
            AcpiLogError ("Table signature (0x%8.8X) is invalid\n",
                *(UINT32 *) Table->Signature);
            return (FALSE);
        }

        /* Check for minimum table length */

        if (Table->Length < sizeof (ACPI_TABLE_HEADER))
        {
            AcpiLogError ("Table length (0x%8.8X) is invalid\n",
                Table->Length);
            return (FALSE);
        }
    }

    return (TRUE);
}


UINT32
ApGetTableLength (
    ACPI_TABLE_HEADER       *Table)
{
    ACPI_TABLE_RSDP         *Rsdp;


    /* Check if table is valid */

    if (!ApIsValidHeader (Table))
    {
        return (0);
    }

    if (ACPI_VALIDATE_RSDP_SIG (Table->Signature))
    {
        Rsdp = ACPI_CAST_PTR (ACPI_TABLE_RSDP, Table);
        return (AcpiTbGetRsdpLength (Rsdp));
    }

    /* Normal ACPI table */

    return (Table->Length);
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

    ACPI_TABLE_HEADER       *Table;
    UINT32                  Instance = 0;
    ACPI_PHYSICAL_ADDRESS   Address;
    ACPI_STATUS             Status;
    int                     TableStatus;
    UINT32                  i;


    /* Get and dump all available ACPI tables */

    for (i = 0; i < 255; i++)
    {
	    Status = AcpiOsGetTableByIndex (i, &Table, &Instance, &Address);
	    if (ACPI_FAILURE (Status))
	    {
		    /* AE_LIMIT means that no more tables are available */

		    if (Status == AE_LIMIT)
		    {
			    break;
		    }
		    else if (i == 0)
		    {
			    AcpiLogError ("Could not get ACPI tables, %s\n",
					  AcpiFormatException (Status));
			    break;
		    }
		    else
		    {
			    AcpiLogError ("Could not get ACPI table at index %u, %s\n",
					  i, AcpiFormatException (Status));
			    continue;
		    }
	    }
    UINT32                  TableLength;

    TableLength = ApGetTableLength (Table);

        AcpiTbPrintTableHeader (Address, Table);


    AcpiUtFilePrintf (1, "%4.4s @ 0x%8.8X%8.8X\n",
        Table->Signature, ACPI_FORMAT_UINT64 (Address));

    AcpiUtDumpBufferToFile (1,
        ACPI_CAST_PTR (UINT8, Table), TableLength,
        DB_BYTE_DISPLAY, 0);
    AcpiUtFilePrintf (1, "\n");

	    ACPI_FREE (Table);
	    if (TableStatus)
	    {
		    break;
	    }
    }

#if 0
    print("initit dables\n");
        status = AcpiLoadTables();
        if (ACPI_FAILURE(status))
		sysfatal("Can't load ACPI tables: %d", status);

	sysfatal("LOADED TABLES. Hi the any key to continue\n"); getchar();
        status = AcpiEnableSubsystem(0);
        if (ACPI_FAILURE(status))
		sysfatal("Can't enable ACPI subsystem");

        status = AcpiInitializeObjects(0);
        if (ACPI_FAILURE(status))
		sysfatal("Can't Initialize ACPI objects");

	status = AcpiInitializeDebugger();
	if (ACPI_FAILURE(status)) {
		sysfatal("Error %d\n", status);
	}
	print("OK on init.\n");
#endif
	exits(0);
}
