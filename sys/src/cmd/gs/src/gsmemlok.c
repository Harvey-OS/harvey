/* Copyright (C) 1998, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsmemlok.c,v 1.6 2004/08/04 19:36:12 stefan Exp $ */
/* Monitor-locked heap memory allocator */

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */
/* Revised 8/6/98 by L. Peter Deutsch (ghost@aladdin.com) for changes */
/*   in memory manager API */
/* Edited 3/23/1999 by L. Peter Deutsch to remove compiler warnings. */

#include "gx.h"
#include "gsmemlok.h"
#include "gserrors.h"

/* Raw memory procedures */
private gs_memory_proc_alloc_bytes(gs_locked_alloc_bytes_immovable);
private gs_memory_proc_resize_object(gs_locked_resize_object);
private gs_memory_proc_free_object(gs_locked_free_object);
private gs_memory_proc_stable(gs_locked_stable);
private gs_memory_proc_status(gs_locked_status);
private gs_memory_proc_free_all(gs_locked_free_all);
private gs_memory_proc_consolidate_free(gs_locked_consolidate_free);

/* Object memory procedures */
private gs_memory_proc_alloc_bytes(gs_locked_alloc_bytes);
private gs_memory_proc_alloc_struct(gs_locked_alloc_struct);
private gs_memory_proc_alloc_struct(gs_locked_alloc_struct_immovable);
private gs_memory_proc_alloc_byte_array(gs_locked_alloc_byte_array);
private gs_memory_proc_alloc_byte_array(gs_locked_alloc_byte_array_immovable);
private gs_memory_proc_alloc_struct_array(gs_locked_alloc_struct_array);
private gs_memory_proc_alloc_struct_array(gs_locked_alloc_struct_array_immovable);
private gs_memory_proc_object_size(gs_locked_object_size);
private gs_memory_proc_object_type(gs_locked_object_type);
private gs_memory_proc_alloc_string(gs_locked_alloc_string);
private gs_memory_proc_alloc_string(gs_locked_alloc_string_immovable);
private gs_memory_proc_resize_string(gs_locked_resize_string);
private gs_memory_proc_free_string(gs_locked_free_string);
private gs_memory_proc_register_root(gs_locked_register_root);
private gs_memory_proc_unregister_root(gs_locked_unregister_root);
private gs_memory_proc_enable_free(gs_locked_enable_free);
private const gs_memory_procs_t locked_procs =
{
    /* Raw memory procedures */
    gs_locked_alloc_bytes_immovable,
    gs_locked_resize_object,
    gs_locked_free_object,
    gs_locked_stable,
    gs_locked_status,
    gs_locked_free_all,
    gs_locked_consolidate_free,
    /* Object memory procedures */
    gs_locked_alloc_bytes,
    gs_locked_alloc_struct,
    gs_locked_alloc_struct_immovable,
    gs_locked_alloc_byte_array,
    gs_locked_alloc_byte_array_immovable,
    gs_locked_alloc_struct_array,
    gs_locked_alloc_struct_array_immovable,
    gs_locked_object_size,
    gs_locked_object_type,
    gs_locked_alloc_string,
    gs_locked_alloc_string_immovable,
    gs_locked_resize_string,
    gs_locked_free_string,
    gs_locked_register_root,
    gs_locked_unregister_root,
    gs_locked_enable_free
};

/* ---------- Public constructors/destructors ---------- */

/* Initialize a gs_memory_locked_t */
int				/* -ve error code or 0 */
gs_memory_locked_init(
		      gs_memory_locked_t * lmem,	/* allocator to init */
		      gs_memory_t * target	/* allocator to monitor lock */
)
{
    lmem->stable_memory = 0;
    lmem->procs = locked_procs;

    lmem->target = target;
    lmem->gs_lib_ctx = target->gs_lib_ctx;

    /* Allocate a monitor to serialize access to structures within */
    lmem->monitor = gx_monitor_alloc(target);
    return (lmem->monitor ? 0 : gs_note_error(gs_error_VMerror));
}

/* Release a locked memory manager. */
/* Note that this has no effect on the target. */
void
gs_memory_locked_release(gs_memory_locked_t *lmem)
{
    gs_memory_free_all((gs_memory_t *)lmem, FREE_ALL_STRUCTURES,
		       "gs_memory_locked_release");
}

/* ---------- Accessors ------------- */

/* Retrieve this allocator's target */
gs_memory_t *
gs_memory_locked_target(const gs_memory_locked_t *lmem)
{
    return lmem->target;
}

/* -------- Private members just wrap a monitor around a gs_memory_heap --- */

/*
 * Contrary to our usual practice, we don't use BEGIN/END here, because
 * that causes some compilers to give bogus error messages.
 */

#define DO_MONITORED(call_target)\
	gs_memory_locked_t * const lmem = (gs_memory_locked_t *)mem;\
\
	gx_monitor_enter(lmem->monitor);\
	call_target;\
	gx_monitor_leave(lmem->monitor)

#define RETURN_MONITORED(result_type, call_target)\
	gs_memory_locked_t * const lmem = (gs_memory_locked_t *)mem;\
	result_type temp;\
\
	gx_monitor_enter(lmem->monitor);\
	temp = call_target;\
	gx_monitor_leave(lmem->monitor);\
	return temp

/* Procedures */
private void
gs_locked_free_all(gs_memory_t * mem, uint free_mask, client_name_t cname)
{
    gs_memory_locked_t * const lmem = (gs_memory_locked_t *)mem;
    gs_memory_t * const target = lmem->target;

    /* Only free the structures and the allocator itself. */
    if (mem->stable_memory) {
	if (mem->stable_memory != mem)
	    gs_memory_free_all(mem->stable_memory, free_mask, cname);
	if (free_mask & FREE_ALL_ALLOCATOR)
	    mem->stable_memory = 0;
    }
    if (free_mask & FREE_ALL_STRUCTURES) {
	/*
	 * Check for monitor == 0, in case this is called after a
	 * failure during initialization.
	 */
	if (lmem->monitor)
	    gx_monitor_free(lmem->monitor);
	lmem->monitor = 0;
	lmem->target = 0;
    }
    if (free_mask & FREE_ALL_ALLOCATOR)
	gs_free_object(target, lmem, cname);
}
private void
gs_locked_consolidate_free(gs_memory_t * mem)
{
    DO_MONITORED(
		 (*lmem->target->procs.consolidate_free)(lmem->target)
		 );
}
private byte *
gs_locked_alloc_bytes(gs_memory_t * mem, uint size, client_name_t cname)
{
    RETURN_MONITORED(
		     byte *,
		     (*lmem->target->procs.alloc_bytes)
		       (lmem->target, size, cname)
		     );
}
private byte *
gs_locked_alloc_bytes_immovable(gs_memory_t * mem, uint size,
				client_name_t cname)
{
    RETURN_MONITORED(
		     byte *,
		     (*lmem->target->procs.alloc_bytes_immovable)
		       (lmem->target, size, cname)
		     );
}
private void *
gs_locked_alloc_struct(gs_memory_t * mem, gs_memory_type_ptr_t pstype,
		       client_name_t cname)
{
    RETURN_MONITORED(
		     void *,
		     (*lmem->target->procs.alloc_struct)
		       (lmem->target, pstype, cname)
		     );
}
private void *
gs_locked_alloc_struct_immovable(gs_memory_t * mem,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    RETURN_MONITORED(
		     void *,
		     (*lmem->target->procs.alloc_struct_immovable)
		       (lmem->target, pstype, cname)
		     );
}
private byte *
gs_locked_alloc_byte_array(gs_memory_t * mem, uint num_elements, uint elt_size,
			   client_name_t cname)
{
    RETURN_MONITORED(
		     byte *,
		     (*lmem->target->procs.alloc_byte_array)
		       (lmem->target, num_elements, elt_size, cname)
		     );
}
private byte *
gs_locked_alloc_byte_array_immovable(gs_memory_t * mem, uint num_elements,
				     uint elt_size, client_name_t cname)
{
    RETURN_MONITORED(
		     byte *,
		     (*lmem->target->procs.alloc_byte_array_immovable)
		       (lmem->target, num_elements, elt_size, cname)
		     );
}
private void *
gs_locked_alloc_struct_array(gs_memory_t * mem, uint num_elements,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    RETURN_MONITORED(
		     void *,
		     (*lmem->target->procs.alloc_struct_array)
		       (lmem->target, num_elements, pstype, cname)
		     );
}
private void *
gs_locked_alloc_struct_array_immovable(gs_memory_t * mem, uint num_elements,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    RETURN_MONITORED(
		     void *,
		     (*lmem->target->procs.alloc_struct_array_immovable)
		       (lmem->target, num_elements, pstype, cname)
		     );
}
private void *
gs_locked_resize_object(gs_memory_t * mem, void *obj, uint new_num_elements,
			client_name_t cname)
{
    RETURN_MONITORED(
		     void *,
		     (*lmem->target->procs.resize_object)
		       (lmem->target, obj, new_num_elements, cname)
		     );
}
private uint
gs_locked_object_size(gs_memory_t * mem, const void *ptr)
{
    RETURN_MONITORED(
		     uint,
		     (*lmem->target->procs.object_size)
		       (lmem->target, ptr)
		     );
}
private gs_memory_type_ptr_t
gs_locked_object_type(gs_memory_t * mem, const void *ptr)
{
    RETURN_MONITORED(
		     gs_memory_type_ptr_t,
		     (*lmem->target->procs.object_type)
		       (lmem->target, ptr)
		     );
}
private void
gs_locked_free_object(gs_memory_t * mem, void *ptr, client_name_t cname)
{
    DO_MONITORED(
		 (*lmem->target->procs.free_object)
	           (lmem->target, ptr, cname)
		 );
}
private byte *
gs_locked_alloc_string(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    RETURN_MONITORED(
		     byte *,
		     (*lmem->target->procs.alloc_string)
		       (lmem->target, nbytes, cname)
		     );
}
private byte *
gs_locked_alloc_string_immovable(gs_memory_t * mem, uint nbytes,
				 client_name_t cname)
{
    RETURN_MONITORED(
		     byte *,
		     (*lmem->target->procs.alloc_string_immovable)
		       (lmem->target, nbytes, cname)
		     );
}
private byte *
gs_locked_resize_string(gs_memory_t * mem, byte * data, uint old_num,
			uint new_num,
			client_name_t cname)
{
    RETURN_MONITORED(
		     byte *,
		     (*lmem->target->procs.resize_string)
		       (lmem->target, data, old_num, new_num, cname)
		     );
}
private void
gs_locked_free_string(gs_memory_t * mem, byte * data, uint nbytes,
		      client_name_t cname)
{
    DO_MONITORED(
		 (*lmem->target->procs.free_string)
		   (lmem->target, data, nbytes, cname)
		 );
}
private int
gs_locked_register_root(gs_memory_t * mem, gs_gc_root_t * rp,
			gs_ptr_type_t ptype, void **up, client_name_t cname)
{
    RETURN_MONITORED(
		     int,
		     (*lmem->target->procs.register_root)
		       (lmem->target, rp, ptype, up, cname)
		     );
}
private void
gs_locked_unregister_root(gs_memory_t * mem, gs_gc_root_t * rp,
			  client_name_t cname)
{
    DO_MONITORED(
		 (*lmem->target->procs.unregister_root)
		   (lmem->target, rp, cname)
		 );
}
private gs_memory_t *
gs_locked_stable(gs_memory_t * mem)
{
    if (!mem->stable_memory) {
	gs_memory_locked_t * const lmem = (gs_memory_locked_t *)mem;
	gs_memory_t *stable;

	gx_monitor_enter(lmem->monitor);
	stable = gs_memory_stable(lmem->target);
	if (stable == lmem->target)
	    mem->stable_memory = mem;
	else {
	    gs_memory_locked_t *locked_stable = (gs_memory_locked_t *)
		gs_alloc_bytes(stable, sizeof(*lmem), "gs_locked_stable");

	    if (locked_stable) {
		int code = gs_memory_locked_init(locked_stable, stable);

		if (code < 0)
		    gs_free_object(stable, locked_stable, "gs_locked_stable");
		else
		    mem->stable_memory = (gs_memory_t *)locked_stable;
	    }
	}
	gx_monitor_leave(lmem->monitor);
    }
    return mem->stable_memory;
}
private void
gs_locked_status(gs_memory_t * mem, gs_memory_status_t * pstat)
{
    DO_MONITORED(
		 (*lmem->target->procs.status)(lmem->target, pstat)
		 );
}
private void
gs_locked_enable_free(gs_memory_t * mem, bool enable)
{
    DO_MONITORED(
		 (*lmem->target->procs.enable_free)(lmem->target, enable)
		 );
}
