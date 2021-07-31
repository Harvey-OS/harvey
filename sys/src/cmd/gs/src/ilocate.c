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

/*$Id: ilocate.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Object locating and validating for Ghostscript memory manager */
#include "ghost.h"
#include "memory_.h"
#include "errors.h"
#include "gsexit.h"
#include "gsstruct.h"
#include "iastate.h"
#include "idict.h"
#include "igc.h"		/* for gc_state_t */
#include "igcstr.h"		/* for prototype */
#include "iname.h"
#include "ipacked.h"
#include "isstate.h"
#include "iutil.h"		/* for packed_get */
#include "ivmspace.h"
#include "store.h"

/* ================ Locating ================ */

/* Locate a pointer in the chunks of a space being collected. */
/* This is only used for string garbage collection and for debugging. */
chunk_t *
gc_locate(const void *ptr, gc_state_t * gcst)
{
    const gs_ref_memory_t *mem;
    const gs_ref_memory_t *other;

    if (chunk_locate(ptr, &gcst->loc))
	return gcst->loc.cp;
    mem = gcst->loc.memory;

    /*
     * Try the stable allocator of this space, or, if the current memory
     * is the stable one, the non-stable allocator of this space.
     */

    if ((other = (const gs_ref_memory_t *)mem->stable_memory) != mem ||
	(other = gcst->spaces_indexed[mem->space >> r_space_shift]) != mem
	) {
	gcst->loc.memory = other;
	gcst->loc.cp = 0;
	if (chunk_locate(ptr, &gcst->loc))
	    return gcst->loc.cp;
    }

    /*
     * Try the other space, if there is one, including its stable allocator
     * and all save levels.  (If the original space is system space, try
     * local space.)
     */

    if (gcst->space_local != gcst->space_global) {
	gcst->loc.memory = other =
	    (mem->space == avm_local ? gcst->space_global : gcst->space_local);
	gcst->loc.cp = 0;
	if (chunk_locate(ptr, &gcst->loc))
	    return gcst->loc.cp;
	/* Try its stable allocator. */
	if (other->stable_memory != (const gs_memory_t *)other) {
	    gcst->loc.memory = (gs_ref_memory_t *)other->stable_memory;
	    gcst->loc.cp = 0;
	    if (chunk_locate(ptr, &gcst->loc))
		return gcst->loc.cp;
	    gcst->loc.memory = other;
	}
	/* Try other save levels of this space. */
	while (gcst->loc.memory->saved != 0) {
	    gcst->loc.memory = &gcst->loc.memory->saved->state;
	    gcst->loc.cp = 0;
	    if (chunk_locate(ptr, &gcst->loc))
		return gcst->loc.cp;
	}
    }

    /*
     * Try system space.  This is simpler because it isn't subject to
     * save/restore and doesn't have a separate stable allocator.
     */

    if (mem != gcst->space_system) {
	gcst->loc.memory = gcst->space_system;
	gcst->loc.cp = 0;
	if (chunk_locate(ptr, &gcst->loc))
	    return gcst->loc.cp;
    }

    /*
     * Try other save levels of the initial space, or of global space if the
     * original space was system space.  In the latter case, try all
     * levels, and its stable allocator.
     */

    switch (mem->space) {
    default:			/* system */
	other = gcst->space_global;
	if (other->stable_memory != (const gs_memory_t *)other) {
	    gcst->loc.memory = (gs_ref_memory_t *)other->stable_memory;
	    gcst->loc.cp = 0;
	    if (chunk_locate(ptr, &gcst->loc))
		return gcst->loc.cp;
	}
	gcst->loc.memory = other;
	break;
    case avm_global:
	gcst->loc.memory = gcst->space_global;
	break;
    case avm_local:
	gcst->loc.memory = gcst->space_local;
	break;
    }
    for (;;) {
	if (gcst->loc.memory != mem) {	/* don't do twice */
	    gcst->loc.cp = 0;
	    if (chunk_locate(ptr, &gcst->loc))
		return gcst->loc.cp;
	}
	if (gcst->loc.memory->saved == 0)
	    break;
	gcst->loc.memory = &gcst->loc.memory->saved->state;
    }

    /* Restore locator to a legal state and report failure. */

    gcst->loc.memory = mem;
    gcst->loc.cp = 0;
    return 0;
}

/* ================ Debugging ================ */

#ifdef DEBUG

/* Define the structure for temporarily saving allocator state. */
typedef struct alloc_temp_save_s {
	chunk_t cc;
	uint rsize;
	ref rlast;
} alloc_temp_save_t;
/* Temporarily save the state of an allocator. */
private void
alloc_temp_save(alloc_temp_save_t *pats, gs_ref_memory_t *mem)
{
    chunk_t *pcc = mem->pcc;
    obj_header_t *rcur = mem->cc.rcur;

    if (pcc != 0) {
	pats->cc = *pcc;
	*pcc = mem->cc;
    }
    if (rcur != 0) {
	pats->rsize = rcur[-1].o_size;
	rcur[-1].o_size = mem->cc.rtop - (byte *) rcur;
	/* Create the final ref, reserved for the GC. */
	pats->rlast = ((ref *) mem->cc.rtop)[-1];
	make_mark((ref *) mem->cc.rtop - 1);
    }
}
/* Restore the temporarily saved state. */
private void
alloc_temp_restore(alloc_temp_save_t *pats, gs_ref_memory_t *mem)
{
    chunk_t *pcc = mem->pcc;
    obj_header_t *rcur = mem->cc.rcur;

    if (rcur != 0) {
	rcur[-1].o_size = pats->rsize;
	((ref *) mem->cc.rtop)[-1] = pats->rlast;
    }
    if (pcc != 0)
	*pcc = pats->cc;
}

/* Validate the contents of an allocator. */
void
ialloc_validate_spaces(const gs_dual_memory_t * dmem)
{
    int i;
    gc_state_t state;
    alloc_temp_save_t
	save[countof(dmem->spaces_indexed)],
	save_stable[countof(dmem->spaces_indexed)];
    gs_ref_memory_t *mem;

    state.spaces = dmem->spaces;
    state.loc.memory = state.space_local;
    state.loc.cp = 0;

    /* Save everything we need to reset temporarily. */

    for (i = 0; i < countof(save); i++)
	if ((mem = dmem->spaces_indexed[i]) != 0) {
	    alloc_temp_save(&save[i], mem);
	    if (mem->stable_memory != (gs_memory_t *)mem)
		alloc_temp_save(&save_stable[i],
				(gs_ref_memory_t *)mem->stable_memory);
	}

    /* Validate memory. */

    for (i = 0; i < countof(save); i++)
	if ((mem = dmem->spaces_indexed[i]) != 0) {
	    ialloc_validate_memory(mem, &state);
	    if (mem->stable_memory != (gs_memory_t *)mem)
		ialloc_validate_memory((gs_ref_memory_t *)mem->stable_memory,
				       &state);
	}

    /* Undo temporary changes. */

    for (i = 0; i < countof(save); i++)
	if ((mem = dmem->spaces_indexed[i]) != 0) {
	    if (mem->stable_memory != (gs_memory_t *)mem)
		alloc_temp_restore(&save_stable[i],
				   (gs_ref_memory_t *)mem->stable_memory);
	    alloc_temp_restore(&save[i], mem);
	}
}
void
ialloc_validate_memory(const gs_ref_memory_t * mem, gc_state_t * gcst)
{
    const gs_ref_memory_t *smem;
    int level;

    for (smem = mem, level = 0; smem != 0;
	 smem = &smem->saved->state, --level
	) {
	const chunk_t *cp;
	int i;

	if_debug3('6', "[6]validating memory 0x%lx, space %d, level %d\n",
		  (ulong) mem, mem->space, level);
	/* Validate chunks. */
	for (cp = smem->cfirst; cp != 0; cp = cp->cnext)
	    ialloc_validate_chunk(cp, gcst);
	/* Validate freelists. */
	for (i = 0; i < num_freelists; ++i) {
	    uint free_size = i << log2_obj_align_mod;
	    const obj_header_t *pfree;

	    for (pfree = mem->freelists[i]; pfree != 0;
		 pfree = *(const obj_header_t * const *)pfree
		) {
		uint size = pfree[-1].o_size;

		if (pfree[-1].o_type != &st_free) {
		    lprintf3("Non-free object 0x%lx(%u) on freelist %i!\n",
			     (ulong) pfree, size, i);
		    break;
		}
		if ((i == LARGE_FREELIST_INDEX && size < max_freelist_size) ||
		 (i != LARGE_FREELIST_INDEX && 
		 (size < free_size - obj_align_mask || size > free_size))) {
		    lprintf3("Object 0x%lx(%u) size wrong on freelist %i!\n",
			     (ulong) pfree, size, i);
		    break;
		}
	    }
	}
    };
}

/* Check the validity of an object's size. */
inline private bool
object_size_valid(const obj_header_t * pre, uint size, const chunk_t * cp)
{
    return (pre->o_alone ? (const byte *)pre == cp->cbase :
	    size <= cp->ctop - (const byte *)(pre + 1));
}

/* Validate all the objects in a chunk. */
private void ialloc_validate_ref(P2(const ref *, gc_state_t *));
private void ialloc_validate_ref_packed(P2(const ref_packed *, gc_state_t *));
void
ialloc_validate_chunk(const chunk_t * cp, gc_state_t * gcst)
{
    if_debug_chunk('6', "[6]validating chunk", cp);
    SCAN_CHUNK_OBJECTS(cp);
    DO_ALL
	if (pre->o_type == &st_free) {
	    if (!object_size_valid(pre, size, cp))
		lprintf3("Bad free object 0x%lx(%lu), in chunk 0x%lx!\n",
			 (ulong) (pre + 1), (ulong) size, (ulong) cp);
	} else
	    ialloc_validate_object(pre + 1, cp, gcst);
    if_debug3('7', " [7]validating %s(%lu) 0x%lx\n",
	      struct_type_name_string(pre->o_type),
	      (ulong) size, (ulong) pre);
    if (pre->o_type == &st_refs) {
	const ref_packed *rp = (const ref_packed *)(pre + 1);
	const char *end = (const char *)rp + size;

	while ((const char *)rp < end) {
	    ialloc_validate_ref_packed(rp, gcst);
	    rp = packed_next(rp);
	}
    } else {
	struct_proc_enum_ptrs((*proc)) = pre->o_type->enum_ptrs;
	uint index = 0;
	enum_ptr_t eptr;
	gs_ptr_type_t ptype;

	if (proc != gs_no_struct_enum_ptrs)
	    for (; (ptype = (*proc) (pre + 1, size, index, &eptr, pre->o_type, NULL)) != 0; ++index)
		if (eptr.ptr == 0)
		    DO_NOTHING;
		else if (ptype == ptr_struct_type)
		    ialloc_validate_object(eptr.ptr, NULL, gcst);
		else if (ptype == ptr_ref_type)
		    ialloc_validate_ref_packed(eptr.ptr, gcst);
    }
    END_OBJECTS_SCAN
}
/* Validate a ref. */
private void
ialloc_validate_ref_packed(const ref_packed * rp, gc_state_t * gcst)
{
    if (r_is_packed(rp)) {
	ref unpacked;

	packed_get(rp, &unpacked);
	ialloc_validate_ref(&unpacked, gcst);
    } else {
	ialloc_validate_ref((const ref *)rp, gcst);
    }
}
private void
ialloc_validate_ref(const ref * pref, gc_state_t * gcst)
{
    const void *optr;
    const ref *rptr;
    const char *tname;
    uint size;

    if (!gs_debug_c('?'))
	return;			/* no check */
    if (r_space(pref) == avm_foreign)
	return;
    switch (r_type(pref)) {
	case t_file:
	    optr = pref->value.pfile;
	    goto cks;
	case t_device:
	    optr = pref->value.pdevice;
	    goto cks;
	case t_fontID:
	case t_struct:
	case t_astruct:
	    optr = pref->value.pstruct;
cks:	    if (optr != 0)
		ialloc_validate_object(optr, NULL, gcst);
	    break;
	case t_name:
	    if (name_index_ptr(r_size(pref)) != pref->value.pname) {
		lprintf3("At 0x%lx, bad name %u, pname = 0x%lx\n",
			 (ulong) pref, (uint) r_size(pref),
			 (ulong) pref->value.pname);
		break;
	    } {
		ref sref;

		name_string_ref(pref, &sref);
		if (r_space(&sref) != avm_foreign &&
		    !gc_locate(sref.value.const_bytes, gcst)
		    ) {
		    lprintf4("At 0x%lx, bad name %u, pname = 0x%lx, string 0x%lx not in any chunk\n",
			     (ulong) pref, (uint) r_size(pref),
			     (ulong) pref->value.pname,
			     (ulong) sref.value.const_bytes);
		}
	    }
	    break;
	case t_string:
	    if (r_size(pref) != 0 && !gc_locate(pref->value.bytes, gcst))
		lprintf3("At 0x%lx, string ptr 0x%lx[%u] not in any chunk\n",
			 (ulong) pref, (ulong) pref->value.bytes,
			 (uint) r_size(pref));
	    break;
	case t_array:
	    if (r_size(pref) == 0)
		break;
	    rptr = pref->value.refs;
	    size = r_size(pref);
	    tname = "array";
cka:	    if (!gc_locate(rptr, gcst)) {
		lprintf3("At 0x%lx, %s 0x%lx not in any chunk\n",
			 (ulong) pref, tname, (ulong) rptr);
		break;
	    } {
		uint i;

		for (i = 0; i < size; ++i) {
		    const ref *elt = rptr + i;

		    if (r_is_packed(elt))
			lprintf5("At 0x%lx, %s 0x%lx[%u] element %u is not a ref\n",
				 (ulong) pref, tname, (ulong) rptr, size, i);
		}
	    }
	    break;
	case t_shortarray:
	case t_mixedarray:
	    if (r_size(pref) == 0)
		break;
	    optr = pref->value.packed;
	    if (!gc_locate(optr, gcst))
		lprintf2("At 0x%lx, packed array 0x%lx not in any chunk\n",
			 (ulong) pref, (ulong) optr);
	    break;
	case t_dictionary:
	    {
		const dict *pdict = pref->value.pdict;

		if (!r_has_type(&pdict->values, t_array) ||
		    !r_is_array(&pdict->keys) ||
		    !r_has_type(&pdict->count, t_integer) ||
		    !r_has_type(&pdict->maxlength, t_integer)
		    )
		    lprintf2("At 0x%lx, invalid dict 0x%lx\n",
			     (ulong) pref, (ulong) pdict);
		rptr = (const ref *)pdict;
	    }
	    size = sizeof(dict) / sizeof(ref);
	    tname = "dict";
	    goto cka;
    }
}

/* Validate an object. */
void
ialloc_validate_object(const obj_header_t * ptr, const chunk_t * cp,
		       gc_state_t * gcst)
{
    const obj_header_t *pre = ptr - 1;
    ulong size = pre_obj_contents_size(pre);
    gs_memory_type_ptr_t otype = pre->o_type;
    const char *oname;

    if (!gs_debug_c('?'))
	return;			/* no check */
    if (cp == 0 && gcst != 0) {
	gc_state_t st;

	st = *gcst;		/* no side effects! */
	if (!(cp = gc_locate(pre, &st))) {
	    lprintf1("Object 0x%lx not in any chunk!\n",
		     (ulong) ptr);
	    return;		/*gs_abort(); */
	}
    }
    if (otype == &st_free) {
	lprintf3("Reference to free object 0x%lx(%lu), in chunk 0x%lx!\n",
		 (ulong) ptr, (ulong) size, (ulong) cp);
	gs_abort();
    }
    if ((cp != 0 && !object_size_valid(pre, size, cp)) ||
	otype->ssize == 0 ||
	size % otype->ssize != 0 ||
	(oname = struct_type_name_string(otype),
	 *oname < 33 || *oname > 126)
	) {
	lprintf4("Bad object 0x%lx(%lu), ssize = %u, in chunk 0x%lx!\n",
		 (ulong) ptr, (ulong) size, otype->ssize, (ulong) cp);
	gs_abort();
    }
}

#else /* !DEBUG */

void
ialloc_validate_spaces(const gs_dual_memory_t * dmem)
{
}

void
ialloc_validate_memory(const gs_ref_memory_t * mem, gc_state_t * gcst)
{
}

void
ialloc_validate_chunk(const chunk_t * cp, gc_state_t * gcst)
{
}

void
ialloc_validate_object(const obj_header_t * ptr, const chunk_t * cp,
		       gc_state_t * gcst)
{
}

#endif /* (!)DEBUG */
