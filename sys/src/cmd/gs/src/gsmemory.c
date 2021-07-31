/* Copyright (C) 1993, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gsmemory.c,v 1.2 2000/09/19 19:00:30 lpd Exp $ */
/* Generic allocator support */
#include "memory_.h"
#include "gdebug.h"
#include "gstypes.h"
#include "gsmemory.h"
#include "gsmdebug.h"
#include "gsrefct.h"		/* to check prototype */
#include "gsstruct.h"		/* ditto */

/* Define the fill patterns for unallocated memory. */
byte gs_alloc_fill_alloc = 0xa1;
byte gs_alloc_fill_block = 0xb1;
byte gs_alloc_fill_collected = 0xc1;
byte gs_alloc_fill_deleted = 0xd1;
byte gs_alloc_fill_free = 0xf1;

/* A 'structure' type descriptor for free blocks. */
gs_public_st_simple(st_free, byte, "(free)");

/* The 'structure' type descriptor for bytes. */
gs_public_st_simple(st_bytes, byte, "bytes");

/* The structure type descriptor for GC roots. */
public_st_gc_root_t();

/* The descriptors for elements and arrays of const strings. */
private_st_const_string();
public_st_const_string_element();

/* GC procedures for bytestrings */
gs_ptr_type_t
enum_bytestring(enum_ptr_t *pep, const gs_bytestring *pbs)
{
    return (pbs->bytes ? ENUM_OBJ(pbs->bytes) : ENUM_STRING(pbs));
}
gs_ptr_type_t
enum_const_bytestring(enum_ptr_t *pep, const gs_const_bytestring *pbs)
{
    return (pbs->bytes ? ENUM_OBJ(pbs->bytes) : ENUM_CONST_STRING(pbs));
}
void
reloc_bytestring(gs_bytestring *pbs, gc_state_t *gcst)
{
    if (pbs->bytes) {
	byte *bytes = pbs->bytes;
	long offset = pbs->data - bytes;

	pbs->bytes = bytes = RELOC_OBJ(bytes);
	pbs->data = bytes + offset;
    } else
	RELOC_STRING_VAR(*(gs_string *)pbs);
}
void
reloc_const_bytestring(gs_const_bytestring *pbs, gc_state_t *gcst)
{
    if (pbs->bytes) {
	const byte *bytes = pbs->bytes;
	long offset = pbs->data - bytes;

	pbs->bytes = bytes = RELOC_OBJ(bytes);
	pbs->data = bytes + offset;
    } else
	RELOC_CONST_STRING_VAR(*(gs_const_string *)pbs);
}

/* Fill an unoccupied block with a pattern. */
/* Note that the block size may be too large for a single memset. */
void
gs_alloc_memset(void *ptr, int /*byte */ fill, ulong lsize)
{
    ulong msize = lsize;
    char *p = ptr;
    int isize;

    for (; msize; msize -= isize, p += isize) {
	isize = min(msize, max_int);
	memset(p, fill, isize);
    }
}

/*
 * Either allocate (if obj == 0) or resize (if obj != 0) a structure array.
 * If obj != 0, pstype is used only for checking (in DEBUG configurations).
 */
void *
gs_resize_struct_array(gs_memory_t *mem, void *obj, uint num_elements,
		       gs_memory_type_ptr_t pstype, client_name_t cname)
{
    if (obj == 0)
	return gs_alloc_struct_array(mem, num_elements, void, pstype, cname);
#ifdef DEBUG
    if (gs_object_type(mem, obj) != pstype) {
	lprintf3("resize_struct_array 0x%lx, type was 0x%lx, expected 0x%lx!\n",
		 (ulong)obj, (ulong)gs_object_type(mem, obj), (ulong)pstype);
	return 0;
    }
#endif
    return gs_resize_object(mem, obj, num_elements, cname);
}

/* Allocate a structure using a "raw memory" allocator. */
void *
gs_raw_alloc_struct_immovable(gs_raw_memory_t * rmem,
			      gs_memory_type_ptr_t pstype,
			      client_name_t cname)
{
    return gs_alloc_bytes_immovable(rmem, gs_struct_type_size(pstype), cname);
}

/* No-op freeing procedures */
void
gs_ignore_free_object(gs_memory_t * mem, void *data, client_name_t cname)
{
}
void
gs_ignore_free_string(gs_memory_t * mem, byte * data, uint nbytes,
		      client_name_t cname)
{
}

/* Deconstifying freeing procedures. */
/* These procedures rely on a severely deprecated pun. */
void
gs_free_const_object(gs_memory_t * mem, const void *data, client_name_t cname)
{
    union { const void *r; void *w; } u;

    u.r = data;
    gs_free_object(mem, u.w, cname);
}
void
gs_free_const_string(gs_memory_t * mem, const byte * data, uint nbytes,
		     client_name_t cname)
{
    union { const byte *r; byte *w; } u;

    u.r = data;
    gs_free_string(mem, u.w, nbytes, cname);
}

/* No-op consolidation procedure */
void
gs_ignore_consolidate_free(gs_memory_t *mem)
{
}

/* No-op pointer enumeration procedure */
ENUM_PTRS_BEGIN_PROC(gs_no_struct_enum_ptrs)
{
    return 0;
    ENUM_PTRS_END_PROC
}

/* No-op pointer relocation procedure */
RELOC_PTRS_BEGIN(gs_no_struct_reloc_ptrs)
{
}
RELOC_PTRS_END

/* Get the size of a structure from the descriptor. */
uint
gs_struct_type_size(gs_memory_type_ptr_t pstype)
{
    return pstype->ssize;
}

/* Get the name of a structure from the descriptor. */
struct_name_t
gs_struct_type_name(gs_memory_type_ptr_t pstype)
{
    return pstype->sname;
}

/* Register a structure root. */
int
gs_register_struct_root(gs_memory_t *mem, gs_gc_root_t *root,
			void **pp, client_name_t cname)
{
    return gs_register_root(mem, root, ptr_struct_type, pp, cname);
}

/* ---------------- Reference counting ---------------- */

#ifdef DEBUG

private const char *
rc_object_type_name(const void *vp, const rc_header *prc)
{
    gs_memory_type_ptr_t pstype;

    if (prc->memory == 0)
	return "(unknown)";
    pstype = gs_object_type(prc->memory, vp);
    if (prc->free != rc_free_struct_only) {
	/*
	 * This object might be stack-allocated or have other unusual memory
	 * management properties.  Make some reasonableness checks.
	 * ****** THIS IS A HACK. ******
	 */
	long dist;

	dist = (const char *)&dist - (const char *)vp;
	if (dist < 10000 && dist > -10000)
	    return "(on stack)";
	if ((ulong)pstype < 0x10000 || (long)pstype < 0)
	    return "(anomalous)";
    }
    return client_name_string(gs_struct_type_name(pstype));
}

/* Trace reference count operations. */
void
rc_trace_init_free(const void *vp, const rc_header *prc)
{
    dprintf3("[^]%s 0x%lx init = %ld\n",
	     rc_object_type_name(vp, prc), (ulong)vp, (long)prc->ref_count);
}
void
rc_trace_free_struct(const void *vp, const rc_header *prc, client_name_t cname)
{
    dprintf3("[^]%s 0x%lx => free (%s)\n",
	      rc_object_type_name(vp, prc),
	      (ulong)vp, client_name_string(cname));
}
void
rc_trace_increment(const void *vp, const rc_header *prc)
{
    dprintf3("[^]%s 0x%lx ++ => %ld\n",
	      rc_object_type_name(vp, prc),
	      (ulong)vp, (long)prc->ref_count);
}
void
rc_trace_adjust(const void *vp, const rc_header *prc, int delta)
{
    dprintf4("[^]%s 0x%lx %+d => %ld\n",
	     rc_object_type_name(vp, prc),
	     (ulong)vp, delta, (long)(prc->ref_count + delta));
}

#endif /* DEBUG */

/* Normal freeing routine for reference-counted structures. */
void
rc_free_struct_only(gs_memory_t * mem, void *data, client_name_t cname)
{
    if (mem != 0)
	gs_free_object(mem, data, cname);
}

/* ---------------- Basic-structure GC procedures ---------------- */

/* Enumerate pointers */
ENUM_PTRS_BEGIN_PROC(basic_enum_ptrs)
{
    const gc_struct_data_t *psd = pstype->proc_data;

    if (index < psd->num_ptrs) {
	const gc_ptr_element_t *ppe = &psd->ptrs[index];
	EV_CONST char *pptr = (EV_CONST char *)vptr + ppe->offset;

	switch ((gc_ptr_type_index_t)ppe->type) {
	    case GC_ELT_OBJ:
		return ENUM_OBJ(*(const void *EV_CONST *)pptr);
	    case GC_ELT_STRING:
		return ENUM_STRING((const gs_string *)pptr);
	    case GC_ELT_CONST_STRING:
		return ENUM_CONST_STRING((const gs_const_string *)pptr);
	}
    }
    if (!psd->super_type)
	return 0;
    return ENUM_USING(*(psd->super_type),
		      (EV_CONST void *)
		        ((EV_CONST char *)vptr + psd->super_offset),
		      pstype->ssize, index - psd->num_ptrs);
}
ENUM_PTRS_END_PROC

/* Relocate pointers */
RELOC_PTRS_BEGIN(basic_reloc_ptrs)
{
    const gc_struct_data_t *psd = pstype->proc_data;
    uint i;

    for (i = 0; i < psd->num_ptrs; ++i) {
	const gc_ptr_element_t *ppe = &psd->ptrs[i];
	char *pptr = (char *)vptr + ppe->offset;

	switch ((gc_ptr_type_index_t) ppe->type) {
	    case GC_ELT_OBJ:
		RELOC_OBJ_VAR(*(void **)pptr);
		break;
	    case GC_ELT_STRING:
		RELOC_STRING_VAR(*(gs_string *)pptr);
		break;
	    case GC_ELT_CONST_STRING:
		RELOC_CONST_STRING_VAR(*(gs_const_string *)pptr);
		break;
	}
    }
    if (psd->super_type)
	RELOC_USING(*(psd->super_type),
		      (void *)((char *)vptr + psd->super_offset),
		      pstype->ssize);
} RELOC_PTRS_END
