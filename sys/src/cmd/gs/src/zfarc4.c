/* Copyright (C) 2001 Artifex Software, Inc.  All rights reserved.
  
  This software is provided AS-IS with no warranty, either express or
  implied.
  
  This software is distributed under license and may not be copied,
  modified or distributed except as expressly authorized under the terms
  of the license contained in the file LICENSE in this distribution.
  
  For more information about licensing, please refer to
  http://www.ghostscript.com/licensing/. For information on
  commercial licensing, go to http://www.artifex.com/licensing/ or
  contact Artifex Software, Inc., 101 Lucas Valley Road #110,
  San Rafael, CA  94903, U.S.A., +1(415)492-9861.
*/

/* $Id: zfarc4.c,v 1.4 2002/02/21 22:24:54 giles Exp $ */

/* this is the ps interpreter interface to the arcfour cipher filter
   used in PDF encryption. We provide both Decode and Encode filters;
   the cipher is symmetric, so the call to the underlying routines is
   identical between the two filters */

#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gsstruct.h"
#include "ialloc.h"
#include "idict.h"
#include "stream.h"
#include "strimpl.h"
#include "ifilter.h"
#include "sarc4.h"

/* <source> <dict> arcfour/filter <file> */

private int
z_arcfour_d(i_ctx_t * i_ctx_p)
{
    os_ptr op = osp;		/* i_ctx_p->op_stack.stack.p defined in osstack.h */
    ref *sop = NULL;
    stream_arcfour_state state;

    /* extract the key from the parameter dictionary */
    check_type(*op, t_dictionary);
    check_dict_read(*op);
    if (dict_find_string(op, "Key", &sop) <= 0)
	return_error(e_rangecheck);

    s_arcfour_set_key(&state, sop->value.const_bytes, r_size(sop));

    /* we pass npop=0, since we've no arguments left to consume */
    /* we pass 0 instead of the usual rspace(sop) will allocate storage for 
       filter state from the same memory pool as the stream it's coding. this
       causes no trouble because we maintain no pointers */
    return filter_read(i_ctx_p, 0, &s_arcfour_template,
		       (stream_state *) & state, 0);
}

/* encode version of the filter */
private int
z_arcfour_e(i_ctx_t * i_ctx_p)
{
    os_ptr op = osp;		/* i_ctx_p->op_stack.stack.p defined in osstack.h */
    ref *sop = NULL;
    stream_arcfour_state state;

    /* extract the key from the parameter dictionary */
    check_type(*op, t_dictionary);
    check_dict_read(*op);
    if (dict_find_string(op, "Key", &sop) <= 0)
	return_error(e_rangecheck);

    s_arcfour_set_key(&state, sop->value.const_bytes, r_size(sop));

    /* we pass npop=0, since we've no arguments left to consume */
    /* we pass 0 instead of the usual rspace(sop) will allocate storage for 
       filter state from the same memory pool as the stream it's coding. this
       causes no trouble because we maintain no pointers */
    return filter_write(i_ctx_p, 0, &s_arcfour_template,
			(stream_state *) & state, 0);
}

/* match the above routines to their postscript filter names
   this is how our 'private' routines get called externally */
const op_def zfarc4_op_defs[] = {
    op_def_begin_filter(),
    {"2ArcfourDecode", z_arcfour_d},
    {"2ArcfourEncode", z_arcfour_e},
    op_def_end(0)
};
