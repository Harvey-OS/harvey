/******************************************************************************
 *
 * Module Name: acapps - common include for ACPI applications/tools
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

#ifndef _ACAPPS
#define _ACAPPS

#include <stdio.h>

/* Common info for tool signons */

#define ACPICA_NAME                 "Intel ACPI Component Architecture"
#define ACPICA_COPYRIGHT            "Copyright (c) 2000 - 2016 Intel Corporation"

#if ACPI_MACHINE_WIDTH == 64
#define ACPI_WIDTH          "-64"

#elif ACPI_MACHINE_WIDTH == 32
#define ACPI_WIDTH          "-32"

#else
#error unknown ACPI_MACHINE_WIDTH
#define ACPI_WIDTH          "-??"

#endif

/* Macros for signons and file headers */

#define ACPI_COMMON_SIGNON(utility_name) \
	"\n%s\n%s version %8.8X%s\n%s\n\n", \
	ACPICA_NAME, \
	utility_name, ((u32) ACPI_CA_VERSION), ACPI_WIDTH, \
	ACPICA_COPYRIGHT

#define ACPI_COMMON_HEADER(utility_name, prefix) \
	"%s%s\n%s%s version %8.8X%s\n%s%s\n%s\n", \
	prefix, ACPICA_NAME, \
	prefix, utility_name, ((u32) ACPI_CA_VERSION), ACPI_WIDTH, \
	prefix, ACPICA_COPYRIGHT, \
	prefix

/* Macros for usage messages */

#define ACPI_USAGE_HEADER(usage) \
	acpi_os_printf ("Usage: %s\nOptions:\n", usage);

#define ACPI_USAGE_TEXT(description) \
	acpi_os_printf (description);

#define ACPI_OPTION(name, description) \
	acpi_os_printf (" %-20s%s\n", name, description);

/* Check for unexpected exceptions */

#define ACPI_CHECK_STATUS(name, status, expected) \
	if (status != expected) \
	{ \
		acpi_os_printf ("Unexpected %s from %s (%s-%d)\n", \
			acpi_format_exception (status), #name, _acpi_module_name, __LINE__); \
	}

/* Check for unexpected non-AE_OK errors */

#define ACPI_CHECK_OK(name, status)   ACPI_CHECK_STATUS (name, status, AE_OK);

#define FILE_SUFFIX_DISASSEMBLY     "dsl"
#define FILE_SUFFIX_BINARY_TABLE    ".dat"	/* Needs the dot */

/* acfileio */

acpi_status
ac_get_all_tables_from_file(char *filename,
			    u8 get_only_aml_tables,
			    struct acpi_new_table_desc **return_list_head);

u8 ac_is_file_binary(FILE * file);

acpi_status ac_validate_table_header(FILE * file, long table_offset);

/* Values for get_only_aml_tables */

#define ACPI_GET_ONLY_AML_TABLES    TRUE
#define ACPI_GET_ALL_TABLES         FALSE

/*
 * getopt
 */
int acpi_getopt(int argc, char **argv, char *opts);

int acpi_getopt_argument(int argc, char **argv);

extern int acpi_gbl_optind;
extern int acpi_gbl_opterr;
extern int acpi_gbl_sub_opt_char;
extern char *acpi_gbl_optarg;

/*
 * cmfsize - Common get file size function
 */
u32 cm_get_file_size(ACPI_FILE file);

/*
 * adwalk
 */
void
acpi_dm_cross_reference_namespace(union acpi_parse_object *parse_tree_root,
				  struct acpi_namespace_node *namespace_root,
				  acpi_owner_id owner_id);

void acpi_dm_dump_tree(union acpi_parse_object *origin);

void acpi_dm_find_orphan_methods(union acpi_parse_object *origin);

void
acpi_dm_finish_namespace_load(union acpi_parse_object *parse_tree_root,
			      struct acpi_namespace_node *namespace_root,
			      acpi_owner_id owner_id);

void
acpi_dm_convert_resource_indexes(union acpi_parse_object *parse_tree_root,
				 struct acpi_namespace_node *namespace_root);

/*
 * adfile
 */
acpi_status ad_initialize(void);

char *fl_generate_filename(char *input_filename, char *suffix);

acpi_status
fl_split_input_pathname(char *input_path,
			char **out_directory_path, char **out_filename);

char *ad_generate_filename(char *prefix, char *table_id);

void
ad_write_table(struct acpi_table_header *table,
	       u32 length, char *table_name, char *oem_table_id);

#endif				/* _ACAPPS */
