/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* zarray.c */
/* Array operators */
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "ialloc.h"
#include "ipacked.h"
#include "oper.h"
#include "store.h"

/* The generic operators (copy, get, put, getinterval, putinterval, */
/* length, and forall) are implemented in zgeneric.c. */

/* <int> array <array> */
int
zarray(register os_ptr op)
{	uint size;
	int code;
	check_int_leu(*op, max_array_size);
	size = op->value.intval;
	code = ialloc_ref_array((ref *)op, a_all, size, "array");
	if ( code < 0 )
		return code;
	refset_null(op->value.refs, size);
	return 0;
}

/* <array> aload <obj_0> ... <obj_n-1> <array> */
int
zaload(register os_ptr op)
{	ref aref;
	ref_assign(&aref, op);
	if ( !r_is_array(&aref) )
	  return_op_typecheck(op);
	check_read(aref);
#define asize r_size(&aref)
	if ( asize > ostop - op )
	{	/* Use the slow, general algorithm. */
		int code = ref_stack_push(&o_stack, asize);
		uint i;
		const ref_packed *packed = aref.value.packed;
		if ( code < 0 )
		  return code;
		for ( i = asize; i > 0; i--, packed = packed_next(packed) )
		  packed_get(packed, ref_stack_index(&o_stack, i));
		*osp = aref;
		return 0;
	}
	if ( r_has_type(&aref, t_array) )
		memcpy((char *)op, (const char *)aref.value.refs,
		       asize * sizeof(ref));
	else
	   {	register ushort i;
		const ref_packed *packed = aref.value.packed;
		os_ptr pdest = op;
		for ( i = 0; i < asize; i++, pdest++ )
			packed_get(packed, pdest),
			packed = packed_next(packed);
	   }
	push(asize);
#undef asize
	ref_assign(op, &aref);
	return 0;
}

/* <obj_0> ... <obj_n-1> <array> astore <array> */
int
zastore(register os_ptr op)
{	uint size;
	int code;
	check_write_type(*op, t_array);
	size = r_size(op);
	if ( size > op - osbot )
	  {	/* The store operation might involve other stack segments. */
		ref arr;
		if ( size >= ref_stack_count(&o_stack) )
		  return_error(e_stackunderflow);
		arr = *op;
		code = ref_stack_store(&o_stack, &arr, size, 1, 0, true,
				       "astore");
		if ( code < 0 )
		  return code;
		ref_stack_pop(&o_stack, size);
		*ref_stack_index(&o_stack, 0) = arr;
	  }
	else
	  {	code = refcpy_to_old(op, 0, op - size, size, "astore");
		if ( code < 0 )
		  return code;
		op[-(int)size] = *op;
		pop(size);
	  }
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zarray_op_defs) {
	{"1aload", zaload},
	{"1array", zarray},
	{"1astore", zastore},
END_OP_DEFS(0) }
