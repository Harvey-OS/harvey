/* Copyright (C) 1989, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zvmem.c,v 1.8 2004/08/04 19:36:13 stefan Exp $ */
/* "Virtual memory" operators */
#include "ghost.h"
#include "gsstruct.h"
#include "oper.h"
#include "estack.h"		/* for checking in restore */
#include "ialloc.h"
#include "idict.h"		/* ditto */
#include "igstate.h"
#include "isave.h"
#include "dstack.h"
#include "stream.h"		/* for files.h */
#include "files.h"		/* for e-stack processing */
#include "store.h"
#include "gsmatrix.h"		/* for gsstate.h */
#include "gsstate.h"

/* Define whether we validate memory before/after save/restore. */
/* Note that we only actually do this if DEBUG is set and -Z? is selected. */
private const bool I_VALIDATE_BEFORE_SAVE = true;
private const bool I_VALIDATE_AFTER_SAVE = true;
private const bool I_VALIDATE_BEFORE_RESTORE = true;
private const bool I_VALIDATE_AFTER_RESTORE = true;

/* 'Save' structure */
typedef struct vm_save_s vm_save_t;
struct vm_save_s {
    gs_state *gsave;		/* old graphics state */
};

gs_private_st_ptrs1(st_vm_save, vm_save_t, "savetype",
		    vm_save_enum_ptrs, vm_save_reloc_ptrs, gsave);

/* Clean up the stacks and validate storage. */
private void
ivalidate_clean_spaces(i_ctx_t *i_ctx_p)
{
    if (gs_debug_c('?')) {
	ref_stack_cleanup(&d_stack);
	ref_stack_cleanup(&e_stack);
	ref_stack_cleanup(&o_stack);
	ivalidate_spaces();
    }
}

/* - save <save> */
int
zsave(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint space = icurrent_space;
    vm_save_t *vmsave;
    ulong sid;
    int code;
    gs_state *prev;

    if (I_VALIDATE_BEFORE_SAVE)
	ivalidate_clean_spaces(i_ctx_p);
    ialloc_set_space(idmemory, avm_local);
    vmsave = ialloc_struct(vm_save_t, &st_vm_save, "zsave");
    ialloc_set_space(idmemory, space);
    if (vmsave == 0)
	return_error(e_VMerror);
    sid = alloc_save_state(idmemory, vmsave);
    if (sid == 0) {
	ifree_object(vmsave, "zsave");
	return_error(e_VMerror);
    }
    if_debug2('u', "[u]vmsave 0x%lx, id = %lu\n",
	      (ulong) vmsave, (ulong) sid);
    code = gs_gsave_for_save(igs, &prev);
    if (code < 0)
	return code;
    code = gs_gsave(igs);
    if (code < 0)
	return code;
    vmsave->gsave = prev;
    push(1);
    make_tav(op, t_save, 0, saveid, sid);
    if (I_VALIDATE_AFTER_SAVE)
	ivalidate_clean_spaces(i_ctx_p);
    return 0;
}

/* <save> restore - */
private int restore_check_operand(os_ptr, alloc_save_t **, gs_dual_memory_t *);
private int restore_check_stack(const ref_stack_t *, const alloc_save_t *, bool);
private void restore_fix_stack(ref_stack_t *, const alloc_save_t *, bool);
int
zrestore(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    alloc_save_t *asave;
    bool last;
    vm_save_t *vmsave;
    int code = restore_check_operand(op, &asave, idmemory);

    if (code < 0)
	return code;
    if_debug2('u', "[u]vmrestore 0x%lx, id = %lu\n",
	      (ulong) alloc_save_client_data(asave),
	      (ulong) op->value.saveid);
    if (I_VALIDATE_BEFORE_RESTORE)
	ivalidate_clean_spaces(i_ctx_p);
    /* Check the contents of the stacks. */
    osp--;
    {
	int code;

	if ((code = restore_check_stack(&o_stack, asave, false)) < 0 ||
	    (code = restore_check_stack(&e_stack, asave, true)) < 0 ||
	    (code = restore_check_stack(&d_stack, asave, false)) < 0
	    ) {
	    osp++;
	    return code;
	}
    }
    /* Reset l_new in all stack entries if the new save level is zero. */
    /* Also do some special fixing on the e-stack. */
    restore_fix_stack(&o_stack, asave, false);
    restore_fix_stack(&e_stack, asave, true);
    restore_fix_stack(&d_stack, asave, false);
    /* Iteratively restore the state of memory, */
    /* also doing a grestoreall at each step. */
    do {
	vmsave = alloc_save_client_data(alloc_save_current(idmemory));
	/* Restore the graphics state. */
	gs_grestoreall_for_restore(igs, vmsave->gsave);
	/*
	 * If alloc_save_space decided to do a second save, the vmsave
	 * object was allocated one save level less deep than the
	 * current level, so ifree_object won't actually free it;
	 * however, it points to a gsave object that definitely
	 * *has* been freed.  In order not to trip up the garbage
	 * collector, we clear the gsave pointer now.
	 */
	vmsave->gsave = 0;
	/* Now it's safe to restore the state of memory. */
	last = alloc_restore_state_step(asave);
    }
    while (!last);
    {
	uint space = icurrent_space;

	ialloc_set_space(idmemory, avm_local);
	ifree_object(vmsave, "zrestore");
	ialloc_set_space(idmemory, space);
    }
    dict_set_top();		/* reload dict stack cache */
    if (I_VALIDATE_AFTER_RESTORE)
	ivalidate_clean_spaces(i_ctx_p);
    /* If the i_ctx_p LockFilePermissions is true, but the userparams */
    /* we just restored is false, we need to make sure that we do not */
    /* cause an 'invalidaccess' in setuserparams. Temporarily set     */
    /* LockFilePermissions false until the gs_lev2.ps can do a        */
    /* setuserparams from the restored userparam dictionary.          */
    i_ctx_p->LockFilePermissions = false;
    return 0;
}
/* Check the operand of a restore. */
private int
restore_check_operand(os_ptr op, alloc_save_t ** pasave,
		      gs_dual_memory_t *idmem)
{
    vm_save_t *vmsave;
    ulong sid;
    alloc_save_t *asave;

    check_type(*op, t_save);
    vmsave = r_ptr(op, vm_save_t);
    if (vmsave == 0)		/* invalidated save */
	return_error(e_invalidrestore);
    sid = op->value.saveid;
    asave = alloc_find_save(idmem, sid);
    if (asave == 0)
	return_error(e_invalidrestore);
    *pasave = asave;
    return 0;
}
/* Check a stack to make sure all its elements are older than a save. */
private int
restore_check_stack(const ref_stack_t * pstack, const alloc_save_t * asave,
		    bool is_estack)
{
    ref_stack_enum_t rsenum;

    ref_stack_enum_begin(&rsenum, pstack);
    do {
	const ref *stkp = rsenum.ptr;
	uint size = rsenum.size;

	for (; size; stkp++, size--) {
	    const void *ptr;

	    switch (r_type(stkp)) {
		case t_array:
		    ptr = stkp->value.refs;
		    break;
		case t_dictionary:
		    ptr = stkp->value.pdict;
		    break;
		case t_file:
		    /* Don't check executable or closed literal */
		    /* files on the e-stack. */
		    {
			stream *s;

			if (is_estack &&
			    (r_has_attr(stkp, a_executable) ||
			     file_is_invalid(s, stkp))
			    )
			    continue;
		    }
		    ptr = stkp->value.pfile;
		    break;
		case t_name:
		    /* Names are special because of how they are allocated. */
		    if (alloc_name_is_since_save((const gs_memory_t *)pstack->memory,
						 stkp, asave))
			return_error(e_invalidrestore);
		    continue;
		case t_string:
		    /* Don't check empty executable strings */
		    /* on the e-stack. */
		    if (r_size(stkp) == 0 &&
			r_has_attr(stkp, a_executable) && is_estack
			)
			continue;
		    ptr = stkp->value.bytes;
		    break;
		case t_mixedarray:
		case t_shortarray:
		    ptr = stkp->value.packed;
		    break;
		case t_device:
		    ptr = stkp->value.pdevice;
		    break;
		case t_fontID:
		case t_struct:
		case t_astruct:
		    ptr = stkp->value.pstruct;
		    break;
		default:
		    continue;
	    }
	    if (alloc_is_since_save(ptr, asave))
		return_error(e_invalidrestore);
	}
    } while (ref_stack_enum_next(&rsenum));
    return 0;		/* OK */
}
/*
 * If the new save level is zero, fix up the contents of a stack
 * by clearing the l_new bit in all the entries (since we can't tolerate
 * values with l_new set if the save level is zero).
 * Also, in any case, fix up the e-stack by replacing empty executable
 * strings and closed executable files that are newer than the save
 * with canonical ones that aren't.
 *
 * Note that this procedure is only called if restore_check_stack succeeded.
 */
private void
restore_fix_stack(ref_stack_t * pstack, const alloc_save_t * asave,
		  bool is_estack)
{
    ref_stack_enum_t rsenum;

    ref_stack_enum_begin(&rsenum, pstack);
    do {
	ref *stkp = rsenum.ptr;
	uint size = rsenum.size;

	for (; size; stkp++, size--) {
	    r_clear_attrs(stkp, l_new);		/* always do it, no harm */
	    if (is_estack) {
		ref ofile;

		ref_assign(&ofile, stkp);
		switch (r_type(stkp)) {
		    case t_string:
			if (r_size(stkp) == 0 &&
			    alloc_is_since_save(stkp->value.bytes,
						asave)
			    ) {
			    make_empty_const_string(stkp,
						    avm_foreign);
			    break;
			}
			continue;
		    case t_file:
			if (alloc_is_since_save(stkp->value.pfile,
						asave)
			    ) {
			    make_invalid_file(stkp);
			    break;
			}
			continue;
		    default:
			continue;
		}
		r_copy_attrs(stkp, a_all | a_executable,
			     &ofile);
	    }
	}
    } while (ref_stack_enum_next(&rsenum));
}

/* - vmstatus <save_level> <vm_used> <vm_maximum> */
private int
zvmstatus(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_memory_status_t mstat, dstat;

    gs_memory_status(imemory, &mstat);
    if (imemory == imemory_global) {
	gs_memory_status_t sstat;

	gs_memory_status(imemory_system, &sstat);
	mstat.allocated += sstat.allocated;
	mstat.used += sstat.used;
    }
    gs_memory_status(imemory->non_gc_memory, &dstat);
    push(3);
    make_int(op - 2, imemory_save_level(iimemory_local));
    make_int(op - 1, mstat.used);
    make_int(op, mstat.allocated + dstat.allocated - dstat.used);
    return 0;
}

/* ------ Non-standard extensions ------ */

/* <save> .forgetsave - */
private int
zforgetsave(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    alloc_save_t *asave;
    vm_save_t *vmsave;
    int code = restore_check_operand(op, &asave, idmemory);

    if (code < 0)
	return 0;
    vmsave = alloc_save_client_data(asave);
    /* Reset l_new in all stack entries if the new save level is zero. */
    restore_fix_stack(&o_stack, asave, false);
    restore_fix_stack(&e_stack, asave, false);
    restore_fix_stack(&d_stack, asave, false);
    /*
     * Forget the gsaves, by deleting the bottom gstate on
     * the current stack and the top one on the saved stack and then
     * concatenating the stacks together.
     */
    {
	gs_state *pgs = igs;
	gs_state *last;

	while (gs_state_saved(last = gs_state_saved(pgs)) != 0)
	    pgs = last;
	gs_state_swap_saved(last, vmsave->gsave);
	gs_grestore(last);
	gs_grestore(last);
    }
    /* Forget the save in the memory manager. */
    alloc_forget_save(asave);
    {
	uint space = icurrent_space;

	ialloc_set_space(idmemory, avm_local);
	/* See above for why we clear the gsave pointer here. */
	vmsave->gsave = 0;
	ifree_object(vmsave, "zrestore");
	ialloc_set_space(idmemory, space);
    }
    pop(1);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zvmem_op_defs[] =
{
    {"1.forgetsave", zforgetsave},
    {"1restore", zrestore},
    {"0save", zsave},
    {"0vmstatus", zvmstatus},
    op_def_end(0)
};
