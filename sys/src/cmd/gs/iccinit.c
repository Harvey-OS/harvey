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

/* iccinit.c */
/* Support for compiled initialization files */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "store.h"
#include "stream.h"
#include "files.h"

/* Import the initialization string from gs_init.c. */
extern const byte far_data gs_ccinit_string[];
extern const uint far_data gs_ccinit_string_sizeof;

/* This substitutes for the normal zinitialfile routine in gsmain.c. */
private int
zccinitialfile(register os_ptr op)
{	int code;
	push(1);
	code = file_read_string(gs_ccinit_string,
				gs_ccinit_string_sizeof, (ref *)op);
	if ( code < 0 )
		pop(1);
	return code;
}

/* Generate the operator initialization table. */
BEGIN_OP_DEFS(ccinit_op_defs) {
	{"0.initialfile", zccinitialfile},
END_OP_DEFS(0) }
