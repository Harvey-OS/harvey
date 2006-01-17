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

/* $Id: zgeneric.c,v 1.8 2004/08/04 19:36:13 stefan Exp $ */
/* Array/string/dictionary generic operators for PostScript */
#include "memory_.h"
#include "ghost.h"
#include "gsstruct.h"		/* for st_bytes */
#include "oper.h"
#include "dstack.h"		/* for systemdict */
#include "estack.h"		/* for forall */
#include "iddict.h"
#include "iname.h"
#include "ipacked.h"
#include "ivmspace.h"
#include "store.h"

/* This file implements copy, get, put, getinterval, putinterval, */
/* length, and forall, which apply generically to */
/* arrays, strings, and dictionaries.  (Copy also has a special */
/* meaning for copying the top N elements of the stack.) */

/* See the comment in opdef.h for an invariant which allows */
/* more efficient implementation of forall. */

/* Forward references */
private int zcopy_integer(i_ctx_t *);
private int zcopy_interval(i_ctx_t *);
private int copy_interval(i_ctx_t *, os_ptr, uint, os_ptr, client_name_t);

/* <various1> <various2> copy <various> */
/* <obj1> ... <objn> <int> copy <obj1> ... <objn> <obj1> ... <objn> */
/* Note that this implements copy for arrays and strings, */
/* but not for dictionaries (see zcopy_dict in zdict.c). */
int
zcopy(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int type = r_type(op);

    if (type == t_integer)
	return zcopy_integer(i_ctx_p);
    check_op(2);
    switch (type) {
	case t_array:
	case t_string:
	    return zcopy_interval(i_ctx_p);
	case t_dictionary:
	    return zcopy_dict(i_ctx_p);
	default:
	    return_op_typecheck(op);
    }
}

/* <obj1> ... <objn> <int> copy <obj1> ... <objn> <obj1> ... <objn> */
private int
zcopy_integer(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    int count, i;
    int code;

    if ((ulong) op->value.intval > op - osbot) {
	/* There might be enough elements in other blocks. */
	check_int_ltu(*op, ref_stack_count(&o_stack));
	count = op->value.intval;
    } else if (op1 + (count = op->value.intval) <= ostop) {
	/* Fast case. */
	memcpy((char *)op, (char *)(op - count), count * sizeof(ref));
	push(count - 1);
	return 0;
    }
    /* Do it the slow, general way. */
    code = ref_stack_push(&o_stack, count - 1);
    if (code < 0)
	return code;
    for (i = 0; i < count; i++)
	*ref_stack_index(&o_stack, i) =
	    *ref_stack_index(&o_stack, i + count);
    return 0;
}

/* <array1> <array2> copy <subarray2> */
/* <string1> <string2> copy <substring2> */
private int
zcopy_interval(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    int code = copy_interval(i_ctx_p, op, 0, op1, "copy");

    if (code < 0)
	return code;
    r_set_size(op, r_size(op1));
    *op1 = *op;
    pop(1);
    return 0;
}

/* <array|dict|name|packedarray|string> length <int> */
private int
zlength(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    switch (r_type(op)) {
	case t_array:
	case t_string:
	case t_mixedarray:
	case t_shortarray:
	    check_read(*op);
	    make_int(op, r_size(op));
	    return 0;
	case t_dictionary:
	    check_dict_read(*op);
	    make_int(op, dict_length(op));
	    return 0;
	case t_name: {
	    ref str;

	    name_string_ref(imemory, op, &str);
	    make_int(op, r_size(&str));
	    return 0;
	}
	case t_astruct:
	    if (gs_object_type(imemory, op->value.pstruct) != &st_bytes)
		return_error(e_typecheck);
	    check_read(*op);
	    make_int(op, gs_object_size(imemory, op->value.pstruct));
	    return 0;
	default:
	    return_op_typecheck(op);
    }
}

/* <array|packedarray|string> <index> get <obj> */
/* <dict> <key> get <obj> */
private int
zget(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    ref *pvalue;

    switch (r_type(op1)) {
	case t_dictionary:
	    check_dict_read(*op1);
	    if (dict_find(op1, op, &pvalue) <= 0)
		return_error(e_undefined);
	    op[-1] = *pvalue;
	    break;
	case t_string:
	    check_read(*op1);
	    check_int_ltu(*op, r_size(op1));
	    make_int(op1, op1->value.bytes[(uint) op->value.intval]);
	    break;
	default: {
	    int code;

	    check_type(*op, t_integer);
	    check_read(*op1);
	    code = array_get(imemory, op1, op->value.intval, op1);
	    if (code < 0) {	/* Might be a stackunderflow reported as typecheck. */
		if (code == e_typecheck)
		    return_op_typecheck(op1);
		else
		    return code;
	    }
	}
    }
    pop(1);
    return 0;
}

/* <array> <index> <obj> put - */
/* <dict> <key> <value> put - */
/* <string> <index> <int> put - */
private int
zput(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    os_ptr op2 = op1 - 1;
    byte *sdata;
    uint ssize;

    switch (r_type(op2)) {
	case t_dictionary:
	    if (i_ctx_p->in_superexec == 0)
	    check_dict_write(*op2);
	    {
		int code = idict_put(op2, op1, op);

		if (code < 0)
		    return code;	/* error */
	    }
	    break;
	case t_array:
	    check_write(*op2);
	    check_int_ltu(*op1, r_size(op2));
	    store_check_dest(op2, op);
	    {
		ref *eltp = op2->value.refs + (uint) op1->value.intval;

		ref_assign_old(op2, eltp, op, "put");
	    }
	    break;
	case t_mixedarray:	/* packed arrays are read-only */
	case t_shortarray:
	    return_error(e_invalidaccess);
	case t_string:
	    sdata = op2->value.bytes;
	    ssize = r_size(op2);
str:	    check_write(*op2);
	    check_int_ltu(*op1, ssize);
	    check_int_leu(*op, 0xff);
	    sdata[(uint)op1->value.intval] = (byte)op->value.intval;
	    break;
	case t_astruct:
	    if (gs_object_type(imemory, op2->value.pstruct) != &st_bytes)
		return_error(e_typecheck);
	    sdata = r_ptr(op2, byte);
	    ssize = gs_object_size(imemory, op2->value.pstruct);
	    goto str;
	default:
	    return_op_typecheck(op2);
    }
    pop(3);
    return 0;
}

/* <array> <index> <obj> .forceput - */
/* <dict> <key> <value> .forceput - */
/*
 * This forces a "put" even if the object is not writable, and (if the
 * object is systemdict or the save level is 0) even if the value is in
 * local VM.  It is meant to be used only for replacing the value of
 * FontDirectory in systemdict when switching between local and global VM,
 * and a few similar applications.  After initialization, this operator
 * should no longer be accessible by name.
 */
private int
zforceput(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    os_ptr op2 = op - 2;
    int code;

    switch (r_type(op2)) {
    case t_array:
	check_int_ltu(*op1, r_size(op2));
	if (r_space(op2) > r_space(op)) {
	    if (imemory_save_level(iimemory))
		return_error(e_invalidaccess);
	}
	{
	    ref *eltp = op2->value.refs + (uint) op1->value.intval;

	    ref_assign_old(op2, eltp, op, "put");
	}
	break;
    case t_dictionary:
	if (op2->value.pdict == systemdict->value.pdict ||
	    !imemory_save_level(iimemory)
	    ) {
	    uint space = r_space(op2);

	    r_set_space(op2, avm_local);
	    code = idict_put(op2, op1, op);
	    r_set_space(op2, space);
	} else
	    code = idict_put(op2, op1, op);
	if (code < 0)
	    return code;
	break;
    default:
	return_error(e_typecheck);
    }
    pop(3);
    return 0;
}

/* <seq:array|packedarray|string> <index> <count> getinterval <subseq> */
private int
zgetinterval(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    os_ptr op2 = op1 - 1;
    uint index;
    uint count;

    switch (r_type(op2)) {
	default:
	    return_op_typecheck(op2);
	case t_array:
	case t_string:
	case t_mixedarray:
	case t_shortarray:;
    }
    check_read(*op2);
    check_int_leu(*op1, r_size(op2));
    index = op1->value.intval;
    check_int_leu(*op, r_size(op2) - index);
    count = op->value.intval;
    switch (r_type(op2)) {
	case t_array:
	    op2->value.refs += index;
	    break;
	case t_string:
	    op2->value.bytes += index;
	    break;
	case t_mixedarray: {
	    const ref_packed *packed = op2->value.packed;

	    for (; index--;)
		packed = packed_next(packed);
	    op2->value.packed = packed;
	    break;
	}
	case t_shortarray:
	    op2->value.packed += index;
	    break;
    }
    r_set_size(op2, count);
    pop(2);
    return 0;
}

/* <array1> <index> <array2|packedarray2> putinterval - */
/* <string1> <index> <string2> putinterval - */
/* <bytestring1> <index> <string2> putinterval - */
private int
zputinterval(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr opindex = op - 1;
    os_ptr opto = opindex - 1;
    int code;

    switch (r_type(opto)) {
	default:
	    return_op_typecheck(opto);
	case t_mixedarray:
	case t_shortarray:
	    return_error(e_invalidaccess);
	case t_array:
	case t_string:
	    check_write(*opto);
	    check_int_leu(*opindex, r_size(opto));
	    code = copy_interval(i_ctx_p, opto, (uint)(opindex->value.intval),
				 op, "putinterval");
	    break;
	case t_astruct: {
	    uint dsize, ssize, index;

	    check_write(*opto);
	    if (gs_object_type(imemory, opto->value.pstruct) != &st_bytes)
		return_error(e_typecheck);
	    dsize = gs_object_size(imemory, opto->value.pstruct);
	    check_int_leu(*opindex, dsize);
	    index = (uint)opindex->value.intval;
	    check_read_type(*op, t_string);
	    ssize = r_size(op);
	    if (ssize > dsize - index)
		return_error(e_rangecheck);
	    memcpy(r_ptr(opto, byte) + index, op->value.const_bytes, ssize);
	    code = 0;
	    break;
	}
    }
    if (code >= 0)
	pop(3);
    return code;
}

/* <array|packedarray|string> <<element> proc> forall - */
/* <dict> <<key> <value> proc> forall - */
private int
    array_continue(i_ctx_t *),
    dict_continue(i_ctx_t *),
    string_continue(i_ctx_t *),
    packedarray_continue(i_ctx_t *);
private int forall_cleanup(i_ctx_t *);
private int
zforall(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr obj = op - 1;
    es_ptr ep = esp;
    es_ptr cproc = ep + 4;

    check_estack(6);
    switch (r_type(obj)) {
	default:
	    return_op_typecheck(obj);
	case t_array:
	    check_read(*obj);
	    make_op_estack(cproc, array_continue);
	    break;
	case t_dictionary:
	    check_dict_read(*obj);
	    make_int(cproc, dict_first(obj));
	    ++cproc;
	    make_op_estack(cproc, dict_continue);
	    break;
	case t_string:
	    check_read(*obj);
	    make_op_estack(cproc, string_continue);
	    break;
	case t_mixedarray:
	case t_shortarray:
	    check_read(*obj);
	    make_op_estack(cproc, packedarray_continue);
	    break;
    }
    check_proc(*op);
    /*
     * Push:
     *   - a mark;
     *   - the composite object;
     *   - the procedure;
     *   - the iteration index (only for dictionaries, done above);
     * and invoke the continuation operator.
     */
    make_mark_estack(ep + 1, es_for, forall_cleanup);
    ep[2] = *obj;
    ep[3] = *op;
    esp = cproc - 1;
    pop(2);
    return (*real_opproc(cproc))(i_ctx_p);
}
/* Continuation operator for arrays */
private int
array_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    es_ptr obj = esp - 1;

    if (r_size(obj)) {		/* continue */
	push(1);
	r_dec_size(obj, 1);
	*op = *obj->value.refs;
	obj->value.refs++;
	esp += 2;
	*esp = obj[1];
	return o_push_estack;
    } else {			/* done */
	esp -= 3;		/* pop mark, object, proc */
	return o_pop_estack;
    }
}
/* Continuation operator for dictionaries */
private int
dict_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    es_ptr obj = esp - 2;
    int index = (int)esp->value.intval;

    push(2);			/* make room for key and value */
    if ((index = dict_next(obj, index, op - 1)) >= 0) {	/* continue */
	esp->value.intval = index;
	esp += 2;
	*esp = obj[1];
	return o_push_estack;
    } else {			/* done */
	pop(2);			/* undo push */
	esp -= 4;		/* pop mark, object, proc, index */
	return o_pop_estack;
    }
}
/* Continuation operator for strings */
private int
string_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    es_ptr obj = esp - 1;

    if (r_size(obj)) {		/* continue */
	r_dec_size(obj, 1);
	push(1);
	make_int(op, *obj->value.bytes);
	obj->value.bytes++;
	esp += 2;
	*esp = obj[1];
	return o_push_estack;
    } else {			/* done */
	esp -= 3;		/* pop mark, object, proc */
	return o_pop_estack;
    }
}
/* Continuation operator for packed arrays */
private int
packedarray_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    es_ptr obj = esp - 1;

    if (r_size(obj)) {		/* continue */
	const ref_packed *packed = obj->value.packed;

	r_dec_size(obj, 1);
	push(1);
	packed_get(imemory, packed, op);
	obj->value.packed = packed_next(packed);
	esp += 2;
	*esp = obj[1];
	return o_push_estack;
    } else {			/* done */
	esp -= 3;		/* pop mark, object, proc */
	return o_pop_estack;
    }
}
/* Vacuous cleanup procedure */
private int
forall_cleanup(i_ctx_t *i_ctx_p)
{
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zgeneric_op_defs[] =
{
    {"1copy", zcopy},
    {"2forall", zforall},
    {"3.forceput", zforceput},
    {"2get", zget},
    {"3getinterval", zgetinterval},
    {"1length", zlength},
    {"3put", zput},
    {"3putinterval", zputinterval},
		/* Internal operators */
    {"0%array_continue", array_continue},
    {"0%dict_continue", dict_continue},
    {"0%packedarray_continue", packedarray_continue},
    {"0%string_continue", string_continue},
    op_def_end(0)
};

/* ------ Shared routines ------ */

/* Copy an interval from one operand to another. */
/* This is used by both putinterval and string/array copy. */
/* The destination is known to be an array or string, */
/* and the starting index is known to be less than or equal to */
/* its length; nothing else has been checked. */
private int
copy_interval(i_ctx_t *i_ctx_p /* for ref_assign_old */, os_ptr prto,
	      uint index, os_ptr prfrom, client_name_t cname)
{
    int fromtype = r_type(prfrom);
    uint fromsize = r_size(prfrom);

    if (!(fromtype == r_type(prto) ||
	  ((fromtype == t_shortarray || fromtype == t_mixedarray) &&
	   r_type(prto) == t_array))
	)
	return_op_typecheck(prfrom);
    check_read(*prfrom);
    check_write(*prto);
    if (fromsize > r_size(prto) - index)
	return_error(e_rangecheck);
    switch (fromtype) {
	case t_array:
	    {			/* We have to worry about aliasing, */
		/* but refcpy_to_old takes care of it for us. */
		return refcpy_to_old(prto, index, prfrom->value.refs,
				     fromsize, idmemory, cname);
	    }
	case t_string:
	    {	/* memmove takes care of aliasing. */
		memmove(prto->value.bytes + index, prfrom->value.bytes,
			fromsize);
	    }
	    break;
	case t_mixedarray:
	case t_shortarray:
	    {	/* We don't have to worry about aliasing, because */
		/* packed arrays are read-only and hence the destination */
		/* can't be a packed array. */
		int i;
		const ref_packed *packed = prfrom->value.packed;
		ref *pdest = prto->value.refs + index;
		ref elt;

		for (i = 0; i < fromsize; i++, pdest++) {
		    packed_get(imemory, packed, &elt);
		    ref_assign_old(prto, pdest, &elt, cname);
		    packed = packed_next(packed);
		}
	    }
	    break;
    }
    return 0;
}
