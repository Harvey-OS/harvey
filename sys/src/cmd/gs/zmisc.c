/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zmisc.c */
/* Miscellaneous operators */
#include "errno_.h"
#include "memory_.h"
#include "string_.h"
#include "ghost.h"
#include "gscdefs.h"			/* for gs_serialnumber */
#include "gp.h"
#include "errors.h"
#include "oper.h"
#include "ialloc.h"
#include "idict.h"
#include "dstack.h"			/* for name lookup in bind */
#include "iname.h"
#include "ipacked.h"
#include "ivmspace.h"
#include "store.h"

/* Import the C getenv function. */
extern char *getenv(P1(const char *));

/* <proc> bind <proc> */
int
zbind(os_ptr op)
{	uint depth = 1;
	ref defn;
	register os_ptr bsp;
	switch ( r_type(op) )
	   {
	case t_array:
	case t_mixedarray:
	case t_shortarray:
		defn = *op;
		break;
	case t_oparray:
		defn = op_array_table.value.refs[op_index(op) - op_def_count];
		break;
	default:
		return_op_typecheck(op);
	   }
	push(1);
	*op = defn;
	bsp = op;
	/*
	 * We must not make the top-level procedure read-only,
	 * but we must bind it even if it is read-only already.
	 *
	 * Here are the invariants for the following loop:
	 *	`depth' elements have been pushed on the ostack;
	 *	For i < depth, p = ref_stack_index(&o_stack, i):
	 *	  *p is an array (or packedarray) ref. */
#define r_is_ex_oper(rp)\
  ((r_btype(rp) == t_operator || r_type(rp) == t_oparray) &&\
   r_has_attr(rp, a_executable))
	while ( depth )
	   {	while ( r_size(bsp) )
		   {	ref *tp = bsp->value.refs;
			r_dec_size(bsp, 1);
			if ( r_is_packed(tp) )
			 { /* Check for a packed executable name */
			   ushort elt = *(ushort *)tp;
			   if ( r_packed_is_exec_name(&elt) )
			    { ref nref;
			      ref *pvalue;
			      name_index_ref(packed_name_index(&elt),
					     &nref);
			      if ( (pvalue = dict_find_name(&nref)) != 0 &&
				   r_is_ex_oper(pvalue)
				 )
				/* Note: can't undo this by restore! */
				*(ushort *)tp =
				  pt_tag(pt_executable_operator) +
				  op_index(pvalue);
			    }
			   bsp->value.refs = (ref *)((ref_packed *)tp + 1);
			 }
			else
			  switch ( bsp->value.refs++, r_type(tp) )
			 {
			case t_name:	/* bind the name if an operator */
			  if ( r_has_attr(tp, a_executable) )
			   {	ref *pvalue;
				if ( (pvalue = dict_find_name(tp)) != 0 &&
				     r_is_ex_oper(pvalue)
				   )
					ref_assign_old(bsp, tp, pvalue, "bind");
			   }
			  break;
			case t_array:	/* push into array if procedure */
			  if ( !r_has_attr(tp, a_write) ) break;
			case t_mixedarray:
			case t_shortarray:
			  if ( r_has_attr(tp, a_executable) )
			   {	/* Make reference read-only */
				r_clear_attrs(tp, a_write);
				if ( bsp >= ostop )
				  {	/* Push a new stack block. */
					ref temp;
					int code;
					temp = *tp;
					osp = bsp;
					code = ref_stack_push(&o_stack, 1);
					if ( code < 0 )
					  {	ref_stack_pop(&o_stack, depth);
						return_error(code);
					  }
					bsp = osp;
					*bsp = temp;
				  }
				else
					*++bsp = *tp;
				depth++;
			   }
			 }
		   }
		bsp--; depth--;
		if ( bsp < osbot )
		  {	/* Pop back to the previous stack block. */
			osp = bsp;
			ref_stack_pop_block(&o_stack);
			bsp = osp;
		  }
	   }
	osp = bsp;
	return 0;
}

/* - serialnumber <int> */
int
zserialnumber(register os_ptr op)
{	push(1);
	make_int(op, gs_serialnumber);
	return 0;
}

/* - usertime <int> */
int
zusertime(register os_ptr op)
{	long date_time[2];
	gp_get_clock(date_time);
	push(1);
	make_int(op, date_time[0] * 86400000L + date_time[1]);
	return 0;
}

/* ---------------- Non-standard operators ---------------- */

/* - .currenttime <int> */
int
zcurrenttime(register os_ptr op)
{	long date_time[2];
	gp_get_clock(date_time);
	push(1);
	make_real(op, date_time[0] * 1440.0 + date_time[1] / 60000.0);
	return 0;
}

/* <string> getenv <value_string> true */
/* <string> getenv false */
int
zgetenv(register os_ptr op)
{	char *str, *value;
	int code;
	check_read_type(*op, t_string);
	str = ref_to_string(op, imemory, "getenv name");
	if ( str == 0 )
		return_error(e_VMerror);
	value = getenv(str);
	ifree_string((byte *)str, r_size(op) + 1, "getenv name");
	if ( value == 0 )		/* not found */
	   {	make_bool(op, 0);
		return 0;
	   }
	code = string_to_ref(value, op, iimemory, "getenv value");
	if ( code < 0 ) return code;
	push(1);
	make_bool(op, 1);
	return 0;
}

/* Turn a procedure into an operator. */
/* The caller has checked that *op is a procedure, and has done */
/* a store check. */
private int
make_operator(register os_ptr op, byte space)
{	check_type(op[-1], t_name);
	if ( op_array_count == r_size(&op_array_table) )
	  return_error(e_limitcheck);
	ref_assign_old(&op_array_table,
		       &op_array_table.value.refs[op_array_count],
		       op, "makeoperator");
	op_array_nx_table[op_array_count] = name_index(op - 1);
	op_array_attrs_table[op_array_count] = space | a_executable;
	op_index_oparray_ref(op_def_count + op_array_count, op - 1);
	op_array_count++;
	pop(1);
	return 0;
}
/* <name> <proc> .makeoperator <oper> */
int
zmakeoperator(register os_ptr op)
{	check_proc(*op);
	store_check_dest(&op_array_table, op);
	return make_operator(op, r_space(op));
}
/* <name> <proc> .makeoperator <oper> */
int
zmakeglobaloperator(register os_ptr op)
{	uint space = r_space(op);
	check_proc(*op);
	store_check_dest(&op_array_table, op);
	if ( space > avm_global )
	  {	if ( alloc_save_level(idmemory) != 0 )
		  return_error(e_invalidaccess);
		space = avm_global;
	  }
	return make_operator(op, space);
}

/* - .oserrno <int> */
int
zoserrno(register os_ptr op)
{	push(1);
	make_int(op, errno);
	return 0;
}

/* <int> .setoserrno - */
int
zsetoserrno(register os_ptr op)
{	check_type(*op, t_integer);
	errno = op->value.intval;
	pop(1);
	return 0;
}

/* <int> .oserrorstring <string> true */
/* <int> .oserrorstring false */
int
zoserrorstring(register os_ptr op)
{	const char *str;
	int code;	
	uint len;
	byte ch;
	check_type(*op, t_integer);
	str = gp_strerror((int)op->value.intval);
	if ( str == 0 || (len = strlen(str)) == 0 )
	{	make_false(op);
		return 0;
	}
	check_ostack(1);
	code = string_to_ref(str, op, iimemory, ".oserrorstring");
	if ( code < 0 )
		return code;
	/* Strip trailing end-of-line characters. */
	while ( (len = r_size(op)) != 0 &&
		((ch = op->value.bytes[--len]) == '\r' || ch == '\n')
	      )
		r_dec_size(op, 1);
	push(1);
	make_true(op);
	return 0;
}

/* <string> <bool> .setdebug - */
int
zsetdebug(register os_ptr op)
{	check_read_type(op[-1], t_string);
	check_type(*op, t_boolean);
	   {	int i;
		for ( i = 0; i < r_size(op - 1); i++ )
			gs_debug[op[-1].value.bytes[i] & 127] =
				op->value.boolval;
	   }
	pop(2);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zmisc_op_defs) {
	{"1bind", zbind},
	{"0.currenttime", zcurrenttime},
	{"1getenv", zgetenv},
	{"2.makeglobaloperator", zmakeglobaloperator},
	{"2.makeoperator", zmakeoperator},
	{"0.oserrno", zoserrno},
	{"1.oserrorstring", zoserrorstring},
	{"1serialnumber", zserialnumber},
	{"2.setdebug", zsetdebug},
	{"1.setoserrno", zsetoserrno},
	{"0usertime", zusertime},
END_OP_DEFS(0) }
