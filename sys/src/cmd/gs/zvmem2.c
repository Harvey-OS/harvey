/* Copyright (C) 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zvmem2.c */
/* Level 2 "Virtual memory" operators */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "estack.h"
#include "ialloc.h"			/* for ivmspace.h */
#include "ivmspace.h"
#include "store.h"

/* Garbage collector control parameters. */
#if arch_ints_are_short
#  define default_vm_threshold 20000
#else
#  define default_vm_threshold 250000
#endif
#define min_vm_threshold 1
#define max_vm_threshold max_long

/* ------ Local/global VM control ------ */

/* <bool> setglobal/setshared - */
int
zsetglobal(register os_ptr op)
{	check_type(*op, t_boolean);
	ialloc_set_space(idmemory,
			 (op->value.boolval ? avm_global : avm_local));
	pop(1);
	return 0;
}

/* <bool> currentglobal/currentshared - */
int
zcurrentglobal(register os_ptr op)
{	push(1);
	make_bool(op, ialloc_space(idmemory) != avm_local);
	return 0;
}

/* <any> gcheck/scheck <bool> */
int
zgcheck(register os_ptr op)
{	check_op(1);
	make_bool(op, (r_is_local(op) ? false : true));
	return 0;
}

/* ------ Garbage collector control ------ */

/* These routines are exported for setuserparams. */

/* <int> setvmthreshold - */
int
set_vm_threshold(long val)
{	gs_memory_gc_status_t stat;
	if ( val < -1 )
		return_error(e_rangecheck);
	else if ( val == -1 )
		val = default_vm_threshold;
	else if ( val < min_vm_threshold )
		val = min_vm_threshold;
	else if ( val > max_vm_threshold )
		val = max_vm_threshold;
	gs_memory_gc_status(idmemory->space_global, &stat);
	stat.vm_threshold = val;
	gs_memory_set_gc_status(idmemory->space_global, &stat);
	gs_memory_gc_status(idmemory->space_local, &stat);
	stat.vm_threshold = val;
	gs_memory_set_gc_status(idmemory->space_local, &stat);
	return 0;
}
int
zsetvmthreshold(register os_ptr op)
{	int code;
	check_type(*op, t_integer);
	code = set_vm_threshold(op->value.intval);
	if ( code >= 0 )
		pop(1);
	return code;
}

/* <int> vmreclaim - */
int
set_vm_reclaim(long val)
{	if ( val >= -2 && val <= 0 )
	{	gs_memory_gc_status_t stat;
		gs_memory_gc_status(idmemory->space_global, &stat);
		stat.enabled = val >= -1;
		gs_memory_set_gc_status(idmemory->space_global, &stat);
		gs_memory_gc_status(idmemory->space_local, &stat);
		stat.enabled = val == 0;
		gs_memory_set_gc_status(idmemory->space_local, &stat);
		return 0;
	}
	else
		return_error(e_rangecheck);
}	
int
zvmreclaim(register os_ptr op)
{	check_type(*op, t_integer);
	if ( op->value.intval == 1 || op->value.intval == 2 )
	  {	/* Force the interpreter to store its state and exit. */
		/* The interpreter's caller will do the actual GC. */
		return_error(e_VMreclaim);
	  }
	else
	  {	int code = set_vm_reclaim(op->value.intval);
		if ( code >= 0 )
		  pop(1);
		return code;
	  }
}

/* ------ Initialization procedure ------ */

/* The VM operators are defined even if the initial language level is 1, */
/* because we need them during initialization. */
BEGIN_OP_DEFS(zvmem2_op_defs) {
	{"0.currentglobal", zcurrentglobal},
	{"1.gcheck", zgcheck},
	{"1.setglobal", zsetglobal},
		/* The rest of the operators are defined only in Level 2. */
		op_def_begin_level2(),
	{"1setvmthreshold", zsetvmthreshold},
	{"1vmreclaim", zvmreclaim},
END_OP_DEFS(0) }
