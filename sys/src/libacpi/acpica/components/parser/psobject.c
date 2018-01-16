/******************************************************************************
 *
 * Module Name: psobject - Support for parse objects
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

#include "acpi.h"
#include "accommon.h"
#include "acparser.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_PARSER
        ACPI_MODULE_NAME    ("psobject")


/* Local prototypes */

static ACPI_STATUS
AcpiPsGetAmlOpcode (
    ACPI_WALK_STATE         *WalkState);


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsGetAmlOpcode
 *
 * PARAMETERS:  WalkState           - Current state
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Extract the next AML opcode from the input stream.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiPsGetAmlOpcode (
    ACPI_WALK_STATE         *WalkState)
{
    UINT32                  AmlOffset;


    ACPI_FUNCTION_TRACE_PTR (PsGetAmlOpcode, WalkState);


    WalkState->Aml = WalkState->ParserState.Aml;
    WalkState->Opcode = AcpiPsPeekOpcode (&(WalkState->ParserState));

    /*
     * First cut to determine what we have found:
     * 1) A valid AML opcode
     * 2) A name string
     * 3) An unknown/invalid opcode
     */
    WalkState->OpInfo = AcpiPsGetOpcodeInfo (WalkState->Opcode);

    switch (WalkState->OpInfo->Class)
    {
    case AML_CLASS_ASCII:
    case AML_CLASS_PREFIX:
        /*
         * Starts with a valid prefix or ASCII char, this is a name
         * string. Convert the bare name string to a namepath.
         */
        WalkState->Opcode = AML_INT_NAMEPATH_OP;
        WalkState->ArgTypes = ARGP_NAMESTRING;
        break;

    case AML_CLASS_UNKNOWN:

        /* The opcode is unrecognized. Complain and skip unknown opcodes */

        if (WalkState->PassNumber == 2)
        {
            AmlOffset = (UINT32) ACPI_PTR_DIFF (WalkState->Aml,
                WalkState->ParserState.AmlStart);

            ACPI_ERROR ((AE_INFO,
                "Unknown opcode 0x%.2X at table offset 0x%.4X, ignoring",
                WalkState->Opcode,
                (UINT32) (AmlOffset + sizeof (ACPI_TABLE_HEADER))));

            ACPI_DUMP_BUFFER ((WalkState->ParserState.Aml - 16), 48);

#ifdef ACPI_ASL_COMPILER
            /*
             * This is executed for the disassembler only. Output goes
             * to the disassembled ASL output file.
             */
            AcpiOsPrintf (
                "/*\nError: Unknown opcode 0x%.2X at table offset 0x%.4X, context:\n",
                WalkState->Opcode,
                (UINT32) (AmlOffset + sizeof (ACPI_TABLE_HEADER)));

            /* Dump the context surrounding the invalid opcode */

            AcpiUtDumpBuffer (((UINT8 *) WalkState->ParserState.Aml - 16),
                48, DB_BYTE_DISPLAY,
                (AmlOffset + sizeof (ACPI_TABLE_HEADER) - 16));
            AcpiOsPrintf (" */\n");
#endif
        }

        /* Increment past one-byte or two-byte opcode */

        WalkState->ParserState.Aml++;
        if (WalkState->Opcode > 0xFF) /* Can only happen if first byte is 0x5B */
        {
            WalkState->ParserState.Aml++;
        }

        return_ACPI_STATUS (AE_CTRL_PARSE_CONTINUE);

    default:

        /* Found opcode info, this is a normal opcode */

        WalkState->ParserState.Aml +=
            AcpiPsGetOpcodeSize (WalkState->Opcode);
        WalkState->ArgTypes = WalkState->OpInfo->ParseArgs;
        break;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsBuildNamedOp
 *
 * PARAMETERS:  WalkState           - Current state
 *              AmlOpStart          - Begin of named Op in AML
 *              UnnamedOp           - Early Op (not a named Op)
 *              Op                  - Returned Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Parse a named Op
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsBuildNamedOp (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       *UnnamedOp,
    ACPI_PARSE_OBJECT       **Op)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PARSE_OBJECT       *Arg = NULL;


    ACPI_FUNCTION_TRACE_PTR (PsBuildNamedOp, WalkState);


    UnnamedOp->Common.Value.Arg = NULL;
    UnnamedOp->Common.ArgListLength = 0;
    UnnamedOp->Common.AmlOpcode = WalkState->Opcode;

    /*
     * Get and append arguments until we find the node that contains
     * the name (the type ARGP_NAME).
     */
    while (GET_CURRENT_ARG_TYPE (WalkState->ArgTypes) &&
          (GET_CURRENT_ARG_TYPE (WalkState->ArgTypes) != ARGP_NAME))
    {
        Status = AcpiPsGetNextArg (WalkState, &(WalkState->ParserState),
            GET_CURRENT_ARG_TYPE (WalkState->ArgTypes), &Arg);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        AcpiPsAppendArg (UnnamedOp, Arg);
        INCREMENT_ARG_LIST (WalkState->ArgTypes);
    }

    /*
     * Make sure that we found a NAME and didn't run out of arguments
     */
    if (!GET_CURRENT_ARG_TYPE (WalkState->ArgTypes))
    {
        return_ACPI_STATUS (AE_AML_NO_OPERAND);
    }

    /* We know that this arg is a name, move to next arg */

    INCREMENT_ARG_LIST (WalkState->ArgTypes);

    /*
     * Find the object. This will either insert the object into
     * the namespace or simply look it up
     */
    WalkState->Op = NULL;

    Status = WalkState->DescendingCallback (WalkState, Op);
    if (ACPI_FAILURE (Status))
    {
        if (Status != AE_CTRL_TERMINATE)
        {
            ACPI_EXCEPTION ((AE_INFO, Status, "During name lookup/catalog"));
        }
        return_ACPI_STATUS (Status);
    }

    if (!*Op)
    {
        return_ACPI_STATUS (AE_CTRL_PARSE_CONTINUE);
    }

    Status = AcpiPsNextParseState (WalkState, *Op, Status);
    if (ACPI_FAILURE (Status))
    {
        if (Status == AE_CTRL_PENDING)
        {
            Status = AE_CTRL_PARSE_PENDING;
        }
        return_ACPI_STATUS (Status);
    }

    AcpiPsAppendArg (*Op, UnnamedOp->Common.Value.Arg);

    if ((*Op)->Common.AmlOpcode == AML_REGION_OP ||
        (*Op)->Common.AmlOpcode == AML_DATA_REGION_OP)
    {
        /*
         * Defer final parsing of an OperationRegion body, because we don't
         * have enough info in the first pass to parse it correctly (i.e.,
         * there may be method calls within the TermArg elements of the body.)
         *
         * However, we must continue parsing because the opregion is not a
         * standalone package -- we don't know where the end is at this point.
         *
         * (Length is unknown until parse of the body complete)
         */
        (*Op)->Named.Data = AmlOpStart;
        (*Op)->Named.Length = 0;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCreateOp
 *
 * PARAMETERS:  WalkState           - Current state
 *              AmlOpStart          - Op start in AML
 *              NewOp               - Returned Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Get Op from AML
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsCreateOp (
    ACPI_WALK_STATE         *WalkState,
    UINT8                   *AmlOpStart,
    ACPI_PARSE_OBJECT       **NewOp)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_PARSE_OBJECT       *Op;
    ACPI_PARSE_OBJECT       *NamedOp = NULL;
    ACPI_PARSE_OBJECT       *ParentScope;
    UINT8                   ArgumentCount;
    const ACPI_OPCODE_INFO  *OpInfo;


    ACPI_FUNCTION_TRACE_PTR (PsCreateOp, WalkState);


    Status = AcpiPsGetAmlOpcode (WalkState);
    if (Status == AE_CTRL_PARSE_CONTINUE)
    {
        return_ACPI_STATUS (AE_CTRL_PARSE_CONTINUE);
    }

    /* Create Op structure and append to parent's argument list */

    WalkState->OpInfo = AcpiPsGetOpcodeInfo (WalkState->Opcode);
    Op = AcpiPsAllocOp (WalkState->Opcode, AmlOpStart);
    if (!Op)
    {
        return_ACPI_STATUS (AE_NO_MEMORY);
    }

    if (WalkState->OpInfo->Flags & AML_NAMED)
    {
        Status = AcpiPsBuildNamedOp (WalkState, AmlOpStart, Op, &NamedOp);
        AcpiPsFreeOp (Op);
        if (ACPI_FAILURE (Status))
        {
            return_ACPI_STATUS (Status);
        }

        *NewOp = NamedOp;
        return_ACPI_STATUS (AE_OK);
    }

    /* Not a named opcode, just allocate Op and append to parent */

    if (WalkState->OpInfo->Flags & AML_CREATE)
    {
        /*
         * Backup to beginning of CreateXXXfield declaration
         * BodyLength is unknown until we parse the body
         */
        Op->Named.Data = AmlOpStart;
        Op->Named.Length = 0;
    }

    if (WalkState->Opcode == AML_BANK_FIELD_OP)
    {
        /*
         * Backup to beginning of BankField declaration
         * BodyLength is unknown until we parse the body
         */
        Op->Named.Data = AmlOpStart;
        Op->Named.Length = 0;
    }

    ParentScope = AcpiPsGetParentScope (&(WalkState->ParserState));
    AcpiPsAppendArg (ParentScope, Op);

    if (ParentScope)
    {
        OpInfo = AcpiPsGetOpcodeInfo (ParentScope->Common.AmlOpcode);
        if (OpInfo->Flags & AML_HAS_TARGET)
        {
            ArgumentCount = AcpiPsGetArgumentCount (OpInfo->Type);
            if (ParentScope->Common.ArgListLength > ArgumentCount)
            {
                Op->Common.Flags |= ACPI_PARSEOP_TARGET;
            }
        }
        else if (ParentScope->Common.AmlOpcode == AML_INCREMENT_OP)
        {
            Op->Common.Flags |= ACPI_PARSEOP_TARGET;
        }
    }

    if (WalkState->DescendingCallback != NULL)
    {
        /*
         * Find the object. This will either insert the object into
         * the namespace or simply look it up
         */
        WalkState->Op = *NewOp = Op;

        Status = WalkState->DescendingCallback (WalkState, &Op);
        Status = AcpiPsNextParseState (WalkState, Op, Status);
        if (Status == AE_CTRL_PENDING)
        {
            Status = AE_CTRL_PARSE_PENDING;
        }
    }

    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCompleteOp
 *
 * PARAMETERS:  WalkState           - Current state
 *              Op                  - Returned Op
 *              Status              - Parse status before complete Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Complete Op
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsCompleteOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       **Op,
    ACPI_STATUS             Status)
{
    ACPI_STATUS             Status2;


    ACPI_FUNCTION_TRACE_PTR (PsCompleteOp, WalkState);


    /*
     * Finished one argument of the containing scope
     */
    WalkState->ParserState.Scope->ParseScope.ArgCount--;

    /* Close this Op (will result in parse subtree deletion) */

    Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
    if (ACPI_FAILURE (Status2))
    {
        return_ACPI_STATUS (Status2);
    }

    *Op = NULL;

    switch (Status)
    {
    case AE_OK:

        break;

    case AE_CTRL_TRANSFER:

        /* We are about to transfer to a called method */

        WalkState->PrevOp = NULL;
        WalkState->PrevArgTypes = WalkState->ArgTypes;
        return_ACPI_STATUS (Status);

    case AE_CTRL_END:

        AcpiPsPopScope (&(WalkState->ParserState), Op,
            &WalkState->ArgTypes, &WalkState->ArgCount);

        if (*Op)
        {
            WalkState->Op = *Op;
            WalkState->OpInfo = AcpiPsGetOpcodeInfo ((*Op)->Common.AmlOpcode);
            WalkState->Opcode = (*Op)->Common.AmlOpcode;

            Status = WalkState->AscendingCallback (WalkState);
            Status = AcpiPsNextParseState (WalkState, *Op, Status);

            Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
            if (ACPI_FAILURE (Status2))
            {
                return_ACPI_STATUS (Status2);
            }
        }

        Status = AE_OK;
        break;

    case AE_CTRL_BREAK:
    case AE_CTRL_CONTINUE:

        /* Pop off scopes until we find the While */

        while (!(*Op) || ((*Op)->Common.AmlOpcode != AML_WHILE_OP))
        {
            AcpiPsPopScope (&(WalkState->ParserState), Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);
        }

        /* Close this iteration of the While loop */

        WalkState->Op = *Op;
        WalkState->OpInfo = AcpiPsGetOpcodeInfo ((*Op)->Common.AmlOpcode);
        WalkState->Opcode = (*Op)->Common.AmlOpcode;

        Status = WalkState->AscendingCallback (WalkState);
        Status = AcpiPsNextParseState (WalkState, *Op, Status);

        Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
        if (ACPI_FAILURE (Status2))
        {
            return_ACPI_STATUS (Status2);
        }

        Status = AE_OK;
        break;

    case AE_CTRL_TERMINATE:

        /* Clean up */
        do
        {
            if (*Op)
            {
                Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
                if (ACPI_FAILURE (Status2))
                {
                    return_ACPI_STATUS (Status2);
                }

                AcpiUtDeleteGenericState (
                    AcpiUtPopGenericState (&WalkState->ControlState));
            }

            AcpiPsPopScope (&(WalkState->ParserState), Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);

        } while (*Op);

        return_ACPI_STATUS (AE_OK);

    default:  /* All other non-AE_OK status */

        do
        {
            if (*Op)
            {
                Status2 = AcpiPsCompleteThisOp (WalkState, *Op);
                if (ACPI_FAILURE (Status2))
                {
                    return_ACPI_STATUS (Status2);
                }
            }

            AcpiPsPopScope (&(WalkState->ParserState), Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);

        } while (*Op);


#if 0
        /*
         * TBD: Cleanup parse ops on error
         */
        if (*Op == NULL)
        {
            AcpiPsPopScope (ParserState, Op,
                &WalkState->ArgTypes, &WalkState->ArgCount);
        }
#endif
        WalkState->PrevOp = NULL;
        WalkState->PrevArgTypes = WalkState->ArgTypes;
        return_ACPI_STATUS (Status);
    }

    /* This scope complete? */

    if (AcpiPsHasCompletedScope (&(WalkState->ParserState)))
    {
        AcpiPsPopScope (&(WalkState->ParserState), Op,
            &WalkState->ArgTypes, &WalkState->ArgCount);
        ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "Popped scope, Op=%p\n", *Op));
    }
    else
    {
        *Op = NULL;
    }

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiPsCompleteFinalOp
 *
 * PARAMETERS:  WalkState           - Current state
 *              Op                  - Current Op
 *              Status              - Current parse status before complete last
 *                                    Op
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Complete last Op.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiPsCompleteFinalOp (
    ACPI_WALK_STATE         *WalkState,
    ACPI_PARSE_OBJECT       *Op,
    ACPI_STATUS             Status)
{
    ACPI_STATUS             Status2;


    ACPI_FUNCTION_TRACE_PTR (PsCompleteFinalOp, WalkState);


    /*
     * Complete the last Op (if not completed), and clear the scope stack.
     * It is easily possible to end an AML "package" with an unbounded number
     * of open scopes (such as when several ASL blocks are closed with
     * sequential closing braces). We want to terminate each one cleanly.
     */
    ACPI_DEBUG_PRINT ((ACPI_DB_PARSE, "AML package complete at Op %p\n", Op));
    do
    {
        if (Op)
        {
            if (WalkState->AscendingCallback != NULL)
            {
                WalkState->Op = Op;
                WalkState->OpInfo = AcpiPsGetOpcodeInfo (Op->Common.AmlOpcode);
                WalkState->Opcode = Op->Common.AmlOpcode;

                Status = WalkState->AscendingCallback (WalkState);
                Status = AcpiPsNextParseState (WalkState, Op, Status);
                if (Status == AE_CTRL_PENDING)
                {
                    Status = AcpiPsCompleteOp (WalkState, &Op, AE_OK);
                    if (ACPI_FAILURE (Status))
                    {
                        return_ACPI_STATUS (Status);
                    }
                }

                if (Status == AE_CTRL_TERMINATE)
                {
                    Status = AE_OK;

                    /* Clean up */
                    do
                    {
                        if (Op)
                        {
                            Status2 = AcpiPsCompleteThisOp (WalkState, Op);
                            if (ACPI_FAILURE (Status2))
                            {
                                return_ACPI_STATUS (Status2);
                            }
                        }

                        AcpiPsPopScope (&(WalkState->ParserState), &Op,
                            &WalkState->ArgTypes, &WalkState->ArgCount);

                    } while (Op);

                    return_ACPI_STATUS (Status);
                }

                else if (ACPI_FAILURE (Status))
                {
                    /* First error is most important */

                    (void) AcpiPsCompleteThisOp (WalkState, Op);
                    return_ACPI_STATUS (Status);
                }
            }

            Status2 = AcpiPsCompleteThisOp (WalkState, Op);
            if (ACPI_FAILURE (Status2))
            {
                return_ACPI_STATUS (Status2);
            }
        }

        AcpiPsPopScope (&(WalkState->ParserState), &Op, &WalkState->ArgTypes,
            &WalkState->ArgCount);

    } while (Op);

    return_ACPI_STATUS (Status);
}
