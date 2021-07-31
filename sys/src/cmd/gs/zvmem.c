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

/* zvmem.c */
/* "Virtual memory" operators */
#include "ghost.h"
#include "gsstruct.h"
#include "errors.h"
#include "oper.h"
#include "estack.h"			/* for checking in restore */
#include "ialloc.h"
#include "idict.h"			/* ditto */
#include "igstate.h"
#include "isave.h"
#include "dstack.h"
#include "store.h"
#include "gsmatrix.h"			/* for gsstate.h */
#include "gsstate.h"

/* Imported operators */
extern int zgsave(P1(os_ptr));
extern int zgrestore(P1(os_ptr));

/* Make an invalid file object. */
extern void make_invalid_file(P1(ref *)); /* in zfile.c */

/* 'Save' structure */
typedef struct vm_save_s vm_save_t;
struct vm_save_s {
	gs_state *gsave;		/* old graphics state */
};
gs_private_st_ptrs1(st_vm_save, vm_save_t, "savetype",
  vm_save_enum_ptrs, vm_save_reloc_ptrs, gsave);

/* - save <save> */
int
zsave(register os_ptr op)
{	vm_save_t *vmsave = ialloc_struct(vm_save_t, &st_vm_save, "zsave");
	ulong sid;
	int code;
	gs_state *prev;
	if ( vmsave == 0 )
	  return_error(e_VMerror);
	sid = alloc_save_state(idmemory, vmsave);
	if ( sid == 0 )
	{	ifree_object(vmsave, "zsave");
		return_error(e_VMerror);
	}
	code = zgsave(op);
	if ( code < 0 )
	  return code;
	/* Cut the chain so we can't grestore past here. */
	prev = gs_state_swap_saved(igs, (gs_state *)0);
	code = zgsave(op);
	if ( code < 0 )
	  return code;
	vmsave->gsave = prev;
	push(1);
	make_tav(op, t_save, 0, saveid, sid);
	return 0;
}

/* <save> restore - */
private int restore_check_operand(P2(os_ptr, alloc_save_t **));
private int restore_check_stack(P3(const ref_stack *, const alloc_save_t *, bool));
private void restore_fix_stack(P3(ref_stack *, const alloc_save_t *, bool));
int
zrestore(register os_ptr op)
{	alloc_save_t *asave;
	vm_save_t *vmsave;
	int code = restore_check_operand(op, &asave);
	if ( code < 0 )
	  return code;
	vmsave = alloc_save_client_data(asave);
	/* Check the contents of the stacks. */
	osp--;
	{	int code;
		if ( (code = restore_check_stack(&o_stack, asave, false)) < 0 ||
		     (code = restore_check_stack(&e_stack, asave, true)) < 0 ||
		     (code = restore_check_stack(&d_stack, asave, false)) < 0
		   )
		  {	osp++;
			return code;
		  }
	}
	/* Reset l_new in all stack entries if the new save level is zero. */
	/* Also do some special fixing on the e-stack. */
	restore_fix_stack(&o_stack, asave, false);
	restore_fix_stack(&e_stack, asave, true);
	restore_fix_stack(&d_stack, asave, false);
	/* Restore the graphics state. */
	gs_grestoreall(igs);
	gs_state_swap_saved(gs_state_saved(igs), vmsave->gsave);
	gs_grestore(igs);
	gs_grestore(igs);
	/* Now it's safe to restore the state of memory. */
	alloc_restore_state(asave);
	ifree_object(vmsave, "zrestore");
	dict_set_top();		/* reload dict stack cache */
	return 0;
}
/* Check the operand of a restore. */
private int
restore_check_operand(os_ptr op, alloc_save_t **pasave)
{	vm_save_t *vmsave;
	ulong sid;
	alloc_save_t *asave;
	check_type(*op, t_save);
	vmsave = r_ptr(op, vm_save_t);
	if ( vmsave == 0 )		/* invalidated save */
	  return_error(e_invalidrestore);
	sid = op->value.saveid;
	asave = alloc_find_save(idmemory, sid);
	if ( asave == 0 )
	  return_error(e_invalidrestore);
	*pasave = asave;
	return 0;
}
/* Check a stack to make sure all its elements are older than a save. */
private int
restore_check_stack(const ref_stack *pstack, const alloc_save_t *asave,
  bool is_estack)
{	STACK_LOOP_BEGIN(pstack, bot, size)
	  {	const ref *stkp;
		for ( stkp = bot; size; stkp++, size-- )
		  {	const void *ptr;
			switch ( r_type(stkp) )
			  {
			  case t_array:
			    ptr = stkp->value.refs; break;
			  case t_dictionary:
			    ptr = stkp->value.pdict; break;
			  case t_file:
			    /* Don't check executable files on the e-stack. */
			    if ( r_has_attr(stkp, a_executable) && is_estack )
			      continue;
			    ptr = stkp->value.pfile; break;
			  case t_name:
			    /* Names are special because of how they are allocated. */
			    if ( alloc_name_is_since_save(stkp, asave) )
			      return_error(e_invalidrestore);
			    continue;
			  case t_string:
			    /* Don't check empty executable strings */
			    /* on the e-stack. */
			    if ( r_size(stkp) == 0 &&
				 r_has_attr(stkp, a_executable) && is_estack
			       )
			      continue;
			    ptr = stkp->value.bytes; break;
			  case t_mixedarray:
			  case t_shortarray:
			    ptr = stkp->value.packed; break;
			  case t_device:
			    ptr = stkp->value.pdevice; break;
			  case t_fontID:
			  case t_struct:
			  case t_astruct:
			    ptr = stkp->value.pstruct; break;
			  default:
			    continue;
			  }
			if ( alloc_is_since_save(ptr, asave) )
			  return_error(e_invalidrestore);
		  }
	  }
	STACK_LOOP_END(bot, size)
	return 0;			/* OK */
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
restore_fix_stack(ref_stack *pstack, const alloc_save_t *asave,
  bool is_estack)
{	STACK_LOOP_BEGIN(pstack, bot, size)
	  {	ref *stkp;
		for ( stkp = bot; size; stkp++, size-- )
		  {	r_clear_attrs(stkp, l_new); /* always do it, no harm */
			if ( is_estack )
			  {	ref ofile;
				ref_assign(&ofile, stkp);
				switch ( r_type(stkp) )
				  {
				  case t_string:
				    if ( r_size(stkp) == 0 &&
					alloc_is_since_save(stkp->value.bytes,
							    asave)
				       )
				      {	make_empty_const_string(stkp,
								avm_foreign);
					break;
				      }
				    continue;
				  case t_file:
				    if ( alloc_is_since_save(stkp->value.pfile,
							     asave)
				       )
				      {	make_invalid_file(stkp);
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
	  }
	STACK_LOOP_END(bot, size)
}

/* - vmstatus <save_level> <vm_used> <vm_maximum> */
int
zvmstatus(register os_ptr op)
{	gs_memory_status_t mstat, dstat;
	gs_memory_status(imemory, &mstat);
	gs_memory_status(&gs_memory_default, &dstat);
	push(3);
	make_int(op - 2, alloc_save_level(idmemory));
	make_int(op - 1, mstat.used);
	make_int(op, mstat.allocated + dstat.allocated - dstat.used);
	return 0;
}

/* ------ Non-standard extensions ------ */

/* <save> .forgetsave - */
int
zforgetsave(register os_ptr op)
{	alloc_save_t *asave;
	vm_save_t *vmsave;
	int code = restore_check_operand(op, &asave);
	if ( code < 0 )
	  return 0;
	vmsave = alloc_save_client_data(asave);
	/* Reset l_new in all stack entries if the new save level is zero. */
	restore_fix_stack(&o_stack, asave, false);
	restore_fix_stack(&e_stack, asave, false);
	restore_fix_stack(&d_stack, asave, false);
	/* Forget the gsaves, by deleting the bottom gstate on */
	/* the current stack and the top one on the saved stack and then */
	/* concatenating the stacks together. */
	  {	gs_state *pgs = igs;
		gs_state *last;
		while ( gs_state_saved(last = gs_state_saved(pgs)) != 0 )
		  pgs = last;
		gs_state_swap_saved(last, vmsave->gsave);
		gs_grestore(last);
		gs_grestore(last);
	  }
	/* Forget the save in the memory manager. */
	alloc_forget_save(asave);
	ifree_object(vmsave, "zforgetsave");
	pop(1);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zvmem_op_defs) {
	{"1.forgetsave", zforgetsave},
	{"1restore", zrestore},
	{"0save", zsave},
	{"0vmstatus", zvmstatus},
END_OP_DEFS(0) }
