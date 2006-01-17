/* Copyright (C) 1992, 1993, 1994, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zvmem2.c,v 1.7 2002/11/20 18:56:56 igor Exp $ */
/* Level 2 "Virtual memory" operators */
#include "ghost.h"
#include "oper.h"
#include "estack.h"
#include "ialloc.h"		/* for ivmspace.h */
#include "ivmspace.h"
#include "ivmem2.h"
#include "store.h"

/* Garbage collector control parameters. */
#define DEFAULT_VM_THRESHOLD_SMALL 100000
#define DEFAULT_VM_THRESHOLD_LARGE 1000000
#define MIN_VM_THRESHOLD 1
#define MAX_VM_THRESHOLD max_long

/* ------ Local/global VM control ------ */

/* <bool> .setglobal - */
private int
zsetglobal(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    check_type(*op, t_boolean);
    ialloc_set_space(idmemory,
		     (op->value.boolval ? avm_global : avm_local));
    pop(1);
    return 0;
}

/* <bool> .currentglobal - */
private int
zcurrentglobal(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_bool(op, ialloc_space(idmemory) != avm_local);
    return 0;
}

/* <any> gcheck/scheck <bool> */
private int
zgcheck(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_op(1);
    make_bool(op, (r_is_local(op) ? false : true));
    return 0;
}

/* ------ Garbage collector control ------ */

/* These routines are exported for setuserparams. */

/*
 * <int> setvmthreshold -
 *
 * This is implemented as a PostScript procedure that calls setuserparams.
 */
int
set_vm_threshold(i_ctx_t *i_ctx_p, long val)
{
    if (val < -1)
	return_error(e_rangecheck);
    else if (val == -1)
	val = (gs_debug_c('.') ? DEFAULT_VM_THRESHOLD_SMALL :
	       DEFAULT_VM_THRESHOLD_LARGE);
    else if (val < MIN_VM_THRESHOLD)
	val = MIN_VM_THRESHOLD;
    else if (val > MAX_VM_THRESHOLD)
	val = MAX_VM_THRESHOLD;
    gs_memory_set_vm_threshold(idmemory->space_global, val);
    gs_memory_set_vm_threshold(idmemory->space_local, val);
    return 0;
}

int
set_vm_reclaim(i_ctx_t *i_ctx_p, long val)
{
    if (val >= -2 && val <= 0) {
	gs_memory_set_vm_reclaim(idmemory->space_system, (val >= -1));
	gs_memory_set_vm_reclaim(idmemory->space_global, (val >= -1));
	gs_memory_set_vm_reclaim(idmemory->space_local, (val == 0));
	return 0;
    } else
	return_error(e_rangecheck);
}

/*
 * <int> .vmreclaim -
 *
 * This implements only immediate garbage collection: enabling and
 * disabling GC is implemented by calling setuserparams.
 */
private int
zvmreclaim(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_integer);
    if (op->value.intval == 1 || op->value.intval == 2) {
	/* Force the interpreter to store its state and exit. */
	/* The interpreter's caller will do the actual GC. */
	return_error(e_VMreclaim);
    }
    return_error(e_rangecheck);
}

/* ------ Initialization procedure ------ */

/* The VM operators are defined even if the initial language level is 1, */
/* because we need them during initialization. */
const op_def zvmem2_op_defs[] =
{
    {"0.currentglobal", zcurrentglobal},
    {"1.gcheck", zgcheck},
    {"1.setglobal", zsetglobal},
		/* The rest of the operators are defined only in Level 2. */
    op_def_begin_level2(),
    {"1.vmreclaim", zvmreclaim},
    op_def_end(0)
};
