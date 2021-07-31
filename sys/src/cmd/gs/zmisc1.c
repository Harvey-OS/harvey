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

/* zmisc1.c */
/* Miscellaneous Type 1 font operators */
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gscrypt1.h"
#include "stream.h"		/* for getting state of PFBD stream */
#include "strimpl.h"
#include "sfilter.h"
#include "ifilter.h"

/* <state> <from_string> <to_string> .type1encrypt <new_state> <substring> */
/* <state> <from_string> <to_string> .type1decrypt <new_state> <substring> */
private int type1crypt(P2(os_ptr,
			  int (*)(P4(byte *, const byte *, uint, ushort *))));
int
ztype1encrypt(os_ptr op)
{	return type1crypt(op, gs_type1_encrypt);
}
int
ztype1decrypt(os_ptr op)
{	return type1crypt(op, gs_type1_decrypt);
}
private int
type1crypt(register os_ptr op, int (*proc)(P4(byte *, const byte *, uint, ushort *)))
{	crypt_state state;
	uint ssize;
	check_type(op[-2], t_integer);
	state = op[-2].value.intval;
	if ( op[-2].value.intval != state )
		return_error(e_rangecheck);	/* state value was truncated */
	check_read_type(op[-1], t_string);
	check_write_type(*op, t_string);
	ssize = r_size(op - 1);
	if ( r_size(op) < ssize )
		return_error(e_rangecheck);
	discard((*proc)(op->value.bytes, op[-1].value.const_bytes, ssize,
			&state));		/* can't fail */
	op[-2].value.intval = state;
	op[-1] = *op;
	r_set_size(op - 1, ssize);
	pop(1);
	return 0;
}

/* <target> <seed> eexecEncode/filter <file> */
int
zexE(register os_ptr op)
{	stream_exE_state state;
	check_type(*op, t_integer);
	state.cstate = op->value.intval;
	if ( op->value.intval != state.cstate )
		return_error(e_rangecheck);	/* state value was truncated */
	return filter_write(op, 1, &s_exE_template, (stream_state *)&state, 0);
}

/* <source> <seed> eexecDecode/filter <file> */
int
zexD(register os_ptr op)
{	stream_exD_state state;
	check_type(*op, t_integer);
	state.cstate = op->value.intval;
	if ( op->value.intval != state.cstate )
	  return_error(e_rangecheck);	/* state value was truncated */
	/* If we're reading a .PFB file, let the filter know about it, */
	/* so it can read recklessly to the end of the binary section. */
	state.pfb_state = 0;
	if ( r_has_type(op - 1, t_file) )
	  {	stream *s = (op - 1)->value.pfile;
		if ( s->state != 0 && s->state->template == &s_PFBD_template )
		  state.pfb_state = (stream_PFBD_state *)s->state;
	  }
	return filter_read(op, 1, &s_exD_template, (stream_state *)&state, 0);
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zmisc1_op_defs) {
	{"3.type1encrypt", ztype1encrypt},
	{"3.type1decrypt", ztype1decrypt},
		op_def_begin_filter(),
	{"2eexecEncode", zexE},
	{"2eexecDecode", zexD},
END_OP_DEFS(0) }
