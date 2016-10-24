/******************************************************************************
 *
 * Module Name: aeinitfile - Support for optional initialization file
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
#include "acdispat.h"

#define _COMPONENT          ACPI_TOOLS
        ACPI_MODULE_NAME    ("aeinitfile")


/* Local prototypes */

static void
AeDoOneOverride (
    char                    *Pathname,
    char                    *ValueString,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState);


#define AE_FILE_BUFFER_SIZE     512

static char                 LineBuffer[AE_FILE_BUFFER_SIZE];
static char                 NameBuffer[AE_FILE_BUFFER_SIZE];
static char                 ValueBuffer[AE_FILE_BUFFER_SIZE];
static FILE                 *InitFile;


/******************************************************************************
 *
 * FUNCTION:    AeOpenInitializationFile
 *
 * PARAMETERS:  Filename            - Path to the init file
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Open the initialization file for the -fi option
 *
 *****************************************************************************/

int
AeOpenInitializationFile (
    char                    *Filename)
{

    InitFile = fopen (Filename, "r");
    if (!InitFile)
    {
        perror ("Could not open initialization file");
        return (-1);
    }

    AcpiOsPrintf ("Opened initialization file [%s]\n", Filename);
    return (0);
}


/******************************************************************************
 *
 * FUNCTION:    AeDoObjectOverrides
 *
 * PARAMETERS:  None
 *
 * RETURN:      None
 *
 * DESCRIPTION: Read the initialization file and perform all overrides
 *
 * NOTE:        The format of the file is multiple lines, each of format:
 *                  <ACPI-pathname> <Integer Value>
 *
 *****************************************************************************/

void
AeDoObjectOverrides (
    void)
{
    ACPI_OPERAND_OBJECT     *ObjDesc;
    ACPI_WALK_STATE         *WalkState;
    int                     i;


    if (!InitFile)
    {
        return;
    }

    /* Create needed objects to be reused for each init entry */

    ObjDesc = AcpiUtCreateIntegerObject (0);
    WalkState = AcpiDsCreateWalkState (0, NULL, NULL, NULL);
    NameBuffer[0] = '\\';

    /* Read the entire file line-by-line */

    while (fgets (LineBuffer, AE_FILE_BUFFER_SIZE, InitFile) != NULL)
    {
        if (sscanf (LineBuffer, "%s %s\n",
                &NameBuffer[1], ValueBuffer) != 2)
        {
            goto CleanupAndExit;
        }

        /* Add a root prefix if not present in the string */

        i = 0;
        if (NameBuffer[1] == '\\')
        {
            i = 1;
        }

        AeDoOneOverride (&NameBuffer[i], ValueBuffer, ObjDesc, WalkState);
    }

    /* Cleanup */

CleanupAndExit:
    fclose (InitFile);
    AcpiDsDeleteWalkState (WalkState);
    AcpiUtRemoveReference (ObjDesc);
}


/******************************************************************************
 *
 * FUNCTION:    AeDoOneOverride
 *
 * PARAMETERS:  Pathname            - AML namepath
 *              ValueString         - New integer value to be stored
 *              ObjDesc             - Descriptor with integer override value
 *              WalkState           - Used for the Store operation
 *
 * RETURN:      None
 *
 * DESCRIPTION: Perform an overrided for a single namespace object
 *
 *****************************************************************************/

static void
AeDoOneOverride (
    char                    *Pathname,
    char                    *ValueString,
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_HANDLE             Handle;
    ACPI_STATUS             Status;
    UINT64                  Value;


    AcpiOsPrintf ("Value Override: %s, ", Pathname);

    /*
     * Get the namespace node associated with the override
     * pathname from the init file.
     */
    Status = AcpiGetHandle (NULL, Pathname, &Handle);
    if (ACPI_FAILURE (Status))
    {
        AcpiOsPrintf ("%s\n", AcpiFormatException (Status));
        return;
    }

    /* Extract the 64-bit integer */

    Status = AcpiUtStrtoul64 (ValueString, ACPI_ANY_BASE,
        ACPI_MAX64_BYTE_WIDTH, &Value);
    if (ACPI_FAILURE (Status))
    {
        AcpiOsPrintf ("%s %s\n", ValueString,
            AcpiFormatException (Status));
        return;
    }

    ObjDesc->Integer.Value = Value;

    /*
     * At the point this function is called, the namespace is fully
     * built and initialized. We can simply store the new object to
     * the target node.
     */
    AcpiExEnterInterpreter ();
    Status = AcpiExStore (ObjDesc, Handle, WalkState);
    AcpiExExitInterpreter ();

    if (ACPI_FAILURE (Status))
    {
        AcpiOsPrintf ("%s\n", AcpiFormatException (Status));
        return;
    }

    AcpiOsPrintf ("New value: 0x%8.8X%8.8X\n",
        ACPI_FORMAT_UINT64 (Value));
}
