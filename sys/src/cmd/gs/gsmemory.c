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

/* gsmemory.c */
/* Generic allocator support for Ghostscript library */
#include "gx.h"
#include "malloc_.h"
#include "memory_.h"
#include "gsrefct.h"		/* to check prototype */
#include "gsstruct.h"		/* ditto */

/* Define the fill patterns for unallocated memory. */
byte gs_alloc_fill_alloc = 0xa1;
byte gs_alloc_fill_collected = 0xc1;
byte gs_alloc_fill_free = 0xf1;

/* The 'structure' type descriptor for bytes. */
gs_public_st_simple(st_bytes, byte, "bytes");

/* ------ Heap allocator ------ */

/*
 * An implementation of Ghostscript's memory manager interface
 * that works directly with the C heap.  We keep track of all allocated
 * blocks so we can free them at cleanup time.
 */
private gs_memory_proc_alloc_bytes(gs_heap_alloc_bytes);
private gs_memory_proc_alloc_struct(gs_heap_alloc_struct);
private gs_memory_proc_alloc_byte_array(gs_heap_alloc_byte_array);
private gs_memory_proc_alloc_struct_array(gs_heap_alloc_struct_array);
private gs_memory_proc_object_size(gs_heap_object_size);
private gs_memory_proc_object_type(gs_heap_object_type);
private gs_memory_proc_free_object(gs_heap_free_object);
private gs_memory_proc_alloc_string(gs_heap_alloc_string);
private gs_memory_proc_resize_string(gs_heap_resize_string);
private gs_memory_proc_free_string(gs_heap_free_string);
private gs_memory_proc_register_root(gs_heap_register_root);
private gs_memory_proc_unregister_root(gs_heap_unregister_root);
private gs_memory_proc_status(gs_heap_status);
gs_memory_t gs_memory_default = {
	{	gs_heap_alloc_bytes,
		gs_heap_alloc_struct,
		gs_heap_alloc_byte_array,
		gs_heap_alloc_struct_array,
		gs_heap_object_size,
		gs_heap_object_type,
		gs_heap_free_object,
		gs_heap_alloc_string,
		gs_heap_resize_string,
		gs_heap_free_string,
		gs_heap_register_root,
		gs_heap_unregister_root,
		gs_heap_status
	}
};
/* We must make sure that malloc_blocks leave the block aligned. */
typedef struct malloc_block_s malloc_block;
#define malloc_block_data\
	malloc_block *next;\
	uint size;\
	gs_memory_type_ptr_t type;\
	client_name_t cname
struct malloc_block_data_s { malloc_block_data; };
struct malloc_block_s {
	malloc_block_data;
/* ANSI C does not allow zero-size arrays, so we need the following */
/* unnecessary and wasteful workaround: */
#define _npad (-sizeof(struct malloc_block_data_s) & 7)
	byte _pad[(_npad == 0 ? 8 : _npad)];	/* pad to double */
#undef _npad
};

private malloc_block *malloc_list;
private long malloc_used;

/* Initialize the malloc heap. */
private long heap_available(P0());
void
gs_malloc_init(void)
{	malloc_list = 0;
	malloc_used = 0;
}
/* Estimate the amount of available memory by probing with mallocs. */
/* We may under-estimate by a lot, but that's better than winding up with */
/* a seriously inflated address space. */
/* This is quite a hack! */
#define max_malloc_probes 20
#define malloc_probe_size 64000
private long
heap_available(void)
{	long avail = 0;
	void *probes[max_malloc_probes];
	uint n;
	for ( n = 0; n < max_malloc_probes; n++ )
	  { if ( (probes[n] = malloc(malloc_probe_size)) == 0 )
	      break;
	    if_debug2('a', "[a]heap_available probe[%d]=0x%lx\n",
		      n, (ulong)probes[n]);
	    avail += malloc_probe_size;
	  }
	while ( n )
	  free(probes[--n]);
	return avail;
}

/* Allocate various kinds of blocks. */
private byte *
gs_heap_alloc_bytes(gs_memory_t *mem, uint size, client_name_t cname)
{	byte *ptr;
	const char *msg = "";
	if ( size > max_uint - sizeof(malloc_block)
	   )
	   {	/* Can't represent the size in a uint! */
		msg = "too large for size_t";
		ptr = 0;
	   }
	else
	   {	ptr = (byte *)malloc(size + sizeof(malloc_block));
		if ( ptr == 0 )
			msg = "failed";
		else
		   {	malloc_block *bp = (malloc_block *)ptr;
			bp->next = malloc_list;
			bp->size = size;
			bp->type = &st_bytes;
			bp->cname = cname;
			malloc_list = bp;
			msg = "OK";
			ptr = (byte *)(bp + 1);
			if ( gs_alloc_debug )
			  { /* Clear the block in an attempt to track down */
			    /* uninitialized data errors. */
			    memset(ptr, gs_alloc_fill_alloc, size);
			  }
			malloc_used += size + sizeof(malloc_block);
		   }
	   }
	if ( gs_debug_c('a') || !*msg )
		dprintf4("[a]gs_malloc(%s)(%u) = 0x%lx: %s\n",
			 client_name_string(cname),
			 size, (ulong)ptr, msg);
	return ptr;
}
private void *
gs_heap_alloc_struct(gs_memory_t *mem, gs_memory_type_ptr_t pstype,
  client_name_t cname)
{	void *ptr = gs_heap_alloc_bytes(mem, gs_struct_type_size(pstype), cname);
	if ( ptr == 0 )
	  return 0;
	((malloc_block *)ptr)[-1].type = pstype;
	return ptr;
}
private byte *
gs_heap_alloc_byte_array(gs_memory_t *mem, uint num_elements, uint elt_size,
  client_name_t cname)
{	ulong lsize = (ulong)num_elements * elt_size;
	if ( lsize != (uint)lsize )
	  return 0;
	return gs_heap_alloc_bytes(mem, (uint)lsize, cname);
}
private void *
gs_heap_alloc_struct_array(gs_memory_t *mem, uint num_elements,
  gs_memory_type_ptr_t pstype, client_name_t cname)
{	void *ptr = gs_heap_alloc_byte_array(mem, num_elements, gs_struct_type_size(pstype), cname);
	if ( ptr == 0 )
	  return 0;
	((malloc_block *)ptr)[-1].type = pstype;
	return ptr;
}
private uint
gs_heap_object_size(gs_memory_t *mem, const void *ptr)
{	return ((malloc_block *)ptr)[-1].size;
}
private gs_memory_type_ptr_t
gs_heap_object_type(gs_memory_t *mem, const void *ptr)
{	return ((malloc_block *)ptr)[-1].type;
}
private void
gs_heap_free_object(gs_memory_t *mem, void *ptr, client_name_t cname)
{	malloc_block *bp = malloc_list;
	if_debug2('a', "[a]gs_free(%s)(0x%lx)\n",
		  client_name_string(cname), (ulong)ptr);
	if ( ptr == 0 )
		return;
	if ( ptr == bp + 1 )
	  { malloc_list = bp->next;
	    malloc_used -= bp->size + sizeof(malloc_block);
	    if ( gs_alloc_debug )
	      memset((char *)(bp + 1), gs_alloc_fill_free, bp->size);
	    free(bp);
	  }
	else
	  { malloc_block *np;
	    for ( ; (np = bp->next) != 0; bp = np )
	      { if ( ptr == np + 1 )
		  { bp->next = np->next;
		    malloc_used -= np->size + sizeof(malloc_block);
		    if ( gs_alloc_debug )
		      memset((char *)(np + 1), gs_alloc_fill_free, np->size);
		    free(np);
		    return;
		  }
	      }
	    lprintf2("%s: free 0x%lx not found!\n",
		     client_name_string(cname), (ulong)ptr);
	    free((char *)((malloc_block *)ptr - 1));
	  }
}
private byte *
gs_heap_alloc_string(gs_memory_t *mem, uint nbytes, client_name_t cname)
{	return gs_heap_alloc_bytes(mem, nbytes, cname);
}
private byte *
gs_heap_resize_string(gs_memory_t *mem, byte *data, uint old_num, uint new_num,
  client_name_t cname)
{	byte *ptr = gs_heap_alloc_string(mem, new_num, cname);
	if ( ptr == 0 )
		return 0;
	memcpy(ptr, data, min(old_num, new_num));
	gs_heap_free_string(mem, data, old_num, cname);
	return ptr;
}
private void
gs_heap_free_string(gs_memory_t *mem, byte *data, uint nbytes,
  client_name_t cname)
{	/****** SHOULD CHECK SIZE IF DEBUGGING ******/
	gs_heap_free_object(mem, (char *)data, cname);
}
private void
gs_heap_register_root(gs_memory_t *mem, gs_gc_root_t *rp, gs_ptr_type_t ptype,
  void **up, client_name_t cname)
{
}
private void
gs_heap_unregister_root(gs_memory_t *mem, gs_gc_root_t *rp,
  client_name_t cname)
{
}
private void
gs_heap_status(gs_memory_t *mem, gs_memory_status_t *pstat)
{	pstat->allocated = malloc_used + heap_available();
	pstat->used = malloc_used;
}

/* Release all malloc'ed blocks. */
void
gs_malloc_release(void)
{	malloc_block *bp = malloc_list;
	malloc_block *np;
	for ( ; bp != 0; bp = np )
	   {	np = bp->next;
		if ( gs_alloc_debug )
		  memset((char *)(bp + 1), gs_alloc_fill_free, bp->size);
		free(bp);
	   }
	malloc_list = 0;
	malloc_used = 0;
}

/* ------ Other memory management ------ */

/* No-op pointer enumeration procedure */
gs_ptr_type_t
gs_no_struct_enum_ptrs(void *vptr, uint size, uint index, void **pep)
{	return 0;
}

/* No-op pointer relocation procedure */
void
gs_no_struct_reloc_ptrs(void *vptr, uint size, gc_state_t *gcst)
{
}

/* Get the size of a structure from the descriptor. */
uint
gs_struct_type_size(gs_memory_type_ptr_t pstype)
{	return pstype->ssize;
}

/* Get the name of a structure from the descriptor. */
struct_name_t
gs_struct_type_name(gs_memory_type_ptr_t pstype)
{	return pstype->sname;
}

/* Normal freeing routine for reference-counted structures. */
void
rc_free_struct_only(gs_memory_t *mem, char *data, client_name_t cname)
{	gs_free_object(mem, data, cname);
}
