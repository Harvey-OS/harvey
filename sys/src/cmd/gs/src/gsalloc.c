/* Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsalloc.c,v 1.24 2005/10/12 10:45:21 leonardo Exp $ */
/* Standard memory allocator */
#include "gx.h"
#include "memory_.h"
#include "gserrors.h"
#include "gsexit.h"
#include "gsmdebug.h"
#include "gsstruct.h"
#include "gxalloc.h"
#include "stream.h"		/* for clearing stream list */

/*
 * Define whether to try consolidating space before adding a new chunk.
 * The default is not to do this, because it is computationally
 * expensive and doesn't seem to help much.  However, this is done for
 * "controlled" spaces whether or not the #define is in effect.
 */
/*#define CONSOLIDATE_BEFORE_ADDING_CHUNK */


/*
 * This allocator produces tracing messages of the form
 *      [aNMOTS]...
 * where
 *   N is the VM space number, +1 if we are allocating from stable memory.
 *   M is : for movable objects, | for immovable,
 *   O is {alloc = +, free = -, grow = >, shrink = <},
 *   T is {bytes = b, object = <, ref = $, string = >}, and
 *   S is {small freelist = f, large freelist = F, LIFO = space,
 *      own chunk = L, lost = #, lost own chunk = ~, other = .}.
 */
#ifdef DEBUG
private int
alloc_trace_space(const gs_ref_memory_t *imem)
{
    return imem->space + (imem->stable_memory == (const gs_memory_t *)imem);
}
private void
alloc_trace(const char *chars, gs_ref_memory_t * imem, client_name_t cname,
	    gs_memory_type_ptr_t stype, uint size, const void *ptr)
{
    if_debug7('A', "[a%d%s]%s %s(%u) %s0x%lx\n",
	      alloc_trace_space(imem), chars, client_name_string(cname),
	      (ptr == 0 || stype == 0 ? "" :
	       struct_type_name_string(stype)),
	      size, (chars[1] == '+' ? "= " : ""), (ulong) ptr);
}
private bool
alloc_size_is_ok(gs_memory_type_ptr_t stype)
{
    return (stype->ssize > 0 && stype->ssize < 0x100000);
}
#  define ALLOC_CHECK_SIZE(stype)\
    BEGIN\
      if (!alloc_size_is_ok(stype)) {\
	lprintf2("size of struct type 0x%lx is 0x%lx!\n",\
		 (ulong)(stype), (ulong)((stype)->ssize));\
	return 0;\
      }\
    END
#else
#  define alloc_trace(chars, imem, cname, stype, size, ptr) DO_NOTHING
#  define ALLOC_CHECK_SIZE(stype) DO_NOTHING
#endif

/*
 * The structure descriptor for allocators.  Even though allocators
 * are allocated outside GC space, they reference objects within it.
 */
public_st_ref_memory();
private 
ENUM_PTRS_BEGIN(ref_memory_enum_ptrs) return 0;
ENUM_PTR3(0, gs_ref_memory_t, streams, names_array, changes);
ENUM_PTR(3, gs_ref_memory_t, saved);
ENUM_PTRS_END
private RELOC_PTRS_WITH(ref_memory_reloc_ptrs, gs_ref_memory_t *mptr)
{
    RELOC_PTR(gs_ref_memory_t, streams);
    RELOC_PTR(gs_ref_memory_t, names_array);
    RELOC_PTR(gs_ref_memory_t, changes);
    /* Don't relocate the saved pointer now -- see igc.c for details. */
    mptr->reloc_saved = RELOC_OBJ(mptr->saved);
}
RELOC_PTRS_END

/*
 * Define the flags for alloc_obj, which implements all but the fastest
 * case of allocation.
 */
typedef enum {
    ALLOC_IMMOVABLE = 1,
    ALLOC_DIRECT = 2		/* called directly, without fast-case checks */
} alloc_flags_t;

/* Forward references */
private void remove_range_from_freelist(gs_ref_memory_t *mem, void* bottom, void* top);
private obj_header_t *large_freelist_alloc(gs_ref_memory_t *mem, uint size);
private obj_header_t *scavenge_low_free(gs_ref_memory_t *mem, unsigned request_size);
private ulong compute_free_objects(gs_ref_memory_t *);
private obj_header_t *alloc_obj(gs_ref_memory_t *, ulong, gs_memory_type_ptr_t, alloc_flags_t, client_name_t);
private void consolidate_chunk_free(chunk_t *cp, gs_ref_memory_t *mem);
private void trim_obj(gs_ref_memory_t *mem, obj_header_t *obj, uint size, chunk_t *cp);
private chunk_t *alloc_acquire_chunk(gs_ref_memory_t *, ulong, bool, client_name_t);
private chunk_t *alloc_add_chunk(gs_ref_memory_t *, ulong, client_name_t);
void alloc_close_chunk(gs_ref_memory_t *);

/*
 * Define the standard implementation (with garbage collection)
 * of Ghostscript's memory manager interface.
 */
/* Raw memory procedures */
private gs_memory_proc_alloc_bytes(i_alloc_bytes_immovable);
private gs_memory_proc_resize_object(i_resize_object);
private gs_memory_proc_free_object(i_free_object);
private gs_memory_proc_stable(i_stable);
private gs_memory_proc_status(i_status);
private gs_memory_proc_free_all(i_free_all);
private gs_memory_proc_consolidate_free(i_consolidate_free);

/* Object memory procedures */
private gs_memory_proc_alloc_bytes(i_alloc_bytes);
private gs_memory_proc_alloc_struct(i_alloc_struct);
private gs_memory_proc_alloc_struct(i_alloc_struct_immovable);
private gs_memory_proc_alloc_byte_array(i_alloc_byte_array);
private gs_memory_proc_alloc_byte_array(i_alloc_byte_array_immovable);
private gs_memory_proc_alloc_struct_array(i_alloc_struct_array);
private gs_memory_proc_alloc_struct_array(i_alloc_struct_array_immovable);
private gs_memory_proc_object_size(i_object_size);
private gs_memory_proc_object_type(i_object_type);
private gs_memory_proc_alloc_string(i_alloc_string);
private gs_memory_proc_alloc_string(i_alloc_string_immovable);
private gs_memory_proc_resize_string(i_resize_string);
private gs_memory_proc_free_string(i_free_string);
private gs_memory_proc_register_root(i_register_root);
private gs_memory_proc_unregister_root(i_unregister_root);
private gs_memory_proc_enable_free(i_enable_free);

/* We export the procedures for subclasses. */
const gs_memory_procs_t gs_ref_memory_procs =
{
    /* Raw memory procedures */
    i_alloc_bytes_immovable,
    i_resize_object,
    i_free_object,
    i_stable,
    i_status,
    i_free_all,
    i_consolidate_free,
    /* Object memory procedures */
    i_alloc_bytes,
    i_alloc_struct,
    i_alloc_struct_immovable,
    i_alloc_byte_array,
    i_alloc_byte_array_immovable,
    i_alloc_struct_array,
    i_alloc_struct_array_immovable,
    i_object_size,
    i_object_type,
    i_alloc_string,
    i_alloc_string_immovable,
    i_resize_string,
    i_free_string,
    i_register_root,
    i_unregister_root,
    i_enable_free
};

/*
 * Allocate and mostly initialize the state of an allocator (system, global,
 * or local).  Does not initialize global or space.
 */
private void *ialloc_solo(gs_memory_t *, gs_memory_type_ptr_t,
			  chunk_t **);
gs_ref_memory_t *
ialloc_alloc_state(gs_memory_t * parent, uint chunk_size)
{
    chunk_t *cp;
    gs_ref_memory_t *iimem = ialloc_solo(parent, &st_ref_memory, &cp);

    if (iimem == 0)
	return 0;
    iimem->stable_memory = (gs_memory_t *)iimem;
    iimem->procs = gs_ref_memory_procs;
    iimem->gs_lib_ctx = parent->gs_lib_ctx;
    iimem->non_gc_memory = parent;
    iimem->chunk_size = chunk_size;
    iimem->large_size = ((chunk_size / 4) & -obj_align_mod) + 1;
    iimem->is_controlled = false;
    iimem->gc_status.vm_threshold = chunk_size * 3L;
    iimem->gc_status.max_vm = max_long;
    iimem->gc_status.psignal = NULL;
    iimem->gc_status.signal_value = 0;
    iimem->gc_status.enabled = false;
    iimem->gc_status.requested = 0;
    iimem->gc_allocated = 0;
    iimem->previous_status.allocated = 0;
    iimem->previous_status.used = 0;
    ialloc_reset(iimem);
    iimem->cfirst = iimem->clast = cp;
    ialloc_set_limit(iimem);
    iimem->cc.cbot = iimem->cc.ctop = 0;
    iimem->pcc = 0;
    iimem->save_level = 0;
    iimem->new_mask = 0;
    iimem->test_mask = ~0;
    iimem->streams = 0;
    iimem->names_array = 0;
    iimem->roots = 0;
    iimem->num_contexts = 0;
    iimem->saved = 0;
    return iimem;
}

/* Allocate a 'solo' object with its own chunk. */
private void *
ialloc_solo(gs_memory_t * parent, gs_memory_type_ptr_t pstype,
	    chunk_t ** pcp)
{	/*
	 * We can't assume that the parent uses the same object header
	 * that we do, but the GC requires that allocators have
	 * such a header.  Therefore, we prepend one explicitly.
	 */
    chunk_t *cp =
	gs_raw_alloc_struct_immovable(parent, &st_chunk,
				      "ialloc_solo(chunk)");
    uint csize =
	ROUND_UP(sizeof(chunk_head_t) + sizeof(obj_header_t) +
		 pstype->ssize,
		 obj_align_mod);
    byte *cdata = gs_alloc_bytes_immovable(parent, csize, "ialloc_solo");
    obj_header_t *obj = (obj_header_t *) (cdata + sizeof(chunk_head_t));

    if (cp == 0 || cdata == 0)
	return 0;
    alloc_init_chunk(cp, cdata, cdata + csize, false, (chunk_t *) NULL);
    cp->cbot = cp->ctop;
    cp->cprev = cp->cnext = 0;
    /* Construct the object header "by hand". */
    obj->o_alone = 1;
    obj->o_size = pstype->ssize;
    obj->o_type = pstype;
    *pcp = cp;
    return (void *)(obj + 1);
}

/*
 * Add a chunk to an externally controlled allocator.  Such allocators
 * allocate all objects as immovable, are not garbage-collected, and
 * don't attempt to acquire additional memory on their own.
 */
int
ialloc_add_chunk(gs_ref_memory_t *imem, ulong space, client_name_t cname)
{
    chunk_t *cp;

    /* Allow acquisition of this chunk. */
    imem->is_controlled = false;
    imem->large_size = imem->chunk_size;
    imem->limit = max_long;
    imem->gc_status.max_vm = max_long;

    /* Acquire the chunk. */
    cp = alloc_add_chunk(imem, space, cname);

    /*
     * Make all allocations immovable.  Since the "movable" allocators
     * allocate within existing chunks, whereas the "immovable" ones
     * allocate in new chunks, we equate the latter to the former, even
     * though this seems backwards.
     */
    imem->procs.alloc_bytes_immovable = imem->procs.alloc_bytes;
    imem->procs.alloc_struct_immovable = imem->procs.alloc_struct;
    imem->procs.alloc_byte_array_immovable = imem->procs.alloc_byte_array;
    imem->procs.alloc_struct_array_immovable = imem->procs.alloc_struct_array;
    imem->procs.alloc_string_immovable = imem->procs.alloc_string;

    /* Disable acquisition of additional chunks. */
    imem->is_controlled = true;
    imem->limit = 0;

    return (cp ? 0 : gs_note_error(gs_error_VMerror));
}

/* Prepare for a GC by clearing the stream list. */
/* This probably belongs somewhere else.... */
void
ialloc_gc_prepare(gs_ref_memory_t * mem)
{	/*
	 * We have to unlink every stream from its neighbors,
	 * so that referenced streams don't keep all streams around.
	 */
    while (mem->streams != 0) {
	stream *s = mem->streams;

	mem->streams = s->next;
	s->prev = s->next = 0;
    }
}

/* Initialize after a save. */
void
ialloc_reset(gs_ref_memory_t * mem)
{
    mem->cfirst = 0;
    mem->clast = 0;
    mem->cc.rcur = 0;
    mem->cc.rtop = 0;
    mem->cc.has_refs = false;
    mem->allocated = 0;
    mem->inherited = 0;
    mem->changes = 0;
    ialloc_reset_free(mem);
}

/* Initialize after a save or GC. */
void
ialloc_reset_free(gs_ref_memory_t * mem)
{
    int i;
    obj_header_t **p;

    mem->lost.objects = 0;
    mem->lost.refs = 0;
    mem->lost.strings = 0;
    mem->cfreed.cp = 0;
    for (i = 0, p = &mem->freelists[0]; i < num_freelists; i++, p++)
	*p = 0;
    mem->largest_free_size = 0;
}

/*
 * Set an arbitrary limit so that the amount of allocated VM does not grow
 * indefinitely even when GC is disabled.  Benchmarks have shown that
 * the resulting GC's are infrequent enough not to degrade performance
 * significantly.
 */
#define FORCE_GC_LIMIT 8000000

/* Set the allocation limit after a change in one or more of */
/* vm_threshold, max_vm, or enabled, or after a GC. */
void
ialloc_set_limit(register gs_ref_memory_t * mem)
{	/*
	 * The following code is intended to set the limit so that
	 * we stop allocating when allocated + previous_status.allocated
	 * exceeds the lesser of max_vm or (if GC is enabled)
	 * gc_allocated + vm_threshold.
	 */
    ulong max_allocated =
    (mem->gc_status.max_vm > mem->previous_status.allocated ?
     mem->gc_status.max_vm - mem->previous_status.allocated :
     0);

    if (mem->gc_status.enabled) {
	ulong limit = mem->gc_allocated + mem->gc_status.vm_threshold;

	if (limit < mem->previous_status.allocated)
	    mem->limit = 0;
	else {
	    limit -= mem->previous_status.allocated;
	    mem->limit = min(limit, max_allocated);
	}
    } else
	mem->limit = min(max_allocated, mem->gc_allocated + FORCE_GC_LIMIT);
    if_debug7('0', "[0]space=%d, max_vm=%ld, prev.alloc=%ld, enabled=%d,\n\
      gc_alloc=%ld, threshold=%ld => limit=%ld\n",
	      mem->space, (long)mem->gc_status.max_vm,
	      (long)mem->previous_status.allocated,
	      mem->gc_status.enabled, (long)mem->gc_allocated,
	      (long)mem->gc_status.vm_threshold, (long)mem->limit);
}

/*
 * Free all the memory owned by the allocator, except the allocator itself.
 * Note that this only frees memory at the current save level: the client
 * is responsible for restoring to the outermost level if desired.
 */
private void
i_free_all(gs_memory_t * mem, uint free_mask, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    chunk_t *cp;

    if (free_mask & FREE_ALL_DATA) {
	chunk_t *csucc;

	/*
	 * Free the chunks in reverse order, to encourage LIFO behavior.
	 * Don't free the chunk holding the allocator itself.
	 */
	for (cp = imem->clast; cp != 0; cp = csucc) {
	    csucc = cp->cprev;	/* save before freeing */
	    if (cp->cbase + sizeof(obj_header_t) != (byte *)mem)
		alloc_free_chunk(cp, imem);
	}
    }
    if (free_mask & FREE_ALL_ALLOCATOR) {
	/* Free the chunk holding the allocator itself. */
	for (cp = imem->clast; cp != 0; cp = cp->cprev)
	    if (cp->cbase + sizeof(obj_header_t) == (byte *)mem) {
		alloc_free_chunk(cp, imem);
		break;
	    }
    }
}

/* ================ Accessors ================ */

/* Get the size of an object from the header. */
private uint
i_object_size(gs_memory_t * mem, const void /*obj_header_t */ *obj)
{
    return pre_obj_contents_size((const obj_header_t *)obj - 1);
}

/* Get the type of a structure from the header. */
private gs_memory_type_ptr_t
i_object_type(gs_memory_t * mem, const void /*obj_header_t */ *obj)
{
    return ((const obj_header_t *)obj - 1)->o_type;
}

/* Get the GC status of a memory. */
void
gs_memory_gc_status(const gs_ref_memory_t * mem, gs_memory_gc_status_t * pstat)
{
    *pstat = mem->gc_status;
}

/* Set the GC status of a memory. */
void
gs_memory_set_gc_status(gs_ref_memory_t * mem, const gs_memory_gc_status_t * pstat)
{
    mem->gc_status = *pstat;
    ialloc_set_limit(mem);
}

/* Set VM threshold. */
void
gs_memory_set_vm_threshold(gs_ref_memory_t * mem, long val)
{
    gs_memory_gc_status_t stat;
    gs_ref_memory_t * stable = (gs_ref_memory_t *)mem->stable_memory;

    gs_memory_gc_status(mem, &stat);
    stat.vm_threshold = val;
    gs_memory_set_gc_status(mem, &stat);
    gs_memory_gc_status(stable, &stat);
    stat.vm_threshold = val;
    gs_memory_set_gc_status(stable, &stat);
}

/* Set VM reclaim. */
void
gs_memory_set_vm_reclaim(gs_ref_memory_t * mem, bool enabled)
{
    gs_memory_gc_status_t stat;
    gs_ref_memory_t * stable = (gs_ref_memory_t *)mem->stable_memory;

    gs_memory_gc_status(mem, &stat);
    stat.enabled = enabled;
    gs_memory_set_gc_status(mem, &stat);
    gs_memory_gc_status(stable, &stat);
    stat.enabled = enabled;
    gs_memory_set_gc_status(stable, &stat);
}

/* ================ Objects ================ */

/* Allocate a small object quickly if possible. */
/* The size must be substantially less than max_uint. */
/* ptr must be declared as obj_header_t *. */
/* pfl must be declared as obj_header_t **. */
#define IF_FREELIST_ALLOC(ptr, imem, size, pstype, pfl)\
	if ( size <= max_freelist_size &&\
	     *(pfl = &imem->freelists[(size + obj_align_mask) >> log2_obj_align_mod]) != 0\
	   )\
	{	ptr = *pfl;\
		*pfl = *(obj_header_t **)ptr;\
		ptr[-1].o_size = size;\
		ptr[-1].o_type = pstype;\
		/* If debugging, clear the block in an attempt to */\
		/* track down uninitialized data errors. */\
		gs_alloc_fill(ptr, gs_alloc_fill_alloc, size);
#define ELSEIF_BIG_FREELIST_ALLOC(ptr, imem, size, pstype)\
	}\
	else if (size > max_freelist_size &&\
		 (ptr = large_freelist_alloc(imem, size)) != 0)\
	{	ptr[-1].o_type = pstype;\
		/* If debugging, clear the block in an attempt to */\
		/* track down uninitialized data errors. */\
		gs_alloc_fill(ptr, gs_alloc_fill_alloc, size);
#define ELSEIF_LIFO_ALLOC(ptr, imem, size, pstype)\
	}\
	else if ( (imem->cc.ctop - (byte *)(ptr = (obj_header_t *)imem->cc.cbot))\
		>= size + (obj_align_mod + sizeof(obj_header_t) * 2) &&\
	     size < imem->large_size\
	   )\
	{	imem->cc.cbot = (byte *)ptr + obj_size_round(size);\
		ptr->o_alone = 0;\
		ptr->o_size = size;\
		ptr->o_type = pstype;\
		ptr++;\
		/* If debugging, clear the block in an attempt to */\
		/* track down uninitialized data errors. */\
		gs_alloc_fill(ptr, gs_alloc_fill_alloc, size);
#define ELSE_ALLOC\
	}\
	else

private byte *
i_alloc_bytes(gs_memory_t * mem, uint size, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *obj;
    obj_header_t **pfl;

    IF_FREELIST_ALLOC(obj, imem, size, &st_bytes, pfl)
	alloc_trace(":+bf", imem, cname, NULL, size, obj);
    ELSEIF_BIG_FREELIST_ALLOC(obj, imem, size, &st_bytes)
	alloc_trace(":+bF", imem, cname, NULL, size, obj);
    ELSEIF_LIFO_ALLOC(obj, imem, size, &st_bytes)
	alloc_trace(":+b ", imem, cname, NULL, size, obj);
    ELSE_ALLOC
    {
	obj = alloc_obj(imem, size, &st_bytes, 0, cname);
	if (obj == 0)
	    return 0;
	alloc_trace(":+b.", imem, cname, NULL, size, obj);
    }
    return (byte *) obj;
}
private byte *
i_alloc_bytes_immovable(gs_memory_t * mem, uint size, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *obj = alloc_obj(imem, size, &st_bytes,
				  ALLOC_IMMOVABLE | ALLOC_DIRECT, cname);

    if (obj == 0)
	return 0;
    alloc_trace("|+b.", imem, cname, NULL, size, obj);
    return (byte *) obj;
}
private void *
i_alloc_struct(gs_memory_t * mem, gs_memory_type_ptr_t pstype,
	       client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    uint size = pstype->ssize;
    obj_header_t *obj;
    obj_header_t **pfl;

    ALLOC_CHECK_SIZE(pstype);
    IF_FREELIST_ALLOC(obj, imem, size, pstype, pfl)
	alloc_trace(":+<f", imem, cname, pstype, size, obj);
    ELSEIF_BIG_FREELIST_ALLOC(obj, imem, size, pstype)
	alloc_trace(":+<F", imem, cname, pstype, size, obj);
    ELSEIF_LIFO_ALLOC(obj, imem, size, pstype)
	alloc_trace(":+< ", imem, cname, pstype, size, obj);
    ELSE_ALLOC
    {
	obj = alloc_obj(imem, size, pstype, 0, cname);
	if (obj == 0)
	    return 0;
	alloc_trace(":+<.", imem, cname, pstype, size, obj);
    }
    return obj;
}
private void *
i_alloc_struct_immovable(gs_memory_t * mem, gs_memory_type_ptr_t pstype,
			 client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    uint size = pstype->ssize;
    obj_header_t *obj;

    ALLOC_CHECK_SIZE(pstype);
    obj = alloc_obj(imem, size, pstype, ALLOC_IMMOVABLE | ALLOC_DIRECT, cname);
    alloc_trace("|+<.", imem, cname, pstype, size, obj);
    return obj;
}
private byte *
i_alloc_byte_array(gs_memory_t * mem, uint num_elements, uint elt_size,
		   client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *obj = alloc_obj(imem, (ulong) num_elements * elt_size,
				  &st_bytes, ALLOC_DIRECT, cname);

    if_debug6('A', "[a%d:+b.]%s -bytes-*(%lu=%u*%u) = 0x%lx\n",
	      alloc_trace_space(imem), client_name_string(cname),
	      (ulong) num_elements * elt_size,
	      num_elements, elt_size, (ulong) obj);
    return (byte *) obj;
}
private byte *
i_alloc_byte_array_immovable(gs_memory_t * mem, uint num_elements,
			     uint elt_size, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *obj = alloc_obj(imem, (ulong) num_elements * elt_size,
				  &st_bytes, ALLOC_IMMOVABLE | ALLOC_DIRECT,
				  cname);

    if_debug6('A', "[a%d|+b.]%s -bytes-*(%lu=%u*%u) = 0x%lx\n",
	      alloc_trace_space(imem), client_name_string(cname),
	      (ulong) num_elements * elt_size,
	      num_elements, elt_size, (ulong) obj);
    return (byte *) obj;
}
private void *
i_alloc_struct_array(gs_memory_t * mem, uint num_elements,
		     gs_memory_type_ptr_t pstype, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *obj;

    ALLOC_CHECK_SIZE(pstype);
#ifdef DEBUG
    if (pstype->enum_ptrs == basic_enum_ptrs) {
	dprintf2("  i_alloc_struct_array: called with incorrect structure type (not element), struct='%s', client='%s'\n",
		pstype->sname, cname);
	return NULL;		/* fail */
    }
#endif
    obj = alloc_obj(imem,
		    (ulong) num_elements * pstype->ssize,
		    pstype, ALLOC_DIRECT, cname);
    if_debug7('A', "[a%d:+<.]%s %s*(%lu=%u*%u) = 0x%lx\n",
	      alloc_trace_space(imem), client_name_string(cname),
	      struct_type_name_string(pstype),
	      (ulong) num_elements * pstype->ssize,
	      num_elements, pstype->ssize, (ulong) obj);
    return (char *)obj;
}
private void *
i_alloc_struct_array_immovable(gs_memory_t * mem, uint num_elements,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *obj;

    ALLOC_CHECK_SIZE(pstype);
    obj = alloc_obj(imem,
		    (ulong) num_elements * pstype->ssize,
		    pstype, ALLOC_IMMOVABLE | ALLOC_DIRECT, cname);
    if_debug7('A', "[a%d|+<.]%s %s*(%lu=%u*%u) = 0x%lx\n",
	      alloc_trace_space(imem), client_name_string(cname),
	      struct_type_name_string(pstype),
	      (ulong) num_elements * pstype->ssize,
	      num_elements, pstype->ssize, (ulong) obj);
    return (char *)obj;
}
private void *
i_resize_object(gs_memory_t * mem, void *obj, uint new_num_elements,
		client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *pp = (obj_header_t *) obj - 1;
    gs_memory_type_ptr_t pstype = pp->o_type;
    ulong old_size = pre_obj_contents_size(pp);
    ulong new_size = (ulong) pstype->ssize * new_num_elements;
    ulong old_size_rounded = obj_align_round(old_size);
    ulong new_size_rounded = obj_align_round(new_size);
    void *new_obj = NULL;

    if (old_size_rounded == new_size_rounded) {
	pp->o_size = new_size;
	new_obj = obj;
    } else
	if ((byte *)obj + old_size_rounded == imem->cc.cbot &&
	    imem->cc.ctop - (byte *)obj >= new_size_rounded ) {
	    imem->cc.cbot = (byte *)obj + new_size_rounded;
	    pp->o_size = new_size;
	    new_obj = obj;
	} else /* try and trim the object -- but only if room for a dummy header */
	    if (new_size_rounded + sizeof(obj_header_t) <= old_size_rounded) {
		trim_obj(imem, obj, new_size, (chunk_t *)0);
		new_obj = obj;
	    }
    if (new_obj) {
	if_debug8('A', "[a%d:%c%c ]%s %s(%lu=>%lu) 0x%lx\n",
		  alloc_trace_space(imem),
		  (new_size > old_size ? '>' : '<'),
		  (pstype == &st_bytes ? 'b' : '<'),
		  client_name_string(cname),
		  struct_type_name_string(pstype),
		  old_size, new_size, (ulong) obj);
	return new_obj;
    }
    /* Punt. */
    new_obj = gs_alloc_struct_array(mem, new_num_elements, void,
				    pstype, cname);
    if (new_obj == 0)
	return 0;
    memcpy(new_obj, obj, min(old_size, new_size));
    gs_free_object(mem, obj, cname);
    return new_obj;
}
private void
i_free_object(gs_memory_t * mem, void *ptr, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    obj_header_t *pp;
    gs_memory_type_ptr_t pstype;

    struct_proc_finalize((*finalize));
    uint size, rounded_size;

    if (ptr == 0)
	return;
    pp = (obj_header_t *) ptr - 1;
    pstype = pp->o_type;
#ifdef DEBUG
    if (gs_debug_c('?')) {
	chunk_locator_t cld;

	if (pstype == &st_free) {
	    lprintf2("%s: object 0x%lx already free!\n",
		     client_name_string(cname), (ulong) ptr);
	    return;		/*gs_abort(); */
	}
	/* Check that this allocator owns the object being freed. */
	cld.memory = imem;
	while ((cld.cp = cld.memory->clast),
	       !chunk_locate_ptr(ptr, &cld)
	    ) {
	    if (!cld.memory->saved) {
		lprintf3("%s: freeing 0x%lx, not owned by memory 0x%lx!\n",
			 client_name_string(cname), (ulong) ptr,
			 (ulong) mem);
		return;		/*gs_abort(); */
	    }
		  /****** HACK: we know the saved state is the first ******
		   ****** member of an alloc_save_t. ******/
	    cld.memory = (gs_ref_memory_t *) cld.memory->saved;
	}
	/* Check that the object is in the allocated region. */
	if (cld.memory == imem && cld.cp == imem->pcc)
	    cld.cp = &imem->cc;
	if (!(PTR_BETWEEN((const byte *)pp, cld.cp->cbase,
			  cld.cp->cbot))
	    ) {
	    lprintf5("%s: freeing 0x%lx,\n\toutside chunk 0x%lx cbase=0x%lx, cbot=0x%lx!\n",
		     client_name_string(cname), (ulong) ptr,
		     (ulong) cld.cp, (ulong) cld.cp->cbase,
		     (ulong) cld.cp->cbot);
	    return;		/*gs_abort(); */
	}
    }
#endif
    size = pre_obj_contents_size(pp);
    rounded_size = obj_align_round(size);
    finalize = pstype->finalize;
    if (finalize != 0) {
	if_debug3('u', "[u]finalizing %s 0x%lx (%s)\n",
		  struct_type_name_string(pstype),
		  (ulong) ptr, client_name_string(cname));
	(*finalize) (ptr);
    }
    if ((byte *) ptr + rounded_size == imem->cc.cbot) {
	alloc_trace(":-o ", imem, cname, pstype, size, ptr);
	gs_alloc_fill(ptr, gs_alloc_fill_free, size);
	imem->cc.cbot = (byte *) pp;
	/* IFF this object is adjacent to (or below) the byte after the
	 * highest free object, do the consolidation within this chunk. */
	if ((byte *)pp <= imem->cc.int_freed_top) {
	    consolidate_chunk_free(&(imem->cc), imem);
	}
	return;
    }
    if (pp->o_alone) {
		/*
		 * We gave this object its own chunk.  Free the entire chunk,
		 * unless it belongs to an older save level, in which case
		 * we mustn't overwrite it.
		 */
	chunk_locator_t cl;

#ifdef DEBUG
	{
	    chunk_locator_t cld;

	    cld.memory = imem;
	    cld.cp = 0;
	    if (gs_debug_c('a'))
		alloc_trace(
			    (chunk_locate_ptr(ptr, &cld) ? ":-oL" : ":-o~"),
			       imem, cname, pstype, size, ptr);
	}
#endif
	cl.memory = imem;
	cl.cp = 0;
	if (chunk_locate_ptr(ptr, &cl)) {
	    if (!imem->is_controlled)
		alloc_free_chunk(cl.cp, imem);
	    return;
	}
	/* Don't overwrite even if gs_alloc_debug is set. */
    }
    if (rounded_size >= sizeof(obj_header_t *)) {
	/*
	 * Put the object on a freelist, unless it belongs to
	 * an older save level, in which case we mustn't
	 * overwrite it.
	 */
	imem->cfreed.memory = imem;
	if (chunk_locate(ptr, &imem->cfreed)) {
	    obj_header_t **pfl;

	    if (size > max_freelist_size) {
		pfl = &imem->freelists[LARGE_FREELIST_INDEX];
		if (rounded_size > imem->largest_free_size)
		    imem->largest_free_size = rounded_size;
	    } else {
		pfl = &imem->freelists[(size + obj_align_mask) >>
				      log2_obj_align_mod];
	    }
	    /* keep track of highest object on a freelist */
	    if ((byte *)pp >= imem->cc.int_freed_top)
		imem->cc.int_freed_top = (byte *)ptr + rounded_size;
	    pp->o_type = &st_free;	/* don't confuse GC */
	    gs_alloc_fill(ptr, gs_alloc_fill_free, size);
	    *(obj_header_t **) ptr = *pfl;
	    *pfl = (obj_header_t *) ptr;
	    alloc_trace((size > max_freelist_size ? ":-oF" : ":-of"),
			imem, cname, pstype, size, ptr);
	    return;
	}
	/* Don't overwrite even if gs_alloc_debug is set. */
    } else {
	pp->o_type = &st_free;	/* don't confuse GC */
	gs_alloc_fill(ptr, gs_alloc_fill_free, size);
    }
    alloc_trace(":-o#", imem, cname, pstype, size, ptr);
    imem->lost.objects += obj_size_round(size);
}
private byte *
i_alloc_string(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    byte *str;
    /*
     * Cycle through the chunks at the current save level, starting
     * with the currently open one.
     */
    chunk_t *cp_orig = imem->pcc;

    if (cp_orig == 0) {
	/* Open an arbitrary chunk. */
	cp_orig = imem->pcc = imem->cfirst;
	alloc_open_chunk(imem);
    }
top:
    if (imem->cc.ctop - imem->cc.cbot > nbytes) {
	if_debug4('A', "[a%d:+> ]%s(%u) = 0x%lx\n",
		  alloc_trace_space(imem), client_name_string(cname), nbytes,
		  (ulong) (imem->cc.ctop - nbytes));
	str = imem->cc.ctop -= nbytes;
	gs_alloc_fill(str, gs_alloc_fill_alloc, nbytes);
	return str;
    }
    /* Try the next chunk. */
    {
	chunk_t *cp = imem->cc.cnext;

	alloc_close_chunk(imem);
	if (cp == 0)
	    cp = imem->cfirst;
	imem->pcc = cp;
	alloc_open_chunk(imem);
	if (cp != cp_orig)
	    goto top;
    }
    if (nbytes > string_space_quanta(max_uint - sizeof(chunk_head_t)) *
	string_data_quantum
	) {			/* Can't represent the size in a uint! */
	return 0;
    }
    if (nbytes >= imem->large_size) {	/* Give it a chunk all its own. */
	return i_alloc_string_immovable(mem, nbytes, cname);
    } else {			/* Add another chunk. */
	chunk_t *cp =
	    alloc_acquire_chunk(imem, (ulong) imem->chunk_size, true, "chunk");

	if (cp == 0)
	    return 0;
	alloc_close_chunk(imem);
	imem->pcc = cp;
	imem->cc = *imem->pcc;
	gs_alloc_fill(imem->cc.cbase, gs_alloc_fill_free,
		      imem->cc.climit - imem->cc.cbase);
	goto top;
    }
}
private byte *
i_alloc_string_immovable(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    byte *str;
    /* Give it a chunk all its own. */
    uint asize = string_chunk_space(nbytes) + sizeof(chunk_head_t);
    chunk_t *cp = alloc_acquire_chunk(imem, (ulong) asize, true,
				      "large string chunk");

    if (cp == 0)
	return 0;
    str = cp->ctop = cp->climit - nbytes;
    if_debug4('a', "[a%d|+>L]%s(%u) = 0x%lx\n",
	      alloc_trace_space(imem), client_name_string(cname), nbytes,
	      (ulong) str);
    gs_alloc_fill(str, gs_alloc_fill_alloc, nbytes);
    return str;
}
private byte *
i_resize_string(gs_memory_t * mem, byte * data, uint old_num, uint new_num,
		client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    byte *ptr;

    if (old_num == new_num)	/* same size returns the same string */
        return data;
    if (data == imem->cc.ctop &&	/* bottom-most string */
	(new_num < old_num ||
	 imem->cc.ctop - imem->cc.cbot > new_num - old_num)
	) {			/* Resize in place. */
	ptr = data + old_num - new_num;
	if_debug6('A', "[a%d:%c> ]%s(%u->%u) 0x%lx\n",
		  alloc_trace_space(imem),
		  (new_num > old_num ? '>' : '<'),
		  client_name_string(cname), old_num, new_num,
		  (ulong) ptr);
	imem->cc.ctop = ptr;
	memmove(ptr, data, min(old_num, new_num));
#ifdef DEBUG
	if (new_num > old_num)
	    gs_alloc_fill(ptr + old_num, gs_alloc_fill_alloc,
			  new_num - old_num);
	else
	    gs_alloc_fill(data, gs_alloc_fill_free, old_num - new_num);
#endif
    } else
	if (new_num < old_num) {
	    /* trim the string and create a free space hole */
	    ptr = data;
	    imem->lost.strings += old_num - new_num;
	    gs_alloc_fill(data + new_num, gs_alloc_fill_free,
			  old_num - new_num);
	    if_debug5('A', "[a%d:<> ]%s(%u->%u) 0x%lx\n",
		      alloc_trace_space(imem), client_name_string(cname),
		      old_num, new_num, (ulong)ptr);
        } else {			/* Punt. */
	    ptr = gs_alloc_string(mem, new_num, cname);
	    if (ptr == 0)
		return 0;
	    memcpy(ptr, data, min(old_num, new_num));
	    gs_free_string(mem, data, old_num, cname);
	}
    return ptr;
}

private void
i_free_string(gs_memory_t * mem, byte * data, uint nbytes,
	      client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    if (data == imem->cc.ctop) {
	if_debug4('A', "[a%d:-> ]%s(%u) 0x%lx\n",
		  alloc_trace_space(imem), client_name_string(cname), nbytes,
		  (ulong) data);
	imem->cc.ctop += nbytes;
    } else {
	if_debug4('A', "[a%d:->#]%s(%u) 0x%lx\n",
		  alloc_trace_space(imem), client_name_string(cname), nbytes,
		  (ulong) data);
	imem->lost.strings += nbytes;
    }
    gs_alloc_fill(data, gs_alloc_fill_free, nbytes);
}

private gs_memory_t *
i_stable(gs_memory_t *mem)
{
    return mem->stable_memory;
}

private void
i_status(gs_memory_t * mem, gs_memory_status_t * pstat)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    ulong unused = imem->lost.refs + imem->lost.strings;
    ulong inner = 0;

    alloc_close_chunk(imem);
    /* Add up unallocated space within each chunk. */
    /* Also keep track of space allocated to inner chunks, */
    /* which are included in previous_status.allocated. */
    {
	const chunk_t *cp = imem->cfirst;

	while (cp != 0) {
	    unused += cp->ctop - cp->cbot;
	    if (cp->outer)
		inner += cp->cend - (byte *) cp->chead;
	    cp = cp->cnext;
	}
    }
    unused += compute_free_objects(imem);
    pstat->used = imem->allocated + inner - unused +
	imem->previous_status.used;
    pstat->allocated = imem->allocated +
	imem->previous_status.allocated;
}

private void
i_enable_free(gs_memory_t * mem, bool enable)
{
    if (enable)
	mem->procs.free_object = i_free_object,
	    mem->procs.free_string = i_free_string;
    else
	mem->procs.free_object = gs_ignore_free_object,
	    mem->procs.free_string = gs_ignore_free_string;
}

/* ------ Internal procedures ------ */

/* Compute the amount of free object space by scanning free lists. */
private ulong
compute_free_objects(gs_ref_memory_t * mem)
{
    ulong unused = mem->lost.objects;
    int i;

    /* Add up space on free lists. */
    for (i = 0; i < num_freelists; i++) {
	const obj_header_t *pfree;

	for (pfree = mem->freelists[i]; pfree != 0;
	     pfree = *(const obj_header_t * const *)pfree
	    )
	    unused += obj_align_round(pfree[-1].o_size);
    }
    return unused;
}
    
/* Allocate an object from the large-block freelist. */
private obj_header_t *	/* rets obj if allocated, else 0 */
large_freelist_alloc(gs_ref_memory_t *mem, uint size)
{
    /* Scan large object freelist. We'll grab an object up to 1/8 bigger */
    /* right away, else use best fit of entire scan. */
    uint aligned_size = obj_align_round(size);
    uint aligned_min_size = aligned_size + sizeof(obj_header_t);
    uint aligned_max_size =
	aligned_min_size + obj_align_round(aligned_min_size / 8);
    obj_header_t *best_fit = 0;
    obj_header_t **best_fit_prev = NULL; /* Initialize against indeteminizm. */
    uint best_fit_size = max_uint;
    obj_header_t *pfree;
    obj_header_t **ppfprev = &mem->freelists[LARGE_FREELIST_INDEX];
    uint largest_size = 0;

    if (aligned_size > mem->largest_free_size)
	return 0;		/* definitely no block large enough */

    while ((pfree = *ppfprev) != 0) {
	uint free_size = obj_align_round(pfree[-1].o_size);

        if (free_size == aligned_size ||
	    (free_size >= aligned_min_size && free_size < best_fit_size)
	    ) {
	    best_fit = pfree;
	    best_fit_prev = ppfprev;
	    best_fit_size = pfree[-1].o_size;
	    if (best_fit_size <= aligned_max_size)
		break;	/* good enough fit to spare scan of entire list */
	}
	ppfprev = (obj_header_t **) pfree;
	if (free_size > largest_size)
	    largest_size = free_size;
    }
    if (best_fit == 0) {
	/*
	 * No single free chunk is large enough, but since we scanned the
	 * entire list, we now have an accurate updated value for
	 * largest_free_size.
	 */
	mem->largest_free_size = largest_size;
	return 0;
    }

    /* Remove from freelist & return excess memory to free */
    *best_fit_prev = *(obj_header_t **)best_fit;
    trim_obj(mem, best_fit, aligned_size, (chunk_t *)0);

    /* Pre-init block header; o_alone & o_type are already init'd */
    best_fit[-1].o_size = size;

    return best_fit;
}

/* Allocate an object.  This handles all but the fastest, simplest case. */
private obj_header_t *
alloc_obj(gs_ref_memory_t *mem, ulong lsize, gs_memory_type_ptr_t pstype,
	  alloc_flags_t flags, client_name_t cname)
{
    obj_header_t *ptr;

    if (lsize >= mem->large_size || (flags & ALLOC_IMMOVABLE)) {
	/*
	 * Give the object a chunk all its own.  Note that this case does
	 * not occur if is_controlled is true.
	 */
	ulong asize =
	    ((lsize + obj_align_mask) & -obj_align_mod) +
	    sizeof(obj_header_t);
	chunk_t *cp =
	    alloc_acquire_chunk(mem, asize + sizeof(chunk_head_t), false,
				"large object chunk");

	if (
#if arch_sizeof_long > arch_sizeof_int
	    asize > max_uint
#else
	    asize < lsize
#endif
	    )
	    return 0;
	if (cp == 0)
	    return 0;
	ptr = (obj_header_t *) cp->cbot;
	cp->cbot += asize;
	ptr->o_alone = 1;
	ptr->o_size = lsize;
    } else {
	/*
	 * Cycle through the chunks at the current save level, starting
	 * with the currently open one.
	 */
	chunk_t *cp_orig = mem->pcc;
	uint asize = obj_size_round((uint) lsize);
	bool allocate_success = false;

	if (lsize > max_freelist_size && (flags & ALLOC_DIRECT)) {
	    /* We haven't checked the large block freelist yet. */
	    if ((ptr = large_freelist_alloc(mem, lsize)) != 0) {
		--ptr;			/* must point to header */
		goto done;
	    }
	}

	if (cp_orig == 0) {
	    /* Open an arbitrary chunk. */
	    cp_orig = mem->pcc = mem->cfirst;
	    alloc_open_chunk(mem);
	}

#define CAN_ALLOC_AT_END(cp)\
  ((cp)->ctop - (byte *) (ptr = (obj_header_t *) (cp)->cbot)\
   > asize + sizeof(obj_header_t))

	do {
	    if (CAN_ALLOC_AT_END(&mem->cc)) {
		allocate_success = true;
		break;
	    } else if (mem->is_controlled) {
		/* Try consolidating free space. */
		gs_consolidate_free((gs_memory_t *)mem);
		if (CAN_ALLOC_AT_END(&mem->cc)) {
		    allocate_success = true;
		    break;
		}
	    }
	    /* No luck, go on to the next chunk. */
	    {
		chunk_t *cp = mem->cc.cnext;

		alloc_close_chunk(mem);
		if (cp == 0)
		    cp = mem->cfirst;
		mem->pcc = cp;
		alloc_open_chunk(mem);
	    }
	} while (mem->pcc != cp_orig);

#ifdef CONSOLIDATE_BEFORE_ADDING_CHUNK
	if (!allocate_success) {
	    /*
	     * Try consolidating free space before giving up.
	     * It's not clear this is a good idea, since it requires quite
	     * a lot of computation and doesn't seem to improve things much.
	     */
	    if (!mem->is_controlled) { /* already did this if controlled */
		chunk_t *cp = cp_orig;

		alloc_close_chunk(mem);
		do {
		    consolidate_chunk_free(cp, mem);
		    if (CAN_ALLOC_AT_END(cp)) {
			mem->pcc = cp;
			alloc_open_chunk(mem);
			allocate_success = true;
			break;
		    }
		    if ((cp = cp->cnext) == 0)
			cp = mem->cfirst;
		} while (cp != cp_orig);
	    }
	}
#endif

#undef CAN_ALLOC_AT_END

	if (!allocate_success) {
	    /* Add another chunk. */
	    chunk_t *cp =
		alloc_add_chunk(mem, (ulong)mem->chunk_size, "chunk");
			
	    if (cp) {
		/* mem->pcc == cp, mem->cc == *mem->pcc. */
		ptr = (obj_header_t *)cp->cbot;
		allocate_success = true;
	    }
	}

	/*
	 * If no success, try to scavenge from low free memory. This is
	 * only enabled for controlled memory (currently only async
	 * renderer) because it's too much work to prevent it from
	 * examining outer save levels in the general case.
	 */
	if (allocate_success)
	    mem->cc.cbot = (byte *) ptr + asize;
	else if (!mem->is_controlled ||
		 (ptr = scavenge_low_free(mem, (uint)lsize)) == 0)
	    return 0;	/* allocation failed */
	ptr->o_alone = 0;
	ptr->o_size = (uint) lsize;
    }
done:
    ptr->o_type = pstype;
#   if IGC_PTR_STABILITY_CHECK
	ptr->d.o.space_id = mem->space_id;
#   endif
    ptr++;
    gs_alloc_fill(ptr, gs_alloc_fill_alloc, lsize);
    return ptr;
}

/*
 * Consolidate free objects contiguous to free space at cbot onto the cbot
 * area. Also keep track of end of highest internal free object
 * (int_freed_top).
 */
private void
consolidate_chunk_free(chunk_t *cp, gs_ref_memory_t *mem)
{
    obj_header_t *begin_free = 0;

    cp->int_freed_top = cp->cbase;	/* below all objects in chunk */
    SCAN_CHUNK_OBJECTS(cp)
    DO_ALL
	if (pre->o_type == &st_free) {
	    if (begin_free == 0)
		begin_free = pre;
	} else {
	    if (begin_free)
		cp->int_freed_top = (byte *)pre; /* first byte following internal free */
	    begin_free = 0;
        }
    END_OBJECTS_SCAN
    if (begin_free) {
	/* We found free objects at the top of the object area. */
	/* Remove the free objects from the freelists. */
	remove_range_from_freelist(mem, begin_free, cp->cbot);
	if_debug4('a', "[a]resetting chunk 0x%lx cbot from 0x%lx to 0x%lx (%lu free)\n",
		  (ulong) cp, (ulong) cp->cbot, (ulong) begin_free,
		  (ulong) ((byte *) cp->cbot - (byte *) begin_free));
	cp->cbot = (byte *) begin_free;
    }
}

/* Consolidate free objects. */
void
ialloc_consolidate_free(gs_ref_memory_t *mem)
{
    chunk_t *cp;
    chunk_t *cprev;

    alloc_close_chunk(mem);

    /* Visit chunks in reverse order to encourage LIFO behavior. */
    for (cp = mem->clast; cp != 0; cp = cprev) {
	cprev = cp->cprev;
	consolidate_chunk_free(cp, mem);
	if (cp->cbot == cp->cbase && cp->ctop == cp->climit) {
	    /* The entire chunk is free. */
	    chunk_t *cnext = cp->cnext;

	    if (!mem->is_controlled) {
		alloc_free_chunk(cp, mem);
		if (mem->pcc == cp)
		    mem->pcc =
			(cnext == 0 ? cprev : cprev == 0 ? cnext :
			 cprev->cbot - cprev->ctop >
			 cnext->cbot - cnext->ctop ? cprev :
			 cnext);
	    }
	}
    }
    alloc_open_chunk(mem);
}
private void
i_consolidate_free(gs_memory_t *mem)
{
    ialloc_consolidate_free((gs_ref_memory_t *)mem);
}

/* try to free-up given amount of space from freespace below chunk base */
private obj_header_t *	/* returns uninitialized object hdr, NULL if none found */
scavenge_low_free(gs_ref_memory_t *mem, unsigned request_size)
{
    /* find 1st range of memory that can be glued back together to fill request */
    obj_header_t *found_pre = 0;

    /* Visit chunks in forward order */
    obj_header_t *begin_free = 0;
    uint found_free;
    uint request_size_rounded = obj_size_round(request_size);
    uint need_free = request_size_rounded + sizeof(obj_header_t);    /* room for GC's dummy hdr */
    chunk_t *cp;

    for (cp = mem->cfirst; cp != 0; cp = cp->cnext) {
	begin_free = 0;
	found_free = 0;
	SCAN_CHUNK_OBJECTS(cp)
	DO_ALL
	    if (pre->o_type == &st_free) {
	    	if (begin_free == 0) {
	    	    found_free = 0;
	    	    begin_free = pre;
	    	}
	    	found_free += pre_obj_rounded_size(pre);
	    	if (begin_free != 0 && found_free >= need_free)
	    	    break;
	    } else
	    	begin_free = 0;
	END_OBJECTS_SCAN_NO_ABORT

	/* Found sufficient range of empty memory */
	if (begin_free != 0 && found_free >= need_free) {

	    /* Fish found pieces out of various freelists */
	    remove_range_from_freelist(mem, (char*)begin_free,
				       (char*)begin_free + found_free);

	    /* Prepare found object */
	    found_pre = begin_free;
	    found_pre->o_type = &st_free;  /* don't confuse GC if gets lost */
	    found_pre->o_size = found_free - sizeof(obj_header_t);

	    /* Chop off excess tail piece & toss it back into free pool */
	    trim_obj(mem, found_pre + 1, request_size, cp);
     	}
    }
    return found_pre;
}

/* Remove range of memory from a mem's freelists */
private void
remove_range_from_freelist(gs_ref_memory_t *mem, void* bottom, void* top)
{
    int num_free[num_freelists];
    int smallest = num_freelists, largest = -1;
    const obj_header_t *cur;
    uint size;
    int i;
    uint removed = 0;

    /*
     * Scan from bottom to top, a range containing only free objects,
     * counting the number of objects of each size.
     */

    for (cur = bottom; cur != top;
	 cur = (const obj_header_t *)
	     ((const byte *)cur + obj_size_round(size))
	) {
	size = cur->o_size;
	i = (size > max_freelist_size ? LARGE_FREELIST_INDEX :
	     (size + obj_align_mask) >> log2_obj_align_mod);
	if (i < smallest) {
	    /*
	     * 0-length free blocks aren't kept on any list, because
	     * they don't have room for a pointer.
	     */
	    if (i == 0)
		continue;
	    if (smallest < num_freelists)
		memset(&num_free[i], 0, (smallest - i) * sizeof(int));
	    else
		num_free[i] = 0;
	    smallest = i;
	}
	if (i > largest) {
	    if (largest >= 0)
		memset(&num_free[largest + 1], 0, (i - largest) * sizeof(int));
	    largest = i;
	}
	num_free[i]++;
    }

    /*
     * Remove free objects from the freelists, adjusting lost.objects by
     * subtracting the size of the region being processed minus the amount
     * of space reclaimed.
     */

    for (i = smallest; i <= largest; i++) {
	int count = num_free[i];
        obj_header_t *pfree;
	obj_header_t **ppfprev;

	if (!count)
	    continue;
	ppfprev = &mem->freelists[i];
	for (;;) {
	    pfree = *ppfprev;
	    if (PTR_GE(pfree, bottom) && PTR_LT(pfree, top)) {
		/* We're removing an object. */
		*ppfprev = *(obj_header_t **) pfree;
		removed += obj_align_round(pfree[-1].o_size);
		if (!--count)
		    break;
	    } else
		ppfprev = (obj_header_t **) pfree;
	}
    }
    mem->lost.objects -= (char*)top - (char*)bottom - removed;
}

/* Trim a memory object down to a given size */
private void
trim_obj(gs_ref_memory_t *mem, obj_header_t *obj, uint size, chunk_t *cp)
/* Obj must have rounded size == req'd size, or have enough room for */
/* trailing dummy obj_header */
{
    uint rounded_size = obj_align_round(size);
    obj_header_t *pre_obj = obj - 1;
    obj_header_t *excess_pre = (obj_header_t*)((char*)obj + rounded_size);
    uint old_rounded_size = obj_align_round(pre_obj->o_size);
    uint excess_size = old_rounded_size - rounded_size - sizeof(obj_header_t);

    /* trim object's size to desired */
    pre_obj->o_size = size;
    if (old_rounded_size == rounded_size)
	return;	/* nothing more to do here */
    /*
     * If the object is alone in its chunk, move cbot to point to the end
     * of the object.
     */
    if (pre_obj->o_alone) {
	if (!cp) {
	    mem->cfreed.memory = mem;
	    if (chunk_locate(obj, &mem->cfreed)) {
		cp = mem->cfreed.cp;
	    }
	}
	if (cp) {
#ifdef DEBUG
	    if (cp->cbot != (byte *)obj + old_rounded_size) {
		lprintf3("resizing 0x%lx, old size %u, new size %u, cbot wrong!\n",
			 (ulong)obj, old_rounded_size, size);
		/* gs_abort */
	    } else
#endif
		{
		    cp->cbot = (byte *)excess_pre;
		    return;
		}
	}
	/*
	 * Something very weird is going on.  This probably shouldn't
	 * ever happen, but if it does....
	 */
	pre_obj->o_alone = 0;
    }
    /* make excess into free obj */
    excess_pre->o_type = &st_free;  /* don't confuse GC */
    excess_pre->o_size = excess_size;
    excess_pre->o_alone = 0;
    if (excess_size >= obj_align_mod) {
	/* Put excess object on a freelist */
	obj_header_t **pfl;

	if ((byte *)excess_pre >= mem->cc.int_freed_top)
	    mem->cc.int_freed_top = (byte *)excess_pre + excess_size;
	if (excess_size <= max_freelist_size)
	    pfl = &mem->freelists[(excess_size + obj_align_mask) >>
				 log2_obj_align_mod];
	else {
	    uint rounded_size = obj_align_round(excess_size);

	    pfl = &mem->freelists[LARGE_FREELIST_INDEX];
	    if (rounded_size > mem->largest_free_size)
		mem->largest_free_size = rounded_size;
	}
	*(obj_header_t **) (excess_pre + 1) = *pfl;
	*pfl = excess_pre + 1;
	mem->cfreed.memory = mem;
    } else {
	/* excess piece will be "lost" memory */
	mem->lost.objects += excess_size + sizeof(obj_header_t);
    }
}    

/* ================ Roots ================ */

/* Register a root. */
private int
i_register_root(gs_memory_t * mem, gs_gc_root_t * rp, gs_ptr_type_t ptype,
		void **up, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;

    if (rp == NULL) {
	rp = gs_raw_alloc_struct_immovable(imem->non_gc_memory, &st_gc_root_t,
					   "i_register_root");
	if (rp == 0)
	    return_error(gs_error_VMerror);
	rp->free_on_unregister = true;
    } else
	rp->free_on_unregister = false;
    if_debug3('8', "[8]register root(%s) 0x%lx -> 0x%lx\n",
	      client_name_string(cname), (ulong)rp, (ulong)up);
    rp->ptype = ptype;
    rp->p = up;
    rp->next = imem->roots;
    imem->roots = rp;
    return 0;
}

/* Unregister a root. */
private void
i_unregister_root(gs_memory_t * mem, gs_gc_root_t * rp, client_name_t cname)
{
    gs_ref_memory_t * const imem = (gs_ref_memory_t *)mem;
    gs_gc_root_t **rpp = &imem->roots;

    if_debug2('8', "[8]unregister root(%s) 0x%lx\n",
	      client_name_string(cname), (ulong) rp);
    while (*rpp != rp)
	rpp = &(*rpp)->next;
    *rpp = (*rpp)->next;
    if (rp->free_on_unregister)
	gs_free_object(imem->non_gc_memory, rp, "i_unregister_root");
}

/* ================ Chunks ================ */

public_st_chunk();

/* Insert a chunk in the chain.  This is exported for the GC and for */
/* the forget_save operation. */
void
alloc_link_chunk(chunk_t * cp, gs_ref_memory_t * imem)
{
    byte *cdata = cp->cbase;
    chunk_t *icp;
    chunk_t *prev;

    /*
     * Allocators tend to allocate in either ascending or descending
     * address order.  The loop will handle the latter well; check for
     * the former first.
     */
    if (imem->clast && PTR_GE(cdata, imem->clast->ctop))
	icp = 0;
    else
	for (icp = imem->cfirst; icp != 0 && PTR_GE(cdata, icp->ctop);
	     icp = icp->cnext
	    );
    cp->cnext = icp;
    if (icp == 0) {		/* add at end of chain */
	prev = imem->clast;
	imem->clast = cp;
    } else {			/* insert before icp */
	prev = icp->cprev;
	icp->cprev = cp;
    }
    cp->cprev = prev;
    if (prev == 0)
	imem->cfirst = cp;
    else
	prev->cnext = cp;
    if (imem->pcc != 0) {
	imem->cc.cnext = imem->pcc->cnext;
	imem->cc.cprev = imem->pcc->cprev;
    }
}

/* Add a chunk for ordinary allocation. */
private chunk_t *
alloc_add_chunk(gs_ref_memory_t * mem, ulong csize, client_name_t cname)
{
    chunk_t *cp = alloc_acquire_chunk(mem, csize, true, cname);

    if (cp) {
	alloc_close_chunk(mem);
	mem->pcc = cp;
	mem->cc = *mem->pcc;
	gs_alloc_fill(mem->cc.cbase, gs_alloc_fill_free,
		      mem->cc.climit - mem->cc.cbase);
    }
    return cp;
}

/* Acquire a chunk.  If we would exceed MaxLocalVM (if relevant), */
/* or if we would exceed the VMThreshold and psignal is NULL, */
/* return 0; if we would exceed the VMThreshold but psignal is valid, */
/* just set the signal and return successfully. */
private chunk_t *
alloc_acquire_chunk(gs_ref_memory_t * mem, ulong csize, bool has_strings,
		    client_name_t cname)
{
    gs_memory_t *parent = mem->non_gc_memory;
    chunk_t *cp;
    byte *cdata;

#if arch_sizeof_long > arch_sizeof_int
    /* If csize is larger than max_uint, punt. */
    if (csize != (uint) csize)
	return 0;
#endif
    cp = gs_raw_alloc_struct_immovable(parent, &st_chunk, cname);

    if( mem->gc_status.psignal != 0) {  
	/* we have a garbage collector */
	if ((ulong) (mem->allocated) >= mem->limit) {
	    mem->gc_status.requested += csize;
	    if (mem->limit >= mem->gc_status.max_vm) {
		gs_free_object(parent, cp, cname);
		return 0;
	    }
	    if_debug4('0', "[0]signaling space=%d, allocated=%ld, limit=%ld, requested=%ld\n",
		      mem->space, (long)mem->allocated,
		      (long)mem->limit, (long)mem->gc_status.requested);
	    *mem->gc_status.psignal = mem->gc_status.signal_value;
	}
    }
    cdata = gs_alloc_bytes_immovable(parent, csize, cname);
    if (cp == 0 || cdata == 0) {
	gs_free_object(parent, cdata, cname);
	gs_free_object(parent, cp, cname);
	mem->gc_status.requested = csize;
	return 0;
    }
    alloc_init_chunk(cp, cdata, cdata + csize, has_strings, (chunk_t *) 0);
    alloc_link_chunk(cp, mem);
    mem->allocated += st_chunk.ssize + csize;
    return cp;
}

/* Initialize the pointers in a chunk.  This is exported for save/restore. */
/* The bottom pointer must be aligned, but the top pointer need not */
/* be aligned. */
void
alloc_init_chunk(chunk_t * cp, byte * bot, byte * top, bool has_strings,
		 chunk_t * outer)
{
    byte *cdata = bot;

    if (outer != 0)
	outer->inner_count++;
    cp->chead = (chunk_head_t *) cdata;
    cdata += sizeof(chunk_head_t);
    cp->cbot = cp->cbase = cp->int_freed_top = cdata;
    cp->cend = top;
    cp->rcur = 0;
    cp->rtop = 0;
    cp->outer = outer;
    cp->inner_count = 0;
    cp->has_refs = false;
    cp->sbase = cdata;
    if (has_strings && top - cdata >= string_space_quantum + sizeof(long) - 1) {
	/*
	 * We allocate a large enough string marking and reloc table
	 * to cover the entire chunk.
	 */
	uint nquanta = string_space_quanta(top - cdata);

	cp->climit = cdata + nquanta * string_data_quantum;
	cp->smark = cp->climit;
	cp->smark_size = string_quanta_mark_size(nquanta);
	cp->sreloc =
	    (string_reloc_offset *) (cp->smark + cp->smark_size);
	cp->sfree1 = (uint *) cp->sreloc;
    } else {
	/* No strings, don't need the string GC tables. */
	cp->climit = cp->cend;
	cp->sfree1 = 0;
	cp->smark = 0;
	cp->smark_size = 0;
	cp->sreloc = 0;
    }
    cp->ctop = cp->climit;
    alloc_init_free_strings(cp);
}

/* Initialize the string freelists in a chunk. */
void
alloc_init_free_strings(chunk_t * cp)
{
    if (cp->sfree1)
	memset(cp->sfree1, 0, STRING_FREELIST_SPACE(cp));
    cp->sfree = 0;
}

/* Close up the current chunk. */
/* This is exported for save/restore and the GC. */
void
alloc_close_chunk(gs_ref_memory_t * mem)
{
    if (mem->pcc != 0) {
	*mem->pcc = mem->cc;
#ifdef DEBUG
	if (gs_debug_c('a')) {
	    dlprintf1("[a%d]", alloc_trace_space(mem));
	    dprintf_chunk("closing chunk", mem->pcc);
	}
#endif
    }
}

/* Reopen the current chunk after a GC or restore. */
void
alloc_open_chunk(gs_ref_memory_t * mem)
{
    if (mem->pcc != 0) {
	mem->cc = *mem->pcc;
#ifdef DEBUG
	if (gs_debug_c('a')) {
	    dlprintf1("[a%d]", alloc_trace_space(mem));
	    dprintf_chunk("opening chunk", mem->pcc);
	}
#endif
    }
}

/* Remove a chunk from the chain.  This is exported for the GC. */
void
alloc_unlink_chunk(chunk_t * cp, gs_ref_memory_t * mem)
{
#ifdef DEBUG
    if (gs_alloc_debug) {	/* Check to make sure this chunk belongs to this allocator. */
	const chunk_t *ap = mem->cfirst;

	while (ap != 0 && ap != cp)
	    ap = ap->cnext;
	if (ap != cp) {
	    lprintf2("unlink_chunk 0x%lx not owned by memory 0x%lx!\n",
		     (ulong) cp, (ulong) mem);
	    return;		/*gs_abort(); */
	}
    }
#endif
    if (cp->cprev == 0)
	mem->cfirst = cp->cnext;
    else
	cp->cprev->cnext = cp->cnext;
    if (cp->cnext == 0)
	mem->clast = cp->cprev;
    else
	cp->cnext->cprev = cp->cprev;
    if (mem->pcc != 0) {
	mem->cc.cnext = mem->pcc->cnext;
	mem->cc.cprev = mem->pcc->cprev;
	if (mem->pcc == cp) {
	    mem->pcc = 0;
	    mem->cc.cbot = mem->cc.ctop = 0;
	}
    }
}

/*
 * Free a chunk.  This is exported for the GC.  Since we eventually use
 * this to free the chunk containing the allocator itself, we must be
 * careful not to reference anything in the allocator after freeing the
 * chunk data.
 */
void
alloc_free_chunk(chunk_t * cp, gs_ref_memory_t * mem)
{
    gs_memory_t *parent = mem->non_gc_memory;
    byte *cdata = (byte *)cp->chead;
    ulong csize = (byte *)cp->cend - cdata;

    alloc_unlink_chunk(cp, mem);
    mem->allocated -= st_chunk.ssize;
    if (mem->cfreed.cp == cp)
	mem->cfreed.cp = 0;
    if (cp->outer == 0) {
	mem->allocated -= csize;
	gs_free_object(parent, cdata, "alloc_free_chunk(data)");
    } else {
	cp->outer->inner_count--;
	gs_alloc_fill(cdata, gs_alloc_fill_free, csize);
    }
    gs_free_object(parent, cp, "alloc_free_chunk(chunk struct)");
}

/* Find the chunk for a pointer. */
/* Note that this only searches the current save level. */
/* Since a given save level can't contain both a chunk and an inner chunk */
/* of that chunk, we can stop when is_within_chunk succeeds, and just test */
/* is_in_inner_chunk then. */
bool
chunk_locate_ptr(const void *ptr, chunk_locator_t * clp)
{
    register chunk_t *cp = clp->cp;

    if (cp == 0) {
	cp = clp->memory->cfirst;
	if (cp == 0)
	    return false;
	/* ptr is in the last chunk often enough to be worth checking for. */
	if (PTR_GE(ptr, clp->memory->clast->cbase))
	    cp = clp->memory->clast;
    }
    if (PTR_LT(ptr, cp->cbase)) {
	do {
	    cp = cp->cprev;
	    if (cp == 0)
		return false;
	}
	while (PTR_LT(ptr, cp->cbase));
	if (PTR_GE(ptr, cp->cend))
	    return false;
    } else {
	while (PTR_GE(ptr, cp->cend)) {
	    cp = cp->cnext;
	    if (cp == 0)
		return false;
	}
	if (PTR_LT(ptr, cp->cbase))
	    return false;
    }
    clp->cp = cp;
    return !ptr_is_in_inner_chunk(ptr, cp);
}

/* ------ Debugging ------ */

#ifdef DEBUG

#include "string_.h"

inline private bool
obj_in_control_region(const void *obot, const void *otop,
		      const dump_control_t *pdc)
{
    return
	((pdc->bottom == NULL || PTR_GT(otop, pdc->bottom)) &&
	 (pdc->top == NULL || PTR_LT(obot, pdc->top)));
}

const dump_control_t dump_control_default =
{
    dump_do_default, NULL, NULL
};
const dump_control_t dump_control_all =
{
    dump_do_strings | dump_do_type_addresses | dump_do_pointers |
    dump_do_pointed_strings | dump_do_contents, NULL, NULL
};

/*
 * Internal procedure to dump a block of memory, in hex and optionally
 * also as characters.
 */
private void
debug_indent(int indent)
{
    int i;

    for (i = indent; i > 0; --i)
	dputc(' ');
}
private void
debug_dump_contents(const byte * bot, const byte * top, int indent,
		    bool as_chars)
{
    const byte *block;

#define block_size 16

    if (bot >= top)
	return;
    for (block = bot - ((bot - (byte *) 0) & (block_size - 1));
	 block < top; block += block_size
	) {
	int i;
	char label[12];

	/* Check for repeated blocks. */
	if (block >= bot + block_size &&
	    block <= top - (block_size * 2) &&
	    !memcmp(block, block - block_size, block_size) &&
	    !memcmp(block, block + block_size, block_size)
	    ) {
	    if (block < bot + block_size * 2 ||
		memcmp(block, block - block_size * 2, block_size)
		) {
		debug_indent(indent);
		dputs("  ...\n");
	    }
	    continue;
	}
	sprintf(label, "0x%lx:", (ulong) block);
	debug_indent(indent);
	dputs(label);
	for (i = 0; i < block_size; ++i) {
	    const char *sepr = ((i & 3) == 0 && i != 0 ? "  " : " ");

	    dputs(sepr);
	    if (block + i >= bot && block + i < top)
		dprintf1("%02x", block[i]);
	    else
		dputs("  ");
	}
	dputc('\n');
	if (as_chars) {
	    debug_indent(indent + strlen(label));
	    for (i = 0; i < block_size; ++i) {
		byte ch;

		if ((i & 3) == 0 && i != 0)
		    dputc(' ');
		if (block + i >= bot && block + i < top &&
		    (ch = block[i]) >= 32 && ch <= 126
		    )
		    dprintf1("  %c", ch);
		else
		    dputs("   ");
	    }
	    dputc('\n');
	}
    }
#undef block_size
}

/* Print one object with the given options. */
/* Relevant options: type_addresses, no_types, pointers, pointed_strings, */
/* contents. */
void
debug_print_object(const gs_memory_t *mem, const void *obj, const dump_control_t * control)
{
    const obj_header_t *pre = ((const obj_header_t *)obj) - 1;
    ulong size = pre_obj_contents_size(pre);
    const gs_memory_struct_type_t *type = pre->o_type;
    dump_options_t options = control->options;

    dprintf3("  pre=0x%lx(obj=0x%lx) size=%lu", (ulong) pre, (ulong) obj,
	     size);
    switch (options & (dump_do_type_addresses | dump_do_no_types)) {
	case dump_do_type_addresses + dump_do_no_types:	/* addresses only */
	    dprintf1(" type=0x%lx", (ulong) type);
	    break;
	case dump_do_type_addresses:	/* addresses & names */
	    dprintf2(" type=%s(0x%lx)", struct_type_name_string(type),
		     (ulong) type);
	    break;
	case 0:		/* names only */
	    dprintf1(" type=%s", struct_type_name_string(type));
	case dump_do_no_types:	/* nothing */
	    ;
    }
    if (options & dump_do_marks) {
	dprintf2(" smark/back=%u (0x%x)", pre->o_smark, pre->o_smark);
    }
    dputc('\n');
    if (type == &st_free)
	return;
    if (options & dump_do_pointers) {
	struct_proc_enum_ptrs((*proc)) = type->enum_ptrs;
	uint index = 0;
	enum_ptr_t eptr;
	gs_ptr_type_t ptype;

	if (proc != gs_no_struct_enum_ptrs)
	    for (; (ptype = (*proc)(mem, pre + 1, size, index, &eptr, type, NULL)) != 0;
		 ++index
		) {
		const void *ptr = eptr.ptr;

		dprintf1("    ptr %u: ", index);
		if (ptype == ptr_string_type || ptype == ptr_const_string_type) {
		    const gs_const_string *str = (const gs_const_string *)ptr;

		    dprintf2("0x%lx(%u)", (ulong) str->data, str->size);
		    if (options & dump_do_pointed_strings) {
			dputs(" =>\n");
			debug_dump_contents(str->data, str->data + str->size, 6,
					    true);
		    } else {
			dputc('\n');
		    }
		} else {
		    dprintf1((PTR_BETWEEN(ptr, obj, (const byte *)obj + size) ?
			      "(0x%lx)\n" : "0x%lx\n"), (ulong) ptr);
		}
	    }
    }
    if (options & dump_do_contents) {
	debug_dump_contents((const byte *)obj, (const byte *)obj + size,
			    0, false);
    }
}

/* Print the contents of a chunk with the given options. */
/* Relevant options: all. */
void
debug_dump_chunk(const gs_memory_t *mem, const chunk_t * cp, const dump_control_t * control)
{
    dprintf1("chunk at 0x%lx:\n", (ulong) cp);
    dprintf3("   chead=0x%lx  cbase=0x%lx sbase=0x%lx\n",
	     (ulong) cp->chead, (ulong) cp->cbase, (ulong) cp->sbase);
    dprintf3("    rcur=0x%lx   rtop=0x%lx  cbot=0x%lx\n",
	     (ulong) cp->rcur, (ulong) cp->rtop, (ulong) cp->cbot);
    dprintf4("    ctop=0x%lx climit=0x%lx smark=0x%lx, size=%u\n",
	     (ulong) cp->ctop, (ulong) cp->climit, (ulong) cp->smark,
	     cp->smark_size);
    dprintf2("  sreloc=0x%lx   cend=0x%lx\n",
	     (ulong) cp->sreloc, (ulong) cp->cend);
    dprintf5("cprev=0x%lx cnext=0x%lx outer=0x%lx inner_count=%u has_refs=%s\n",
	     (ulong) cp->cprev, (ulong) cp->cnext, (ulong) cp->outer,
	     cp->inner_count, (cp->has_refs ? "true" : "false"));

    dprintf2("  sfree1=0x%lx   sfree=0x%x\n",
	     (ulong) cp->sfree1, cp->sfree);
    if (control->options & dump_do_strings) {
	debug_dump_contents((control->bottom == 0 ? cp->ctop :
			     max(control->bottom, cp->ctop)),
			    (control->top == 0 ? cp->climit :
			     min(control->top, cp->climit)),
			    0, true);
    }
    SCAN_CHUNK_OBJECTS(cp)
	DO_ALL
	if (obj_in_control_region(pre + 1,
				  (const byte *)(pre + 1) + size,
				  control)
	)
	debug_print_object(mem, pre + 1, control);
    END_OBJECTS_SCAN_NO_ABORT
}
void 
debug_print_chunk(const gs_memory_t *mem, const chunk_t * cp)
{
    dump_control_t control;

    control = dump_control_default;
    debug_dump_chunk(mem, cp, &control);
}

/* Print the contents of all chunks managed by an allocator. */
/* Relevant options: all. */
void
debug_dump_memory(const gs_ref_memory_t * mem, const dump_control_t * control)
{
    const chunk_t *mcp;

    for (mcp = mem->cfirst; mcp != 0; mcp = mcp->cnext) {
	const chunk_t *cp = (mcp == mem->pcc ? &mem->cc : mcp);

	if (obj_in_control_region(cp->cbase, cp->cend, control))
	    debug_dump_chunk((const gs_memory_t *)mem, cp, control);
    }
}

/* Find all the objects that contain a given pointer. */
void
debug_find_pointers(const gs_ref_memory_t *mem, const void *target)
{
    dump_control_t control;
    const chunk_t *mcp;

    control.options = 0;
    for (mcp = mem->cfirst; mcp != 0; mcp = mcp->cnext) {
	const chunk_t *cp = (mcp == mem->pcc ? &mem->cc : mcp);

	SCAN_CHUNK_OBJECTS(cp);
	DO_ALL
	    struct_proc_enum_ptrs((*proc)) = pre->o_type->enum_ptrs;
	    uint index = 0;
	    enum_ptr_t eptr;

	    if (proc)		/* doesn't trace refs */
		for (; (*proc)((const gs_memory_t *)mem, pre + 1, size, index, 
			       &eptr, pre->o_type, NULL); 
		     ++index)
		    if (eptr.ptr == target) {
			dprintf1("Index %d in", index);
			debug_print_object((const gs_memory_t *)mem, pre + 1, &control);
		    }
	END_OBJECTS_SCAN_NO_ABORT
    }
}

#endif /* DEBUG */
