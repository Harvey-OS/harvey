/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zdict.c,v 1.6 2004/08/04 19:36:13 stefan Exp $ */
/* Dictionary operators */
#include "ghost.h"
#include "oper.h"
#include "iddict.h"
#include "dstack.h"
#include "ilevel.h"		/* for [count]dictstack */
#include "iname.h"		/* for dict_find_name */
#include "ipacked.h"		/* for inline dict lookup */
#include "ivmspace.h"
#include "store.h"

/* <int> dict <dict> */
int
zdict(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_integer);
#if arch_sizeof_int < arch_sizeof_long
    check_int_leu(*op, max_uint);
#else
    if (op->value.intval < 0)
	return_error(e_rangecheck);
#endif
    return dict_create((uint) op->value.intval, op);
}

/* <dict> maxlength <int> */
private int
zmaxlength(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    make_int(op, dict_maxlength(op));
    return 0;
}

/* <dict> begin - */
int
zbegin(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    if (dsp == dstop)
	return_error(e_dictstackoverflow);
    ++dsp;
    ref_assign(dsp, op);
    dict_set_top();
    pop(1);
    return 0;
}

/* - end - */
int
zend(i_ctx_t *i_ctx_p)
{
    if (ref_stack_count_inline(&d_stack) == min_dstack_size) {
	/* We would underflow the d-stack. */
	return_error(e_dictstackunderflow);
    }
    while (dsp == dsbot) {
	/* We would underflow the current block. */
	ref_stack_pop_block(&d_stack);
    }
    dsp--;
    dict_set_top();
    return 0;
}

/* <key> <value> def - */
/*
 * We make this into a separate procedure because
 * the interpreter will almost always call it directly.
 */
int
zop_def(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    ref *pvslot;

    /* The following combines a check_op(2) with a type check. */
    switch (r_type(op1)) {
	case t_name: {
	    /* We can use the fast single-probe lookup here. */
	    uint nidx = name_index(imemory, op1);
	    uint htemp;

	    if_dict_find_name_by_index_top(nidx, htemp, pvslot) {
		if (dtop_can_store(op))
		    goto ra;
	    }
	    break;		/* handle all slower cases */
	    }
	case t_null:
	    return_error(e_typecheck);
	case t__invalid:
	    return_error(e_stackunderflow);
    }
    /*
     * Combine the check for a writable top dictionary with
     * the global/local store check.  See dstack.h for details.
     */
    if (!dtop_can_store(op)) {
	check_dict_write(*dsp);
	/*
	 * If the dictionary is writable, the problem must be
	 * an invalid store.
	 */
	return_error(e_invalidaccess);
    }
    /*
     * Save a level of procedure call in the common (redefinition)
     * case.  With the current interfaces, we pay a double lookup
     * in the uncommon case.
     */
    if (dict_find(dsp, op1, &pvslot) <= 0)
	return idict_put(dsp, op1, op);
ra:
    ref_assign_old_inline(&dsp->value.pdict->values, pvslot, op,
			  "dict_put(value)");
    return 0;
}
int
zdef(i_ctx_t *i_ctx_p)
{
    int code = zop_def(i_ctx_p);

    if (code >= 0) {
	pop(2);
    }
    return code;
}

/* <key> load <value> */
private int
zload(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref *pvalue;

    switch (r_type(op)) {
	case t_name:
	    /* Use the fast lookup. */
	    if ((pvalue = dict_find_name(op)) == 0)
		return_error(e_undefined);
	    ref_assign(op, pvalue);
	    return 0;
	case t_null:
	    return_error(e_typecheck);
	case t__invalid:
	    return_error(e_stackunderflow);
	default: {
		/* Use an explicit loop. */
		uint size = ref_stack_count(&d_stack);
		uint i;

		for (i = 0; i < size; i++) {
		    ref *dp = ref_stack_index(&d_stack, i);

		    check_dict_read(*dp);
		    if (dict_find(dp, op, &pvalue) > 0) {
			ref_assign(op, pvalue);
			return 0;
		    }
		}
		return_error(e_undefined);
	    }
    }
}

/* get - implemented in zgeneric.c */

/* put - implemented in zgeneric.c */

/* <dict> <key> .undef - */
/* <dict> <key> undef - */
private int
zundef(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(op[-1], t_dictionary);
    check_dict_write(op[-1]);
    idict_undef(op - 1, op);	/* ignore undefined error */
    pop(2);
    return 0;
}

/* <dict> <key> known <bool> */
private int
zknown(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    register os_ptr op1 = op - 1;
    ref *pvalue;

    check_type(*op1, t_dictionary);
    check_dict_read(*op1);
    make_bool(op1, (dict_find(op1, op, &pvalue) > 0 ? 1 : 0));
    pop(1);
    return 0;
}

/* <key> where <dict> true */
/* <key> where false */
int
zwhere(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref_stack_enum_t rsenum;

    check_op(1);
    ref_stack_enum_begin(&rsenum, &d_stack);
    do {
	const ref *const bot = rsenum.ptr;
	const ref *pdref = bot + rsenum.size;
	ref *pvalue;

	while (pdref-- > bot) {
	    check_dict_read(*pdref);
	    if (dict_find(pdref, op, &pvalue) > 0) {
		push(1);
		ref_assign(op - 1, pdref);
		make_true(op);
		return 0;
	    }
	}
    } while (ref_stack_enum_next(&rsenum));
    make_false(op);
    return 0;
}

/* copy for dictionaries -- called from zcopy in zgeneric.c. */
/* Only the type of *op has been checked. */
int
zcopy_dict(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    int code;

    check_type(*op1, t_dictionary);
    check_dict_read(*op1);
    check_dict_write(*op);
    if (!imemory->gs_lib_ctx->dict_auto_expand &&
	(dict_length(op) != 0 || dict_maxlength(op) < dict_length(op1))
	)
	return_error(e_rangecheck);
    code = idict_copy(op1, op);
    if (code < 0)
	return code;
    /*
     * In Level 1 systems, we must copy the access attributes too.
     * The only possible effect this can have is to make the
     * copy read-only if the original dictionary is read-only.
     */
    if (!level2_enabled)
	r_copy_attrs(dict_access_ref(op), a_write, dict_access_ref(op1));
    ref_assign(op1, op);
    pop(1);
    return 0;
}

/* - currentdict <dict> */
private int
zcurrentdict(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    ref_assign(op, dsp);
    return 0;
}

/* - countdictstack <int> */
private int
zcountdictstack(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint count = ref_stack_count(&d_stack);

    push(1);
    if (!level2_enabled)
	count--;		/* see dstack.h */
    make_int(op, count);
    return 0;
}

/* <array> dictstack <subarray> */
private int
zdictstack(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint count = ref_stack_count(&d_stack);

    check_write_type(*op, t_array);
    if (!level2_enabled)
	count--;		/* see dstack.h */
    return ref_stack_store(&d_stack, op, count, 0, 0, true, idmemory,
			   "dictstack");
}

/* - cleardictstack - */
private int
zcleardictstack(i_ctx_t *i_ctx_p)
{
    while (zend(i_ctx_p) >= 0)
	DO_NOTHING;
    return 0;
}

/* ------ Extensions ------ */

/* <dict1> <dict2> .dictcopynew <dict2> */
private int
zdictcopynew(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    int code;

    check_type(*op1, t_dictionary);
    check_dict_read(*op1);
    check_type(*op, t_dictionary);
    check_dict_write(*op);
    /* This is only recognized in Level 2 mode. */
    if (!imemory->gs_lib_ctx->dict_auto_expand)
	return_error(e_undefined);
    code = idict_copy_new(op1, op);
    if (code < 0)
	return code;
    ref_assign(op1, op);
    pop(1);
    return 0;
}

/* -mark- <key0> <value0> <key1> <value1> ... .dicttomark <dict> */
/* This is the Level 2 >> operator. */
private int
zdicttomark(i_ctx_t *i_ctx_p)
{
    uint count2 = ref_stack_counttomark(&o_stack);
    ref rdict;
    int code;
    uint idx;

    if (count2 == 0)
	return_error(e_unmatchedmark);
    count2--;
    if ((count2 & 1) != 0)
	return_error(e_rangecheck);
    code = dict_create(count2 >> 1, &rdict);
    if (code < 0)
	return code;
    /* << /a 1 /a 2 >> => << /a 1 >>, i.e., */
    /* we must enter the keys in top-to-bottom order. */
    for (idx = 0; idx < count2; idx += 2) {
	code = idict_put(&rdict,
			 ref_stack_index(&o_stack, idx + 1),
			 ref_stack_index(&o_stack, idx));
	if (code < 0) {		/* There's no way to free the dictionary -- too bad. */
	    return code;
	}
    }
    ref_stack_pop(&o_stack, count2);
    ref_assign(osp, &rdict);
    return code;
}

/* <dict> <key> .forceundef - */
/*
 * This forces an "undef" even if the dictionary is not writable.
 * Like .forceput, it is meant to be used only in a few special situations,
 * and should not be accessible by name after initialization.
 */
private int
zforceundef(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(op[-1], t_dictionary);
    /* Don't check_dict_write */
    idict_undef(op - 1, op);	/* ignore undefined error */
    pop(2);
    return 0;
}

/* <dict> <key> .knownget <value> true */
/* <dict> <key> .knownget false */
private int
zknownget(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    register os_ptr op1 = op - 1;
    ref *pvalue;

    check_type(*op1, t_dictionary);
    check_dict_read(*op1);
    if (dict_find(op1, op, &pvalue) <= 0) {
	make_false(op1);
	pop(1);
    } else {
	ref_assign(op1, pvalue);
	make_true(op);
    }
    return 0;
}

/* <dict> <key> .knownundef <bool> */
private int
zknownundef(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    int code;

    check_type(*op1, t_dictionary);
    check_dict_write(*op1);
    code = idict_undef(op1, op);
    make_bool(op1, code == 0);
    pop(1);
    return 0;
}

/* <dict> <int> .setmaxlength - */
private int
zsetmaxlength(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    uint new_size;
    int code;

    check_type(*op1, t_dictionary);
    check_dict_write(*op1);
    check_type(*op, t_integer);
#if arch_sizeof_int < arch_sizeof_long
    check_int_leu(*op, max_uint);
#else
    if (op->value.intval < 0)
	return_error(e_rangecheck);
#endif
    new_size = (uint) op->value.intval;
    if (dict_length(op - 1) > new_size)
	return_error(e_dictfull);
    code = idict_resize(op - 1, new_size);
    if (code >= 0)
	pop(2);
    return code;
}

/* ------ Initialization procedure ------ */

/* We need to split the table because of the 16-element limit. */
const op_def zdict1_op_defs[] = {
    {"0cleardictstack", zcleardictstack},
    {"1begin", zbegin},
    {"0countdictstack", zcountdictstack},
    {"0currentdict", zcurrentdict},
    {"2def", zdef},
    {"1dict", zdict},
    {"0dictstack", zdictstack},
    {"0end", zend},
    {"2known", zknown},
    {"1load", zload},
    {"1maxlength", zmaxlength},
    {"2.undef", zundef},	/* we need this even in Level 1 */
    {"1where", zwhere},
    op_def_end(0)
};
const op_def zdict2_op_defs[] = {
		/* Extensions */
    {"2.dictcopynew", zdictcopynew},
    {"1.dicttomark", zdicttomark},
    {"2.forceundef", zforceundef},
    {"2.knownget", zknownget},
    {"1.knownundef", zknownundef},
    {"2.setmaxlength", zsetmaxlength},
    op_def_end(0)
};
