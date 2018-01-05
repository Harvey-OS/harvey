/******************************************************************************
 *
 * Name: acuuid.h - ACPI-related UUID/GUID definitions
 *
 *****************************************************************************/

/*
 * Copyright (C) 2000 - 2016, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#ifndef __ACUUID_H__
#define __ACUUID_H__

/*
 * Note1: UUIDs and GUIDs are defined to be identical in ACPI.
 *
 * Note2: This file is standalone and should remain that way.
 */

/* Controllers */

#define UUID_GPIO_CONTROLLER            "4f248f40-d5e2-499f-834c-27758ea1cd3f"
#define UUID_USB_CONTROLLER             "ce2ee385-00e6-48cb-9f05-2edb927c4899"
#define UUID_SATA_CONTROLLER            "e4db149b-fcfe-425b-a6d8-92357d78fc7f"

/* Devices */

#define UUID_PCI_HOST_BRIDGE            "33db4d5b-1ff7-401c-9657-7441c03dd766"
#define UUID_I2C_DEVICE                 "3cdff6f7-4267-4555-ad05-b30a3d8938de"
#define UUID_POWER_BUTTON               "dfbcf3c5-e7a5-44e6-9c1f-29c76f6e059c"

/* Interfaces */

#define UUID_DEVICE_LABELING            "e5c937d0-3553-4d7a-9117-ea4d19c3434d"
#define UUID_PHYSICAL_PRESENCE          "3dddfaa6-361b-4eb4-a424-8d10089d1653"

/* NVDIMM - NFIT table */

#define UUID_VOLATILE_MEMORY            "7305944f-fdda-44e3-b16c-3f22d252e5d0"
#define UUID_PERSISTENT_MEMORY          "66f0d379-b4f3-4074-ac43-0d3318b78cdb"
#define UUID_CONTROL_REGION             "92f701f6-13b4-405d-910b-299367e8234c"
#define UUID_DATA_REGION                "91af0530-5d86-470e-a6b0-0a2db9408249"
#define UUID_VOLATILE_VIRTUAL_DISK      "77ab535a-45fc-624b-5560-f7b281d1f96e"
#define UUID_VOLATILE_VIRTUAL_CD        "3d5abd30-4175-87ce-6d64-d2ade523c4bb"
#define UUID_PERSISTENT_VIRTUAL_DISK    "5cea02c9-4d07-69d3-269f-4496fbe096f9"
#define UUID_PERSISTENT_VIRTUAL_CD      "08018188-42cd-bb48-100f-5387d53ded3d"

/* Miscellaneous */

#define UUID_PLATFORM_CAPABILITIES      "0811b06e-4a27-44f9-8d60-3cbbc22e7b48"
#define UUID_DYNAMIC_ENUMERATION        "d8c1a3a6-be9b-4c9b-91bf-c3cb81fc5daf"
#define UUID_BATTERY_THERMAL_LIMIT      "4c2067e3-887d-475c-9720-4af1d3ed602e"
#define UUID_THERMAL_EXTENSIONS         "14d399cd-7a27-4b18-8fb4-7cb7b9f4e500"
#define UUID_DEVICE_PROPERTIES          "daffd814-6eba-4d8c-8a91-bc9bbf4aa301"

#endif				/* __AUUID_H__ */
