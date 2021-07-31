/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gsmemraw.h,v 1.2 2000/09/19 19:00:30 lpd Exp $ */
/* Client interface for "raw memory" allocator */

/* Initial version 02/03/1998 by John Desrosiers (soho@crl.com) */
/* Completely rewritten 6/26/1998 by L. Peter Deutsch <ghost@aladdin.com> */

#ifndef gsmemraw_INCLUDED
#  define gsmemraw_INCLUDED

/*
 * This interface provides minimal memory allocation and freeing capability.
 * It is meant to be used for "wholesale" allocation of blocks -- typically,
 * but not only, via malloc -- which are then divided up into "retail"
 * objects.  However, since it is a subset (superclass) of the "retail"
 * interface defined in gsmemory.h, retail allocators implement it as
 * well, and in fact the malloc interface defined in gsmalloc.h is used for
 * both wholesale and retail allocation.
 */

/*
 * Define the structure for reporting memory manager statistics.
 */
typedef struct gs_memory_status_s {
    /*
     * "Allocated" space is the total amount of space acquired from
     * the parent of the memory manager.  It includes space used for
     * allocated data, space available for allocation, and overhead.
     */
    ulong allocated;
    /*
     * "Used" space is the amount of space used by allocated data
     * plus overhead.
     */
    ulong used;
} gs_memory_status_t;

/* Define the abstract type for the memory manager. */
typedef struct gs_raw_memory_s gs_raw_memory_t;

/* Define the procedures for raw memory management.  Memory managers have no
 * standard constructor: each implementation defines its own, and is
 * responsible for calling its superclass' initialization code first.
 * Similarly, each implementation's destructor (release) must first take
 * care of its own cleanup and then call the superclass' release.
 */

		/*
		 * Allocate bytes.  The bytes are always aligned maximally
		 * if the processor requires alignment.
		 *
		 * Note that the object memory level can allocate bytes as
		 * either movable or immovable: raw memory blocks are
		 * always immovable.
		 */

#define gs_memory_t_proc_alloc_bytes(proc, mem_t)\
  byte *proc(P3(mem_t *mem, uint nbytes, client_name_t cname))

#define gs_alloc_bytes_immovable(mem, nbytes, cname)\
  ((mem)->procs.alloc_bytes_immovable(mem, nbytes, cname))

		/*
		 * Resize an object to a new number of elements.  At the raw
		 * memory level, the "element" is a byte; for object memory
		 * (gsmemory.h), the object may be an an array of either
		 * bytes or structures.  The new size may be larger than,
		 * the same as, or smaller than the old.  If the new size is
		 * the same as the old, resize_object returns the same
		 * object; otherwise, it preserves the first min(old_size,
		 * new_size) bytes of the object's contents.
		 */

#define gs_memory_t_proc_resize_object(proc, mem_t)\
  void *proc(P4(mem_t *mem, void *obj, uint new_num_elements,\
		client_name_t cname))

#define gs_resize_object(mem, obj, newn, cname)\
  ((mem)->procs.resize_object(mem, obj, newn, cname))

		/*
		 * Free an object (at the object memory level, this includes
		 * everything except strings).  Note: data == 0 must be
		 * allowed, and must be a no-op.
		 */

#define gs_memory_t_proc_free_object(proc, mem_t)\
  void proc(P3(mem_t *mem, void *data, client_name_t cname))

#define gs_free_object(mem, data, cname)\
  ((mem)->procs.free_object(mem, data, cname))

		/*
		 * Report status (assigned, used).
		 */

#define gs_memory_t_proc_status(proc, mem_t)\
  void proc(P2(mem_t *mem, gs_memory_status_t *status))

#define gs_memory_status(mem, pst)\
  ((mem)->procs.status(mem, pst))

		/*
		 * Return the stable allocator for this allocator.  The
		 * stable allocator allocates from the same heap and in
		 * the same VM space, but is not subject to save and restore.
		 * (It is the client's responsibility to avoid creating
		 * dangling pointers.)
		 *
		 * Note that the stable allocator may be the same allocator
		 * as this one.
		 */

#define gs_memory_t_proc_stable(proc, mem_t)\
  mem_t *proc(P1(mem_t *mem))

#define gs_memory_stable(mem)\
  ((mem)->procs.stable(mem))

		/*
		 * Free one or more of: data memory acquired by the allocator
		 * (FREE_ALL_DATA), overhead structures other than the
		 * allocator itself (FREE_ALL_STRUCTURES), and the allocator
		 * itself (FREE_ALL_ALLOCATOR).  Note that this requires
		 * allocators to keep track of all the memory they have ever
		 * acquired, and where they acquired it.  Note that this
		 * operation propagates to the stable allocator (if
		 * different).
		 */

#define FREE_ALL_DATA 1
#define FREE_ALL_STRUCTURES 2
#define FREE_ALL_ALLOCATOR 4
#define FREE_ALL_EVERYTHING\
  (FREE_ALL_DATA | FREE_ALL_STRUCTURES | FREE_ALL_ALLOCATOR)

#define gs_memory_t_proc_free_all(proc, mem_t)\
  void proc(P3(mem_t *mem, uint free_mask, client_name_t cname))

#define gs_memory_free_all(mem, free_mask, cname)\
  ((mem)->procs.free_all(mem, free_mask, cname))
/* Backward compatibility */
#define gs_free_all(mem)\
  gs_memory_free_all(mem, FREE_ALL_DATA, "(free_all)")

		/*
		 * Consolidate free space.  This may be used as part of (or
		 * as an alternative to) garbage collection, or before
		 * giving up on an attempt to allocate.
		 */

#define gs_memory_t_proc_consolidate_free(proc, mem_t)\
  void proc(P1(mem_t *mem))

#define gs_consolidate_free(mem)\
  ((mem)->procs.consolidate_free(mem))

/* Define the members of the procedure structure. */
#define gs_raw_memory_procs(mem_t)\
    gs_memory_t_proc_alloc_bytes((*alloc_bytes_immovable), mem_t);\
    gs_memory_t_proc_resize_object((*resize_object), mem_t);\
    gs_memory_t_proc_free_object((*free_object), mem_t);\
    gs_memory_t_proc_stable((*stable), mem_t);\
    gs_memory_t_proc_status((*status), mem_t);\
    gs_memory_t_proc_free_all((*free_all), mem_t);\
    gs_memory_t_proc_consolidate_free((*consolidate_free), mem_t)

/*
 * Define an abstract raw-memory allocator instance.
 * Subclasses may have additional state.
 */
typedef struct gs_raw_memory_procs_s {
    gs_raw_memory_procs(gs_raw_memory_t);
} gs_raw_memory_procs_t;
struct gs_raw_memory_s {
    gs_raw_memory_t *stable_memory;	/* cache the stable allocator */
    gs_raw_memory_procs_t procs;
};

#endif /* gsmemraw_INCLUDED */
