/******************************************************************************
 *
 * Module Name: tbxface - ACPI table-oriented external interfaces
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999 - 2016, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights. You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code. No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision. In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change. Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee. Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution. In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE. ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT, ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES. INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS. INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES. THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government. In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#define EXPORT_ACPI_INTERFACES

#include "acpi.h"
#include "accommon.h"
#include "actables.h"

#define _COMPONENT          ACPI_TABLES
        ACPI_MODULE_NAME    ("tbxface")


/*******************************************************************************
 *
 * FUNCTION:    AcpiAllocateRootTable
 *
 * PARAMETERS:  InitialTableCount   - Size of InitialTableArray, in number of
 *                                    ACPI_TABLE_DESC structures
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Allocate a root table array. Used by iASL compiler and
 *              AcpiInitializeTables.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiAllocateRootTable (
    UINT32                  InitialTableCount)
{

    AcpiGbl_RootTableList.MaxTableCount = InitialTableCount;
    AcpiGbl_RootTableList.Flags = ACPI_ROOT_ALLOW_RESIZE;

    return (AcpiTbResizeRootTableList ());
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiInitializeTables
 *
 * PARAMETERS:  InitialTableArray   - Pointer to an array of pre-allocated
 *                                    ACPI_TABLE_DESC structures. If NULL, the
 *                                    array is dynamically allocated.
 *              InitialTableCount   - Size of InitialTableArray, in number of
 *                                    ACPI_TABLE_DESC structures
 *              AllowResize         - Flag to tell Table Manager if resize of
 *                                    pre-allocated array is allowed. Ignored
 *                                    if InitialTableArray is NULL.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Initialize the table manager, get the RSDP and RSDT/XSDT.
 *
 * NOTE:        Allows static allocation of the initial table array in order
 *              to avoid the use of dynamic memory in confined environments
 *              such as the kernel boot sequence where it may not be available.
 *
 *              If the host OS memory managers are initialized, use NULL for
 *              InitialTableArray, and the table will be dynamically allocated.
 *
 ******************************************************************************/

ACPI_STATUS ACPI_INIT_FUNCTION
AcpiInitializeTables (
    ACPI_TABLE_DESC         *InitialTableArray,
    UINT32                  InitialTableCount,
    BOOLEAN                 AllowResize)
{
    ACPI_PHYSICAL_ADDRESS   RsdpAddress;
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInitializeTables);


    /*
     * Setup the Root Table Array and allocate the table array
     * if requested
     */
    if (!InitialTableArray)
    {
        Status = AcpiAllocateRootTable (InitialTableCount);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }
    }
    else
    {
        /* Root Table Array has been statically allocated by the host */

        memset (InitialTableArray, 0,
            (ACPI_SIZE) InitialTableCount * sizeof (ACPI_TABLE_DESC));

        AcpiGbl_RootTableList.Tables = InitialTableArray;
        AcpiGbl_RootTableList.MaxTableCount = InitialTableCount;
        AcpiGbl_RootTableList.Flags = ACPI_ROOT_ORIGIN_UNKNOWN;
        if (AllowResize)
        {
            AcpiGbl_RootTableList.Flags |= ACPI_ROOT_ALLOW_RESIZE;
        }
    }

    /* Get the address of the RSDP */

    RsdpAddress = AcpiOsGetRootPointer ();
    if (!RsdpAddress)
    {
        return_ACPI_STATUS (AE_NOT_FOUND);
    }

    /*
     * Get the root table (RSDT or XSDT) and extract all entries to the local
     * Root Table Array. This array contains the information of the RSDT/XSDT
     * in a common, more useable format.
     */
    Status = AcpiTbParseRootTable (RsdpAddress);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiInitializeTables)


/*******************************************************************************
 *
 * FUNCTION:    AcpiReallocateRootTable
 *
 * PARAMETERS:  None
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Reallocate Root Table List into dynamic memory. Copies the
 *              root list from the previously provided scratch area. Should
 *              be called once dynamic memory allocation is available in the
 *              kernel.
 *
 ******************************************************************************/

ACPI_STATUS ACPI_INIT_FUNCTION
AcpiReallocateRootTable (
    void)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiReallocateRootTable);


    /*
     * Only reallocate the root table if the host provided a static buffer
     * for the table array in the call to AcpiInitializeTables.
     */
    if (AcpiGbl_RootTableList.Flags & ACPI_ROOT_ORIGIN_ALLOCATED)
    {
        return_ACPI_STATUS (AE_SUPPORT);
    }

    AcpiGbl_RootTableList.Flags |= ACPI_ROOT_ALLOW_RESIZE;

    Status = AcpiTbResizeRootTableList ();
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL_INIT (AcpiReallocateRootTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetTableHeader
 *
 * PARAMETERS:  Signature           - ACPI signature of needed table
 *              Instance            - Which instance (for SSDTs)
 *              OutTableHeader      - The pointer to the table header to fill
 *
 * RETURN:      Status and pointer to mapped table header
 *
 * DESCRIPTION: Finds an ACPI table header.
 *
 * NOTE:        Caller is responsible in unmapping the header with
 *              AcpiOsUnmapMemory
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetTableHeader (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       *OutTableHeader)
{
    UINT32                  i;
    UINT32                  j;
    ACPI_TABLE_HEADER       *Header;


    /* Parameter validation */

    if (!Signature || !OutTableHeader)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Walk the root table list */

    for (i = 0, j = 0; i < AcpiGbl_RootTableList.CurrentTableCount; i++)
    {
        if (!ACPI_COMPARE_NAME (
                &(AcpiGbl_RootTableList.Tables[i].Signature), Signature))
        {
            continue;
        }

        if (++j < Instance)
        {
            continue;
        }

        if (!AcpiGbl_RootTableList.Tables[i].Pointer)
        {
            if ((AcpiGbl_RootTableList.Tables[i].Flags &
                    ACPI_TABLE_ORIGIN_MASK) ==
                ACPI_TABLE_ORIGIN_INTERNAL_PHYSICAL)
            {
                Header = AcpiOsMapMemory (
                    AcpiGbl_RootTableList.Tables[i].Address,
                    sizeof (ACPI_TABLE_HEADER));
                if (!Header)
                {
                    return (AE_NO_MEMORY);
                }

                memcpy (OutTableHeader, Header, sizeof (ACPI_TABLE_HEADER));
                AcpiOsUnmapMemory (Header, sizeof (ACPI_TABLE_HEADER));
            }
            else
            {
                return (AE_NOT_FOUND);
            }
        }
        else
        {
            memcpy (OutTableHeader,
                AcpiGbl_RootTableList.Tables[i].Pointer,
                sizeof (ACPI_TABLE_HEADER));
        }

        return (AE_OK);
    }

    return (AE_NOT_FOUND);
}

ACPI_EXPORT_SYMBOL (AcpiGetTableHeader)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetTable
 *
 * PARAMETERS:  Signature           - ACPI signature of needed table
 *              Instance            - Which instance (for SSDTs)
 *              OutTable            - Where the pointer to the table is returned
 *
 * RETURN:      Status and pointer to the requested table
 *
 * DESCRIPTION: Finds and verifies an ACPI table. Table must be in the
 *              RSDT/XSDT.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetTable (
    char                    *Signature,
    UINT32                  Instance,
    ACPI_TABLE_HEADER       **OutTable)
{
    UINT32                  i;
    UINT32                  j;
    ACPI_STATUS             Status;


    /* Parameter validation */

    if (!Signature || !OutTable)
    {
        return (AE_BAD_PARAMETER);
    }

    /* Walk the root table list */

    for (i = 0, j = 0; i < AcpiGbl_RootTableList.CurrentTableCount; i++)
    {
        if (!ACPI_COMPARE_NAME (
                &(AcpiGbl_RootTableList.Tables[i].Signature), Signature))
        {
            continue;
        }

        if (++j < Instance)
        {
            continue;
        }

        Status = AcpiTbValidateTable (&AcpiGbl_RootTableList.Tables[i]);
        if (ACPI_SUCCESS (Status))
        {
            *OutTable = AcpiGbl_RootTableList.Tables[i].Pointer;
        }

        return (Status);
    }

    return (AE_NOT_FOUND);
}

ACPI_EXPORT_SYMBOL (AcpiGetTable)


/*******************************************************************************
 *
 * FUNCTION:    AcpiGetTableByIndex
 *
 * PARAMETERS:  TableIndex          - Table index
 *              Table               - Where the pointer to the table is returned
 *
 * RETURN:      Status and pointer to the requested table
 *
 * DESCRIPTION: Obtain a table by an index into the global table list. Used
 *              internally also.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiGetTableByIndex (
    UINT32                  TableIndex,
    ACPI_TABLE_HEADER       **Table)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiGetTableByIndex);


    /* Parameter validation */

    if (!Table)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    (void) AcpiUtAcquireMutex (ACPI_MTX_TABLES);

    /* Validate index */

    if (TableIndex >= AcpiGbl_RootTableList.CurrentTableCount)
    {
        (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    if (!AcpiGbl_RootTableList.Tables[TableIndex].Pointer)
    {
        /* Table is not mapped, map it */

        Status = AcpiTbValidateTable (
            &AcpiGbl_RootTableList.Tables[TableIndex]);
        if (ACPI_FAILURE (Status))
        {
            (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
            return_ACPI_STATUS (Status);
        }
    }

    *Table = AcpiGbl_RootTableList.Tables[TableIndex].Pointer;
    (void) AcpiUtReleaseMutex (ACPI_MTX_TABLES);
    return_ACPI_STATUS (AE_OK);
}

ACPI_EXPORT_SYMBOL (AcpiGetTableByIndex)


/*******************************************************************************
 *
 * FUNCTION:    AcpiInstallTableHandler
 *
 * PARAMETERS:  Handler         - Table event handler
 *              Context         - Value passed to the handler on each event
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Install a global table event handler.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiInstallTableHandler (
    ACPI_TABLE_HANDLER      Handler,
    void                    *Context)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiInstallTableHandler);


    if (!Handler)
    {
        return_ACPI_STATUS (AE_BAD_PARAMETER);
    }

    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Don't allow more than one handler */

    if (AcpiGbl_TableHandler)
    {
        Status = AE_ALREADY_EXISTS;
        goto Cleanup;
    }

    /* Install the handler */

    AcpiGbl_TableHandler = Handler;
    AcpiGbl_TableHandlerContext = Context;

Cleanup:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiInstallTableHandler)


/*******************************************************************************
 *
 * FUNCTION:    AcpiRemoveTableHandler
 *
 * PARAMETERS:  Handler         - Table event handler that was installed
 *                                previously.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Remove a table event handler
 *
 ******************************************************************************/

ACPI_STATUS
AcpiRemoveTableHandler (
    ACPI_TABLE_HANDLER      Handler)
{
    ACPI_STATUS             Status;


    ACPI_FUNCTION_TRACE (AcpiRemoveTableHandler);


    Status = AcpiUtAcquireMutex (ACPI_MTX_EVENTS);
    if (ACPI_FAILURE (Status))
    {
        return_ACPI_STATUS (Status);
    }

    /* Make sure that the installed handler is the same */

    if (!Handler ||
        Handler != AcpiGbl_TableHandler)
    {
        Status = AE_BAD_PARAMETER;
        goto Cleanup;
    }

    /* Remove the handler */

    AcpiGbl_TableHandler = NULL;

Cleanup:
    (void) AcpiUtReleaseMutex (ACPI_MTX_EVENTS);
    return_ACPI_STATUS (Status);
}

ACPI_EXPORT_SYMBOL (AcpiRemoveTableHandler)
