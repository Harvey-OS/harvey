/* Copyright (C) 1998, 2000, 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsmalloc.c,v 1.13 2004/08/18 22:24:47 stefan Exp $ */
/* C heap allocator */
#include "malloc_.h"
#include "gdebug.h"
#include "gserror.h"
#include "gserrors.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gsmdebug.h"
#include "gsstruct.h"		/* for st_bytes */
#include "gsmalloc.h"
#include "gsmemlok.h"		/* locking (multithreading) wrapper */
#include "gsmemret.h"		/* retrying wrapper */


/* ------ Heap allocator ------ */

/*
 * An implementation of Ghostscript's memory manager interface
 * that works directly with the C heap.  We keep track of all allocated
 * blocks so we can free them at cleanup time.
 */
/* Raw memory procedures */
private gs_memory_proc_alloc_bytes(gs_heap_alloc_bytes);
private gs_memory_proc_resize_object(gs_heap_resize_object);
private gs_memory_proc_free_object(gs_heap_free_object);
private gs_memory_proc_stable(gs_heap_stable);
private gs_memory_proc_status(gs_heap_status);
private gs_memory_proc_free_all(gs_heap_free_all);

/* Object memory procedures */
private gs_memory_proc_alloc_struct(gs_heap_alloc_struct);
private gs_memory_proc_alloc_byte_array(gs_heap_alloc_byte_array);
private gs_memory_proc_alloc_struct_array(gs_heap_alloc_struct_array);
private gs_memory_proc_object_size(gs_heap_object_size);
private gs_memory_proc_object_type(gs_heap_object_type);
private gs_memory_proc_alloc_string(gs_heap_alloc_string);
private gs_memory_proc_resize_string(gs_heap_resize_string);
private gs_memory_proc_free_string(gs_heap_free_string);
private gs_memory_proc_register_root(gs_heap_register_root);
private gs_memory_proc_unregister_root(gs_heap_unregister_root);
private gs_memory_proc_enable_free(gs_heap_enable_free);
private const gs_memory_procs_t gs_malloc_memory_procs =
{
    /* Raw memory procedures */
    gs_heap_alloc_bytes,
    gs_heap_resize_object,
    gs_heap_free_object,
    gs_heap_stable,
    gs_heap_status,
    gs_heap_free_all,
    gs_ignore_consolidate_free,
    /* Object memory procedures */
    gs_heap_alloc_bytes,
    gs_heap_alloc_struct,
    gs_heap_alloc_struct,
    gs_heap_alloc_byte_array,
    gs_heap_alloc_byte_array,
    gs_heap_alloc_struct_array,
    gs_heap_alloc_struct_array,
    gs_heap_object_size,
    gs_heap_object_type,
    gs_heap_alloc_string,
    gs_heap_alloc_string,
    gs_heap_resize_string,
    gs_heap_free_string,
    gs_heap_register_root,
    gs_heap_unregister_root,
    gs_heap_enable_free
};

/* We must make sure that malloc_blocks leave the block aligned. */
/*typedef struct gs_malloc_block_s gs_malloc_block_t; */
#define malloc_block_data\
	gs_malloc_block_t *next;\
	gs_malloc_block_t *prev;\
	uint size;\
	gs_memory_type_ptr_t type;\
	client_name_t cname
struct malloc_block_data_s {
    malloc_block_data;
};
struct gs_malloc_block_s {
    malloc_block_data;
/* ANSI C does not allow zero-size arrays, so we need the following */
/* unnecessary and wasteful workaround: */
#define _npad (-size_of(struct malloc_block_data_s) & (arch_align_memory_mod - 1))
    byte _pad[(_npad == 0 ? arch_align_memory_mod : _npad)];
#undef _npad
};

/* Initialize a malloc allocator. */
private long heap_available(void);
gs_malloc_memory_t *
gs_malloc_memory_init(void)
{
    gs_malloc_memory_t *mem =
	(gs_malloc_memory_t *)malloc(sizeof(gs_malloc_memory_t));

    mem->stable_memory = 0;	/* just for tidyness, never referenced */
    mem->procs = gs_malloc_memory_procs;
    mem->allocated = 0;
    mem->limit = max_long;
    mem->used = 0;
    mem->max_used = 0;
    mem->gs_lib_ctx = 0;
    mem->non_gc_memory = (gs_memory_t *)mem;

    return mem;
}
/*
 * Estimate the amount of available memory by probing with mallocs.
 * We may under-estimate by a lot, but that's better than winding up with
 * a seriously inflated address space.  This is quite a hack!
 */
#define max_malloc_probes 20
#define malloc_probe_size 64000
private long
heap_available()
{
    long avail = 0;
    void *probes[max_malloc_probes];
    uint n;

    for (n = 0; n < max_malloc_probes; n++) {
	if ((probes[n] = malloc(malloc_probe_size)) == 0)
	    break;
	if_debug2('a', "[a]heap_available probe[%d]=0x%lx\n",
		  n, (ulong) probes[n]);
	avail += malloc_probe_size;
    }
    while (n)
	free(probes[--n]);
    return avail;
}

/* Allocate various kinds of blocks. */
private byte *
gs_heap_alloc_bytes(gs_memory_t * mem, uint size, client_name_t cname)
{
    gs_malloc_memory_t *mmem = (gs_malloc_memory_t *) mem;
    byte *ptr = 0;

#ifdef DEBUG
    const char *msg;
    static const char *const ok_msg = "OK";

#  define set_msg(str) (msg = (str))
#else
#  define set_msg(str) DO_NOTHING
#endif

    if (size > mmem->limit - sizeof(gs_malloc_block_t)) {
	/* Definitely too large to allocate; also avoids overflow. */
	set_msg("exceeded limit");
    } else {
	uint added = size + sizeof(gs_malloc_block_t);

	if (mmem->limit - added < mmem->used)
	    set_msg("exceeded limit");
	else if ((ptr = (byte *) malloc(added)) == 0)
	    set_msg("failed");
	else {
	    gs_malloc_block_t *bp = (gs_malloc_block_t *) ptr;

	    /*
	     * We would like to check that malloc aligns blocks at least as
	     * strictly as the compiler (as defined by arch_align_memory_mod).
	     * However, Microsoft VC 6 does not satisfy this requirement.
	     * See gsmemory.h for more explanation.
	     */
	    set_msg(ok_msg);
	    if (mmem->allocated)
		mmem->allocated->prev = bp;
	    bp->next = mmem->allocated;
	    bp->prev = 0;
	    bp->size = size;
	    bp->type = &st_bytes;
	    bp->cname = cname;
	    mmem->allocated = bp;
	    ptr = (byte *) (bp + 1);
	    gs_alloc_fill(ptr, gs_alloc_fill_alloc, size);
	    mmem->used += size + sizeof(gs_malloc_block_t);
	    if (mmem->used > mmem->max_used)
		mmem->max_used = mmem->used;
	}
    }
#ifdef DEBUG
    if (gs_debug_c('a') || msg != ok_msg)
	dlprintf4("[a+]gs_malloc(%s)(%u) = 0x%lx: %s\n",
		  client_name_string(cname), size, (ulong) ptr, msg);
#endif
    return ptr;
#undef set_msg
}
private void *
gs_heap_alloc_struct(gs_memory_t * mem, gs_memory_type_ptr_t pstype,
		     client_name_t cname)
{
    void *ptr =
    gs_heap_alloc_bytes(mem, gs_struct_type_size(pstype), cname);

    if (ptr == 0)
	return 0;
    ((gs_malloc_block_t *) ptr)[-1].type = pstype;
    return ptr;
}
private byte *
gs_heap_alloc_byte_array(gs_memory_t * mem, uint num_elements, uint elt_size,
			 client_name_t cname)
{
    ulong lsize = (ulong) num_elements * elt_size;

    if (lsize != (uint) lsize)
	return 0;
    return gs_heap_alloc_bytes(mem, (uint) lsize, cname);
}
private void *
gs_heap_alloc_struct_array(gs_memory_t * mem, uint num_elements,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    void *ptr =
    gs_heap_alloc_byte_array(mem, num_elements,
			     gs_struct_type_size(pstype), cname);

    if (ptr == 0)
	return 0;
    ((gs_malloc_block_t *) ptr)[-1].type = pstype;
    return ptr;
}
private void *
gs_heap_resize_object(gs_memory_t * mem, void *obj, uint new_num_elements,
		      client_name_t cname)
{
    gs_malloc_memory_t *mmem = (gs_malloc_memory_t *) mem;
    gs_malloc_block_t *ptr = (gs_malloc_block_t *) obj - 1;
    gs_memory_type_ptr_t pstype = ptr->type;
    uint old_size = gs_object_size(mem, obj) + sizeof(gs_malloc_block_t);
    uint new_size =
	gs_struct_type_size(pstype) * new_num_elements +
	sizeof(gs_malloc_block_t);
    gs_malloc_block_t *new_ptr;

    if (new_size == old_size)
        return obj;
    new_ptr = (gs_malloc_block_t *) gs_realloc(ptr, old_size, new_size);
    if (new_ptr == 0)
	return 0;
    if (new_ptr->prev)
	new_ptr->prev->next = new_ptr;
    else
	mmem->allocated = new_ptr;
    if (new_ptr->next)
	new_ptr->next->prev = new_ptr;
    new_ptr->size = new_size - sizeof(gs_malloc_block_t);
    mmem->used -= old_size;
    mmem->used += new_size;
    if (new_size > old_size)
	gs_alloc_fill((byte *) new_ptr + old_size,
		      gs_alloc_fill_alloc, new_size - old_size);
    return new_ptr + 1;
}
private uint
gs_heap_object_size(gs_memory_t * mem, const void *ptr)
{
    return ((const gs_malloc_block_t *)ptr)[-1].size;
}
private gs_memory_type_ptr_t
gs_heap_object_type(gs_memory_t * mem, const void *ptr)
{
    return ((const gs_malloc_block_t *)ptr)[-1].type;
}
private void
gs_heap_free_object(gs_memory_t * mem, void *ptr, client_name_t cname)
{
    gs_malloc_memory_t *mmem = (gs_malloc_memory_t *) mem;
    gs_malloc_block_t *bp = mmem->allocated;
    gs_memory_type_ptr_t pstype;
    struct_proc_finalize((*finalize));

    if_debug3('a', "[a-]gs_free(%s) 0x%lx(%u)\n",
	      client_name_string(cname), (ulong) ptr,
	      (ptr == 0 ? 0 : ((gs_malloc_block_t *) ptr)[-1].size));
    if (ptr == 0)
	return;
    pstype = ((gs_malloc_block_t *) ptr)[-1].type;
    finalize = pstype->finalize;
    if (finalize != 0) {
	if_debug3('u', "[u]finalizing %s 0x%lx (%s)\n",
		  struct_type_name_string(pstype),
		  (ulong) ptr, client_name_string(cname));
	(*finalize) (ptr);
    }
    if (ptr == bp + 1) {
	mmem->allocated = bp->next;
	mmem->used -= bp->size + sizeof(gs_malloc_block_t);

	if (mmem->allocated)
	    mmem->allocated->prev = 0;
	gs_alloc_fill(bp, gs_alloc_fill_free,
		      bp->size + sizeof(gs_malloc_block_t));
	free(bp);
    } else {
	gs_malloc_block_t *np;

	/*
	 * bp == 0 at this point is an error, but we'd rather have an
	 * error message than an invalid access.
	 */
	if (bp) {
	    for (; (np = bp->next) != 0; bp = np) {
		if (ptr == np + 1) {
		    bp->next = np->next;
		    if (np->next)
			np->next->prev = bp;
		    mmem->used -= np->size + sizeof(gs_malloc_block_t);
		    gs_alloc_fill(np, gs_alloc_fill_free,
				  np->size + sizeof(gs_malloc_block_t));
		    free(np);
		    return;
		}
	    }
	}
	lprintf2("%s: free 0x%lx not found!\n",
		 client_name_string(cname), (ulong) ptr);
	free((char *)((gs_malloc_block_t *) ptr - 1));
    }
}
private byte *
gs_heap_alloc_string(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    return gs_heap_alloc_bytes(mem, nbytes, cname);
}
private byte *
gs_heap_resize_string(gs_memory_t * mem, byte * data, uint old_num, uint new_num,
		      client_name_t cname)
{
    if (gs_heap_object_type(mem, data) != &st_bytes)
	lprintf2("%s: resizing non-string 0x%lx!\n",
		 client_name_string(cname), (ulong) data);
    return gs_heap_resize_object(mem, data, new_num, cname);
}
private void
gs_heap_free_string(gs_memory_t * mem, byte * data, uint nbytes,
		    client_name_t cname)
{
    /****** SHOULD CHECK SIZE IF DEBUGGING ******/
    gs_heap_free_object(mem, data, cname);
}
private int
gs_heap_register_root(gs_memory_t * mem, gs_gc_root_t * rp,
		      gs_ptr_type_t ptype, void **up, client_name_t cname)
{
    return 0;
}
private void
gs_heap_unregister_root(gs_memory_t * mem, gs_gc_root_t * rp,
			client_name_t cname)
{
}
private gs_memory_t *
gs_heap_stable(gs_memory_t *mem)
{
    return mem;			/* heap memory is stable */
}
private void
gs_heap_status(gs_memory_t * mem, gs_memory_status_t * pstat)
{
    gs_malloc_memory_t *mmem = (gs_malloc_memory_t *) mem;

    pstat->allocated = mmem->used + heap_available();
    pstat->used = mmem->used;
}
private void
gs_heap_enable_free(gs_memory_t * mem, bool enable)
{
    if (enable)
	mem->procs.free_object = gs_heap_free_object,
	    mem->procs.free_string = gs_heap_free_string;
    else
	mem->procs.free_object = gs_ignore_free_object,
	    mem->procs.free_string = gs_ignore_free_string;
}

/* Release all memory acquired by this allocator. */
private void
gs_heap_free_all(gs_memory_t * mem, uint free_mask, client_name_t cname)
{
    gs_malloc_memory_t *const mmem = (gs_malloc_memory_t *) mem;

    if (free_mask & FREE_ALL_DATA) {
	gs_malloc_block_t *bp = mmem->allocated;
	gs_malloc_block_t *np;

	for (; bp != 0; bp = np) {
	    np = bp->next;
	    if_debug3('a', "[a]gs_heap_free_all(%s) 0x%lx(%u)\n",
		      client_name_string(bp->cname), (ulong) (bp + 1),
		      bp->size);
	    gs_alloc_fill(bp + 1, gs_alloc_fill_free, bp->size);
	    free(bp);
	}
    }
    if (free_mask & FREE_ALL_ALLOCATOR)
	free(mem);
}

/* ------ Wrapping ------ */

/* Create the retrying and the locked wrapper for the heap allocator. */
int
gs_malloc_wrap(gs_memory_t **wrapped, gs_malloc_memory_t *contents)
{
    gs_memory_t *cmem = (gs_memory_t *)contents;
    gs_memory_locked_t *lmem = (gs_memory_locked_t *)
	gs_alloc_bytes_immovable(cmem, sizeof(gs_memory_locked_t),
				 "gs_malloc_wrap(locked)");
    gs_memory_retrying_t *rmem;
    int code;

    if (lmem == 0)
	return_error(gs_error_VMerror);
    code = gs_memory_locked_init(lmem, cmem);
    if (code < 0) {
	gs_free_object(cmem, lmem, "gs_malloc_wrap(locked)");
	return code;
    }

    rmem = (gs_memory_retrying_t *)
	gs_alloc_bytes_immovable((gs_memory_t *)lmem,
				 sizeof(gs_memory_retrying_t),
				 "gs_malloc_wrap(retrying)");
    if (rmem == 0) {
	gs_memory_locked_release(lmem);
	gs_free_object(cmem, lmem, "gs_malloc_wrap(locked)");
	return_error(gs_error_VMerror);
    }
    code = gs_memory_retrying_init(rmem, (gs_memory_t *)lmem);
    if (code < 0) {
	gs_free_object((gs_memory_t *)lmem, rmem, "gs_malloc_wrap(retrying)");
	gs_memory_locked_release(lmem);
	gs_free_object(cmem, lmem, "gs_malloc_wrap(locked)");
	return code;
    }

    *wrapped = (gs_memory_t *)rmem;
    return 0;
}

/* Get the wrapped contents. */
gs_malloc_memory_t *
gs_malloc_wrapped_contents(gs_memory_t *wrapped)
{
    gs_memory_retrying_t *rmem = (gs_memory_retrying_t *)wrapped;
    gs_memory_locked_t *lmem =
	(gs_memory_locked_t *)gs_memory_retrying_target(rmem);
    if (lmem) 
	return (gs_malloc_memory_t *)gs_memory_locked_target(lmem);
    return (gs_malloc_memory_t *) wrapped;
}

/* Free the wrapper, and return the wrapped contents. */
gs_malloc_memory_t *
gs_malloc_unwrap(gs_memory_t *wrapped)
{
    gs_memory_retrying_t *rmem = (gs_memory_retrying_t *)wrapped;
    gs_memory_locked_t *lmem =
	(gs_memory_locked_t *)gs_memory_retrying_target(rmem);
    gs_memory_t *contents = gs_memory_locked_target(lmem);

    gs_free_object((gs_memory_t *)lmem, rmem, "gs_malloc_unwrap(retrying)");
    gs_memory_locked_release(lmem);
    gs_free_object(contents, lmem, "gs_malloc_unwrap(locked)");
    return (gs_malloc_memory_t *)contents;
}


/* Create the default allocator, and return it. */
gs_memory_t *
gs_malloc_init(const gs_memory_t *parent)
{
    gs_malloc_memory_t *malloc_memory_default = gs_malloc_memory_init();
    gs_memory_t *memory_t_default;

    if (parent)
	malloc_memory_default->gs_lib_ctx = parent->gs_lib_ctx;
    else 
        gs_lib_ctx_init((gs_memory_t *)malloc_memory_default);

    gs_malloc_wrap(&memory_t_default, malloc_memory_default);
    memory_t_default->stable_memory = memory_t_default;
    return memory_t_default;
}

/* Release the default allocator. */
void
gs_malloc_release(gs_memory_t *mem)
{
    gs_malloc_memory_t * malloc_memory_default = gs_malloc_unwrap(mem);
    gs_malloc_memory_release(malloc_memory_default);
}
