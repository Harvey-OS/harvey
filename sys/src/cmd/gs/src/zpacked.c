/* Copyright (C) 1990, 1992, 1993, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zpacked.c,v 1.7 2004/08/19 19:33:09 stefan Exp $ */
/* Packed array operators */
#include "ghost.h"
#include "ialloc.h"
#include "idict.h"
#include "iname.h"
#include "istack.h"		/* for iparray.h */
#include "ipacked.h"
#include "iparray.h"
#include "ivmspace.h"
#include "oper.h"
#include "store.h"
#include "gxalloc.h"

/* - currentpacking <bool> */
private int
zcurrentpacking(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    ref_assign(op, &ref_array_packing);
    return 0;
}

/* <obj_0> ... <obj_n-1> <n> packedarray <packedarray> */
int
zpackedarray(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
    ref parr;

    check_type(*op, t_integer);
    if (op->value.intval < 0 ||
	(op->value.intval > op - osbot &&
	 op->value.intval >= ref_stack_count(&o_stack))
	)
	return_error(e_rangecheck);
    osp--;
    code = make_packed_array(&parr, &o_stack, (uint) op->value.intval,
			     idmemory, "packedarray");
    osp++;
    if (code >= 0)
	*osp = parr;
    return code;
}

/* <bool> setpacking - */
private int
zsetpacking(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref cont;

    check_type(*op, t_boolean);
    make_struct(&cont, avm_local, ref_array_packing_container);
    ref_assign_old(&cont, &ref_array_packing, op, "setpacking");
    pop(1);
    return 0;
}

/* ------ Non-operator routines ------ */

/* Make a packed array.  See the comment in packed.h about */
/* ensuring that refs in mixed arrays are properly aligned. */
#undef idmemory			/****** NOTA BENE ******/
int
make_packed_array(ref * parr, ref_stack_t * pstack, uint size,
		  gs_dual_memory_t *idmemory, client_name_t cname)
{
    uint i;
    const ref *pref;
    uint idest = 0, ishort = 0;
    ref_packed *pbody;
    ref_packed *pdest;
    ref_packed *pshort;		/* points to start of */
				/* last run of short elements */
    gs_ref_memory_t *imem = idmemory->current;
    uint space = imemory_space(imem);
    int skip = 0, pad;
    ref rtemp;
    int code;

    /* Do a first pass to calculate the size of the array, */
    /* and to detect local-into-global stores. */

    for (i = size; i != 0; i--) {
	pref = ref_stack_index(pstack, i - 1);
	switch (r_btype(pref)) {	/* not r_type, opers are special */
	    case t_name:
	      if (name_index(imem, pref) >= packed_name_max_index)
		    break;	/* can't pack */
		idest++;
		continue;
	    case t_integer:
		if (pref->value.intval < packed_min_intval ||
		    pref->value.intval > packed_max_intval
		    )
		    break;
		idest++;
		continue;
	    case t_oparray:
		/* Check for local-into-global store. */
		store_check_space(space, pref);
		/* falls through */
	    case t_operator:
		{
		    uint oidx;

		    if (!r_has_attr(pref, a_executable))
			break;
		    oidx = op_index(pref);
		    if (oidx == 0 || oidx > packed_int_mask)
			break;
		}
		idest++;
		continue;
	    default:
		/* Check for local-into-global store. */
		store_check_space(space, pref);
	}
	/* Can't pack this element, use a full ref. */
	/* We may have to unpack up to align_packed_per_ref - 1 */
	/* preceding short elements. */
	/* If we are at the beginning of the array, however, */
	/* we can just move the elements up. */
	{
	    int i = (idest - ishort) & (align_packed_per_ref - 1);

	    if (ishort == 0)	/* first time */
		idest += skip = -i & (align_packed_per_ref - 1);
	    else
		idest += (packed_per_ref - 1) * i;
	}
	ishort = idest += packed_per_ref;
    }
    pad = -(int)idest & (packed_per_ref - 1);	/* padding at end */

    /* Now we can allocate the array. */

    code = gs_alloc_ref_array(imem, &rtemp, 0, (idest + pad) / packed_per_ref,
			      cname);
    if (code < 0)
	return code;
    pbody = (ref_packed *) rtemp.value.refs;

    /* Make sure any initial skipped elements contain legal packed */
    /* refs, so that the garbage collector can scan storage. */

    pshort = pbody;
    for (; skip; skip--)
	*pbody++ = pt_tag(pt_integer);
    pdest = pbody;

    for (i = size; i != 0; i--) {
	pref = ref_stack_index(pstack, i - 1);
	switch (r_btype(pref)) {	/* not r_type, opers are special */
	    case t_name:
		{
		    uint nidx = name_index(imem, pref);

		    if (nidx >= packed_name_max_index)
			break;	/* can't pack */
		    *pdest++ = nidx +
			(r_has_attr(pref, a_executable) ?
			 pt_tag(pt_executable_name) :
			 pt_tag(pt_literal_name));
		}
		continue;
	    case t_integer:
		if (pref->value.intval < packed_min_intval ||
		    pref->value.intval > packed_max_intval
		    )
		    break;
		*pdest++ = pt_tag(pt_integer) +
		    ((short)pref->value.intval - packed_min_intval);
		continue;
	    case t_oparray:
	    case t_operator:
		{
		    uint oidx;

		    if (!r_has_attr(pref, a_executable))
			break;
		    oidx = op_index(pref);
		    if (oidx == 0 || oidx > packed_int_mask)
			break;
		    *pdest++ = pt_tag(pt_executable_operator) + oidx;
		}
		continue;
	}
	/* Can't pack this element, use a full ref. */
	/* We may have to unpack up to align_packed_per_ref - 1 */
	/* preceding short elements. */
	/* Note that if we are at the beginning of the array, */
	/* 'skip' already ensures that we don't need to do this. */
	{
	    int i = (pdest - pshort) & (align_packed_per_ref - 1);
	    const ref_packed *psrc = pdest;
	    ref *pmove =
	    (ref *) (pdest += (packed_per_ref - 1) * i);

	    ref_assign_new(pmove, pref);
	    while (--i >= 0) {
		--psrc;
		--pmove;
		packed_get(imem->non_gc_memory, psrc, pmove);
	    }
	}
	pshort = pdest += packed_per_ref;
    }

    {
	int atype =
	(pdest == pbody + size ? t_shortarray : t_mixedarray);

	/* Pad with legal packed refs so that the garbage collector */
	/* can scan storage. */

	for (; pad; pad--)
	    *pdest++ = pt_tag(pt_integer);

	/* Finally, make the array. */

	ref_stack_pop(pstack, size);
	make_tasv_new(parr, atype, a_readonly | space, size,
		      packed, pbody + skip);
    }
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zpacked_op_defs[] =
{
    {"0currentpacking", zcurrentpacking},
    {"1packedarray", zpackedarray},
    {"1setpacking", zsetpacking},
    op_def_end(0)
};
