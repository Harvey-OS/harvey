/* Copyright (C) 1994, 1997, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zmisc1.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
/* Miscellaneous Type 1 font operators */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gscrypt1.h"
#include "stream.h"		/* for getting state of PFBD stream */
#include "strimpl.h"
#include "sfilter.h"
#include "idict.h"
#include "idparam.h"
#include "ifilter.h"

/* <state> <from_string> <to_string> .type1encrypt <new_state> <substring> */
/* <state> <from_string> <to_string> .type1decrypt <new_state> <substring> */
private int type1crypt(P2(i_ctx_t *,
			int (*)(P4(byte *, const byte *, uint, ushort *))));
private int
ztype1encrypt(i_ctx_t *i_ctx_p)
{
    return type1crypt(i_ctx_p, gs_type1_encrypt);
}
private int
ztype1decrypt(i_ctx_t *i_ctx_p)
{
    return type1crypt(i_ctx_p, gs_type1_decrypt);
}
private int
type1crypt(i_ctx_t *i_ctx_p,
	   int (*proc)(P4(byte *, const byte *, uint, ushort *)))
{
    os_ptr op = osp;
    crypt_state state;
    uint ssize;

    check_type(op[-2], t_integer);
    state = op[-2].value.intval;
    if (op[-2].value.intval != state)
	return_error(e_rangecheck);	/* state value was truncated */
    check_read_type(op[-1], t_string);
    check_write_type(*op, t_string);
    ssize = r_size(op - 1);
    if (r_size(op) < ssize)
	return_error(e_rangecheck);
    discard((*proc)(op->value.bytes, op[-1].value.const_bytes, ssize,
		    &state));	/* can't fail */
    op[-2].value.intval = state;
    op[-1] = *op;
    r_set_size(op - 1, ssize);
    pop(1);
    return 0;
}

/* Get the seed parameter for eexecEncode/Decode. */
/* Return npop if OK. */
private int
eexec_param(os_ptr op, ushort * pcstate)
{
    int npop = 1;

    if (r_has_type(op, t_dictionary))
	++npop, --op;
    check_type(*op, t_integer);
    *pcstate = op->value.intval;
    if (op->value.intval != *pcstate)
	return_error(e_rangecheck);	/* state value was truncated */
    return npop;
}

/* <target> <seed> eexecEncode/filter <file> */
/* <target> <seed> <dict_ignored> eexecEncode/filter <file> */
private int
zexE(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream_exE_state state;
    int code = eexec_param(op, &state.cstate);

    if (code < 0)
	return code;
    return filter_write(i_ctx_p, code, &s_exE_template, (stream_state *)&state, 0);
}

/* <source> <seed> eexecDecode/filter <file> */
/* <source> <dict> eexecDecode/filter <file> */
private int
zexD(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    stream_exD_state state;
    int code;

    (*s_exD_template.set_defaults)((stream_state *)&state);
    if (r_has_type(op, t_dictionary)) {
	uint cstate;

	check_dict_read(*op);
	if ((code = dict_uint_param(op, "seed", 0, 0xffff, 0x10000,
				    &cstate)) < 0 ||
	    (code = dict_int_param(op, "lenIV", 0, max_int, 4,
				   &state.lenIV)) < 0
	    )
	    return code;
	state.cstate = cstate;
	code = 1;
    } else {
	code = eexec_param(op, &state.cstate);
    }
    if (code < 0)
	return code;
    /*
     * If we're reading a .PFB file, let the filter know about it,
     * so it can read recklessly to the end of the binary section.
     */
    state.pfb_state = 0;
    if (r_has_type(op - 1, t_file)) {
	stream *s = (op - 1)->value.pfile;

	if (s->state != 0 && s->state->template == &s_PFBD_template)
	    state.pfb_state = (stream_PFBD_state *) s->state;
    }
    state.binary = -1;
    return filter_read(i_ctx_p, code, &s_exD_template, (stream_state *)&state, 0);
}

/* ------ Initialization procedure ------ */

const op_def zmisc1_op_defs[] =
{
    {"3.type1encrypt", ztype1encrypt},
    {"3.type1decrypt", ztype1decrypt},
    op_def_begin_filter(),
    {"2eexecEncode", zexE},
    {"2eexecDecode", zexD},
    op_def_end(0)
};
