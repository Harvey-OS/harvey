/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zfpdiff.c */
/* Pixel-difference filter creation */
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsstruct.h"
#include "idict.h"
#include "idparam.h"
#include "strimpl.h"
#include "sfilter.h"
#include "spdiffx.h"
#include "ifilter.h"


/* ------ Color differencing filters ------ */

/* Common setup for encoding and decoding filters. */
private int
diff_setup(os_ptr op, stream_PDiff_state *ss)
{	int code, bpc;
	if ( (code = dict_int_param(op, "Colors", 1, 4, 1,
				    &ss->Colors)) < 0 ||
	     (code = dict_int_param(op, "BitsPerComponent", 1, 8, 8,
				    &bpc)) < 0 ||	     
	     (bpc & (bpc - 1)) != 0 ||
	     (code = dict_int_param(op, "Columns", 0, 9999, -1,
				    &ss->Columns)) < 0
	   )
		return (code < 0 ? code : gs_note_error(e_rangecheck));
	ss->BitsPerComponent = bpc;
	return 0;
}

/* <target> <dict> PixelDifferenceEncode/filter <file> */
int
zPDiffE(os_ptr op)
{	stream_PDiff_state diffs;
	int code;
	check_type(*op, t_dictionary);
	check_dict_read(*op);
	code = diff_setup(op, &diffs);
	if ( code < 0 )
		return code;
	return filter_write(op, 1, &s_PDiffE_template, (stream_state *)&diffs, 0);
}

/* <source> <dict> PixelDifferenceDecode/filter <file> */
int
zPDiffD(os_ptr op)
{	stream_PDiff_state diffs;
	int code;
	check_type(*op, t_dictionary);
	check_dict_read(*op);
	code = diff_setup(op, &diffs);
	if ( code < 0 )
		return code;
	return filter_read(op, 1, &s_PDiffD_template, (stream_state *)&diffs, 0);
}

/* ================ Initialization procedure ================ */

BEGIN_OP_DEFS(zfpdiff_op_defs) {
		op_def_begin_filter(),
	{"2PixelDifferenceDecode", zPDiffD},
	{"2PixelDifferenceEncode", zPDiffE},
END_OP_DEFS(0) }
