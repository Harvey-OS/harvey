/******************************************************************************
 *
 * Module Name: aecommon - common include for the AcpiExec utility
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

#ifndef _AECOMMON
#define _AECOMMON

#ifdef _MSC_VER                 /* disable some level-4 warnings */
#pragma warning(disable:4100)   /* warning C4100: unreferenced formal parameter */
#endif

#include "acpi.h"
#include "accommon.h"
#include "acparser.h"
#include "amlcode.h"
#include "acnamesp.h"
#include "acdebug.h"
#include "actables.h"
#include "acinterp.h"
#include "amlresrc.h"
#include "acapps.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


/*
 * Debug Regions
 */
typedef struct ae_region
{
    ACPI_PHYSICAL_ADDRESS   Address;
    UINT32                  Length;
    void                    *Buffer;
    void                    *NextRegion;
    UINT8                   SpaceId;

} AE_REGION;

typedef struct ae_debug_regions
{
    UINT32                  NumberOfRegions;
    AE_REGION               *RegionList;

} AE_DEBUG_REGIONS;


extern BOOLEAN              AcpiGbl_IgnoreErrors;
extern UINT8                AcpiGbl_RegionFillValue;
extern UINT8                AcpiGbl_UseHwReducedFadt;
extern BOOLEAN              AcpiGbl_DisplayRegionAccess;
extern BOOLEAN              AcpiGbl_DoInterfaceTests;
extern BOOLEAN              AcpiGbl_LoadTestTables;
extern FILE                 *AcpiGbl_NamespaceInitFile;
extern ACPI_CONNECTION_INFO AeMyContext;


#define TEST_OUTPUT_LEVEL(lvl)          if ((lvl) & OutputLevel)

#define OSD_PRINT(lvl,fp)               TEST_OUTPUT_LEVEL(lvl) {\
                                            AcpiOsPrintf PARAM_LIST(fp);}

void ACPI_SYSTEM_XFACE
AeCtrlCHandler (
    int                     Sig);

ACPI_STATUS
AeBuildLocalTables (
    ACPI_NEW_TABLE_DESC     *TableList);

ACPI_STATUS
AeInstallTables (
    void);

ACPI_STATUS
AeLoadTables (
    void);

void
AeDumpNamespace (
    void);

void
AeDumpObject (
    char                    *MethodName,
    ACPI_BUFFER             *ReturnObj);

void
AeDumpBuffer (
    UINT32                  Address);

void
AeExecute (
    char                    *Name);

void
AeSetScope (
    char                    *Name);

void
AeCloseDebugFile (
    void);

void
AeOpenDebugFile (
    char                    *Name);

ACPI_STATUS
AeDisplayAllMethods (
    UINT32                  DisplayCount);

ACPI_STATUS
AeInstallEarlyHandlers (
    void);

ACPI_STATUS
AeInstallLateHandlers (
    void);

void
AeMiscellaneousTests (
    void);

ACPI_STATUS
AeRegionHandler (
    UINT32                  Function,
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT32                  BitWidth,
    UINT64                  *Value,
    void                    *HandlerContext,
    void                    *RegionContext);

UINT32
AeGpeHandler (
    ACPI_HANDLE             GpeDevice,
    UINT32                  GpeNumber,
    void                    *Context);

void
AeGlobalEventHandler (
    UINT32                  Type,
    ACPI_HANDLE             GpeDevice,
    UINT32                  EventNumber,
    void                    *Context);

/* aeregion */

ACPI_STATUS
AeInstallDeviceHandlers (
    void);

void
AeInstallRegionHandlers (
    void);

void
AeOverrideRegionHandlers (
    void);


/* aeinitfile */

int
AeOpenInitializationFile (
    char                    *Filename);

void
AeDoObjectOverrides (
    void);

#endif /* _AECOMMON */
