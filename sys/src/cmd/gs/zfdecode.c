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

/* zfdecode.c */
/* Additional decoding filter creation */
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsstruct.h"
#include "ialloc.h"
#include "idict.h"
#include "idparam.h"
#include "store.h"
#include "strimpl.h"
#include "sfilter.h"
#include "scfx.h"
#include "slzwx.h"
#include "ifilter.h"

/* Import the Level 2 scanner extensions. */
extern const stream_template _ds *scan_ascii85_template;
extern const stream_template s_A85D_template;

/* Initialize the Level 2 scanner for ASCII85 strings. */
private void
zfdecode_init(void)
{	scan_ascii85_template = &s_A85D_template;
}

/* ------ ASCII85 filters ------ */

/* We include both encoding and decoding filters here, */
/* because it would be a nuisance to separate them. */

/* <target> ASCII85Encode/filter <file> */
int
zA85E(os_ptr op)
{	return filter_write(op, 0, &s_A85E_template, NULL, 0);
}

/* <source> ASCII85Decode/filter <file> */
int
zA85D(os_ptr op)
{	return filter_read(op, 0, &s_A85D_template, NULL, 0);
}

/* ------ CCITTFaxDecode filter ------ */

/* Common setup for encoding and decoding filters. */
int
zcf_setup(os_ptr op, stream_CF_state *pcfs)
{	int code;
	if ( (code = dict_bool_param(op, "Uncompressed", false,
				     &pcfs->Uncompressed)) < 0 ||
	     (code = dict_int_param(op, "K", -9999, 9999, 0,
				    &pcfs->K)) < 0 ||
	     (code = dict_bool_param(op, "EndOfLine", false,
				     &pcfs->EndOfLine)) < 0 ||
	     (code = dict_bool_param(op, "EncodedByteAlign", false,
				     &pcfs->EncodedByteAlign)) < 0 ||
	     (code = dict_int_param(op, "Columns", 0, 9999, 1728,
				    &pcfs->Columns)) < 0 ||
	     (code = dict_int_param(op, "Rows", 0, 9999, 0,
				    &pcfs->Rows)) < 0 ||
	     (code = dict_bool_param(op, "EndOfBlock", true,
				     &pcfs->EndOfBlock)) < 0 ||
	     (code = dict_bool_param(op, "BlackIs1", false,
				     &pcfs->BlackIs1)) < 0 ||
	     (code = dict_int_param(op, "DamagedRowsBeforeError", 0, 9999,
				    0, &pcfs->DamagedRowsBeforeError)) < 0 ||
	     (code = dict_bool_param(op, "FirstBitLowOrder", false,
				     &pcfs->FirstBitLowOrder)) < 0
	   )
		return code;
	pcfs->raster = (pcfs->Columns + 7) >> 3;
	return 0;
}

/* <source> <dict> CCITTFaxDecode/filter <file> */
/* <source> CCITTFaxDecode/filter <file> */
int
zCFD(os_ptr op)
{	os_ptr dop;
	int npop;
	stream_CFD_state cfs;
	int code;
	if ( r_has_type(op, t_dictionary) )
	{	check_dict_read(*op);
		dop = op, npop = 1;
	}
	else
		dop = 0, npop = 0;
	code = zcf_setup(dop, (stream_CF_state *)&cfs);
	if ( code < 0 )
	  return code;
	return filter_read(op, npop, &s_CFD_template, (stream_state *)&cfs, 0);
}

/* ------ Generalized LZW/GIF decoding filter ------ */

/* <source> LZWDecode/filter <file> */
/* <source> <dict> LZWDecode/filter <file> */
int
zLZWD(os_ptr op)
{	stream_LZW_state lzs;
	os_ptr dop;
	int code;
	int npop;
	if ( r_has_type(op, t_dictionary) )
	{	check_dict_read(*op);
		dop = op, npop = 1;
	}
	else
		dop = 0, npop = 0;
	if ( (code = dict_int_param(dop, "InitialCodeLength", 2, 11, 8,
				    &lzs.InitialCodeLength)) < 0 ||
	     (code = dict_bool_param(dop, "FirstBitLowOrder", false,
				     &lzs.FirstBitLowOrder)) < 0 ||
	     (code = dict_bool_param(dop, "BlockData", false,
				     &lzs.BlockData)) < 0 ||
	     (code = dict_int_param(dop, "EarlyChange", 0, 1, 1,
				    &lzs.EarlyChange)) < 0
	   )
		return code;
	return filter_read(op, npop, &s_LZWD_template, (stream_state *)&lzs, 0);
}

/* ---------------- Initialization procedure ---------------- */

BEGIN_OP_DEFS(zfdecode_op_defs) {
		op_def_begin_filter(),
	{"1ASCII85Encode", zA85E},
	{"1ASCII85Decode", zA85D},
	{"2CCITTFaxDecode", zCFD},
	{"1LZWDecode", zLZWD},
END_OP_DEFS(zfdecode_init) }
