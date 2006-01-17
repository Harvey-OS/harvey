/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsmemret.c,v 1.5 2004/08/04 19:36:12 stefan Exp $ */
/* Retrying memory allocator */

#include "gx.h"
#include "gsmemret.h"
#include "gserrors.h"

/* Raw memory procedures */
private gs_memory_proc_alloc_bytes(gs_retrying_alloc_bytes_immovable);
private gs_memory_proc_resize_object(gs_retrying_resize_object);
private gs_memory_proc_free_object(gs_forward_free_object);
private gs_memory_proc_stable(gs_retrying_stable);
private gs_memory_proc_status(gs_forward_status);
private gs_memory_proc_free_all(gs_forward_free_all);
private gs_memory_proc_consolidate_free(gs_forward_consolidate_free);

/* Object memory procedures */
private gs_memory_proc_alloc_bytes(gs_retrying_alloc_bytes);
private gs_memory_proc_alloc_struct(gs_retrying_alloc_struct);
private gs_memory_proc_alloc_struct(gs_retrying_alloc_struct_immovable);
private gs_memory_proc_alloc_byte_array(gs_retrying_alloc_byte_array);
private gs_memory_proc_alloc_byte_array(gs_retrying_alloc_byte_array_immovable);
private gs_memory_proc_alloc_struct_array(gs_retrying_alloc_struct_array);
private gs_memory_proc_alloc_struct_array(gs_retrying_alloc_struct_array_immovable);
private gs_memory_proc_object_size(gs_forward_object_size);
private gs_memory_proc_object_type(gs_forward_object_type);
private gs_memory_proc_alloc_string(gs_retrying_alloc_string);
private gs_memory_proc_alloc_string(gs_retrying_alloc_string_immovable);
private gs_memory_proc_resize_string(gs_retrying_resize_string);
private gs_memory_proc_free_string(gs_forward_free_string);
private gs_memory_proc_register_root(gs_retrying_register_root);
private gs_memory_proc_unregister_root(gs_forward_unregister_root);
private gs_memory_proc_enable_free(gs_forward_enable_free);
private const gs_memory_procs_t retrying_procs = {
    /* Raw memory procedures */
    gs_retrying_alloc_bytes_immovable,
    gs_retrying_resize_object,
    gs_forward_free_object,
    gs_retrying_stable,
    gs_forward_status,
    gs_forward_free_all,
    gs_forward_consolidate_free,
    /* Object memory procedures */
    gs_retrying_alloc_bytes,
    gs_retrying_alloc_struct,
    gs_retrying_alloc_struct_immovable,
    gs_retrying_alloc_byte_array,
    gs_retrying_alloc_byte_array_immovable,
    gs_retrying_alloc_struct_array,
    gs_retrying_alloc_struct_array_immovable,
    gs_forward_object_size,
    gs_forward_object_type,
    gs_retrying_alloc_string,
    gs_retrying_alloc_string_immovable,
    gs_retrying_resize_string,
    gs_forward_free_string,
    gs_retrying_register_root,
    gs_forward_unregister_root,
    gs_forward_enable_free
};

/* Define a vacuous recovery procedure. */
private gs_memory_recover_status_t
no_recover_proc(gs_memory_retrying_t *rmem, void *proc_data)
{
    return RECOVER_STATUS_NO_RETRY;
}

/* ---------- Public constructors/destructors ---------- */

/* Initialize a gs_memory_retrying_t */
int				/* -ve error code or 0 */
gs_memory_retrying_init(
		      gs_memory_retrying_t * rmem,	/* allocator to init */
		      gs_memory_t * target	/* allocator to wrap */
)
{
    rmem->stable_memory = 0;
    rmem->procs = retrying_procs;
    rmem->target = target;
    rmem->gs_lib_ctx = target->gs_lib_ctx;
    rmem->non_gc_memory = (gs_memory_t *)rmem;
    gs_memory_retrying_set_recover(rmem, no_recover_proc, NULL);
    return 0;
}

/* Set the recovery closure of a retrying memory manager. */
void
gs_memory_retrying_set_recover(gs_memory_retrying_t *rmem,
			       gs_memory_recover_proc_t recover_proc,
			       void *recover_proc_data)
{
    rmem->recover_proc = recover_proc;
    rmem->recover_proc_data = recover_proc_data;
}

/* Release a retrying memory manager. */
/* Note that this has no effect on the target. */
void
gs_memory_retrying_release(gs_memory_retrying_t *rmem)
{
    gs_memory_free_all((gs_memory_t *)rmem, FREE_ALL_STRUCTURES,
		       "gs_memory_retrying_release");
}

/* ---------- Accessors ------------- */

/* Retrieve this allocator's target */
gs_memory_t *
gs_memory_retrying_target(const gs_memory_retrying_t *rmem)
{
    return rmem->target;
}

/* -------- Private members just wrap retrying around a gs_memory --- */

/*
 * Contrary to our usual practice, we don't use BEGIN/END here, because
 * that causes some compilers to give bogus error messages.
 */

#define DO_FORWARD(call_target)\
	gs_memory_retrying_t * const rmem = (gs_memory_retrying_t *)mem;\
	gs_memory_t *const target = rmem->target;\
\
	call_target

#define RETURN_RETRYING(result_type, call_target)\
	gs_memory_retrying_t * const rmem = (gs_memory_retrying_t *)mem;\
	gs_memory_t *const target = rmem->target;\
	result_type temp;\
	gs_memory_recover_status_t retry = RECOVER_STATUS_RETRY_OK;\
\
	for (;;) {\
	    temp = call_target;\
	    if (temp != 0 || retry != RECOVER_STATUS_RETRY_OK)\
		break;\
	    retry = rmem->recover_proc(rmem, rmem->recover_proc_data);\
	}\
	return temp

/* Procedures */
private void
gs_forward_free_all(gs_memory_t * mem, uint free_mask, client_name_t cname)
{
    gs_memory_retrying_t * const rmem = (gs_memory_retrying_t *)mem;
    gs_memory_t * const target = rmem->target;

    /* Only free the structures and the allocator itself. */
    rmem->target = 0;
    if (free_mask & FREE_ALL_ALLOCATOR)
	gs_free_object(target, rmem, cname);
}
private void
gs_forward_consolidate_free(gs_memory_t * mem)
{
    DO_FORWARD(target->procs.consolidate_free(target));
}
private byte *
gs_retrying_alloc_bytes(gs_memory_t * mem, uint size, client_name_t cname)
{
    RETURN_RETRYING(
		    byte *,
		    target->procs.alloc_bytes(target, size, cname)
		    );
}
private byte *
gs_retrying_alloc_bytes_immovable(gs_memory_t * mem, uint size,
				client_name_t cname)
{
    RETURN_RETRYING(
		    byte *,
		    target->procs.alloc_bytes_immovable(target, size, cname)
		    );
}
private void *
gs_retrying_alloc_struct(gs_memory_t * mem, gs_memory_type_ptr_t pstype,
		       client_name_t cname)
{
    RETURN_RETRYING(
		    void *,
		    target->procs.alloc_struct(target, pstype, cname)
		    );
}
private void *
gs_retrying_alloc_struct_immovable(gs_memory_t * mem,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    RETURN_RETRYING(
		    void *,
		    target->procs.alloc_struct_immovable(target, pstype, cname)
		    );
}
private byte *
gs_retrying_alloc_byte_array(gs_memory_t * mem, uint num_elements, uint elt_size,
			   client_name_t cname)
{
    RETURN_RETRYING(
		    byte *,
		    target->procs.alloc_byte_array(target, num_elements,
						   elt_size, cname)
		    );
}
private byte *
gs_retrying_alloc_byte_array_immovable(gs_memory_t * mem, uint num_elements,
				     uint elt_size, client_name_t cname)
{
    RETURN_RETRYING(
		    byte *,
		    target->procs.alloc_byte_array_immovable(target,
							num_elements, elt_size,
							cname)
		    );
}
private void *
gs_retrying_alloc_struct_array(gs_memory_t * mem, uint num_elements,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    RETURN_RETRYING(
		    void *,
		    target->procs.alloc_struct_array(target, num_elements,
						     pstype, cname)
		    );
}
private void *
gs_retrying_alloc_struct_array_immovable(gs_memory_t * mem, uint num_elements,
			   gs_memory_type_ptr_t pstype, client_name_t cname)
{
    RETURN_RETRYING(
		    void *,
		    target->procs.alloc_struct_array_immovable(target,
							num_elements, pstype,
						        cname)
		    );
}
private void *
gs_retrying_resize_object(gs_memory_t * mem, void *obj, uint new_num_elements,
			client_name_t cname)
{
    RETURN_RETRYING(
		    void *,
		    target->procs.resize_object(target, obj, new_num_elements,
						cname)
		    );
}
private uint
gs_forward_object_size(gs_memory_t * mem, const void *ptr)
{
    DO_FORWARD(return target->procs.object_size(target, ptr));
}
private gs_memory_type_ptr_t
gs_forward_object_type(gs_memory_t * mem, const void *ptr)
{
    DO_FORWARD(return target->procs.object_type(target, ptr));
}
private void
gs_forward_free_object(gs_memory_t * mem, void *ptr, client_name_t cname)
{
    DO_FORWARD(target->procs.free_object(target, ptr, cname));
}
private byte *
gs_retrying_alloc_string(gs_memory_t * mem, uint nbytes, client_name_t cname)
{
    RETURN_RETRYING(
		    byte *,
		    target->procs.alloc_string(target, nbytes, cname)
		    );
}
private byte *
gs_retrying_alloc_string_immovable(gs_memory_t * mem, uint nbytes,
				 client_name_t cname)
{
    RETURN_RETRYING(
		    byte *,
		    target->procs.alloc_string_immovable(target, nbytes, cname)
		    );
}
private byte *
gs_retrying_resize_string(gs_memory_t * mem, byte * data, uint old_num,
			uint new_num,
			client_name_t cname)
{
    RETURN_RETRYING(
		    byte *,
		    target->procs.resize_string(target, data, old_num, new_num,
						cname)
		    );
}
private void
gs_forward_free_string(gs_memory_t * mem, byte * data, uint nbytes,
		      client_name_t cname)
{
    DO_FORWARD(target->procs.free_string(target, data, nbytes, cname));
}
private int
gs_retrying_register_root(gs_memory_t * mem, gs_gc_root_t * rp,
			gs_ptr_type_t ptype, void **up, client_name_t cname)
{
    RETURN_RETRYING(
		    int,
		    target->procs.register_root(target, rp, ptype, up, cname)
		    );
}
private void
gs_forward_unregister_root(gs_memory_t * mem, gs_gc_root_t * rp,
			  client_name_t cname)
{
    DO_FORWARD(target->procs.unregister_root(target, rp, cname));
}
private gs_memory_t *
gs_retrying_stable(gs_memory_t * mem)
{
    if (!mem->stable_memory) {
	gs_memory_retrying_t * const rmem = (gs_memory_retrying_t *)mem;
	gs_memory_t *stable = gs_memory_stable(rmem->target);

	if (stable == rmem->target)
	    mem->stable_memory = mem;
	else {
	    gs_memory_retrying_t *retrying_stable = (gs_memory_retrying_t *)
		gs_alloc_bytes(stable, sizeof(*rmem), "gs_retrying_stable");

	    if (retrying_stable) {
		int code = gs_memory_retrying_init(retrying_stable, stable);

		if (code < 0)
		    gs_free_object(stable, retrying_stable, "gs_retrying_stable");
		else
		    mem->stable_memory = (gs_memory_t *)retrying_stable;
	    }
	}
    }
    return mem->stable_memory;
}
private void
gs_forward_status(gs_memory_t * mem, gs_memory_status_t * pstat)
{
    DO_FORWARD(target->procs.status(target, pstat));
}
private void
gs_forward_enable_free(gs_memory_t * mem, bool enable)
{
    DO_FORWARD(target->procs.enable_free(target, enable));
}
