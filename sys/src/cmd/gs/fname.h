/* Copyright (C) 1993 Aladdin Enterprises.  All rights reserved.
  
  This file is part of Aladdin Ghostscript.
  
  Aladdin Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
  or distributor accepts any responsibility for the consequences of using it,
  or for whether it serves any particular purpose or works at all, unless he
  or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
  License (the "License") for full details.
  
  Every copy of Aladdin Ghostscript must include a copy of the License,
  normally in a plain ASCII text file named PUBLIC.  The License grants you
  the right to copy, modify and redistribute Aladdin Ghostscript, but only
  under certain conditions described in the License.  Among other things, the
  License requires that the copyright notice and this notice be preserved on
  all copies.
*/

/* fname.h */
/* File name parsing interface */
/* Requires gxiodev.h */

/* Parsed file name type.  Note that the file name may be either a */
/* gs_string (no terminator) or a C string (null terminator). */
typedef struct parsed_file_name_s {
	gx_io_device *iodev;
	const char *fname;
	uint len;
} parsed_file_name;
int parse_file_name(P2(const ref *, parsed_file_name *));
int parse_real_file_name(P3(const ref *, parsed_file_name *, client_name_t));
int terminate_file_name(P2(parsed_file_name *, client_name_t));
void free_file_name(P2(parsed_file_name *, client_name_t));
