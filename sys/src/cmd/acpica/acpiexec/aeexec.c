/******************************************************************************
 *
 * Module Name: aeexec - Support routines for AcpiExec utility
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

#include "aecommon.h"

#define _COMPONENT          ACPI_TOOLS
        ACPI_MODULE_NAME    ("aeexec")

/* Local prototypes */

static ACPI_STATUS
AeSetupConfiguration (
    void                    *RegionAddr);

static void
AeTestBufferArgument (
    void);

static void
AeTestPackageArgument (
    void);

static ACPI_STATUS
AeGetDevices (
    ACPI_HANDLE             ObjHandle,
    UINT32                  NestingLevel,
    void                    *Context,
    void                    **ReturnValue);

static ACPI_STATUS
ExecuteOSI (
    char                    *OsiString,
    UINT32                  ExpectedResult);

static void
AeMutexInterfaces (
    void);

static void
AeHardwareInterfaces (
    void);

static void
AeGenericRegisters (
    void);

static void
AeTestSleepData (
    void);

#if (!ACPI_REDUCED_HARDWARE)
static void
AfInstallGpeBlock (
    void);
#endif /* !ACPI_REDUCED_HARDWARE */

extern unsigned char Ssdt2Code[];
extern unsigned char Ssdt3Code[];
extern unsigned char Ssdt4Code[];


/******************************************************************************
 *
 * FUNCTION:    AeSetupConfiguration
 *
 * PARAMETERS:  RegionAddr          - Address for an ACPI table to be loaded
 *                                    dynamically. Test purposes only.
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Call AML _CFG configuration control method
 *
 *****************************************************************************/

static ACPI_STATUS
AeSetupConfiguration (
    void                    *RegionAddr)
{
    ACPI_OBJECT_LIST        ArgList;
    ACPI_OBJECT             Arg[3];


    /*
     * Invoke _CFG method if present
     */
    ArgList.Count = 1;
    ArgList.Pointer = Arg;

    Arg[0].Type = ACPI_TYPE_INTEGER;
    Arg[0].Integer.Value = ACPI_TO_INTEGER (RegionAddr);

    (void) AcpiEvaluateObject (NULL, "\\_CFG", &ArgList, NULL);
    return (AE_OK);
}


#if (!ACPI_REDUCED_HARDWARE)
/******************************************************************************
 *
 * FUNCTION:    AfInstallGpeBlock
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Test GPE block device initialization. Requires test ASL with
 *              A \GPE2 device.
 *
 *****************************************************************************/

static void
AfInstallGpeBlock (
    void)
{
    ACPI_STATUS                 Status;
    ACPI_HANDLE                 Handle;
    ACPI_GENERIC_ADDRESS        BlockAddress;
    ACPI_HANDLE                 GpeDevice;
    ACPI_OBJECT_TYPE            Type;


    /* _GPE should always exist */

    Status = AcpiGetHandle (NULL, "\\_GPE", &Handle);
    ACPI_CHECK_OK (AcpiGetHandle, Status);
    if (ACPI_FAILURE (Status))
    {
        return;
    }

    memset (&BlockAddress, 0, sizeof (ACPI_GENERIC_ADDRESS));
    BlockAddress.SpaceId = ACPI_ADR_SPACE_SYSTEM_MEMORY;
    BlockAddress.Address = 0x76540000;

    /* Attempt to install a GPE block on GPE2 (if present) */

    Status = AcpiGetHandle (NULL, "\\GPE2", &Handle);
    if (ACPI_SUCCESS (Status))
    {
        Status = AcpiGetType (Handle, &Type);
        if (ACPI_FAILURE (Status) ||
           (Type != ACPI_TYPE_DEVICE))
        {
            return;
        }

        Status = AcpiInstallGpeBlock (Handle, &BlockAddress, 7, 8);
        ACPI_CHECK_OK (AcpiInstallGpeBlock, Status);

        Status = AcpiInstallGpeHandler (Handle, 8,
            ACPI_GPE_LEVEL_TRIGGERED, AeGpeHandler, NULL);
        ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

        Status = AcpiEnableGpe (Handle, 8);
        ACPI_CHECK_OK (AcpiEnableGpe, Status);

        Status = AcpiGetGpeDevice (0x30, &GpeDevice);
        ACPI_CHECK_OK (AcpiGetGpeDevice, Status);

        Status = AcpiGetGpeDevice (0x42, &GpeDevice);
        ACPI_CHECK_OK (AcpiGetGpeDevice, Status);

        Status = AcpiGetGpeDevice (AcpiCurrentGpeCount-1, &GpeDevice);
        ACPI_CHECK_OK (AcpiGetGpeDevice, Status);

        Status = AcpiGetGpeDevice (AcpiCurrentGpeCount, &GpeDevice);
        ACPI_CHECK_STATUS (AcpiGetGpeDevice, Status, AE_NOT_EXIST);

        Status = AcpiRemoveGpeHandler (Handle, 8, AeGpeHandler);
        ACPI_CHECK_OK (AcpiRemoveGpeHandler, Status);
    }

    /* Attempt to install a GPE block on GPE3 (if present) */

    Status = AcpiGetHandle (NULL, "\\GPE3", &Handle);
    if (ACPI_SUCCESS (Status))
    {
        Status = AcpiGetType (Handle, &Type);
        if (ACPI_FAILURE (Status) ||
           (Type != ACPI_TYPE_DEVICE))
        {
            return;
        }

        Status = AcpiInstallGpeBlock (Handle, &BlockAddress, 8, 11);
        ACPI_CHECK_OK (AcpiInstallGpeBlock, Status);
    }
}
#endif /* !ACPI_REDUCED_HARDWARE */


/* Test using a Buffer object as a method argument */

static void
AeTestBufferArgument (
    void)
{
    ACPI_OBJECT_LIST        Params;
    ACPI_OBJECT             BufArg;
    UINT8                   Buffer[] =
    {
        0,0,0,0,
        4,0,0,0,
        1,2,3,4
    };


    BufArg.Type = ACPI_TYPE_BUFFER;
    BufArg.Buffer.Length = 12;
    BufArg.Buffer.Pointer = Buffer;

    Params.Count = 1;
    Params.Pointer = &BufArg;

    (void) AcpiEvaluateObject (NULL, "\\BUF", &Params, NULL);
}


static ACPI_OBJECT                 PkgArg;
static ACPI_OBJECT                 PkgElements[5];
static ACPI_OBJECT                 Pkg2Elements[5];
static ACPI_OBJECT_LIST            Params;


/*
 * Test using a Package object as an method argument
 */
static void
AeTestPackageArgument (
    void)
{

    /* Main package */

    PkgArg.Type = ACPI_TYPE_PACKAGE;
    PkgArg.Package.Count = 4;
    PkgArg.Package.Elements = PkgElements;

    /* Main package elements */

    PkgElements[0].Type = ACPI_TYPE_INTEGER;
    PkgElements[0].Integer.Value = 0x22228888;

    PkgElements[1].Type = ACPI_TYPE_STRING;
    PkgElements[1].String.Length = sizeof ("Top-level package");
    PkgElements[1].String.Pointer = "Top-level package";

    PkgElements[2].Type = ACPI_TYPE_BUFFER;
    PkgElements[2].Buffer.Length = sizeof ("XXXX");
    PkgElements[2].Buffer.Pointer = (UINT8 *) "XXXX";

    PkgElements[3].Type = ACPI_TYPE_PACKAGE;
    PkgElements[3].Package.Count = 2;
    PkgElements[3].Package.Elements = Pkg2Elements;

    /* Subpackage elements */

    Pkg2Elements[0].Type = ACPI_TYPE_INTEGER;
    Pkg2Elements[0].Integer.Value = 0xAAAABBBB;

    Pkg2Elements[1].Type = ACPI_TYPE_STRING;
    Pkg2Elements[1].String.Length = sizeof ("Nested Package");
    Pkg2Elements[1].String.Pointer = "Nested Package";

    /* Parameter object */

    Params.Count = 1;
    Params.Pointer = &PkgArg;

    (void) AcpiEvaluateObject (NULL, "\\_PKG", &Params, NULL);
}


static ACPI_STATUS
AeGetDevices (
    ACPI_HANDLE                     ObjHandle,
    UINT32                          NestingLevel,
    void                            *Context,
    void                            **ReturnValue)
{

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    ExecuteOSI
 *
 * PARAMETERS:  OsiString           - String passed to _OSI method
 *              ExpectedResult      - 0 (FALSE) or 0xFFFFFFFF (TRUE)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute the internally implemented (in ACPICA) _OSI method.
 *
 *****************************************************************************/

static ACPI_STATUS
ExecuteOSI (
    char                    *OsiString,
    UINT32                  ExpectedResult)
{
    ACPI_STATUS             Status;
    ACPI_OBJECT_LIST        ArgList;
    ACPI_OBJECT             Arg[1];
    ACPI_BUFFER             ReturnValue;
    ACPI_OBJECT             *Obj;


    /* Setup input argument */

    ArgList.Count = 1;
    ArgList.Pointer = Arg;

    Arg[0].Type = ACPI_TYPE_STRING;
    Arg[0].String.Pointer = OsiString;
    Arg[0].String.Length = strlen (Arg[0].String.Pointer);

    /* Ask ACPICA to allocate space for the return object */

    ReturnValue.Length = ACPI_ALLOCATE_BUFFER;

    Status = AcpiEvaluateObject (NULL, "\\_OSI", &ArgList, &ReturnValue);

    if (ACPI_FAILURE (Status))
    {
        AcpiOsPrintf (
            "Could not execute _OSI method, %s\n",
            AcpiFormatException (Status));
        return (Status);
    }

    Status = AE_ERROR;

    if (ReturnValue.Length < sizeof (ACPI_OBJECT))
    {
        AcpiOsPrintf (
            "Return value from _OSI method too small, %.8X\n",
            ReturnValue.Length);
        goto ErrorExit;
    }

    Obj = ReturnValue.Pointer;
    if (Obj->Type != ACPI_TYPE_INTEGER)
    {
        AcpiOsPrintf (
            "Invalid return type from _OSI method, %.2X\n", Obj->Type);
        goto ErrorExit;
    }

    if (Obj->Integer.Value != ExpectedResult)
    {
        AcpiOsPrintf (
            "Invalid return value from _OSI, expected %.8X found %.8X\n",
            ExpectedResult, (UINT32) Obj->Integer.Value);
        goto ErrorExit;
    }

    Status = AE_OK;

    /* Reset the OSI data */

    AcpiGbl_OsiData = 0;

ErrorExit:

    /* Free a buffer created via ACPI_ALLOCATE_BUFFER */

    AcpiOsFree (ReturnValue.Pointer);
    return (Status);
}


/******************************************************************************
 *
 * FUNCTION:    AeGenericRegisters
 *
 * DESCRIPTION: Call the AcpiRead/Write interfaces.
 *
 *****************************************************************************/

static ACPI_GENERIC_ADDRESS       GenericRegister;

static void
AeGenericRegisters (
    void)
{
    ACPI_STATUS             Status;
    UINT64                  Value;


    GenericRegister.Address = 0x1234;
    GenericRegister.BitWidth = 64;
    GenericRegister.BitOffset = 0;
    GenericRegister.SpaceId = ACPI_ADR_SPACE_SYSTEM_IO;

    Status = AcpiRead (&Value, &GenericRegister);
    ACPI_CHECK_OK (AcpiRead, Status);

    Status = AcpiWrite (Value, &GenericRegister);
    ACPI_CHECK_OK (AcpiWrite, Status);

    GenericRegister.Address = 0x12345678;
    GenericRegister.BitOffset = 0;
    GenericRegister.SpaceId = ACPI_ADR_SPACE_SYSTEM_MEMORY;

    Status = AcpiRead (&Value, &GenericRegister);
    ACPI_CHECK_OK (AcpiRead, Status);

    Status = AcpiWrite (Value, &GenericRegister);
    ACPI_CHECK_OK (AcpiWrite, Status);
}


/******************************************************************************
 *
 * FUNCTION:    AeMutexInterfaces
 *
 * DESCRIPTION: Exercise the AML mutex access interfaces
 *
 *****************************************************************************/

static void
AeMutexInterfaces (
    void)
{
    ACPI_STATUS             Status;
    ACPI_HANDLE             MutexHandle;


    /* Get a handle to an AML mutex */

    Status = AcpiGetHandle (NULL, "\\MTX1", &MutexHandle);
    if (Status == AE_NOT_FOUND)
    {
        return;
    }

    ACPI_CHECK_OK (AcpiGetHandle, Status);
    if (ACPI_FAILURE (Status))
    {
        return;
    }

    /* Acquire the  mutex */

    Status = AcpiAcquireMutex (NULL, "\\MTX1", 0xFFFF);
    ACPI_CHECK_OK (AcpiAcquireMutex, Status);
    if (ACPI_FAILURE (Status))
    {
        return;
    }

    /* Release mutex with different parameters */

    Status = AcpiReleaseMutex (MutexHandle, NULL);
    ACPI_CHECK_OK (AcpiReleaseMutex, Status);
}


/******************************************************************************
 *
 * FUNCTION:    AeHardwareInterfaces
 *
 * DESCRIPTION: Call various hardware support interfaces
 *
 *****************************************************************************/

static void
AeHardwareInterfaces (
    void)
{
#if (!ACPI_REDUCED_HARDWARE)

    ACPI_STATUS             Status;
    UINT32                  Value;


    /* If Hardware Reduced flag is set, we are all done */

    if (AcpiGbl_ReducedHardware)
    {
        return;
    }

    Status = AcpiWriteBitRegister (ACPI_BITREG_WAKE_STATUS, 1);
    ACPI_CHECK_OK (AcpiWriteBitRegister, Status);

    Status = AcpiWriteBitRegister (ACPI_BITREG_GLOBAL_LOCK_ENABLE, 1);
    ACPI_CHECK_OK (AcpiWriteBitRegister, Status);

    Status = AcpiWriteBitRegister (ACPI_BITREG_SLEEP_ENABLE, 1);
    ACPI_CHECK_OK (AcpiWriteBitRegister, Status);

    Status = AcpiWriteBitRegister (ACPI_BITREG_ARB_DISABLE, 1);
    ACPI_CHECK_OK (AcpiWriteBitRegister, Status);


    Status = AcpiReadBitRegister (ACPI_BITREG_WAKE_STATUS, &Value);
    ACPI_CHECK_OK (AcpiReadBitRegister, Status);

    Status = AcpiReadBitRegister (ACPI_BITREG_GLOBAL_LOCK_ENABLE, &Value);
    ACPI_CHECK_OK (AcpiReadBitRegister, Status);

    Status = AcpiReadBitRegister (ACPI_BITREG_SLEEP_ENABLE, &Value);
    ACPI_CHECK_OK (AcpiReadBitRegister, Status);

    Status = AcpiReadBitRegister (ACPI_BITREG_ARB_DISABLE, &Value);
    ACPI_CHECK_OK (AcpiReadBitRegister, Status);

#endif /* !ACPI_REDUCED_HARDWARE */
}


/******************************************************************************
 *
 * FUNCTION:    AeTestSleepData
 *
 * DESCRIPTION: Exercise the sleep/wake support (_S0, _S1, etc.)
 *
 *****************************************************************************/

static void
AeTestSleepData (
    void)
{
    int                     State;
    UINT8                   TypeA;
    UINT8                   TypeB;
    ACPI_STATUS             Status;


    /* Attempt to get sleep data for all known sleep states */

    for (State = ACPI_STATE_S0; State <= ACPI_S_STATES_MAX; State++)
    {
        Status = AcpiGetSleepTypeData ((UINT8) State, &TypeA, &TypeB);

        /* All sleep methods are optional */

        if (Status != AE_NOT_FOUND)
        {
            ACPI_CHECK_OK (AcpiGetSleepTypeData, Status);
        }
    }
}


/******************************************************************************
 *
 * FUNCTION:    AeMiscellaneousTests
 *
 * DESCRIPTION: Various ACPICA validation tests.
 *
 *****************************************************************************/

void
AeMiscellaneousTests (
    void)
{
    ACPI_BUFFER             ReturnBuf;
    char                    Buffer[32];
    ACPI_STATUS             Status;
    ACPI_STATISTICS         Stats;
    ACPI_HANDLE             Handle;

#if (!ACPI_REDUCED_HARDWARE)
    UINT32                  LockHandle1;
    UINT32                  LockHandle2;
    ACPI_VENDOR_UUID        Uuid =
        {0, {ACPI_INIT_UUID (0,0,0,0,0,0,0,0,0,0,0)}};
#endif /* !ACPI_REDUCED_HARDWARE */


    Status = AcpiGetHandle (NULL, "\\", &Handle);
    ACPI_CHECK_OK (AcpiGetHandle, Status);

    if (AcpiGbl_DoInterfaceTests)
    {
        /*
         * Tests for AcpiLoadTable and AcpiUnloadParentTable
         */

        /* Attempt unload of DSDT, should fail */

        Status = AcpiGetHandle (NULL, "\\_SB_", &Handle);
        ACPI_CHECK_OK (AcpiGetHandle, Status);

        Status = AcpiUnloadParentTable (Handle);
        ACPI_CHECK_STATUS (AcpiUnloadParentTable, Status, AE_TYPE);

        /* Load and unload SSDT4 */

        Status = AcpiLoadTable ((ACPI_TABLE_HEADER *) Ssdt4Code);
        ACPI_CHECK_OK (AcpiLoadTable, Status);

        Status = AcpiGetHandle (NULL, "\\_T96", &Handle);
        ACPI_CHECK_OK (AcpiGetHandle, Status);

        Status = AcpiUnloadParentTable (Handle);
        ACPI_CHECK_OK (AcpiUnloadParentTable, Status);

        /* Re-load SSDT4 */

        Status = AcpiLoadTable ((ACPI_TABLE_HEADER *) Ssdt4Code);
        ACPI_CHECK_OK (AcpiLoadTable, Status);

        /* Unload and re-load SSDT2 (SSDT2 is in the XSDT) */

        Status = AcpiGetHandle (NULL, "\\_T99", &Handle);
        ACPI_CHECK_OK (AcpiGetHandle, Status);

        Status = AcpiUnloadParentTable (Handle);
        ACPI_CHECK_OK (AcpiUnloadParentTable, Status);

        Status = AcpiLoadTable ((ACPI_TABLE_HEADER *) Ssdt2Code);
        ACPI_CHECK_OK (AcpiLoadTable, Status);

        /* Load OEM9 table (causes table override) */

        Status = AcpiLoadTable ((ACPI_TABLE_HEADER *) Ssdt3Code);
        ACPI_CHECK_OK (AcpiLoadTable, Status);
    }

    AeHardwareInterfaces ();
    AeGenericRegisters ();
    AeSetupConfiguration (Ssdt3Code);

    AeTestBufferArgument();
    AeTestPackageArgument ();
    AeMutexInterfaces ();
    AeTestSleepData ();

    /* Test _OSI install/remove */

    Status = AcpiInstallInterface ("");
    ACPI_CHECK_STATUS (AcpiInstallInterface, Status, AE_BAD_PARAMETER);

    Status = AcpiInstallInterface ("TestString");
    ACPI_CHECK_OK (AcpiInstallInterface, Status);

    Status = AcpiInstallInterface ("TestString");
    ACPI_CHECK_STATUS (AcpiInstallInterface, Status, AE_ALREADY_EXISTS);

    Status = AcpiRemoveInterface ("Windows 2006");
    ACPI_CHECK_OK (AcpiRemoveInterface, Status);

    Status = AcpiRemoveInterface ("TestString");
    ACPI_CHECK_OK (AcpiRemoveInterface, Status);

    Status = AcpiRemoveInterface ("XXXXXX");
    ACPI_CHECK_STATUS (AcpiRemoveInterface, Status, AE_NOT_EXIST);

    Status = AcpiInstallInterface ("AnotherTestString");
    ACPI_CHECK_OK (AcpiInstallInterface, Status);

    /* Test _OSI execution */

    Status = ExecuteOSI ("Extended Address Space Descriptor", 0xFFFFFFFF);
    ACPI_CHECK_OK (ExecuteOSI, Status);

    Status = ExecuteOSI ("Windows 2001", 0xFFFFFFFF);
    ACPI_CHECK_OK (ExecuteOSI, Status);

    Status = ExecuteOSI ("MichiganTerminalSystem", 0);
    ACPI_CHECK_OK (ExecuteOSI, Status);


    ReturnBuf.Length = 32;
    ReturnBuf.Pointer = Buffer;

    Status = AcpiGetName (ACPI_ROOT_OBJECT,
        ACPI_FULL_PATHNAME_NO_TRAILING, &ReturnBuf);
    ACPI_CHECK_OK (AcpiGetName, Status);

    /* Get Devices */

    Status = AcpiGetDevices (NULL, AeGetDevices, NULL, NULL);
    ACPI_CHECK_OK (AcpiGetDevices, Status);

    Status = AcpiGetStatistics (&Stats);
    ACPI_CHECK_OK (AcpiGetStatistics, Status);


#if (!ACPI_REDUCED_HARDWARE)

    Status = AcpiInstallGlobalEventHandler (AeGlobalEventHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGlobalEventHandler, Status);

    /* If Hardware Reduced flag is set, we are all done */

    if (AcpiGbl_ReducedHardware)
    {
        return;
    }

    Status = AcpiEnableEvent (ACPI_EVENT_GLOBAL, 0);
    ACPI_CHECK_OK (AcpiEnableEvent, Status);

    /*
     * GPEs: Handlers, enable/disable, etc.
     */
    Status = AcpiInstallGpeHandler (NULL, 0,
        ACPI_GPE_LEVEL_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiEnableGpe (NULL, 0);
    ACPI_CHECK_OK (AcpiEnableGpe, Status);

    Status = AcpiRemoveGpeHandler (NULL, 0, AeGpeHandler);
    ACPI_CHECK_OK (AcpiRemoveGpeHandler, Status);

    Status = AcpiInstallGpeHandler (NULL, 0,
        ACPI_GPE_LEVEL_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiEnableGpe (NULL, 0);
    ACPI_CHECK_OK (AcpiEnableGpe, Status);

    Status = AcpiSetGpe (NULL, 0, ACPI_GPE_DISABLE);
    ACPI_CHECK_OK (AcpiSetGpe, Status);

    Status = AcpiSetGpe (NULL, 0, ACPI_GPE_ENABLE);
    ACPI_CHECK_OK (AcpiSetGpe, Status);


    Status = AcpiInstallGpeHandler (NULL, 1,
        ACPI_GPE_EDGE_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiEnableGpe (NULL, 1);
    ACPI_CHECK_OK (AcpiEnableGpe, Status);


    Status = AcpiInstallGpeHandler (NULL, 2,
        ACPI_GPE_LEVEL_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiEnableGpe (NULL, 2);
    ACPI_CHECK_OK (AcpiEnableGpe, Status);


    Status = AcpiInstallGpeHandler (NULL, 3,
        ACPI_GPE_EDGE_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiInstallGpeHandler (NULL, 4,
        ACPI_GPE_LEVEL_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiInstallGpeHandler (NULL, 5,
        ACPI_GPE_EDGE_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiGetHandle (NULL, "\\_SB", &Handle);
    ACPI_CHECK_OK (AcpiGetHandle, Status);

    Status = AcpiSetupGpeForWake (Handle, NULL, 5);
    ACPI_CHECK_OK (AcpiSetupGpeForWake, Status);

    Status = AcpiSetGpeWakeMask (NULL, 5, ACPI_GPE_ENABLE);
    ACPI_CHECK_OK (AcpiSetGpeWakeMask, Status);

    Status = AcpiSetupGpeForWake (Handle, NULL, 6);
    ACPI_CHECK_OK (AcpiSetupGpeForWake, Status);

    Status = AcpiSetupGpeForWake (ACPI_ROOT_OBJECT, NULL, 6);
    ACPI_CHECK_OK (AcpiSetupGpeForWake, Status);

    Status = AcpiSetupGpeForWake (Handle, NULL, 9);
    ACPI_CHECK_OK (AcpiSetupGpeForWake, Status);

    Status = AcpiInstallGpeHandler (NULL, 0x19,
        ACPI_GPE_LEVEL_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiEnableGpe (NULL, 0x19);
    ACPI_CHECK_OK (AcpiEnableGpe, Status);


    /* GPE block 1 */

    Status = AcpiInstallGpeHandler (NULL, 101,
        ACPI_GPE_LEVEL_TRIGGERED, AeGpeHandler, NULL);
    ACPI_CHECK_OK (AcpiInstallGpeHandler, Status);

    Status = AcpiEnableGpe (NULL, 101);
    ACPI_CHECK_OK (AcpiEnableGpe, Status);

    Status = AcpiDisableGpe (NULL, 101);
    ACPI_CHECK_OK (AcpiDisableGpe, Status);

    AfInstallGpeBlock ();

    /* Here is where the GPEs are actually "enabled" */

    Status = AcpiUpdateAllGpes ();
    ACPI_CHECK_OK (AcpiUpdateAllGpes, Status);

    Status = AcpiGetHandle (NULL, "RSRC", &Handle);
    if (ACPI_SUCCESS (Status))
    {
        ReturnBuf.Length = ACPI_ALLOCATE_BUFFER;

        Status = AcpiGetVendorResource (Handle, "_CRS", &Uuid, &ReturnBuf);
        if (ACPI_SUCCESS (Status))
        {
            AcpiOsFree (ReturnBuf.Pointer);
        }
    }

    /* Test global lock */

    Status = AcpiAcquireGlobalLock (0xFFFF, &LockHandle1);
    ACPI_CHECK_OK (AcpiAcquireGlobalLock, Status);

    Status = AcpiAcquireGlobalLock (0x5, &LockHandle2);
    ACPI_CHECK_OK (AcpiAcquireGlobalLock, Status);

    Status = AcpiReleaseGlobalLock (LockHandle1);
    ACPI_CHECK_OK (AcpiReleaseGlobalLock, Status);

    Status = AcpiReleaseGlobalLock (LockHandle2);
    ACPI_CHECK_OK (AcpiReleaseGlobalLock, Status);

#endif /* !ACPI_REDUCED_HARDWARE */
}
