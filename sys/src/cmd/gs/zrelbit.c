/* Copyright (C) 1989, 1990, 1991, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zrelbit.c */
/* Relational, boolean, and bit operators */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsutil.h"
#include "idict.h"
#include "store.h"

/* Forward references */
private int near obj_le(P2(os_ptr, os_ptr));

/* <obj1> <obj2> eq <bool> */
int
zeq(register os_ptr op)
{	register os_ptr op1 = op - 1;
#define eq_check_read(opp, dflt)\
  switch ( r_type(opp) )\
   {	case t_string: case t_array: case t_mixedarray: case t_shortarray:\
	  check_read(*opp); break;\
	case t_dictionary: check_dict_read(*opp); break;\
	default: dflt; break;\
   }
	eq_check_read(op1, check_op(2));
	eq_check_read(op, DO_NOTHING);
	make_bool(op1, (obj_eq(op1, op) ? 1 : 0));
	pop(1);
	return 0;
}

/* <obj1> <obj2> ne <bool> */
int
zne(register os_ptr op)
{	/* We'll just be lazy and use eq. */
	int code = zeq(op);
	if ( !code ) op[-1].value.boolval ^= 1;
	return code;
}

/* <num1> <num2> ge <bool> */
/* <str1> <str2> ge <bool> */
int
zge(register os_ptr op)
{	int code = obj_le(op, op - 1);
	if ( code < 0 ) return code;
	make_bool(op - 1, code);
	pop(1);
	return 0;
}

/* <num1> <num2> gt <bool> */
/* <str1> <str2> gt <bool> */
int
zgt(register os_ptr op)
{	int code = obj_le(op - 1, op);
	if ( code < 0 ) return code;
	make_bool(op - 1, code ^ 1);
	pop(1);
	return 0;
}

/* <num1> <num2> le <bool> */
/* <str1> <str2> le <bool> */
int
zle(register os_ptr op)
{	int code = obj_le(op - 1, op);
	if ( code < 0 ) return code;
	make_bool(op - 1, code);
	pop(1);
	return 0;
}

/* <num1> <num2> lt <bool> */
/* <str1> <str2> lt <bool> */
int
zlt(register os_ptr op)
{	int code = obj_le(op, op - 1);
	if ( code < 0 ) return code;
	make_bool(op - 1, code ^ 1);
	pop(1);
	return 0;
}

/* <num1> <num2> .max <num> */
/* <str1> <str2> .max <str> */
int
zmax(register os_ptr op)
{	int code = obj_le(op - 1, op);
	if ( code < 0 ) return code;
	if ( code )
	   {	ref_assign(op - 1, op);
	   }
	pop(1);
	return 0;
}

/* <num1> <num2> .min <num> */
/* <str1> <str2> .min <str> */
int
zmin(register os_ptr op)
{	int code = obj_le(op - 1, op);
	if ( code < 0 ) return code;
	if ( !code )
	   {	ref_assign(op - 1, op);
	   }
	pop(1);
	return 0;
}

/* <bool1> <bool2> and <bool> */
/* <int1> <int2> and <int> */
int
zand(register os_ptr op)
{	switch ( r_type(op) )
	   {
	case t_boolean:
		check_type(op[-1], t_boolean);
		op[-1].value.boolval &= op->value.boolval;
		break;
	case t_integer:
		check_type(op[-1], t_integer);
		op[-1].value.intval &= op->value.intval;
		break;
	default:
		return_op_typecheck(op);
	   }
	pop(1);
	return 0;
}

/* <bool> not <bool> */
/* <int> not <int> */
int
znot(register os_ptr op)
{	switch ( r_type(op) )
	   {
	case t_boolean:
		op->value.boolval = !op->value.boolval;
		break;
	case t_integer:
		op->value.intval = ~op->value.intval;
		break;
	default:
		return_op_typecheck(op);
	   }
	return 0;
}

/* <bool1> <bool2> or <bool> */
/* <int1> <int2> or <int> */
int
zor(register os_ptr op)
{	switch ( r_type(op) )
	   {
	case t_boolean:
		check_type(op[-1], t_boolean);
		op[-1].value.boolval |= op->value.boolval;
		break;
	case t_integer:
		check_type(op[-1], t_integer);
		op[-1].value.intval |= op->value.intval;
		break;
	default:
		return_op_typecheck(op);
	   }
	pop(1);
	return 0;
}

/* <bool1> <bool2> xor <bool> */
/* <int1> <int2> xor <int> */
int
zxor(register os_ptr op)
{	switch ( r_type(op) )
	   {
	case t_boolean:
		check_type(op[-1], t_boolean);
		op[-1].value.boolval ^= op->value.boolval;
		break;
	case t_integer:
		check_type(op[-1], t_integer);
		op[-1].value.intval ^= op->value.intval;
		break;
	default:
		return_op_typecheck(op);
	   }
	pop(1);
	return 0;
}

/* <int> <shift> bitshift <int> */
int
zbitshift(register os_ptr op)
{	int shift;
	check_type(*op, t_integer);
	check_type(op[-1], t_integer);
#define max_shift (arch_sizeof_long * 8 - 1)
	if ( op->value.intval < -max_shift || op->value.intval > max_shift )
		op[-1].value.intval = 0;
#undef max_shift
	else if ( (shift = op->value.intval) < 0 )
		op[-1].value.intval = ((ulong)(op[-1].value.intval)) >> -shift;
	else
		op[-1].value.intval <<= shift;
	pop(1);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zrelbit_op_defs) {
	{"2and", zand},
	{"2bitshift", zbitshift},
	{"2eq", zeq},
	{"2ge", zge},
	{"2gt", zgt},
	{"2le", zle},
	{"2lt", zlt},
	{"2.max", zmax},
	{"2.min", zmin},
	{"2ne", zne},
	{"1not", znot},
	{"2or", zor},
	{"2xor", zxor},
END_OP_DEFS(0) }

/* ------ Internal routines ------ */

/* Compare two operands (both numeric, or both strings). */
/* Return 1 if op[-1] <= op[0], 0 if op[-1] > op[0], */
/* or a (negative) error code. */
#define bcval(v1, rel, v2) (op1->value.v1 rel op->value.v2 ? 1 : 0)
private int near
obj_le(register os_ptr op1, register os_ptr op)
{	switch ( r_type(op1) )
	  {
	  case t_integer:
	    switch ( r_type(op) )
	      {
	      case t_integer:
		      return bcval(intval, <=, intval);
	      case t_real:
		      return bcval(intval, <=, realval);
	      default:
		      return_op_typecheck(op);
	      }
	  case t_real:
	    switch ( r_type(op) )
	      {
	      case t_real:
		      return bcval(realval, <=, realval);
	      case t_integer:
		      return bcval(realval, <=, intval);
	      default:
		      return_op_typecheck(op);
	      }
	  case t_string:
	    check_read(*op1);
	    check_read_type(*op, t_string);
	    return (bytes_compare(op1->value.bytes, r_size(op1),
			          op->value.bytes, r_size(op)) <= 0 ? 1 : 0);
	  default:
		  return_op_typecheck(op1);
	  }
}
