/* Copyright (C) 1989, 1990, 1991, 1992 Aladdin Enterprises.  All rights reserved.
  
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

/* zstring.c */
/* String operators */
#include "memory_.h"
#include "ghost.h"
#include "errors.h"
#include "gsutil.h"
#include "ialloc.h"
#include "iname.h"
#include "ivmspace.h"
#include "oper.h"
#include "store.h"

/* The generic operators (copy, get, put, getinterval, putinterval, */
/* length, and forall) are implemented in zgeneric.c. */

/* <int> string <string> */
int
zstring(register os_ptr op)
{	byte *sbody;
	uint size;
	check_int_leu(*op, max_string_size);
	size = op->value.intval;
	sbody = ialloc_string(size, "string");
	if ( sbody == 0 )
		return_error(e_VMerror);
	make_string(op, a_all | icurrent_space, size, sbody);
	memset(sbody, 0, size);
	return 0;
}

/* <string> <pattern> anchorsearch <post> <match> -true- */
/* <string> <pattern> anchorsearch <string> -false- */
int
zanchorsearch(register os_ptr op)
{	os_ptr op1 = op - 1;
	uint size = r_size(op);
	check_read_type(*op1, t_string);
	check_read_type(*op, t_string);
	if ( size <= r_size(op1) && !memcmp(op1->value.bytes, op->value.bytes, size) )
	{	os_ptr op0 = op;
		push(1);
		*op0 = *op1;
		r_set_size(op0, size);
		op1->value.bytes += size;
		r_dec_size(op1, size);
		make_true(op);
	}
	else
		make_false(op);
	return 0;
}

/* <string> <pattern> search <post> <match> <pre> -true- */
/* <string> <pattern> search <string> -false- */
int
zsearch(register os_ptr op)
{	os_ptr op1 = op - 1;
	uint size = r_size(op);
	uint count;
	byte *ptr;
	check_read_type(*op1, t_string);
	check_read_type(*op, t_string);
	if ( size > r_size(op1) )		/* can't match */
	   {	make_false(op);
		return 0;
	   }
	count = r_size(op1) - size;
	ptr = op1->value.bytes;
	do
	   {	if ( !memcmp(ptr, op->value.bytes, size) )
		   {	op->tas.type_attrs = op1->tas.type_attrs;
			op->value.bytes = ptr;
			r_set_size(op, size);
			push(2);
			op[-1] = *op1;
			r_set_size(op - 1, ptr - op[-1].value.bytes);
			op1->value.bytes = ptr + size;
			r_set_size(op1, count);
			make_true(op);
			return 0;
		   }
		ptr++;
	   }
	while ( count-- );
	/* No match */
	make_false(op);
	return 0;
}

/* <obj> <pattern> .stringmatch <bool> */
int
zstringmatch(register os_ptr op)
{	os_ptr op1 = op - 1;
	bool result;
	check_read_type(*op, t_string);
	switch ( r_type(op1) )
	   {
	case t_string:
		check_read(*op1);
		goto cmp;
	case t_name:
		name_string_ref(op1, op1);		/* can't fail */
cmp:		result = string_match(op1->value.const_bytes, r_size(op1),
				      op->value.const_bytes, r_size(op),
				      NULL);
		break;
	default:
		result = (r_size(op) == 1 && *op->value.bytes == '*');
	   }
	make_bool(op1, result);
	pop(1);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zstring_op_defs) {
	{"2anchorsearch", zanchorsearch},
	{"2search", zsearch},
	{"1string", zstring},
	{"2.stringmatch", zstringmatch},
END_OP_DEFS(0) }
