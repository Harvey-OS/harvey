/* Copyright (C) 1993, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ialloc.c,v 1.8 2005/10/12 10:45:21 leonardo Exp $ */
/* Memory allocator for Ghostscript interpreter */
#include "gx.h"
#include "memory_.h"
#include "ierrors.h"
#include "gsstruct.h"
#include "iref.h"		/* must precede iastate.h */
#include "iastate.h"
#include "igc.h"		/* for gs_gc_reclaim */
#include "ipacked.h"
#include "iutil.h"
#include "ivmspace.h"
#include "store.h"

/*
 * Define global and local instances.
 */
public_st_gs_dual_memory();

/* Initialize the allocator */
int
ialloc_init(gs_dual_memory_t *dmem, gs_memory_t * rmem, uint chunk_size,
	    bool level2)
{
    gs_ref_memory_t *ilmem = ialloc_alloc_state(rmem, chunk_size);
    gs_ref_memory_t *ilmem_stable = ialloc_alloc_state(rmem, chunk_size);
    gs_ref_memory_t *igmem = 0;
    gs_ref_memory_t *igmem_stable = 0;
    gs_ref_memory_t *ismem = ialloc_alloc_state(rmem, chunk_size);
    int i;

    if (ilmem == 0 || ilmem_stable == 0 || ismem == 0)
	goto fail;
    ilmem->stable_memory = (gs_memory_t *)ilmem_stable;
    if (level2) {
	igmem = ialloc_alloc_state(rmem, chunk_size);
	igmem_stable = ialloc_alloc_state(rmem, chunk_size);
	if (igmem == 0 || igmem_stable == 0)
	    goto fail;
	igmem->stable_memory = (gs_memory_t *)igmem_stable;
    } else
	igmem = ilmem, igmem_stable = ilmem_stable;
    for (i = 0; i < countof(dmem->spaces_indexed); i++)
	dmem->spaces_indexed[i] = 0;
    dmem->space_local = ilmem;
    dmem->space_global = igmem;
    dmem->space_system = ismem;
    dmem->spaces.vm_reclaim = gs_gc_reclaim; /* real GC */
    dmem->reclaim = 0;		/* no interpreter GC yet */
    /* Level 1 systems have only local VM. */
    igmem->space = avm_global;
    igmem_stable->space = avm_global;
    ilmem->space = avm_local;	/* overrides if ilmem == igmem */
    ilmem_stable->space = avm_local; /* ditto */
    ismem->space = avm_system;
#   if IGC_PTR_STABILITY_CHECK
    igmem->space_id = (i_vm_global << 1) + 1;
    igmem_stable->space_id = i_vm_global << 1;
    ilmem->space_id = (i_vm_local << 1) + 1;	/* overrides if ilmem == igmem */
    ilmem_stable->space_id = i_vm_local << 1; /* ditto */
    ismem->space_id = (i_vm_system << 1);
#   endif
    ialloc_set_space(dmem, avm_global);
    return 0;
 fail:
    gs_free_object(rmem, igmem_stable, "ialloc_init failure");
    gs_free_object(rmem, igmem, "ialloc_init failure");
    gs_free_object(rmem, ismem, "ialloc_init failure");
    gs_free_object(rmem, ilmem_stable, "ialloc_init failure");
    gs_free_object(rmem, ilmem, "ialloc_init failure");
    return_error(e_VMerror);
}

/* ================ Local/global VM ================ */

/* Get the space attribute of an allocator */
uint
imemory_space(const gs_ref_memory_t * iimem)
{
    return iimem->space;
}

/* Select the allocation space. */
void
ialloc_set_space(gs_dual_memory_t * dmem, uint space)
{
    gs_ref_memory_t *mem = dmem->spaces_indexed[space >> r_space_shift];

    dmem->current = mem;
    dmem->current_space = mem->space;
}

/* Get the l_new attribute of a current allocator. */
/* (A copy of the new_mask in the gs_dual_memory_t.) */
uint
imemory_new_mask(const gs_ref_memory_t *imem)
{
    return imem->new_mask;
}

/* Get the save level of an allocator. */
int
imemory_save_level(const gs_ref_memory_t *imem)
{
    return imem->save_level;
}

/* Reset the requests. */
void
ialloc_reset_requested(gs_dual_memory_t * dmem)
{
    dmem->space_system->gc_status.requested = 0;
    dmem->space_global->gc_status.requested = 0;
    dmem->space_local->gc_status.requested = 0;
}

/* ================ Refs ================ */

#ifdef DEBUG
private int
ialloc_trace_space(const gs_ref_memory_t *imem)
{
    return imem->space + (imem->stable_memory == (const gs_memory_t *)imem);
}
#endif

/* Register a ref root. */
int
gs_register_ref_root(gs_memory_t *mem, gs_gc_root_t *root,
		     void **pp, client_name_t cname)
{
    return gs_register_root(mem, root, ptr_ref_type, pp, cname);
}

/*
 * As noted in iastate.h, every run of refs has an extra ref at the end
 * to hold relocation information for the garbage collector;
 * since sizeof(ref) % obj_align_mod == 0, we never need to
 * allocate any additional padding space at the end of the block.
 */

/* Allocate an array of refs. */
int
gs_alloc_ref_array(gs_ref_memory_t * mem, ref * parr, uint attrs,
		   uint num_refs, client_name_t cname)
{
    ref *obj;

    /* If we're allocating a run of refs already, */
    /* and we aren't about to overflow the maximum run length, use it. */
    if (mem->cc.rtop == mem->cc.cbot &&
	num_refs < (mem->cc.ctop - mem->cc.cbot) / sizeof(ref) &&
	mem->cc.rtop - (byte *) mem->cc.rcur + num_refs * sizeof(ref) <
	max_size_st_refs
	) {
	ref *end;

	obj = (ref *) mem->cc.rtop - 1;		/* back up over last ref */
	if_debug4('A', "[a%d:+$ ]%s(%u) = 0x%lx\n",
		  ialloc_trace_space(mem), client_name_string(cname),
		  num_refs, (ulong) obj);
	mem->cc.rcur[-1].o_size += num_refs * sizeof(ref);
	end = (ref *) (mem->cc.rtop = mem->cc.cbot +=
		       num_refs * sizeof(ref));
	make_mark(end - 1);
    } else {
	/*
	 * Allocate a new run.  We have to distinguish 3 cases:
	 *      - Same chunk: pcc unchanged, end == cc.cbot.
	 *      - Large chunk: pcc unchanged, end != cc.cbot.
	 *      - New chunk: pcc changed.
	 */
	chunk_t *pcc = mem->pcc;
	ref *end;

	obj = gs_alloc_struct_array((gs_memory_t *) mem, num_refs + 1,
				    ref, &st_refs, cname);
	if (obj == 0)
	    return_error(e_VMerror);
	/* Set the terminating ref now. */
	end = (ref *) obj + num_refs;
	make_mark(end);
	/* Set has_refs in the chunk. */
	if (mem->pcc != pcc || mem->cc.cbot == (byte *) (end + 1)) {
	    /* Ordinary chunk. */
	    mem->cc.rcur = (obj_header_t *) obj;
	    mem->cc.rtop = (byte *) (end + 1);
	    mem->cc.has_refs = true;
	} else {
	    /* Large chunk. */
	    /* This happens only for very large arrays, */
	    /* so it doesn't need to be cheap. */
	    chunk_locator_t cl;

	    cl.memory = mem;
	    cl.cp = mem->clast;
	    chunk_locate_ptr(obj, &cl);
	    cl.cp->has_refs = true;
	}
    }
    make_array(parr, attrs | mem->space, num_refs, obj);
    return 0;
}

/* Resize an array of refs.  Currently this is only implemented */
/* for shrinking, not for growing. */
int
gs_resize_ref_array(gs_ref_memory_t * mem, ref * parr,
		    uint new_num_refs, client_name_t cname)
{
    uint old_num_refs = r_size(parr);
    uint diff;
    ref *obj = parr->value.refs;

    if (new_num_refs > old_num_refs || !r_has_type(parr, t_array))
	return_error(e_Fatal);
    diff = old_num_refs - new_num_refs;
    /* Check for LIFO.  See gs_free_ref_array for more details. */
    if (mem->cc.rtop == mem->cc.cbot &&
	(byte *) (obj + (old_num_refs + 1)) == mem->cc.rtop
	) {
	/* Shorten the refs object. */
	ref *end = (ref *) (mem->cc.cbot = mem->cc.rtop -=
			    diff * sizeof(ref));

	if_debug4('A', "[a%d:<$ ]%s(%u) 0x%lx\n",
		  ialloc_trace_space(mem), client_name_string(cname), diff,
		  (ulong) obj);
	mem->cc.rcur[-1].o_size -= diff * sizeof(ref);
	make_mark(end - 1);
    } else {
	/* Punt. */
	if_debug4('A', "[a%d:<$#]%s(%u) 0x%lx\n",
		  ialloc_trace_space(mem), client_name_string(cname), diff,
		  (ulong) obj);
	mem->lost.refs += diff * sizeof(ref);
    }
    r_set_size(parr, new_num_refs);
    return 0;
}

/* Deallocate an array of refs.  Only do this if LIFO, or if */
/* the array occupies an entire chunk by itself. */
void
gs_free_ref_array(gs_ref_memory_t * mem, ref * parr, client_name_t cname)
{
    uint num_refs = r_size(parr);
    ref *obj = parr->value.refs;

    /*
     * Compute the storage size of the array, and check for LIFO
     * freeing or a separate chunk.  Note that the array might be packed;
     * for the moment, if it's anything but a t_array, punt.
     * The +1s are for the extra ref for the GC.
     */
    if (!r_has_type(parr, t_array))
	DO_NOTHING;		/* don't look for special cases */
    else if (mem->cc.rtop == mem->cc.cbot &&
	     (byte *) (obj + (num_refs + 1)) == mem->cc.rtop
	) {
	if ((obj_header_t *) obj == mem->cc.rcur) {
	    /* Deallocate the entire refs object. */
	    gs_free_object((gs_memory_t *) mem, obj, cname);
	    mem->cc.rcur = 0;
	    mem->cc.rtop = 0;
	} else {
	    /* Deallocate it at the end of the refs object. */
	    if_debug4('A', "[a%d:-$ ]%s(%u) 0x%lx\n",
		      ialloc_trace_space(mem), client_name_string(cname),
		      num_refs, (ulong) obj);
	    mem->cc.rcur[-1].o_size -= num_refs * sizeof(ref);
	    mem->cc.rtop = mem->cc.cbot = (byte *) (obj + 1);
	    make_mark(obj);
	}
	return;
    } else if (num_refs >= (mem->large_size / arch_sizeof_ref - 1)) {
	/* See if this array has a chunk all to itself. */
	/* We only make this check when freeing very large objects, */
	/* so it doesn't need to be cheap. */
	chunk_locator_t cl;

	cl.memory = mem;
	cl.cp = mem->clast;
	if (chunk_locate_ptr(obj, &cl) &&
	    obj == (ref *) ((obj_header_t *) (cl.cp->cbase) + 1) &&
	    (byte *) (obj + (num_refs + 1)) == cl.cp->cend
	    ) {
	    /* Free the chunk. */
	    if_debug4('a', "[a%d:-$L]%s(%u) 0x%lx\n",
		      ialloc_trace_space(mem), client_name_string(cname),
		      num_refs, (ulong) obj);
	    alloc_free_chunk(cl.cp, mem);
	    return;
	}
    }
    /* Punt, but fill the array with nulls so that there won't be */
    /* dangling references to confuse the garbage collector. */
    if_debug4('A', "[a%d:-$#]%s(%u) 0x%lx\n",
	      ialloc_trace_space(mem), client_name_string(cname), num_refs,
	      (ulong) obj);
    {
	uint size;

	switch (r_type(parr)) {
	    case t_shortarray:
		size = num_refs * sizeof(ref_packed);
		break;
	    case t_mixedarray:{
		/* We have to parse the array to compute the storage size. */
		uint i = 0;
		const ref_packed *p = parr->value.packed;

		for (; i < num_refs; ++i)
		    p = packed_next(p);
		size = (const byte *)p - (const byte *)parr->value.packed;
		break;
	    }
	    case t_array:
		size = num_refs * sizeof(ref);
		break;
	    default:
		lprintf3("Unknown type 0x%x in free_ref_array(%u,0x%lx)!",
			 r_type(parr), num_refs, (ulong) obj);
		return;
	}
	/*
	 * If there are any leftover packed elements, we don't
	 * worry about them, since they can't be dangling references.
	 */
	refset_null_new(obj, size / sizeof(ref), 0);
	mem->lost.refs += size;
    }
}

/* Allocate a string ref. */
int
gs_alloc_string_ref(gs_ref_memory_t * mem, ref * psref,
		    uint attrs, uint nbytes, client_name_t cname)
{
    byte *str = gs_alloc_string((gs_memory_t *) mem, nbytes, cname);

    if (str == 0)
	return_error(e_VMerror);
    make_string(psref, attrs | mem->space, nbytes, str);
    return 0;
}
