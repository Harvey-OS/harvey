/* Copyright (C) 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: ireclaim.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Interpreter's interface to garbage collector */
#include "ghost.h"
#include "errors.h"
#include "gsstruct.h"
#include "iastate.h"
#include "icontext.h"
#include "interp.h"
#include "isave.h"		/* for isstate.h */
#include "isstate.h"		/* for mem->saved->state */
#include "dstack.h"		/* for dsbot, dsp, dict_set_top */
#include "estack.h"		/* for esbot, esp */
#include "ostack.h"		/* for osbot, osp */
#include "opdef.h"		/* for defining init procedure */
#include "store.h"		/* for make_array */

/* Import preparation and cleanup routines. */
extern void ialloc_gc_prepare(P1(gs_ref_memory_t *));

/* Forward references */
private void gs_vmreclaim(P2(gs_dual_memory_t *, bool));

/* Initialize the GC hook in the allocator. */
private int ireclaim(P2(gs_dual_memory_t *, int));
private int
ireclaim_init(i_ctx_t *i_ctx_p)
{
    gs_imemory.reclaim = ireclaim;
    return 0;
}

/* GC hook called when the allocator returns a VMerror (space = -1), */
/* or for vmreclaim (space = the space to collect). */
private int
ireclaim(gs_dual_memory_t * dmem, int space)
{
    bool global;
    gs_ref_memory_t *mem;

    if (space < 0) {
	/* Determine which allocator got the VMerror. */
	gs_memory_status_t stats;
	ulong allocated;
	int i;

	mem = dmem->space_global;	/* just in case */
	for (i = 0; i < countof(dmem->spaces_indexed); ++i) {
	    mem = dmem->spaces_indexed[i];
	    if (mem == 0)
		continue;
	    if (mem->gc_status.requested > 0 ||
		((gs_ref_memory_t *)mem->stable_memory)->gc_status.requested > 0
		)
		break;
	}
	gs_memory_status((gs_memory_t *) mem, &stats);
	allocated = stats.allocated;
	if (mem->stable_memory != (gs_memory_t *)mem) {
	    gs_memory_status(mem->stable_memory, &stats);
	    allocated += stats.allocated;
	}
	if (allocated >= mem->gc_status.max_vm) {
	    /* We can't satisfy this request within max_vm. */
	    return_error(e_VMerror);
	}
    } else {
	mem = dmem->spaces_indexed[space >> r_space_shift];
    }
    if_debug3('0', "[0]GC called, space=%d, requestor=%d, requested=%ld\n",
	      space, mem->space, (long)mem->gc_status.requested);
    global = mem->space != avm_local;
    /* Since dmem may move, reset the request now. */
    ialloc_reset_requested(dmem);
    gs_vmreclaim(dmem, global);

    ialloc_set_limit(mem);
    return 0;
}

/* Interpreter entry to garbage collector. */
private void
gs_vmreclaim(gs_dual_memory_t *dmem, bool global)
{
    /* HACK: we know the gs_dual_memory_t is embedded in a context state. */
    i_ctx_t *i_ctx_p =
	(i_ctx_t *)((char *)dmem - offset_of(i_ctx_t, memory));
    gs_ref_memory_t *lmem = dmem->space_local;
    int code = context_state_store(i_ctx_p);
    gs_ref_memory_t *memories[5];
    gs_ref_memory_t *mem;
    int nmem, i;

    memories[0] = dmem->space_system;
    memories[1] = mem = dmem->space_global;
    nmem = 2;
    if (lmem != dmem->space_global)
	memories[nmem++] = lmem;
    for (i = nmem; --i >= 0;) {
	mem = memories[i];
	if (mem->stable_memory != (gs_memory_t *)mem)
	    memories[nmem++] = (gs_ref_memory_t *)mem->stable_memory;
    }

    /****** ABORT IF code < 0 ******/
    for (i = nmem; --i >= 0; )
	alloc_close_chunk(memories[i]);

    /* Prune the file list so it won't retain potentially collectible */
    /* files. */

    for (i = (global ? i_vm_system : i_vm_local);
	 i < countof(dmem->spaces_indexed);
	 ++i
	 ) {
	gs_ref_memory_t *mem = dmem->spaces_indexed[i];

	if (mem == 0 || (i > 0 && mem == dmem->spaces_indexed[i - 1]))
	    continue;
	if (mem->stable_memory != (gs_memory_t *)mem)
	    ialloc_gc_prepare((gs_ref_memory_t *)mem->stable_memory);
	for (;; mem = &mem->saved->state) {
	    ialloc_gc_prepare(mem);
	    if (mem->saved == 0)
		break;
	}
    }

    /* Do the actual collection. */

    {
	void *ctxp = i_ctx_p;
	gs_gc_root_t context_root;

	gs_register_struct_root((gs_memory_t *)lmem, &context_root,
				&ctxp, "i_ctx_p root");
	GS_RECLAIM(&dmem->spaces, global);
	gs_unregister_root((gs_memory_t *)lmem, &context_root, "i_ctx_p root");
	i_ctx_p = ctxp;
	dmem = &i_ctx_p->memory;
    }

    /* Update caches not handled by context_state_load. */

    *systemdict = *ref_stack_index(&d_stack, ref_stack_count(&d_stack) - 1);

    /* Reload the context state. */

    code = context_state_load(i_ctx_p);
    /****** ABORT IF code < 0 ******/

    /* Update the cached value pointers in names. */

    dicts_gc_cleanup();

    /* Reopen the active chunks. */

    for (i = 0; i < nmem; ++i)
	alloc_open_chunk(memories[i]);
}

/* ------ Initialization procedure ------ */

const op_def ireclaim_l2_op_defs[] =
{
    op_def_end(ireclaim_init)
};
