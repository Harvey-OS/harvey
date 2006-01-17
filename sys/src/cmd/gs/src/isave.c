/* Copyright (C) 1993, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: isave.c,v 1.14 2005/06/23 07:35:30 igor Exp $ */
/* Save/restore manager for Ghostscript interpreter */
#include "ghost.h"
#include "memory_.h"
#include "ierrors.h"
#include "gsexit.h"
#include "gsstruct.h"
#include "stream.h"		/* for linking for forgetsave */
#include "iastate.h"
#include "inamedef.h"
#include "iname.h"
#include "ipacked.h"
#include "isave.h"
#include "isstate.h"
#include "store.h"		/* for ref_assign */
#include "ivmspace.h"
#include "gsutil.h"		/* gs_next_ids prototype */


/* Structure descriptor */
private_st_alloc_save();

/* Define the maximum amount of data we are willing to scan repeatedly -- */
/* see below for details. */
private const long max_repeated_scan = 100000;

/* Define the minimum space for creating an inner chunk. */
/* Must be at least sizeof(chunk_head_t). */
private const long min_inner_chunk_space = sizeof(chunk_head_t) + 500;

/*
 * The logic for saving and restoring the state is complex.
 * Both the changes to individual objects, and the overall state
 * of the memory manager, must be saved and restored.
 */

/*
 * To save the state of the memory manager:
 *      Save the state of the current chunk in which we are allocating.
 *      Shrink all chunks to their inner unallocated region.
 *      Save and reset the free block chains.
 * By doing this, we guarantee that no object older than the save
 * can be freed.
 *
 * To restore the state of the memory manager:
 *      Free all chunks newer than the save, and the descriptors for
 *        the inner chunks created by the save.
 *      Make current the chunk that was current at the time of the save.
 *      Restore the state of the current chunk.
 *
 * In addition to save ("start transaction") and restore ("abort transaction"),
 * we support forgetting a save ("commit transation").  To forget a save:
 *      Reassign to the next outer save all chunks newer than the save.
 *      Free the descriptors for the inners chunk, updating their outer
 *        chunks to reflect additional allocations in the inner chunks.
 *      Concatenate the free block chains with those of the outer save.
 */

/*
 * For saving changes to individual objects, we add an "attribute" bit
 * (l_new) that logically belongs to the slot where the ref is stored,
 * not to the ref itself.  The bit means "the contents of this slot
 * have been changed, or the slot was allocated, since the last save."
 * To keep track of changes since the save, we associate a chain of
 * <slot, old_contents> pairs that remembers the old contents of slots.
 *
 * When creating an object, if the save level is non-zero:
 *      Set l_new in all slots.
 *
 * When storing into a slot, if the save level is non-zero:
 *      If l_new isn't set, save the address and contents of the slot
 *        on the current contents chain.
 *      Set l_new after storing the new value.
 *
 * To do a save:
 *      If the save level is non-zero:
 *              Reset l_new in all slots on the contents chain, and in all
 *                objects created since the previous save.
 *      Push the head of the contents chain, and reset the chain to empty.
 *
 * To do a restore:
 *      Check all the stacks to make sure they don't contain references
 *        to objects created since the save.
 *      Restore all the slots on the contents chain.
 *      Pop the contents chain head.
 *      If the save level is now non-zero:
 *              Scan the newly restored contents chain, and set l_new in all
 *                the slots it references.
 *              Scan all objects created since the previous save, and set
 *                l_new in all the slots of each object.
 *
 * To forget a save:
 *      If the save level is greater than 1:
 *              Set l_new as for a restore, per the next outer save.
 *              Concatenate the next outer contents chain to the end of
 *                the current one.
 *      If the save level is 1:
 *              Reset l_new as for a save.
 *              Free the contents chain.
 */

/*
 * A consequence of the foregoing algorithms is that the cost of a save is
 * proportional to the total amount of data allocated since the previous
 * save.  If a PostScript program reads in a large amount of setup code and
 * then uses save/restore heavily, each save/restore will be expensive.  To
 * mitigate this, we check to see how much data we have scanned at this save
 * level: if it is large, we do a second, invisible save.  This greatly
 * reduces the cost of inner saves, at the expense of possibly saving some
 * changes twice that otherwise would only have to be saved once.
 */

/*
 * The presence of global and local VM complicates the situation further.
 * There is a separate save chain and contents chain for each VM space.
 * When multiple contexts are fully implemented, save and restore will have
 * the following effects, according to the privacy status of the current
 * context's global and local VM:
 *      Private global, private local:
 *              The outermost save saves both global and local VM;
 *                otherwise, save only saves local VM.
 *      Shared global, private local:
 *              Save only saves local VM.
 *      Shared global, shared local:
 *              Save only saves local VM, and suspends all other contexts
 *                sharing the same local VM until the matching restore.
 * Since we do not currently implement multiple contexts, only the first
 * case is relevant.
 *
 * Note that when saving the contents of a slot, the choice of chain
 * is determined by the VM space in which the slot is allocated,
 * not by the current allocation mode.
 */

/* Tracing printout */
private void
print_save(const char *str, uint spacen, const alloc_save_t *sav)
{
  if_debug5('u', "[u]%s space %u 0x%lx: cdata = 0x%lx, id = %lu\n",\
	    str, spacen, (ulong)sav, (ulong)sav->client_data, (ulong)sav->id);
}

/*
 * Structure for saved change chain for save/restore.  Because of the
 * garbage collector, we need to distinguish the cases where the change
 * is in a static object, a dynamic ref, or a dynamic struct.
 */
typedef struct alloc_change_s alloc_change_t;
struct alloc_change_s {
    alloc_change_t *next;
    ref_packed *where;
    ref contents;
#define AC_OFFSET_STATIC (-2)	/* static object */
#define AC_OFFSET_REF (-1)	/* dynamic ref */
    short offset;		/* if >= 0, offset within struct */
};

private 
CLEAR_MARKS_PROC(change_clear_marks)
{
    alloc_change_t *const ptr = (alloc_change_t *)vptr;

    if (r_is_packed(&ptr->contents))
	r_clear_pmark((ref_packed *) & ptr->contents);
    else
	r_clear_attrs(&ptr->contents, l_mark);
}
private 
ENUM_PTRS_WITH(change_enum_ptrs, alloc_change_t *ptr) return 0;
ENUM_PTR(0, alloc_change_t, next);
case 1:
    if (ptr->offset >= 0)
	ENUM_RETURN((byte *) ptr->where - ptr->offset);
    else
	ENUM_RETURN_REF(ptr->where);
case 2:
    ENUM_RETURN_REF(&ptr->contents);
ENUM_PTRS_END
private RELOC_PTRS_WITH(change_reloc_ptrs, alloc_change_t *ptr)
{
    RELOC_VAR(ptr->next);
    switch (ptr->offset) {
	case AC_OFFSET_STATIC:
	    break;
	case AC_OFFSET_REF:
	    RELOC_REF_PTR_VAR(ptr->where);
	    break;
	default:
	    {
		byte *obj = (byte *) ptr->where - ptr->offset;

		RELOC_VAR(obj);
		ptr->where = (ref_packed *) (obj + ptr->offset);
	    }
	    break;
    }
    if (r_is_packed(&ptr->contents))
	r_clear_pmark((ref_packed *) & ptr->contents);
    else {
	RELOC_REF_VAR(ptr->contents);
	r_clear_attrs(&ptr->contents, l_mark);
    }
}
RELOC_PTRS_END
gs_private_st_complex_only(st_alloc_change, alloc_change_t, "alloc_change",
		change_clear_marks, change_enum_ptrs, change_reloc_ptrs, 0);

/* Debugging printout */
#ifdef DEBUG
private void
alloc_save_print(alloc_change_t * cp, bool print_current)
{
    dprintf2(" 0x%lx: 0x%lx: ", (ulong) cp, (ulong) cp->where);
    if (r_is_packed(&cp->contents)) {
	if (print_current)
	    dprintf2("saved=%x cur=%x\n", *(ref_packed *) & cp->contents,
		     *cp->where);
	else
	    dprintf1("%x\n", *(ref_packed *) & cp->contents);
    } else {
	if (print_current)
	    dprintf6("saved=%x %x %lx cur=%x %x %lx\n",
		     r_type_attrs(&cp->contents), r_size(&cp->contents),
		     (ulong) cp->contents.value.intval,
		     r_type_attrs((ref *) cp->where),
		     r_size((ref *) cp->where),
		     (ulong) ((ref *) cp->where)->value.intval);
	else
	    dprintf3("%x %x %lx\n",
		     r_type_attrs(&cp->contents), r_size(&cp->contents),
		     (ulong) cp->contents.value.intval);
    }
}
#endif

/* Forward references */
private void restore_resources(alloc_save_t *, gs_ref_memory_t *);
private void restore_free(gs_ref_memory_t *);
private long save_set_new(gs_ref_memory_t *, bool);
private void save_set_new_changes(gs_ref_memory_t *, bool);

/* Initialize the save/restore machinery. */
void
alloc_save_init(gs_dual_memory_t * dmem)
{
    alloc_set_not_in_save(dmem);
}

/* Record that we are in a save. */
private void
alloc_set_masks(gs_dual_memory_t *dmem, uint new_mask, uint test_mask)
{
    int i;
    gs_ref_memory_t *mem;

    dmem->new_mask = new_mask;
    dmem->test_mask = test_mask;
    for (i = 0; i < countof(dmem->spaces.memories.indexed); ++i)
	if ((mem = dmem->spaces.memories.indexed[i]) != 0) {
	    mem->new_mask = new_mask, mem->test_mask = test_mask;
	    if (mem->stable_memory != (gs_memory_t *)mem) {
		mem = (gs_ref_memory_t *)mem->stable_memory;
		mem->new_mask = new_mask, mem->test_mask = test_mask;
	    }
	}
}
void
alloc_set_in_save(gs_dual_memory_t *dmem)
{
    alloc_set_masks(dmem, l_new, l_new);
}

/* Record that we are not in a save. */
void
alloc_set_not_in_save(gs_dual_memory_t *dmem)
{
    alloc_set_masks(dmem, 0, ~0);
}

/* Save the state. */
private alloc_save_t *alloc_save_space(gs_ref_memory_t *mem,
				       gs_dual_memory_t *dmem,
				       ulong sid);
private void
alloc_free_save(gs_ref_memory_t *mem, alloc_save_t *save, const char *scn)
{
    gs_free_object((gs_memory_t *)mem, save, scn);
    /* Free any inner chunk structures.  This is the easiest way to do it. */
    restore_free(mem);
}
ulong
alloc_save_state(gs_dual_memory_t * dmem, void *cdata)
{
    gs_ref_memory_t *lmem = dmem->space_local;
    gs_ref_memory_t *gmem = dmem->space_global;
    ulong sid = gs_next_ids((const gs_memory_t *)lmem->stable_memory, 2);
    bool global =
	lmem->save_level == 0 && gmem != lmem &&
	gmem->num_contexts == 1;
    alloc_save_t *gsave =
	(global ? alloc_save_space(gmem, dmem, sid + 1) : (alloc_save_t *) 0);
    alloc_save_t *lsave = alloc_save_space(lmem, dmem, sid);

    if (lsave == 0 || (global && gsave == 0)) {
	if (lsave != 0)
	    alloc_free_save(lmem, lsave, "alloc_save_state(local save)");
	if (gsave != 0)
	    alloc_free_save(gmem, gsave, "alloc_save_state(global save)");
	return 0;
    }
    if (gsave != 0) {
	gsave->client_data = 0;
	print_save("save", gmem->space, gsave);
	/* Restore names when we do the local restore. */
	lsave->restore_names = gsave->restore_names;
	gsave->restore_names = false;
    }
    lsave->id = sid;
    lsave->client_data = cdata;
    print_save("save", lmem->space, lsave);
    /* Reset the l_new attribute in all slots.  The only slots that */
    /* can have the attribute set are the ones on the changes chain, */
    /* and ones in objects allocated since the last save. */
    if (lmem->save_level > 1) {
	long scanned = save_set_new(&lsave->state, false);

	if ((lsave->state.total_scanned += scanned) > max_repeated_scan) {
	    /* Do a second, invisible save. */
	    alloc_save_t *rsave;

	    rsave = alloc_save_space(lmem, dmem, 0L);
	    if (rsave != 0) {
		rsave->client_data = cdata;
#if 0 /* Bug 688153 */
		rsave->id = lsave->id;
		print_save("save", lmem->space, rsave);
		lsave->id = 0;	/* mark as invisible */
		rsave->state.save_level--; /* ditto */
		lsave->client_data = 0;
#else
		rsave->id = 0;  /* mark as invisible */
		print_save("save", lmem->space, rsave);
		rsave->state.save_level--; /* ditto */
		rsave->client_data = 0;
#endif
		/* Inherit the allocated space count -- */
		/* we need this for triggering a GC. */
		rsave->state.inherited =
		    lsave->state.allocated + lsave->state.inherited;
		lmem->inherited = rsave->state.inherited;
		print_save("save", lmem->space, lsave);
	    }
	}
    }
    alloc_set_in_save(dmem);
    return sid;
}
/* Save the state of one space (global or local). */
private alloc_save_t *
alloc_save_space(gs_ref_memory_t * mem, gs_dual_memory_t * dmem, ulong sid)
{
    gs_ref_memory_t save_mem;
    alloc_save_t *save;
    chunk_t *cp;
    chunk_t *new_pcc = 0;

    save_mem = *mem;
    alloc_close_chunk(mem);
    mem->pcc = 0;
    gs_memory_status((gs_memory_t *) mem, &mem->previous_status);
    ialloc_reset(mem);

    /* Create inner chunks wherever it's worthwhile. */

    for (cp = save_mem.cfirst; cp != 0; cp = cp->cnext) {
	if (cp->ctop - cp->cbot > min_inner_chunk_space) {
	    /* Create an inner chunk to cover only the unallocated part. */
	    chunk_t *inner =
		gs_raw_alloc_struct_immovable(mem->non_gc_memory, &st_chunk,
					      "alloc_save_space(inner)");

	    if (inner == 0)
		break;		/* maybe should fail */
	    alloc_init_chunk(inner, cp->cbot, cp->ctop, cp->sreloc != 0, cp);
	    alloc_link_chunk(inner, mem);
	    if_debug2('u', "[u]inner chunk: cbot=0x%lx ctop=0x%lx\n",
		      (ulong) inner->cbot, (ulong) inner->ctop);
	    if (cp == save_mem.pcc)
		new_pcc = inner;
	}
    }
    mem->pcc = new_pcc;
    alloc_open_chunk(mem);

    save = gs_alloc_struct((gs_memory_t *) mem, alloc_save_t,
			   &st_alloc_save, "alloc_save_space(save)");
    if_debug2('u', "[u]save space %u at 0x%lx\n",
	      mem->space, (ulong) save);
    if (save == 0) {
	/* Free the inner chunk structures.  This is the easiest way. */
	restore_free(mem);
	*mem = save_mem;
	return 0;
    }
    save->state = save_mem;
    save->spaces = dmem->spaces;
    save->restore_names = (name_memory(mem) == (gs_memory_t *) mem);
    save->is_current = (dmem->current == mem);
    save->id = sid;
    mem->saved = save;
    if_debug2('u', "[u%u]file_save 0x%lx\n",
	      mem->space, (ulong) mem->streams);
    mem->streams = 0;
    mem->total_scanned = 0;
    if (sid)
	mem->save_level++;
    return save;
}

/* Record a state change that must be undone for restore, */
/* and mark it as having been saved. */
int
alloc_save_change_in(gs_ref_memory_t *mem, const ref * pcont,
		  ref_packed * where, client_name_t cname)
{
    register alloc_change_t *cp;

    if (mem->new_mask == 0)
	return 0;		/* no saving */
    cp = gs_alloc_struct((gs_memory_t *)mem, alloc_change_t,
			 &st_alloc_change, "alloc_save_change");
    if (cp == 0)
	return -1;
    cp->next = mem->changes;
    cp->where = where;
    if (pcont == NULL)
	cp->offset = AC_OFFSET_STATIC;
    else if (r_is_array(pcont) || r_has_type(pcont, t_dictionary))
	cp->offset = AC_OFFSET_REF;
    else if (r_is_struct(pcont))
	cp->offset = (byte *) where - (byte *) pcont->value.pstruct;
    else {
	lprintf3("Bad type %u for save!  pcont = 0x%lx, where = 0x%lx\n",
		 r_type(pcont), (ulong) pcont, (ulong) where);
	gs_abort((const gs_memory_t *)mem);
    }
    if (r_is_packed(where))
	*(ref_packed *)&cp->contents = *where;
    else {
	ref_assign_inline(&cp->contents, (ref *) where);
	r_set_attrs((ref *) where, l_new);
    }
    mem->changes = cp;
#ifdef DEBUG
    if (gs_debug_c('U')) {
	dlprintf1("[U]save(%s)", client_name_string(cname));
	alloc_save_print(cp, false);
    }
#endif
    return 0;
}
int
alloc_save_change(gs_dual_memory_t * dmem, const ref * pcont,
		  ref_packed * where, client_name_t cname)
{
    gs_ref_memory_t *mem =
	(pcont == NULL ? dmem->space_local :
	 dmem->spaces_indexed[r_space(pcont) >> r_space_shift]);

    return alloc_save_change_in(mem, pcont, where, cname);
}

/* Return (the id of) the innermost externally visible save object, */
/* i.e., the innermost save with a non-zero ID. */
ulong
alloc_save_current_id(const gs_dual_memory_t * dmem)
{
    const alloc_save_t *save = dmem->space_local->saved;

    while (save != 0 && save->id == 0)
	save = save->state.saved;
    return save->id;
}
alloc_save_t *
alloc_save_current(const gs_dual_memory_t * dmem)
{
    return alloc_find_save(dmem, alloc_save_current_id(dmem));
}

/* Test whether a reference would be invalidated by a restore. */
bool
alloc_is_since_save(const void *vptr, const alloc_save_t * save)
{
    /* A reference postdates a save iff it is in a chunk allocated */
    /* since the save (including any carried-over inner chunks). */

    const char *const ptr = (const char *)vptr;
    register const gs_ref_memory_t *mem = save->space_local;

    if_debug2('U', "[U]is_since_save 0x%lx, 0x%lx:\n",
	      (ulong) ptr, (ulong) save);
    if (mem->saved == 0) {	/* This is a special case, the final 'restore' from */
	/* alloc_restore_all. */
	return true;
    }
    /* Check against chunks allocated since the save. */
    /* (There may have been intermediate saves as well.) */
    for (;; mem = &mem->saved->state) {
	const chunk_t *cp;

	if_debug1('U', "[U]checking mem=0x%lx\n", (ulong) mem);
	for (cp = mem->cfirst; cp != 0; cp = cp->cnext) {
	    if (ptr_is_within_chunk(ptr, cp)) {
		if_debug3('U', "[U+]in new chunk 0x%lx: 0x%lx, 0x%lx\n",
			  (ulong) cp,
			  (ulong) cp->cbase, (ulong) cp->cend);
		return true;
	    }
	    if_debug1('U', "[U-]not in 0x%lx\n", (ulong) cp);
	}
	if (mem->saved == save) {	/* We've checked all the more recent saves, */
	    /* must be OK. */
	    break;
	}
    }

    /*
     * If we're about to do a global restore (a restore to the level 0),
     * and there is only one context using this global VM
     * (the normal case, in which global VM is saved by the
     * outermost save), we also have to check the global save.
     * Global saves can't be nested, which makes things easy.
     */
    if (save->state.save_level == 0 /* Restoring to save level 0 - see bug 688157, 688161 */ &&
	(mem = save->space_global) != save->space_local &&
	save->space_global->num_contexts == 1
	) {
	const chunk_t *cp;

	if_debug1('U', "[U]checking global mem=0x%lx\n", (ulong) mem);
	for (cp = mem->cfirst; cp != 0; cp = cp->cnext)
	    if (ptr_is_within_chunk(ptr, cp)) {
		if_debug3('U', "[U+]  new chunk 0x%lx: 0x%lx, 0x%lx\n",
			  (ulong) cp, (ulong) cp->cbase, (ulong) cp->cend);
		return true;
	    }
    }
    return false;

#undef ptr
}

/* Test whether a name would be invalidated by a restore. */
bool
alloc_name_is_since_save(const gs_memory_t *mem,
			 const ref * pnref, const alloc_save_t * save)
{
    const name_string_t *pnstr;

    if (!save->restore_names)
	return false;
    pnstr = names_string_inline(mem->gs_lib_ctx->gs_name_table, pnref);
    if (pnstr->foreign_string)
	return false;
    return alloc_is_since_save(pnstr->string_bytes, save);
}
bool
alloc_name_index_is_since_save(const gs_memory_t *mem,
			       uint nidx, const alloc_save_t *save)
{
    const name_string_t *pnstr;

    if (!save->restore_names)
	return false;
    pnstr = names_index_string_inline(mem->gs_lib_ctx->gs_name_table, nidx);
    if (pnstr->foreign_string)
	return false;
    return alloc_is_since_save(pnstr->string_bytes, save);
}

/* Check whether any names have been created since a given save */
/* that might be released by the restore. */
bool
alloc_any_names_since_save(const alloc_save_t * save)
{
    return save->restore_names;
}

/* Get the saved state with a given ID. */
alloc_save_t *
alloc_find_save(const gs_dual_memory_t * dmem, ulong sid)
{
    alloc_save_t *sprev = dmem->space_local->saved;

    if (sid == 0)
	return 0;		/* invalid id */
    while (sprev != 0) {
	if (sprev->id == sid)
	    return sprev;
	sprev = sprev->state.saved;
    }
    return 0;
}

/* Get the client data from a saved state. */
void *
alloc_save_client_data(const alloc_save_t * save)
{
    return save->client_data;
}

/*
 * Do one step of restoring the state.  The client is responsible for
 * calling alloc_find_save to get the save object, and for ensuring that
 * there are no surviving pointers for which alloc_is_since_save is true.
 * Return true if the argument was the innermost save, in which case
 * this is the last (or only) step.
 * Note that "one step" may involve multiple internal steps,
 * if this is the outermost restore (which requires restoring both local
 * and global VM) or if we created extra save levels to reduce scanning.
 */
private void restore_finalize(gs_ref_memory_t *);
private void restore_space(gs_ref_memory_t *, gs_dual_memory_t *);

bool
alloc_restore_step_in(gs_dual_memory_t *dmem, alloc_save_t * save)
{
    /* Get save->space_* now, because the save object will be freed. */
    gs_ref_memory_t *lmem = save->space_local;
    gs_ref_memory_t *gmem = save->space_global;
    gs_ref_memory_t *mem = lmem;
    alloc_save_t *sprev;

    /* Finalize all objects before releasing resources or undoing changes. */
    do {
	ulong sid;

	sprev = mem->saved;
	sid = sprev->id;
	restore_finalize(mem);	/* finalize objects */
	mem = &sprev->state;
	if (sid != 0)
	    break;
    }
    while (sprev != save);
    if (mem->save_level == 0) {
	/* This is the outermost save, which might also */
	/* need to restore global VM. */
	mem = gmem;
	if (mem != lmem && mem->saved != 0)
	    restore_finalize(mem);
    }

    /* Do one (externally visible) step of restoring the state. */
    mem = lmem;
    do {
	ulong sid;

	sprev = mem->saved;
	sid = sprev->id;
	restore_resources(sprev, mem);	/* release other resources */
	restore_space(mem, dmem);	/* release memory */
	if (sid != 0)
	    break;
    }
    while (sprev != save);

    if (mem->save_level == 0) {
	/* This is the outermost save, which might also */
	/* need to restore global VM. */
	mem = gmem;
	if (mem != lmem && mem->saved != 0) {
	    restore_resources(mem->saved, mem);
	    restore_space(mem, dmem);
	}
	alloc_set_not_in_save(dmem);
    } else {			/* Set the l_new attribute in all slots that are now new. */
	save_set_new(mem, true);
    }

    return sprev == save;
}
/* Restore the memory of one space, by undoing changes and freeing */
/* memory allocated since the save. */
private void
restore_space(gs_ref_memory_t * mem, gs_dual_memory_t *dmem)
{
    alloc_save_t *save = mem->saved;
    alloc_save_t saved;

    print_save("restore", mem->space, save);

    /* Undo changes since the save. */
    {
	register alloc_change_t *cp = mem->changes;

	while (cp) {
#ifdef DEBUG
	    if (gs_debug_c('U')) {
		dlputs("[U]restore");
		alloc_save_print(cp, true);
	    }
#endif
	    if (r_is_packed(&cp->contents))
		*cp->where = *(ref_packed *) & cp->contents;
	    else
		ref_assign_inline((ref *) cp->where, &cp->contents);
	    cp = cp->next;
	}
    }

    /* Free memory allocated since the save. */
    /* Note that this frees all chunks except the inner ones */
    /* belonging to this level. */
    saved = *save;
    restore_free(mem);

    /* Restore the allocator state. */
    {
	int num_contexts = mem->num_contexts;	/* don't restore */

	*mem = saved.state;
	mem->num_contexts = num_contexts;
    }
    alloc_open_chunk(mem);

    /* Make the allocator current if it was current before the save. */
    if (saved.is_current) {
	dmem->current = mem;
	dmem->current_space = mem->space;
    }
}

/* Restore to the initial state, releasing all resources. */
/* The allocator is no longer usable after calling this routine! */
void
alloc_restore_all(gs_dual_memory_t * dmem)
{
    /*
     * Save the memory pointers, since freeing space_local will also
     * free dmem itself.
     */
    gs_ref_memory_t *lmem = dmem->space_local;
    gs_ref_memory_t *gmem = dmem->space_global;
    gs_ref_memory_t *smem = dmem->space_system;
    gs_ref_memory_t *mem;

    /* Restore to a state outside any saves. */
    while (lmem->save_level != 0)
	discard(alloc_restore_step_in(dmem, lmem->saved));

    /* Finalize memory. */
    restore_finalize(lmem);
    if ((mem = (gs_ref_memory_t *)lmem->stable_memory) != lmem)
	restore_finalize(mem);
    if (gmem != lmem && gmem->num_contexts == 1) {
	restore_finalize(gmem);
	if ((mem = (gs_ref_memory_t *)gmem->stable_memory) != gmem)
	    restore_finalize(mem);
    }
    restore_finalize(smem);

    /* Release resources other than memory, using fake */
    /* save and memory objects. */
    {
	alloc_save_t empty_save;

	empty_save.spaces = dmem->spaces;
	empty_save.restore_names = false;	/* don't bother to release */
	restore_resources(&empty_save, NULL);
    }

    /* Finally, release memory. */
    restore_free(lmem);
    if ((mem = (gs_ref_memory_t *)lmem->stable_memory) != lmem)
	restore_free(mem);
    if (gmem != lmem) {
	if (!--(gmem->num_contexts)) {
	    restore_free(gmem);
	    if ((mem = (gs_ref_memory_t *)gmem->stable_memory) != gmem)
		restore_free(mem);
	}
    }
    restore_free(smem);

}

/*
 * Finalize objects that will be freed by a restore.
 * Note that we must temporarily disable the freeing operations
 * of the allocator while doing this.
 */
private void
restore_finalize(gs_ref_memory_t * mem)
{
    chunk_t *cp;

    alloc_close_chunk(mem);
    gs_enable_free((gs_memory_t *) mem, false);
    for (cp = mem->clast; cp != 0; cp = cp->cprev) {
	SCAN_CHUNK_OBJECTS(cp)
	    DO_ALL
	    struct_proc_finalize((*finalize)) =
	    pre->o_type->finalize;
	if (finalize != 0) {
	    if_debug2('u', "[u]restore finalizing %s 0x%lx\n",
		      struct_type_name_string(pre->o_type),
		      (ulong) (pre + 1));
	    (*finalize) (pre + 1);
	}
	END_OBJECTS_SCAN
    }
    gs_enable_free((gs_memory_t *) mem, true);
}

/* Release resources for a restore */
private void
restore_resources(alloc_save_t * sprev, gs_ref_memory_t * mem)
{
#ifdef DEBUG
    if (mem) {
	/* Note restoring of the file list. */
	if_debug4('u', "[u%u]file_restore 0x%lx => 0x%lx for 0x%lx\n",
		  mem->space, (ulong)mem->streams,
		  (ulong)sprev->state.streams, (ulong) sprev);
    }
#endif

    /* Remove entries from font and character caches. */
    font_restore(sprev);

    /* Adjust the name table. */
    if (sprev->restore_names)
	names_restore(mem->gs_lib_ctx->gs_name_table, sprev);
}

/* Release memory for a restore. */
private void
restore_free(gs_ref_memory_t * mem)
{
    /* Free chunks allocated since the save. */
    gs_free_all((gs_memory_t *) mem);
}

/* Forget a save, by merging this level with the next outer one. */
private void file_forget_save(gs_ref_memory_t *);
private void combine_space(gs_ref_memory_t *);
private void forget_changes(gs_ref_memory_t *);
void
alloc_forget_save_in(gs_dual_memory_t *dmem, alloc_save_t * save)
{
    gs_ref_memory_t *mem = save->space_local;
    alloc_save_t *sprev;

    print_save("forget_save", mem->space, save);

    /* Iteratively combine the current level with the previous one. */
    do {
	sprev = mem->saved;
	if (sprev->id != 0)
	    mem->save_level--;
	if (mem->save_level != 0) {
	    alloc_change_t *chp = mem->changes;

	    save_set_new(&sprev->state, true);
	    /* Concatenate the changes chains. */
	    if (chp == 0)
		mem->changes = sprev->state.changes;
	    else {
		while (chp->next != 0)
		    chp = chp->next;
		chp->next = sprev->state.changes;
	    }
	    file_forget_save(mem);
	    combine_space(mem);	/* combine memory */
	} else {
	    forget_changes(mem);
	    save_set_new(mem, false);
	    file_forget_save(mem);
	    combine_space(mem);	/* combine memory */
	    /* This is the outermost save, which might also */
	    /* need to combine global VM. */
	    mem = save->space_global;
	    if (mem != save->space_local && mem->saved != 0) {
		forget_changes(mem);
		save_set_new(mem, false);
		file_forget_save(mem);
		combine_space(mem);
	    }
	    alloc_set_not_in_save(dmem);
	    break;		/* must be outermost */
	}
    }
    while (sprev != save);
}
/* Combine the chunks of the next outer level with those of the current one, */
/* and free the bookkeeping structures. */
private void
combine_space(gs_ref_memory_t * mem)
{
    alloc_save_t *saved = mem->saved;
    gs_ref_memory_t *omem = &saved->state;
    chunk_t *cp;
    chunk_t *csucc;

    alloc_close_chunk(mem);
    for (cp = mem->cfirst; cp != 0; cp = csucc) {
	csucc = cp->cnext;	/* save before relinking */
	if (cp->outer == 0)
	    alloc_link_chunk(cp, omem);
	else {
	    chunk_t *outer = cp->outer;

	    outer->inner_count--;
	    if (mem->pcc == cp)
		mem->pcc = outer;
	    if (mem->cfreed.cp == cp)
		mem->cfreed.cp = outer;
	    /* "Free" the header of the inner chunk, */
	    /* and any immediately preceding gap left by */
	    /* the GC having compacted the outer chunk. */
	    {
		obj_header_t *hp = (obj_header_t *) outer->cbot;

		hp->o_alone = 0;
		hp->o_size = (char *)(cp->chead + 1)
		    - (char *)(hp + 1);
		hp->o_type = &st_bytes;
		/* The following call is probably not safe. */
#if 0				/* **************** */
		gs_free_object((gs_memory_t *) mem,
			       hp + 1, "combine_space(header)");
#endif /* **************** */
	    }
	    /* Update the outer chunk's allocation pointers. */
	    outer->cbot = cp->cbot;
	    outer->rcur = cp->rcur;
	    outer->rtop = cp->rtop;
	    outer->ctop = cp->ctop;
	    outer->has_refs |= cp->has_refs;
	    gs_free_object(mem->non_gc_memory, cp,
			   "combine_space(inner)");
	}
    }
    /* Update relevant parts of allocator state. */
    mem->cfirst = omem->cfirst;
    mem->clast = omem->clast;
    mem->allocated += omem->allocated;
    mem->gc_allocated += omem->allocated;
    mem->lost.objects += omem->lost.objects;
    mem->lost.refs += omem->lost.refs;
    mem->lost.strings += omem->lost.strings;
    mem->saved = omem->saved;
    mem->previous_status = omem->previous_status;
    {				/* Concatenate free lists. */
	int i;

	for (i = 0; i < num_freelists; i++) {
	    obj_header_t *olist = omem->freelists[i];
	    obj_header_t *list = mem->freelists[i];

	    if (olist == 0);
	    else if (list == 0)
		mem->freelists[i] = olist;
	    else {
		while (*(obj_header_t **) list != 0)
		    list = *(obj_header_t **) list;
		*(obj_header_t **) list = olist;
	    }
	}
	if (omem->largest_free_size > mem->largest_free_size)
	    mem->largest_free_size = omem->largest_free_size;
    }
    gs_free_object((gs_memory_t *) mem, saved, "combine_space(saved)");
    alloc_open_chunk(mem);
}
/* Free the changes chain for a level 0 .forgetsave, */
/* resetting the l_new flag in the changed refs. */
private void
forget_changes(gs_ref_memory_t * mem)
{
    register alloc_change_t *chp = mem->changes;
    alloc_change_t *next;

    for (; chp; chp = next) {
	ref_packed *prp = chp->where;

	if_debug1('U', "[U]forgetting change 0x%lx\n", (ulong) chp);
	if (!r_is_packed(prp))
	    r_clear_attrs((ref *) prp, l_new);
	next = chp->next;
	gs_free_object((gs_memory_t *) mem, chp, "forget_changes");
    }
    mem->changes = 0;
}
/* Update the streams list when forgetting a save. */
private void
file_forget_save(gs_ref_memory_t * mem)
{
    const alloc_save_t *save = mem->saved;
    stream *streams = mem->streams;
    stream *saved_streams = save->state.streams;

    if_debug4('u', "[u%d]file_forget_save 0x%lx + 0x%lx for 0x%lx\n",
	      mem->space, (ulong) streams, (ulong) saved_streams,
	      (ulong) save);
    if (streams == 0)
	mem->streams = saved_streams;
    else if (saved_streams != 0) {
	while (streams->next != 0)
	    streams = streams->next;
	streams->next = saved_streams;
	saved_streams->prev = streams;
    }
}

/* ------ Internal routines ------ */

/* Set or reset the l_new attribute in every relevant slot. */
/* This includes every slot on the current change chain, */
/* and every (ref) slot allocated at this save level. */
/* Return the number of bytes of data scanned. */
private long
save_set_new(gs_ref_memory_t * mem, bool to_new)
{
    long scanned = 0;

    /* Handle the change chain. */
    save_set_new_changes(mem, to_new);

    /* Handle newly allocated ref objects. */
    SCAN_MEM_CHUNKS(mem, cp) {
	if (cp->has_refs) {
	    bool has_refs = false;

	    SCAN_CHUNK_OBJECTS(cp)
		DO_ALL
		if_debug3('U', "[U]set_new scan(0x%lx(%u), %d)\n",
			  (ulong) pre, size, to_new);
	    if (pre->o_type == &st_refs) {
		/* These are refs, scan them. */
		ref_packed *prp = (ref_packed *) (pre + 1);
		ref_packed *next = (ref_packed *) ((char *)prp + size);
#ifdef ALIGNMENT_ALIASING_BUG
		ref *rpref;
# define RP_REF(rp) (rpref = (ref *)rp, rpref)
#else
# define RP_REF(rp) ((ref *)rp)
#endif

		if_debug2('U', "[U]refs 0x%lx to 0x%lx\n",
			  (ulong) prp, (ulong) next);
		has_refs = true;
		scanned += size;
		/* We know that every block of refs ends with */
		/* a full-size ref, so we only need the end check */
		/* when we encounter one of those. */
		if (to_new)
		    while (1) {
			if (r_is_packed(prp))
			    prp++;
			else {
			    RP_REF(prp)->tas.type_attrs |= l_new;
			    prp += packed_per_ref;
			    if (prp >= next)
				break;
			}
		} else
		    while (1) {
			if (r_is_packed(prp))
			    prp++;
			else {
			    RP_REF(prp)->tas.type_attrs &= ~l_new;
			    prp += packed_per_ref;
			    if (prp >= next)
				break;
			}
		    }
#undef RP_REF
	    } else
		scanned += sizeof(obj_header_t);
	    END_OBJECTS_SCAN
		cp->has_refs = has_refs;
	}
    }
    END_CHUNKS_SCAN
	if_debug2('u', "[u]set_new (%s) scanned %ld\n",
		  (to_new ? "restore" : "save"), scanned);
    return scanned;
}

/* Set or reset the l_new attribute on the changes chain. */
private void
save_set_new_changes(gs_ref_memory_t * mem, bool to_new)
{
    register alloc_change_t *chp = mem->changes;
    register uint new = (to_new ? l_new : 0);

    for (; chp; chp = chp->next) {
	ref_packed *prp = chp->where;

	if_debug3('U', "[U]set_new 0x%lx: (0x%lx, %d)\n",
		  (ulong)chp, (ulong)prp, new);
	if (!r_is_packed(prp)) {
	    ref *const rp = (ref *) prp;

	    rp->tas.type_attrs =
		(rp->tas.type_attrs & ~l_new) + new;
	}
    }
}
