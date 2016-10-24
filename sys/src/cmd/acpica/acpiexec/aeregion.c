/******************************************************************************
 *
 * Module Name: aeregion - Operation region support for acpiexec
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
        ACPI_MODULE_NAME    ("aeregion")


/* Local prototypes */

static ACPI_STATUS
AeRegionInit (
    ACPI_HANDLE             RegionHandle,
    UINT32                  Function,
    void                    *HandlerContext,
    void                    **RegionContext);

static ACPI_STATUS
AeInstallEcHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);

static ACPI_STATUS
AeInstallPciHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue);


static AE_DEBUG_REGIONS     AeRegions;
BOOLEAN                     AcpiGbl_DisplayRegionAccess = FALSE;
ACPI_CONNECTION_INFO        AeMyContext;


/*
 * We will override some of the default region handlers, especially
 * the SystemMemory handler, which must be implemented locally.
 * These handlers are installed "early" - before any _REG methods
 * are executed - since they are special in the sense that the ACPI spec
 * declares that they must "always be available". Cannot override the
 * DataTable region handler either -- needed for test execution.
 *
 * NOTE: The local region handler will simulate access to these address
 * spaces by creating a memory buffer behind each operation region.
 */
static ACPI_ADR_SPACE_TYPE  DefaultSpaceIdList[] =
{
    ACPI_ADR_SPACE_SYSTEM_MEMORY,
    ACPI_ADR_SPACE_SYSTEM_IO,
    ACPI_ADR_SPACE_PCI_CONFIG,
    ACPI_ADR_SPACE_EC
};

/*
 * We will install handlers for some of the various address space IDs.
 * Test one user-defined address space (used by aslts).
 */
#define ACPI_ADR_SPACE_USER_DEFINED1        0x80
#define ACPI_ADR_SPACE_USER_DEFINED2        0xE4

static ACPI_ADR_SPACE_TYPE  SpaceIdList[] =
{
    ACPI_ADR_SPACE_SMBUS,
    ACPI_ADR_SPACE_CMOS,
    ACPI_ADR_SPACE_PCI_BAR_TARGET,
    ACPI_ADR_SPACE_IPMI,
    ACPI_ADR_SPACE_GPIO,
    ACPI_ADR_SPACE_GSBUS,
    ACPI_ADR_SPACE_FIXED_HARDWARE,
    ACPI_ADR_SPACE_USER_DEFINED1,
    ACPI_ADR_SPACE_USER_DEFINED2
};


/******************************************************************************
 *
 * FUNCTION:    AeRegionInit
 *
 * PARAMETERS:  Region init handler
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Opregion init function.
 *
 *****************************************************************************/

static ACPI_STATUS
AeRegionInit (
    ACPI_HANDLE                 RegionHandle,
    UINT32                      Function,
    void                        *HandlerContext,
    void                        **RegionContext)
{

    if (Function == ACPI_REGION_DEACTIVATE)
    {
        *RegionContext = NULL;
    }
    else
    {
        *RegionContext = RegionHandle;
    }

    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AeOverrideRegionHandlers
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Override the default region handlers for memory, i/o, and
 *              pci_config. Also install a handler for EC. This is part of
 *              the "install early handlers" functionality.
 *
 *****************************************************************************/

void
AeOverrideRegionHandlers (
    void)
{
    UINT32                  i;
    ACPI_STATUS             Status;

    /*
     * Install handlers that will override the default handlers for some of
     * the space IDs.
     */
    for (i = 0; i < ACPI_ARRAY_LENGTH (DefaultSpaceIdList); i++)
    {
        /* Install handler at the root object */

        Status = AcpiInstallAddressSpaceHandler (ACPI_ROOT_OBJECT,
            DefaultSpaceIdList[i], AeRegionHandler,
            AeRegionInit, &AeMyContext);

        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not install an OpRegion handler for %s space(%u)",
                AcpiUtGetRegionName ((UINT8) DefaultSpaceIdList[i]),
                DefaultSpaceIdList[i]));
        }
    }
}


/******************************************************************************
 *
 * FUNCTION:    AeInstallRegionHandlers
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Install handlers for the address spaces other than memory,
 *              i/o, and pci_config.
 *
 *****************************************************************************/

void
AeInstallRegionHandlers (
    void)
{
    UINT32                  i;
    ACPI_STATUS             Status;

    /*
     * Install handlers for some of the "device driver" address spaces
     * such as SMBus, etc.
     */
    for (i = 0; i < ACPI_ARRAY_LENGTH (SpaceIdList); i++)
    {
        /* Install handler at the root object */

        Status = AcpiInstallAddressSpaceHandler (ACPI_ROOT_OBJECT,
            SpaceIdList[i], AeRegionHandler,
            AeRegionInit, &AeMyContext);

        if (ACPI_FAILURE (Status))
        {
            ACPI_EXCEPTION ((AE_INFO, Status,
                "Could not install an OpRegion handler for %s space(%u)",
                AcpiUtGetRegionName((UINT8) SpaceIdList[i]), SpaceIdList[i]));
            return;
        }
    }
}


/*******************************************************************************
 *
 * FUNCTION:    AeInstallDeviceHandlers,
 *              AeInstallEcHandler,
 *              AeInstallPciHandler
 *
 * PARAMETERS:  ACPI_WALK_NAMESPACE callback
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Walk entire namespace, install a handler for every EC
 *              and PCI device found.
 *
 ******************************************************************************/

static ACPI_STATUS
AeInstallEcHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_STATUS             Status;


    /* Install the handler for this EC device */

    Status = AcpiInstallAddressSpaceHandler (ObjHandle, ACPI_ADR_SPACE_EC,
        AeRegionHandler, AeRegionInit, &AeMyContext);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not install an OpRegion handler for EC device (%p)",
            ObjHandle));
    }

    return (Status);
}


static ACPI_STATUS
AeInstallPciHandler (
    ACPI_HANDLE             ObjHandle,
    UINT32                  Level,
    void                    *Context,
    void                    **ReturnValue)
{
    ACPI_STATUS             Status;


    /* Install memory and I/O handlers for the PCI device */

    Status = AcpiInstallAddressSpaceHandler (ObjHandle, ACPI_ADR_SPACE_SYSTEM_IO,
        AeRegionHandler, AeRegionInit, &AeMyContext);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not install an OpRegion handler for PCI device (%p)",
            ObjHandle));
    }

    Status = AcpiInstallAddressSpaceHandler (ObjHandle, ACPI_ADR_SPACE_SYSTEM_MEMORY,
        AeRegionHandler, AeRegionInit, &AeMyContext);
    if (ACPI_FAILURE (Status))
    {
        ACPI_EXCEPTION ((AE_INFO, Status,
            "Could not install an OpRegion handler for PCI device (%p)",
            ObjHandle));
    }

    return (AE_CTRL_TERMINATE);
}


ACPI_STATUS
AeInstallDeviceHandlers (
    void)
{

    /* Find all Embedded Controller devices */

    AcpiGetDevices ("PNP0C09", AeInstallEcHandler, NULL, NULL);

    /* Install a PCI handler */

    AcpiGetDevices ("PNP0A08", AeInstallPciHandler, NULL, NULL);
    return (AE_OK);
}


/******************************************************************************
 *
 * FUNCTION:    AeRegionHandler
 *
 * PARAMETERS:  Standard region handler parameters
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Test handler - Handles some dummy regions via memory that can
 *              be manipulated in Ring 3. Simulates actual reads and writes.
 *
 *****************************************************************************/

ACPI_STATUS
AeRegionHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext)
{

    ACPI_OPERAND_OBJECT     *RegionObject = ACPI_CAST_PTR (ACPI_OPERAND_OBJECT, RegionContext);
    UINT8                   *Buffer = ACPI_CAST_PTR (UINT8, Value);
    UINT8                   *OldBuffer;
    UINT8                   *NewBuffer;
    ACPI_PHYSICAL_ADDRESS   BaseAddress;
    ACPI_PHYSICAL_ADDRESS   BaseAddressEnd;
    ACPI_PHYSICAL_ADDRESS   RegionAddress;
    ACPI_PHYSICAL_ADDRESS   RegionAddressEnd;
    ACPI_SIZE               Length;
    BOOLEAN                 BufferExists;
    BOOLEAN                 BufferResize;
    AE_REGION               *RegionElement;
    void                    *BufferValue;
    ACPI_STATUS             Status;
    UINT32                  ByteWidth;
    UINT32                  RegionLength;
    UINT32                  i;
    UINT8                   SpaceId;
    ACPI_CONNECTION_INFO    *MyContext;
    UINT32                  Value1;
    UINT32                  Value2;
    ACPI_RESOURCE           *Resource;


    ACPI_FUNCTION_NAME (AeRegionHandler);

    /*
     * If the object is not a region, simply return
     */
    if (RegionObject->Region.Type != ACPI_TYPE_REGION)
    {
        return (AE_OK);
    }

    /* Check that we actually got back our context parameter */

    if (HandlerContext != &AeMyContext)
    {
        printf ("Region handler received incorrect context %p, should be %p\n",
            HandlerContext, &AeMyContext);
    }

    MyContext = ACPI_CAST_PTR (ACPI_CONNECTION_INFO, HandlerContext);

    /*
     * Find the region's address space and length before searching
     * the linked list.
     */
    BaseAddress = RegionObject->Region.Address;
    Length = (ACPI_SIZE) RegionObject->Region.Length;
    SpaceId = RegionObject->Region.SpaceId;

    ACPI_DEBUG_PRINT ((ACPI_DB_OPREGION,
        "Operation Region request on %s at 0x%X\n",
        AcpiUtGetRegionName (RegionObject->Region.SpaceId),
        (UINT32) Address));

    /*
     * Region support can be disabled with the -do option.
     * We use this to support dynamically loaded tables where we pass a valid
     * address to the AML.
     */
    if (AcpiGbl_DbOpt_NoRegionSupport)
    {
        BufferValue = ACPI_TO_POINTER (Address);
        ByteWidth = (BitWidth / 8);

        if (BitWidth % 8)
        {
            ByteWidth += 1;
        }
        goto DoFunction;
    }

    switch (SpaceId)
    {
    case ACPI_ADR_SPACE_SYSTEM_IO:
        /*
         * For I/O space, exercise the port validation
         * Note: ReadPort currently always returns all ones, length=BitLength
         */
        switch (Function & ACPI_IO_MASK)
        {
        case ACPI_READ:

            if (BitWidth == 64)
            {
                /* Split the 64-bit request into two 32-bit requests */

                Status = AcpiHwReadPort (Address, &Value1, 32);
                ACPI_CHECK_OK (AcpiHwReadPort, Status);
                Status = AcpiHwReadPort (Address+4, &Value2, 32);
                ACPI_CHECK_OK (AcpiHwReadPort, Status);

                *Value = Value1 | ((UINT64) Value2 << 32);
            }
            else
            {
                Status = AcpiHwReadPort (Address, &Value1, BitWidth);
                ACPI_CHECK_OK (AcpiHwReadPort, Status);
                *Value = (UINT64) Value1;
            }
            break;

        case ACPI_WRITE:

            if (BitWidth == 64)
            {
                /* Split the 64-bit request into two 32-bit requests */

                Status = AcpiHwWritePort (Address, ACPI_LODWORD (*Value), 32);
                ACPI_CHECK_OK (AcpiHwWritePort, Status);
                Status = AcpiHwWritePort (Address+4, ACPI_HIDWORD (*Value), 32);
                ACPI_CHECK_OK (AcpiHwWritePort, Status);
            }
            else
            {
                Status = AcpiHwWritePort (Address, (UINT32) *Value, BitWidth);
                ACPI_CHECK_OK (AcpiHwWritePort, Status);
            }
            break;

        default:

            Status = AE_BAD_PARAMETER;
            break;
        }

        if (ACPI_FAILURE (Status))
        {
            return (Status);
        }

        /* Now go ahead and simulate the hardware */
        break;

    /*
     * SMBus and GenericSerialBus support the various bidirectional
     * protocols.
     */
    case ACPI_ADR_SPACE_SMBUS:
    case ACPI_ADR_SPACE_GSBUS:  /* ACPI 5.0 */

        Length = 0;

        switch (Function & ACPI_IO_MASK)
        {
        case ACPI_READ:

            switch (Function >> 16)
            {
            case AML_FIELD_ATTRIB_QUICK:

                Length = 0;
                break;

            case AML_FIELD_ATTRIB_SEND_RCV:
            case AML_FIELD_ATTRIB_BYTE:

                Length = 1;
                break;

            case AML_FIELD_ATTRIB_WORD:
            case AML_FIELD_ATTRIB_WORD_CALL:

                Length = 2;
                break;

            case AML_FIELD_ATTRIB_BLOCK:
            case AML_FIELD_ATTRIB_BLOCK_CALL:

                Length = 32;
                break;

            case AML_FIELD_ATTRIB_MULTIBYTE:
            case AML_FIELD_ATTRIB_RAW_BYTES:
            case AML_FIELD_ATTRIB_RAW_PROCESS:

                Length = MyContext->AccessLength;
                break;

            default:

                break;
            }
            break;

        case ACPI_WRITE:

            switch (Function >> 16)
            {
            case AML_FIELD_ATTRIB_QUICK:
            case AML_FIELD_ATTRIB_SEND_RCV:
            case AML_FIELD_ATTRIB_BYTE:
            case AML_FIELD_ATTRIB_WORD:
            case AML_FIELD_ATTRIB_BLOCK:

                Length = 0;
                break;

            case AML_FIELD_ATTRIB_WORD_CALL:
                Length = 2;
                break;

            case AML_FIELD_ATTRIB_BLOCK_CALL:
                Length = 32;
                break;

            case AML_FIELD_ATTRIB_MULTIBYTE:
            case AML_FIELD_ATTRIB_RAW_BYTES:
            case AML_FIELD_ATTRIB_RAW_PROCESS:

                Length = MyContext->AccessLength;
                break;

            default:

                break;
            }
            break;

        default:

            break;
        }

        if (AcpiGbl_DisplayRegionAccess)
        {
            AcpiOsPrintf ("AcpiExec: %s "
                "%s: Attr %X Addr %.4X BaseAddr %.4X Len %.2X Width %X BufLen %X",
                AcpiUtGetRegionName (SpaceId),
                (Function & ACPI_IO_MASK) ? "Write" : "Read ",
                (UINT32) (Function >> 16),
                (UINT32) Address, (UINT32) BaseAddress,
                Length, BitWidth, Buffer[1]);

            /* GenericSerialBus has a Connection() parameter */

            if (SpaceId == ACPI_ADR_SPACE_GSBUS)
            {
                Status = AcpiBufferToResource (MyContext->Connection,
                    MyContext->Length, &Resource);

                AcpiOsPrintf (" [AccLen %.2X Conn %p]",
                    MyContext->AccessLength, MyContext->Connection);
            }
            AcpiOsPrintf ("\n");
        }

        /* Setup the return buffer. Note: ASLTS depends on these fill values */

        for (i = 0; i < Length; i++)
        {
            Buffer[i+2] = (UINT8) (0xA0 + i);
        }

        Buffer[0] = 0x7A;
        Buffer[1] = (UINT8) Length;
        return (AE_OK);


    case ACPI_ADR_SPACE_IPMI: /* ACPI 4.0 */

        if (AcpiGbl_DisplayRegionAccess)
        {
            AcpiOsPrintf ("AcpiExec: IPMI "
                "%s: Attr %X Addr %.4X BaseAddr %.4X Len %.2X Width %X BufLen %X\n",
                (Function & ACPI_IO_MASK) ? "Write" : "Read ",
                (UINT32) (Function >> 16), (UINT32) Address, (UINT32) BaseAddress,
                Length, BitWidth, Buffer[1]);
        }

        /*
         * Regardless of a READ or WRITE, this handler is passed a 66-byte
         * buffer in which to return the IPMI status/length/data.
         *
         * Return some example data to show use of the bidirectional buffer
         */
        Buffer[0] = 0;       /* Status byte */
        Buffer[1] = 64;      /* Return buffer data length */
        Buffer[2] = 0;       /* Completion code */
        Buffer[3] = 0;       /* Reserved */

        /*
         * Fill the 66-byte buffer with the return data.
         * Note: ASLTS depends on these fill values.
         */
        for (i = 4; i < 66; i++)
        {
            Buffer[i] = (UINT8) (i);
        }
        return (AE_OK);

    /*
     * GPIO has some special semantics:
     * 1) Address is the pin number index into the Connection() pin list
     * 2) BitWidth is the actual number of bits (pins) defined by the field
     */
    case ACPI_ADR_SPACE_GPIO: /* ACPI 5.0 */

        if (AcpiGbl_DisplayRegionAccess)
        {
            AcpiOsPrintf ("AcpiExec: GPIO "
                "%s: Addr %.4X Width %X Conn %p\n",
                (Function & ACPI_IO_MASK) ? "Write" : "Read ",
                (UINT32) Address, BitWidth, MyContext->Connection);
        }
        return (AE_OK);

    default:
        break;
    }

    /*
     * Search through the linked list for this region's buffer
     */
    BufferExists = FALSE;
    BufferResize = FALSE;
    RegionElement = AeRegions.RegionList;

    if (AeRegions.NumberOfRegions)
    {
        BaseAddressEnd = BaseAddress + Length - 1;
        while (!BufferExists && RegionElement)
        {
            RegionAddress = RegionElement->Address;
            RegionAddressEnd = RegionElement->Address + RegionElement->Length - 1;
            RegionLength = RegionElement->Length;

            /*
             * Overlapping Region Support
             *
             * While searching through the region buffer list, determine if an
             * overlap exists between the requested buffer space and the current
             * RegionElement space. If there is an overlap then replace the old
             * buffer with a new buffer of increased size before continuing to
             * do the read or write
             */
            if (RegionElement->SpaceId != SpaceId ||
                BaseAddressEnd < RegionAddress ||
                BaseAddress > RegionAddressEnd)
            {
                /*
                 * Requested buffer is outside of the current RegionElement
                 * bounds
                 */
                RegionElement = RegionElement->NextRegion;
            }
            else
            {
                /*
                 * Some amount of buffer space sharing exists. There are 4 cases
                 * to consider:
                 *
                 * 1. Right overlap
                 * 2. Left overlap
                 * 3. Left and right overlap
                 * 4. Fully contained - no resizing required
                 */
                BufferExists = TRUE;

                if ((BaseAddress >= RegionAddress) &&
                    (BaseAddress <= RegionAddressEnd) &&
                    (BaseAddressEnd > RegionAddressEnd))
                {
                    /* Right overlap */

                    RegionElement->Length = (UINT32) (BaseAddress -
                        RegionAddress + Length);
                    BufferResize = TRUE;
                }

                else if ((BaseAddressEnd >= RegionAddress) &&
                         (BaseAddressEnd <= RegionAddressEnd) &&
                         (BaseAddress < RegionAddress))
                {
                    /* Left overlap */

                    RegionElement->Address = BaseAddress;
                    RegionElement->Length = (UINT32) (RegionAddress -
                        BaseAddress + RegionElement->Length);
                    BufferResize = TRUE;
                }

                else if ((BaseAddress < RegionAddress) &&
                         (BaseAddressEnd > RegionAddressEnd))
                {
                    /* Left and right overlap */

                    RegionElement->Address = BaseAddress;
                    RegionElement->Length = Length;
                    BufferResize = TRUE;
                }

                /*
                 * only remaining case is fully contained for which we don't
                 * need to do anything
                 */
                if (BufferResize)
                {
                    NewBuffer = AcpiOsAllocate (RegionElement->Length);
                    if (!NewBuffer)
                    {
                        return (AE_NO_MEMORY);
                    }

                    OldBuffer = RegionElement->Buffer;
                    RegionElement->Buffer = NewBuffer;
                    NewBuffer = NULL;

                    /* Initialize the region with the default fill value */

                    memset (RegionElement->Buffer,
                        AcpiGbl_RegionFillValue, RegionElement->Length);

                    /*
                     * Get BufferValue to point (within the new buffer) to the
                     * base address of the old buffer
                     */
                    BufferValue = (UINT8 *) RegionElement->Buffer +
                        (UINT64) RegionAddress -
                        (UINT64) RegionElement->Address;

                    /*
                     * Copy the old buffer to its same location within the new
                     * buffer
                     */
                    memcpy (BufferValue, OldBuffer, RegionLength);
                    AcpiOsFree (OldBuffer);
                }
            }
        }
    }

    /*
     * If the Region buffer does not exist, create it now
     */
    if (!BufferExists)
    {
        /* Do the memory allocations first */

        RegionElement = AcpiOsAllocate (sizeof (AE_REGION));
        if (!RegionElement)
        {
            return (AE_NO_MEMORY);
        }

        RegionElement->Buffer = AcpiOsAllocate (Length);
        if (!RegionElement->Buffer)
        {
            AcpiOsFree (RegionElement);
            return (AE_NO_MEMORY);
        }

        /* Initialize the region with the default fill value */

        memset (RegionElement->Buffer, AcpiGbl_RegionFillValue, Length);

        RegionElement->Address      = BaseAddress;
        RegionElement->Length       = Length;
        RegionElement->SpaceId      = SpaceId;
        RegionElement->NextRegion   = NULL;

        /*
         * Increment the number of regions and put this one
         * at the head of the list as it will probably get accessed
         * more often anyway.
         */
        AeRegions.NumberOfRegions += 1;

        if (AeRegions.RegionList)
        {
            RegionElement->NextRegion = AeRegions.RegionList;
        }

        AeRegions.RegionList = RegionElement;
    }

    /* Calculate the size of the memory copy */

    ByteWidth = (BitWidth / 8);

    if (BitWidth % 8)
    {
        ByteWidth += 1;
    }

    /*
     * The buffer exists and is pointed to by RegionElement.
     * We now need to verify the request is valid and perform the operation.
     *
     * NOTE: RegionElement->Length is in bytes, therefore it we compare against
     * ByteWidth (see above)
     */
    if ((RegionObject->Region.SpaceId != ACPI_ADR_SPACE_GPIO) &&
        ((UINT64) Address + ByteWidth) >
        ((UINT64)(RegionElement->Address) + RegionElement->Length))
    {
        ACPI_WARNING ((AE_INFO,
            "Request on [%4.4s] is beyond region limit "
            "Req-0x%X+0x%X, Base=0x%X, Len-0x%X",
            (RegionObject->Region.Node)->Name.Ascii, (UINT32) Address,
            ByteWidth, (UINT32)(RegionElement->Address),
            RegionElement->Length));

        return (AE_AML_REGION_LIMIT);
    }

    /*
     * Get BufferValue to point to the "address" in the buffer
     */
    BufferValue = ((UINT8 *) RegionElement->Buffer +
        ((UINT64) Address - (UINT64) RegionElement->Address));

DoFunction:
    /*
     * Perform a read or write to the buffer space
     */
    switch (Function)
    {
    case ACPI_READ:
        /*
         * Set the pointer Value to whatever is in the buffer
         */
        memcpy (Value, BufferValue, ByteWidth);
        break;

    case ACPI_WRITE:
        /*
         * Write the contents of Value to the buffer
         */
        memcpy (BufferValue, Value, ByteWidth);
        break;

    default:

        return (AE_BAD_PARAMETER);
    }

    if (AcpiGbl_DisplayRegionAccess)
    {
        switch (SpaceId)
        {
        case ACPI_ADR_SPACE_SYSTEM_MEMORY:

            AcpiOsPrintf ("AcpiExec: SystemMemory "
                "%s: Val %.8X Addr %.4X Width %X [REGION: BaseAddr %.4X Len %.2X]\n",
                (Function & ACPI_IO_MASK) ? "Write" : "Read ",
                (UINT32) *Value, (UINT32) Address, BitWidth, (UINT32) BaseAddress, Length);
            break;

        case ACPI_ADR_SPACE_GPIO:   /* ACPI 5.0 */

            /* This space is required to always be ByteAcc */

            Status = AcpiBufferToResource (MyContext->Connection,
                MyContext->Length, &Resource);

            AcpiOsPrintf ("AcpiExec: GeneralPurposeIo "
                "%s: Val %.8X Addr %.4X BaseAddr %.4X Len %.2X Width %X AccLen %.2X Conn %p\n",
                (Function & ACPI_IO_MASK) ? "Write" : "Read ", (UINT32) *Value,
                (UINT32) Address, (UINT32) BaseAddress, Length, BitWidth,
                MyContext->AccessLength, MyContext->Connection);
            break;

        default:

            break;
        }
    }

    return (AE_OK);
}
