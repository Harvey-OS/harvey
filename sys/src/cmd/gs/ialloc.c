/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* ialloc.c */
/* Memory allocator for Ghostscript interpreter */
#include "gx.h"
#include "memory_.h"
#include "errors.h"
#include "gsstruct.h"
#include "gxarith.h"			/* for small_exact_log2 */
#include "iref.h"			/* must precede iastate.h */
#include "iastate.h"
#include "ivmspace.h"
#include "store.h"

/* The structure descriptor for allocators.  Even though allocators */
/* are allocated outside GC space, they reference objects within it. */
public_st_ref_memory();
#define mptr ((gs_ref_memory_t *)vptr)
private ENUM_PTRS_BEGIN(ref_memory_enum_ptrs) return 0;
	ENUM_PTR(0, gs_ref_memory_t, changes);
	ENUM_PTR(1, gs_ref_memory_t, saved);
ENUM_PTRS_END
private RELOC_PTRS_BEGIN(ref_memory_reloc_ptrs) {
	RELOC_PTR(gs_ref_memory_t, changes);
	/* Don't relocate the pointer now -- see igc.c for details. */
	mptr->reloc_saved = gs_reloc_struct_ptr(mptr->saved, gcst);
} RELOC_PTRS_END

/* Import the debugging variables from gsmemory.c. */
extern byte gs_alloc_fill_alloc;
extern byte gs_alloc_fill_free;

/* Forward references */
private gs_ref_memory_t *ialloc_alloc_state(P2(gs_memory_t *, uint));
private obj_header_t *alloc_obj(P4(gs_ref_memory_t *, ulong, gs_memory_type_ptr_t, client_name_t));
private chunk_t *alloc_add_chunk(P4(gs_ref_memory_t *, ulong, bool, client_name_t));
void alloc_close_chunk(P1(gs_ref_memory_t *));
private void alloc_close_refs(P1(chunk_t *));

/*
 * Define the standard implementation (for the interpreter)
 * of Ghostscript's memory manager interface.
 */
private gs_memory_proc_alloc_bytes(i_alloc_bytes);
private gs_memory_proc_alloc_struct(i_alloc_struct);
private gs_memory_proc_alloc_byte_array(i_alloc_byte_array);
private gs_memory_proc_alloc_struct_array(i_alloc_struct_array);
private gs_memory_proc_object_size(i_object_size);
private gs_memory_proc_object_type(i_object_type);
private gs_memory_proc_free_object(i_free_object);
private gs_memory_proc_alloc_string(i_alloc_string);
private gs_memory_proc_resize_string(i_resize_string);
private gs_memory_proc_free_string(i_free_string);
private gs_memory_proc_register_root(i_register_root);
private gs_memory_proc_unregister_root(i_unregister_root);
private gs_memory_proc_status(i_status);
private const gs_memory_procs_t imemory_procs = {
	i_alloc_bytes,
	i_alloc_struct,
	i_alloc_byte_array,
	i_alloc_struct_array,
	i_object_size,
	i_object_type,
	i_free_object,
	i_alloc_string,
	i_resize_string,
	i_free_string,
	i_register_root,
	i_unregister_root,
	i_status
};
/*
 * Define global and local instances.
 */
gs_dual_memory_t gs_imemory;

#define imem ((gs_ref_memory_t *)mem)

/* Initialize the allocator */
void
ialloc_init(gs_memory_t *mem, uint chunk_size, bool level2)
{	gs_ref_memory_t *ilmem = ialloc_alloc_state(mem, chunk_size);
	gs_ref_memory_t *igmem =
		(level2 ?
		 ialloc_alloc_state(mem, chunk_size) :
		 ilmem);
	gs_ref_memory_t *ismem = ialloc_alloc_state(mem, chunk_size);
	int i;

	for ( i = 0; i < countof(gs_imemory.spaces.indexed); i++ )
		gs_imemory.spaces.indexed[i] = 0;
	gs_imemory.space_local = ilmem;
	gs_imemory.space_global = igmem;
	gs_imemory.space_system = ismem;
	gs_imemory.reclaim = 0;
	ilmem->space = avm_local;
	igmem->global = ilmem->global = igmem;
	igmem->space = avm_global;
	ismem->space = avm_system;
	ialloc_set_space(&gs_imemory, avm_global);
}
/* Allocate the state of an allocator (system, global, or local). */
/* Does not initialize global or space. */
private gs_ref_memory_t *
ialloc_alloc_state(gs_memory_t *parent, uint chunk_size)
{	/* We can't assume that the parent uses the same object header */
	/* that we do, but the GC requires that allocators have */
	/* such a header.  Therefore, we prepend one explicitly. */
	typedef struct _rmo {
	  obj_header_t header;
	  gs_ref_memory_t memory;
	} gs_ref_memory_object;
	gs_ref_memory_object *memo =
	  (gs_ref_memory_object *)
	    gs_alloc_bytes(parent, sizeof(gs_ref_memory_object),
			   "ialloc_alloc_state");
	gs_ref_memory_t *iimem;
	if ( memo == 0 )
		return 0;
	/* Construct the object header "by hand". */
	memo->header.o_large = 0;
	memo->header.o_size = sizeof(gs_ref_memory_t);
	memo->header.o_type = &st_ref_memory;
	iimem = &memo->memory;
	iimem->procs = imemory_procs;
	iimem->parent = parent;
	iimem->chunk_size = chunk_size;
	iimem->large_size = ((chunk_size / 4) & -obj_align_mod) + 1;
	iimem->gc_status.vm_threshold = chunk_size * 3L;
	iimem->gc_status.max_vm = max_long;
	iimem->gc_status.psignal = NULL;
	iimem->gc_status.enabled = false;
	iimem->previous_status.allocated = 0;
	iimem->previous_status.used = 0;
	ialloc_reset(iimem);
	ialloc_set_limit(iimem);
	iimem->cc.cbot = iimem->cc.ctop = 0;
	iimem->pcc = 0;
	iimem->roots = 0;
	iimem->num_contexts = 1;
	iimem->saved = 0;
	return iimem;
}
/* Initialize after a save. */
void
ialloc_reset(gs_ref_memory_t *mem)
{	mem->cfirst = 0;
	mem->clast = 0;
	mem->cc.rcur = 0;
	mem->cc.rtop = 0;
	mem->cc.has_refs = false;
	mem->allocated = 0;
	mem->changes = 0;
	ialloc_reset_free(mem);
}
/* Initialize after a save or GC. */
void
ialloc_reset_free(gs_ref_memory_t *mem)
{	int i;
	obj_header_t **p;
	mem->freed_lost = 0;
	mem->cfreed.cp = 0;
	for ( i = 0, p = &mem->freelists[0]; i < num_freelists; i++, p++ )
	  *p = 0;
}
/* Set the allocation limit after a change in one or more of */
/* vm_threshold, max_vm, or enabled, or after a GC. */
void
ialloc_set_limit(register gs_ref_memory_t *mem)
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
	if ( mem->gc_status.enabled )
	  {	ulong limit = mem->gc_allocated + mem->gc_status.vm_threshold;
		if ( limit < mem->previous_status.allocated )
		  mem->limit = 0;
		else
		  {	limit -= mem->previous_status.allocated;
			mem->limit = min(limit, max_allocated);
		  }
	  }
	else
	  mem->limit = max_allocated;
}

/* ================ Accessors ================ */

/* Get the size of an object from the header. */
private uint
i_object_size(gs_memory_t *mem, const void /*obj_header_t*/ *obj)
{	return pre_obj_contents_size((obj_header_t *)obj - 1);
}

/* Get the type of a structure from the header. */
private gs_memory_type_ptr_t
i_object_type(gs_memory_t *mem, const void /*obj_header_t*/ *obj)
{	return ((obj_header_t *)obj - 1)->o_type;
}

/* Get the GC status of a memory. */
void
gs_memory_gc_status(const gs_ref_memory_t *mem, gs_memory_gc_status_t *pstat)
{	*pstat = mem->gc_status;
}

/* Set the GC status of a memory. */
void
gs_memory_set_gc_status(gs_ref_memory_t *mem, const gs_memory_gc_status_t *pstat)
{	mem->gc_status = *pstat;
	ialloc_set_limit(mem);
}

/* Get the space index of an allocator for printing. */
#define imem_space_index\
  ((imem)->space >> r_space_shift)

/* ================ Objects ================ */

/* Allocate a small object fast if possible. */
/* The size must be substantially less than max_uint. */
/* ptr must be declared as obj_header_t *. */
/* pfl must be declared as obj_header_t **. */
#define if_not_alloc_small(ptr, imem, size, pstype, pfl)\
	if ( size <= max_freelist_size &&\
	     *(pfl = &imem->freelists[(size + obj_align_mask) >> log2_obj_align_mod]) != 0\
	   )\
	{	ptr = *pfl;\
		*pfl = *(obj_header_t **)ptr;\
		ptr[-1].o_size = size;\
		ptr[-1].o_type = pstype;\
		if ( gs_alloc_debug )\
		  { /* Clear the block in an attempt to track down */\
		    /* uninitialized data errors. */\
		    memset(ptr, gs_alloc_fill_alloc, size);\
		  }\
	}\
	else if ( (imem->cc.ctop - (byte *)(ptr = (obj_header_t *)imem->cc.cbot))\
		>= size + (obj_align_mod + sizeof(obj_header_t) * 2) &&\
	     size < imem->large_size\
	   )\
	{	imem->cc.cbot = (byte *)ptr + obj_size_round(size);\
		ptr->o_large = 0;\
		ptr->o_size = size;\
		ptr->o_type = pstype;\
		ptr++;\
		if ( gs_alloc_debug )\
		  { /* Clear the block in an attempt to track down */\
		    /* uninitialized data errors. */\
		    memset(ptr, gs_alloc_fill_alloc, size);\
		  }\
	}\
	else

private byte *
i_alloc_bytes(gs_memory_t *mem, uint size, client_name_t cname)
{	obj_header_t *obj;
	obj_header_t **pfl;
	if_not_alloc_small(obj, imem, size, &st_bytes, pfl)
	{	obj = alloc_obj(imem, size, &st_bytes, cname);
		if ( obj == 0 )
			return 0;
	}
	if_debug4('A', "[a%d:#< ]%s -bytes-(%u) = 0x%lx\n", imem_space_index,
		  client_name_string(cname), size, (ulong)obj);
	return (byte *)obj;
}
private void *
i_alloc_struct(gs_memory_t *mem, gs_memory_type_ptr_t pstype,
  client_name_t cname)
{	uint size = pstype->ssize;
	obj_header_t *obj;
	obj_header_t **pfl;
	if_not_alloc_small(obj, imem, size, pstype, pfl)
	{	obj = alloc_obj(imem, size, pstype, cname);
		if ( obj == 0 )
			return 0;
	}
	if_debug5('A', "[a%d:+< ]%s %s(%u) = 0x%lx\n", imem_space_index,
		  client_name_string(cname), struct_type_name_string(pstype),
		  size, (ulong)obj);
	return (char *)obj;
}
private byte *
i_alloc_byte_array(gs_memory_t *mem, uint num_elements, uint elt_size,
  client_name_t cname)
{	obj_header_t *obj = alloc_obj(imem, (ulong)num_elements * elt_size,
				      &st_bytes, cname);
	if_debug6('A', "[a%d:#< ]%s -bytes-*(%lu=%u*%u) = 0x%lx\n",
		  imem_space_index, client_name_string(cname),
		  (ulong)num_elements * elt_size,
		  num_elements, elt_size, (ulong)obj);
	return (byte *)obj;
}
private void *
i_alloc_struct_array(gs_memory_t *mem, uint num_elements,
  gs_memory_type_ptr_t pstype, client_name_t cname)
{	obj_header_t *obj = alloc_obj(imem,
				(ulong)num_elements * pstype->ssize,
				pstype, cname);
	if_debug7('A', "[a%d:+< ]%s %s*(%lu=%u*%u) = 0x%lx\n",
		  imem_space_index,
		  client_name_string(cname), struct_type_name_string(pstype),
		  (ulong)num_elements * pstype->ssize,
		  num_elements, pstype->ssize, (ulong)obj);
	return (char *)obj;
}
private void
i_free_object(gs_memory_t *mem, void *ptr, client_name_t cname)
{	obj_header_t *pp;
	struct_proc_finalize((*finalize));
	uint size;

	if ( ptr == 0 )
	  return;
	pp = (obj_header_t *)ptr - 1;
	size = pre_obj_contents_size(pp);
	finalize = pp->o_type->finalize;
	if ( finalize != 0 )
	  (*finalize)(ptr);
	if ( (byte *)ptr + obj_align_round(size) == imem->cc.cbot )
	{	if_debug4('A', "[a%d:-< ]%s(%u) 0x%lx\n", imem_space_index,
			  client_name_string(cname), size, (ulong)pp);
		if ( gs_alloc_debug )
		  memset(ptr, gs_alloc_fill_free, size);
		imem->cc.cbot = (byte *)pp;
		return;
	}
	if ( pp->o_large )
	{	/* We gave this object its own chunk. */
		/* Free the entire chunk. */
		chunk_locator_t cl;
		if_debug4('a', "[a%d:-<L]%s(%u) 0x%lx\n", imem_space_index,
			  client_name_string(cname), size, (ulong)pp);
		cl.memory = imem;
		cl.cp = 0;
		if ( !chunk_locate_ptr(ptr, &cl) )
		{	/* Something is very wrong! */
			lprintf2("%s: free large 0x%lx chunk not found\n",
				 client_name_string(cname), (ulong)ptr);
			return;
		}
		alloc_free_chunk(cl.cp, imem);
		return;
	}
	if ( size <= max_freelist_size &&
	     size >= sizeof(obj_header_t *)
	   )
	  {	/* Put it on a freelist, unless it belongs to */
		/* an older save level, in which case we mustn't */
		/* overwrite it. */
		imem->cfreed.memory = imem;
		if ( chunk_locate(ptr, &imem->cfreed) )
		  {	obj_header_t **pfl =
			  &imem->freelists[(size + obj_align_mask) >>
					   log2_obj_align_mod];
			pp->o_type = &st_bytes;	/* don't confuse GC */
			if ( gs_alloc_debug )
			  memset(ptr, gs_alloc_fill_free, size);
			*(obj_header_t **)ptr = *pfl;
			*pfl = (obj_header_t *)ptr;
			if_debug4('A', "[a%d:F< ]%s(%u) 0x%lx\n",
				  imem_space_index, client_name_string(cname),
				  size, (ulong)pp);
			return;
		  }
		/* Don't overwrite even if gs_alloc_debug is set. */
	  }
	else
	  if ( gs_alloc_debug )
	    memset(ptr, gs_alloc_fill_free, size);
	if_debug4('A', "[a%d:~< ]%s(%u) 0x%lx\n", imem_space_index,
		  client_name_string(cname), size, (ulong)pp);
	imem->freed_lost += obj_size_round(size);
}
private byte *
i_alloc_string(gs_memory_t *mem, uint nbytes, client_name_t cname)
{	byte *str;
top:	if ( imem->cc.ctop - imem->cc.cbot > nbytes )
	{	if_debug4('A', "[a%d:+> ]%s(%u) = 0x%lx\n", imem_space_index,
			  client_name_string(cname), nbytes,
			  (ulong)(imem->cc.ctop - nbytes));
		return (imem->cc.ctop -= nbytes);
	}
	if ( nbytes > string_space_quanta(max_uint - sizeof(chunk_head_t)) *
	      string_data_quantum
	   )
	{	/* Can't represent the size in a uint! */
		return 0;
	}
	if ( nbytes >= imem->large_size )
	{	/* Give it a chunk all its own. */
		uint asize = string_chunk_space(nbytes) +
		  sizeof(chunk_head_t);
		chunk_t *cp =
		  alloc_add_chunk(imem, (ulong)asize, true,
				  "large string chunk");
		if ( cp == 0 )
			return 0;
		str = cp->ctop = cp->climit - nbytes;
		if_debug4('a', "[a%d:+>L]%s(%u) = 0x%lx\n", imem_space_index,
			  client_name_string(cname), nbytes, (ulong)str);
	}
	else
	{	/* Add another chunk. */
		chunk_t *cp =
		  alloc_add_chunk(imem, (ulong)imem->chunk_size, true,
				  "chunk");
		if ( cp == 0 )
			return 0;
		alloc_close_chunk(imem);
		imem->pcc = cp;
		imem->cc = *imem->pcc;
		goto top;
	}
	if ( gs_alloc_debug )
	  { /* Clear the block in an attempt to track down */
	    /* uninitialized data errors. */
	    memset(str, gs_alloc_fill_alloc, nbytes);
	  }
	return str;
}
private byte *
i_resize_string(gs_memory_t *mem, byte *data, uint old_num, uint new_num,
  client_name_t cname)
{	byte *ptr;
	if ( data == imem->cc.ctop &&
	       (new_num < old_num ||
		imem->cc.ctop - imem->cc.cbot > new_num - old_num)
	   )
	{	/* Resize in place. */
		register uint i;
		ptr = data + old_num - new_num;
		if_debug6('A', "[a%d:%c> ]%s(%u->%u) 0x%lx\n",
			  imem_space_index, (new_num > old_num ? '>' : '<'),
			  client_name_string(cname), old_num, new_num,
			  (ulong)ptr);
		imem->cc.ctop = ptr;
		if ( new_num < old_num )
		  for ( i = new_num; i > 0; ) --i, ptr[i] = data[i];
		else
		  for ( i = 0; i < old_num; ) ptr[i] = data[i], i++;
	}
	else
	{	/* Punt. */
		ptr = gs_alloc_string(mem, new_num, cname);
		if ( ptr == 0 )
		  return 0;
		memcpy(ptr, data, min(old_num, new_num));
		gs_free_string(mem, data, old_num, cname);
	}
	return ptr;
}
private void
i_free_string(gs_memory_t *mem, byte *data, uint nbytes,
  client_name_t cname)
{	if ( data == imem->cc.ctop )
	{	if_debug4('A', "[a%d:-> ]%s(%u) 0x%lx\n", imem_space_index,
			  client_name_string(cname), nbytes, (ulong)data);
		imem->cc.ctop += nbytes;
	}
	else
	{	if_debug4('A', "[a%d:~> ]%s(%u) 0x%lx\n", imem_space_index,
			  client_name_string(cname), nbytes, (ulong)data);
		imem->freed_lost += nbytes;
	}
}
private void
i_status(gs_memory_t *mem, gs_memory_status_t *pstat)
{	ulong unused = 0;
	chunk_t *cp = imem->cfirst;
	alloc_close_chunk(imem);
	while ( cp != 0 )
	{	unused += cp->ctop - cp->cbot;
		cp = cp->cnext;
	}
	pstat->used = imem->allocated - unused - imem->freed_lost +
	  imem->previous_status.used;
	pstat->allocated = imem->allocated +
	  imem->previous_status.allocated;
}

/* ------ Internal procedures ------ */

/* Allocate an object.  This handles all but the fastest, simplest case. */
private obj_header_t *
alloc_obj(gs_ref_memory_t *mem, ulong lsize, gs_memory_type_ptr_t pstype,
  client_name_t cname)
{	obj_header_t *ptr;
	if ( lsize >= mem->large_size )
	{	ulong asize =
		  ((lsize + obj_align_mask) & -obj_align_mod) +
		    sizeof(obj_header_t);
		/* Give it a chunk all its own. */
		chunk_t *cp =
		  alloc_add_chunk(mem, asize + sizeof(chunk_head_t), false,
				  "large object chunk");
		if ( cp == 0 )
			return 0;
		ptr = (obj_header_t *)cp->cbot;
		cp->cbot += asize;
		if_debug4('a', "[a%d:+<L]%s(%lu) = 0x%lx\n", imem_space_index,
			  client_name_string(cname), lsize, (ulong)ptr);
		ptr->o_large = 1;
		pre_obj_set_large_size(ptr, lsize);
		if ( pstype == &st_refs )
		  cp->has_refs = true;
	}
	else
	{	uint asize = obj_size_round((uint)lsize);
		while ( mem->cc.ctop -
			 (byte *)(ptr = (obj_header_t *)mem->cc.cbot)
			  <= asize + sizeof(obj_header_t) )
		{	/* Add another chunk. */
			chunk_t *cp =
			  alloc_add_chunk(mem, (ulong)mem->chunk_size,
					  true, "chunk");
			if ( cp == 0 )
				return 0;
			alloc_close_chunk(mem);
			mem->pcc = cp;
			mem->cc = *mem->pcc;
		}
		mem->cc.cbot = (byte *)ptr + asize;
		ptr->o_large = 0;
		ptr->o_size = (uint)lsize;
	}
	ptr->o_type = pstype;
	ptr++;
	if ( gs_alloc_debug )
	  { /* Clear the block in an attempt to track down */
	    /* uninitialized data errors. */
	    /* Note that the block may be too large for a single memset. */
	    ulong msize = lsize;
	    char *p = (char *)ptr;
	    int isize;
	    for ( ; msize; msize -= isize, p += isize )
	      { isize = min(msize, max_int);
		memset(p, gs_alloc_fill_alloc, isize);
	      }
	  }
	return ptr;
}

/* ================ Roots ================ */

/* Register a root. */
private void
i_register_root(gs_memory_t *mem, gs_gc_root_t *rp, gs_ptr_type_t ptype,
  void **up, client_name_t cname)
{	if_debug3('8', "[8]register root(%s) 0x%lx -> 0x%lx\n",
		 client_name_string(cname), (ulong)rp, (ulong)up);
	rp->ptype = ptype, rp->p = up;
	rp->next = imem->roots, imem->roots = rp;
}

/* Unregister a root. */
private void
i_unregister_root(gs_memory_t *mem, gs_gc_root_t *rp, client_name_t cname)
{	gs_gc_root_t **rpp = &imem->roots;
	if_debug2('8', "[8]unregister root(%s) 0x%lx\n",
		client_name_string(cname), (ulong)rp);
	while ( *rpp != rp ) rpp = &(*rpp)->next;
	*rpp = (*rpp)->next;
}

/* ================ Local/global VM ================ */

/* Get the space attribute of an allocator */
uint
imemory_space(gs_ref_memory_t *iimem)
{	return iimem->space;
}

/* Select the allocation space. */
void
ialloc_set_space(gs_dual_memory_t *dmem, uint space)
{	gs_ref_memory_t *mem = dmem->spaces.indexed[space >> r_space_shift];
	dmem->current = mem;
	dmem->current_space = mem->space;
}

/* Reset the requests. */
void
ialloc_reset_requested(gs_dual_memory_t *dmem)
{	dmem->space_system->gc_status.requested = 0;
	dmem->space_global->gc_status.requested = 0;
	dmem->space_local->gc_status.requested = 0;
}

/* ================ Refs ================ */

/*
 * As noted in iastate.h, every run of refs has an extra ref at the end
 * to hold relocation information for the garbage collector;
 * since sizeof(ref) % obj_align_mod == 0, we never need to
 * allocate any additional padding space at the end of the block.
 */
		
/* Allocate an array of refs. */
int
gs_alloc_ref_array(gs_ref_memory_t *mem, ref *parr, uint attrs,
  uint num_refs, client_name_t cname)
{	ref *obj;
	/* If we're allocating a run of refs already, use it. */
	if ( mem->cc.rtop == mem->cc.cbot &&
	     num_refs < (mem->cc.ctop - mem->cc.cbot) / sizeof(ref)
	   )
	  {	obj = (ref *)mem->cc.rtop - 1;	/* back up over last ref */
		if_debug4('A', "[a%d:+$ ]%s(%u) = 0x%lx\n", imem_space_index,
			  client_name_string(cname), num_refs, (ulong)obj);
		mem->cc.rtop = mem->cc.cbot += num_refs * sizeof(ref);
	  }
	else
	  {	/* Use a new run.  We have to be careful, because */
		/* the new run might stay in the same chunk, create a new */
		/* current chunk, or allocate a separate large chunk. */
		byte *top = mem->cc.cbot;
		chunk_t *pcc = mem->pcc;
		obj = gs_alloc_struct_array((gs_memory_t *)mem, num_refs + 1,
					    ref, &st_refs, cname);
		if ( obj == 0 )
		  return_error(e_VMerror);
		if ( mem->cc.cbot == top )
		  {	/* We allocated a separate large chunk. */
			/* Set the terminating ref now. */
			/* alloc_obj made a special check to set has_refs. */
			ref *end = (ref *)obj + num_refs;
			make_mark(end);
		  }
		else
		  {	if ( mem->pcc == pcc )
				alloc_close_refs(&mem->cc);
			else if ( pcc != 0 )
				alloc_close_refs(pcc);
			mem->cc.rcur = (obj_header_t *)obj;
			mem->cc.rtop = mem->cc.cbot;
			mem->cc.has_refs = true;
		  }
	  }
	make_array(parr, attrs | mem->space, num_refs, obj);
	return 0;
}

/* Resize an array of refs.  Currently this is only implemented */
/* for shrinking, not for growing. */
int
gs_resize_ref_array(gs_ref_memory_t *mem, ref *parr,
  uint new_num_refs, client_name_t cname)
{	uint old_num_refs = r_size(parr);
	uint diff;
	ref *obj = parr->value.refs;
	if ( new_num_refs > old_num_refs || !r_has_type(parr, t_array) )
	  return_error(e_Fatal);
	diff = old_num_refs - new_num_refs;
	/* Check for LIFO.  See gs_free_ref_array for more details. */
	if ( mem->cc.rtop == mem->cc.cbot &&
	     (byte *)(obj + (old_num_refs + 1)) == mem->cc.rtop
	   )
	  {	/* Shorten the refs object. */
		if_debug4('A', "[a%d:-/ ]%s(%u) 0x%lx\n", imem_space_index,
			  client_name_string(cname), diff, (ulong)obj);
		mem->cc.cbot = mem->cc.rtop -= diff * sizeof(ref);
	  }
	else
	  {	/* Punt. */
		if_debug4('A', "[a%d:~/ ]%s(%u) 0x%lx\n", imem_space_index,
			  client_name_string(cname), diff, (ulong)obj);
		imem->freed_lost += diff * sizeof(ref);
	  }
	r_set_size(parr, new_num_refs);
	return 0;
}

/* Deallocate an array of refs.  Only do this if LIFO. */
void
gs_free_ref_array(gs_ref_memory_t *mem, ref *parr, client_name_t cname)
{	uint num_refs = r_size(parr);
	ref *obj = parr->value.refs;
	/*
	 * Compute the storage size of the array, and check for LIFO
	 * freeing.  Note that the array might be packed;
	 * for the moment, if it's anything but a t_array, punt.
	 * The +1s are for the extra ref for the GC.
	 */
	if ( r_has_type(parr, t_array) &&
	      mem->cc.rtop == mem->cc.cbot &&
	      (byte *)(obj + (num_refs + 1)) == mem->cc.rtop
	   )
	  {	if ( (obj_header_t *)obj == mem->cc.rcur )
		  {	/* Deallocate the entire refs object. */
			/* Make sure the size is correct. */
			((obj_header_t *)obj)[-1].o_size =
			  mem->cc.rtop - (byte *)obj;
			gs_free_object((gs_memory_t *)mem, obj, cname);
			mem->cc.rcur = 0;
			mem->cc.rtop = 0;
		  }
		else
		  {	/* Deallocate it at the end of the refs object. */
			if_debug4('A', "[a%d:-$ ]%s(%u) 0x%lx\n",
				  imem_space_index, client_name_string(cname),
				  num_refs, (ulong)obj);
			mem->cc.rtop = mem->cc.cbot = (byte *)(obj + 1);
		  }
	  }
	else
	  {	/* Punt. */
		if_debug4('A', "[a%d:~$ ]%s(%u) 0x%lx\n", imem_space_index,
			  client_name_string(cname), num_refs, (ulong)obj);
		imem->freed_lost += num_refs * sizeof(ref);
	  }
}

/* Allocate a string ref. */
int
gs_alloc_string_ref(gs_ref_memory_t *mem, ref *psref,
  uint attrs, uint nbytes, client_name_t cname)
{	byte *str = gs_alloc_string((gs_memory_t *)mem, nbytes, cname);
	if ( str == 0 )
	  return_error(e_VMerror);
	make_string(psref, attrs | mem->space, nbytes, str);
	return 0;
}

/* ================ Chunks ================ */

public_st_chunk();

/* Insert a chunk in the chain.  This is exported for the GC and for */
/* the forget_save operation. */
void
alloc_link_chunk(chunk_t *cp, gs_ref_memory_t *mem)
{	byte *cdata = cp->cbase;
	chunk_t *icp;
	chunk_t *prev;
	for ( icp = mem->cfirst; icp != 0 && ptr_ge(cdata, icp->ctop);
	      icp = icp->cnext
	    )
		;
	cp->cnext = icp;
	if ( icp == 0 )			/* add at end of chain */
	{	prev = imem->clast;
		imem->clast = cp;
	}
	else				/* insert before icp */
	{	prev = icp->cprev;
		icp->cprev = cp;
	}
	cp->cprev = prev;
	if ( prev == 0 )
		imem->cfirst = cp;
	else
		prev->cnext= cp;
	if ( imem->pcc != 0 )
	{	imem->cc.cnext = imem->pcc->cnext;
		imem->cc.cprev = imem->pcc->cprev;
	}
}

/* Allocate a chunk.  If we would exceed MaxLocalVM (if relevant), */
/* or if we would exceed the VMThreshold and psignal is NULL, */
/* return 0; if we would exceed the VMThreshold but psignal is valid, */
/* just set the signal and return successfully. */
private chunk_t *
alloc_add_chunk(gs_ref_memory_t *mem, ulong csize, bool has_strings,
  client_name_t cname)
{	gs_memory_t *parent = mem->parent;
	chunk_t *cp = gs_alloc_struct(parent, chunk_t, &st_chunk, cname);
	byte *cdata;
	/* If csize is larger than max_uint, */
	/* we have to fake it using gs_alloc_byte_array. */
	ulong elt_size = csize;
	uint num_elts = 1;
	if ( mem->allocated >= mem->limit )
	  {	mem->gc_status.requested += csize;
		if ( mem->limit >= mem->gc_status.max_vm ||
		     mem->gc_status.psignal == 0
		   )
			return 0;
		*mem->gc_status.psignal = mem->gc_status.signal_value;
	  }
	while ( (uint)elt_size != elt_size )
	  elt_size = (elt_size + 1) >> 1,
	  num_elts <<= 1;
	cdata = gs_alloc_byte_array(parent, num_elts, elt_size, cname);
	if ( cp == 0 || cdata == 0 )
	{	gs_free_object(parent, cdata, cname);
		gs_free_object(parent, cp, cname);
		mem->gc_status.requested = csize;
		return 0;
	}
	alloc_init_chunk(cp, cdata, cdata + csize, has_strings, (chunk_t *)0);
	alloc_link_chunk(cp, mem);
	mem->allocated += gs_object_size(parent, cdata) + sizeof(chunk_t);
	return cp;
}

/* Initialize the pointers in a chunk.  This is exported for save/restore. */
/* The bottom pointer must be aligned,  but the top pointer need not */
/* be aligned. */
void
alloc_init_chunk(chunk_t *cp, byte *bot, byte *top, bool has_strings,
  chunk_t *outer)
{	byte *cdata = bot;
	if ( outer != 0 )
	  outer->inner_count++;
	cp->chead = (chunk_head_t *)cdata;
	cdata += sizeof(chunk_head_t);
	cp->cbot = cp->cbase = cdata;
	cp->cend = top;
	cp->rcur = 0;
	cp->rtop = 0;
	cp->outer = outer;
	cp->inner_count = 0;
	cp->has_refs = false;
	cp->sbase = cdata;
	if ( has_strings && top - cdata >= string_space_quantum + sizeof(long) - 1)
	{	/*
		 * We allocate a large enough string marking and reloc table
		 * to cover the entire chunk.
		 */
		uint nquanta = string_space_quanta(top - cdata);
		cp->climit = cdata + nquanta * string_data_quantum;
		cp->smark = cp->climit;
		cp->smark_size = string_quanta_mark_size(nquanta);
		cp->sreloc = (ushort *)(cp->smark + cp->smark_size);
	}
	else
	{	/* No strings, don't need the string GC tables. */
		cp->climit = cp->cend;
		cp->smark = 0;
		cp->smark_size = 0;
		cp->sreloc = 0;
	}
	cp->ctop = cp->climit;
}

/* Close up the current chunk. */
/* This is exported for save/restore and the GC. */
void
alloc_close_chunk(gs_ref_memory_t *mem)
{	alloc_close_refs(&mem->cc);
	if ( mem->pcc != 0 )
	{	*mem->pcc = mem->cc;
		if_debug_chunk('a', "[a]closing chunk", mem->pcc);
	}
}

/* Reopen the current chunk after a GC or restore. */
void
alloc_open_chunk(gs_ref_memory_t *mem)
{	if ( mem->pcc != 0 )
	{	mem->cc = *mem->pcc;
		if_debug_chunk('a', "[a]opening chunk", mem->pcc);
	}
}

/* Remove a chunk from the chain.  This is exported for the GC. */
void
alloc_unlink_chunk(chunk_t *cp, gs_ref_memory_t *mem)
{	if ( cp->cprev == 0 )
		mem->cfirst = cp->cnext;
	else
		cp->cprev->cnext = cp->cnext;
	if ( cp->cnext == 0 )
		mem->clast = cp->cprev;
	else
		cp->cnext->cprev = cp->cprev;
	if ( mem->pcc != 0 )
	{	mem->cc.cnext = mem->pcc->cnext;
		mem->cc.cprev = mem->pcc->cprev;
		if ( mem->pcc == cp )
		{	mem->pcc = 0;
			mem->cc.cbot = mem->cc.ctop = 0;
		}
	}
}

/* Free a chunk.  This is exported for save/restore and for the GC. */
void
alloc_free_chunk(chunk_t *cp, gs_ref_memory_t *mem)
{	gs_memory_t *parent = mem->parent;
	alloc_unlink_chunk(cp, mem);
	if ( cp->outer == 0 )
	  {	byte *cdata = (byte *)cp->chead;
		mem->allocated -= gs_object_size(parent, cdata);
		gs_free_object(parent, cdata, "alloc_free_chunk(data)");
	  }
	else
	  cp->outer->inner_count--;
	mem->allocated -= gs_object_size(parent, cp);
	gs_free_object(parent, cp, "alloc_free_chunk(chunk struct)");
}

/* Close the refs object of a chunk. */
private void
alloc_close_refs(chunk_t *cp)
{	obj_header_t *rcur = cp->rcur;
	if_debug3('a', "[a]closing refs 0x%lx: 0x%lx - 0x%lx\n",
		  (ulong)cp, (ulong)rcur, (ulong)cp->rtop);
	if ( rcur != 0 )
	{	rcur[-1].o_size = cp->rtop - (byte *)rcur;
		/* Create the final ref, reserved for the GC. */
		make_mark((ref *)cp->rtop - 1);
	}
}

/* Find the chunk for a pointer. */
/* Note that this only searches the current save level. */
/* Since a given save level can't contain both a chunk and an inner chunk */
/* of that chunk, we can stop when is_within_chunk succeeds, and just test */
/* is_in_inner_chunk then. */
bool
chunk_locate_ptr(const void *vptr, chunk_locator_t *clp)
{	register chunk_t *cp = clp->cp;
	if ( cp == 0 )
	{	cp = clp->memory->cfirst;
		if ( cp == 0 )
			return false;
	}
#define ptr (const byte *)vptr
	if ( ptr_lt(ptr, cp->cbase) )
	{	do
		{	cp = cp->cprev;
			if ( cp == 0 )
				return false;
		}
		while ( ptr_lt(ptr, cp->cbase) );
		if ( ptr_ge(ptr, cp->cend) )
		  return false;
	}
	else
	{	while ( ptr_ge(ptr, cp->cend) )
		{	cp = cp->cnext;
			if ( cp == 0 )
				return false;
		}
		if ( ptr_lt(ptr, cp->cbase) )
		  return false;
	}
	clp->cp = cp;
	return !ptr_is_in_inner_chunk(ptr, cp);
#undef ptr
}
