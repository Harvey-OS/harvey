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

/* gsmemory.h */
/* Client interface for memory allocation */

#ifndef gsmemory_INCLUDED
#  define gsmemory_INCLUDED

/*
 * Define the (opaque) type for a structure descriptor.
 */
typedef struct gs_memory_struct_type_s gs_memory_struct_type_t;
typedef const gs_memory_struct_type_t _ds *gs_memory_type_ptr_t;

	/* Accessors for structure types. */

typedef client_name_t struct_name_t;

/* Get the size of a structure from the descriptor. */
uint gs_struct_type_size(P1(gs_memory_type_ptr_t));

/* Get the name of a structure from the descriptor. */
struct_name_t gs_struct_type_name(P1(gs_memory_type_ptr_t));
#define gs_struct_type_name_string(styp)\
  ((const char *)gs_struct_type_name(styp))

/* An opaque type for the garbage collector state. */
/* We need this because it is passed to pointer implementation procedures. */
typedef struct gc_state_s gc_state_t;

/*
 * A pointer type defines how to mark the referent of the pointer.
 * We define it here so that we can define ptr_struct_type,
 * which is needed so we can define gs_register_struct_root.
 */
typedef struct gs_ptr_procs_s {

		/* Unmark the referent of a pointer. */

#define ptr_proc_unmark(proc)\
  void proc(P2(void *, gc_state_t *))
	ptr_proc_unmark((*unmark));

		/* Mark the referent of a pointer. */
		/* Return true iff it was unmarked before. */

#define ptr_proc_mark(proc)\
  bool proc(P2(void *, gc_state_t *))
	ptr_proc_mark((*mark));

		/* Relocate a pointer. */

#define ptr_proc_reloc(proc, typ)\
  typ *proc(P2(typ *, gc_state_t *))
	ptr_proc_reloc((*reloc), void);

} gs_ptr_procs_t;
typedef const gs_ptr_procs_t _ds *gs_ptr_type_t;

/* Define the pointer type for ordinary structure pointers. */
extern const gs_ptr_procs_t ptr_struct_procs;
#define ptr_struct_type (&ptr_struct_procs)

/* Define the pointer types for a pointer to a gs_[const_]string. */
extern const gs_ptr_procs_t ptr_string_procs;
#define ptr_string_type (&ptr_string_procs)
extern const gs_ptr_procs_t ptr_const_string_procs;
#define ptr_const_string_type (&ptr_const_string_procs)

/* Register a structure root. */
#define gs_register_struct_root(mem, root, pp, cname)\
  gs_register_root(mem, root, ptr_struct_type, pp, cname)

/*
 * Define the type for a GC root.
 */
typedef struct gs_gc_root_s gs_gc_root_t;
struct gs_gc_root_s {
	gs_gc_root_t *next;
	gs_ptr_type_t ptype;
	void **p;
};

/* Print a root debugging message. */
#define if_debug_root(c, msg, rp)\
  if_debug4(c, "%s 0x%lx: 0x%lx -> 0x%lx\n",\
	    msg, (ulong)(rp), (ulong)(rp)->p, (ulong)*(rp)->p)

/* Define the type for memory manager statistics. */
typedef struct gs_memory_status_s {
	ulong allocated;
	ulong used;
} gs_memory_status_t;

/*
 * Define the memory manager procedural interface.
 */
struct gs_memory_s;
typedef struct gs_memory_s gs_memory_t;
typedef struct gs_memory_procs_s {

		/* Allocate bytes.  The bytes are always aligned */
		/* maximally if the processor requires alignment. */

#define gs_memory_proc_alloc_bytes(proc)\
  byte *proc(P3(gs_memory_t *mem, uint nbytes, client_name_t cname))
#define gs_alloc_bytes(mem, nbytes, cname)\
  (*(mem)->procs.alloc_bytes)(mem, nbytes, cname)
	gs_memory_proc_alloc_bytes((*alloc_bytes));

		/* Allocate a structure. */

#define gs_memory_proc_alloc_struct(proc)\
  void *proc(P3(gs_memory_t *mem, gs_memory_type_ptr_t pstype,\
    client_name_t cname))
#define gs_alloc_struct(mem, typ, pstype, cname)\
  (typ *)(*(mem)->procs.alloc_struct)(mem, pstype, cname)
	gs_memory_proc_alloc_struct((*alloc_struct));

		/* Allocate an array of bytes. */

#define gs_memory_proc_alloc_byte_array(proc)\
  byte *proc(P4(gs_memory_t *mem, uint num_elements, uint elt_size,\
    client_name_t cname))
#define gs_alloc_byte_array(mem, nelts, esize, cname)\
  (*(mem)->procs.alloc_byte_array)(mem, nelts, esize, cname)
	gs_memory_proc_alloc_byte_array((*alloc_byte_array));

		/* Allocate an array of structures. */

#define gs_memory_proc_alloc_struct_array(proc)\
  void *proc(P4(gs_memory_t *mem, uint num_elements,\
    gs_memory_type_ptr_t pstype, client_name_t cname))
#define gs_alloc_struct_array(mem, nelts, typ, pstype, cname)\
  (typ *)(*(mem)->procs.alloc_struct_array)(mem, nelts, pstype, cname)
	gs_memory_proc_alloc_struct_array((*alloc_struct_array));

		/* Get the size of an object (anything except a string). */

#define gs_memory_proc_object_size(proc)\
  uint proc(P2(gs_memory_t *mem, const void *obj))
#define gs_object_size(mem, obj)\
  (*(mem)->procs.object_size)(mem, obj)
	gs_memory_proc_object_size((*object_size));

		/* Get the type of an object (anything except a string). */
		/* The value returned for byte objects is useful */
		/* only for printing. */

#define gs_memory_proc_object_type(proc)\
  gs_memory_type_ptr_t proc(P2(gs_memory_t *mem, const void *obj))
#define gs_object_type(mem, obj)\
  (*(mem)->procs.object_type)(mem, obj)
	gs_memory_proc_object_type((*object_type));

		/* Free an object (anything except a string). */
		/* Note: data == 0 must be allowed, and must be a no-op. */

#define gs_memory_proc_free_object(proc)\
  void proc(P3(gs_memory_t *mem, void *data, client_name_t cname))
#define gs_free_object(mem, data, cname)\
  (*(mem)->procs.free_object)(mem, data, cname)
	gs_memory_proc_free_object((*free_object));

		/* Allocate a string (unaligned bytes). */

#define gs_memory_proc_alloc_string(proc)\
  byte *proc(P3(gs_memory_t *mem, uint nbytes, client_name_t cname))
#define gs_alloc_string(mem, nbytes, cname)\
  (*(mem)->procs.alloc_string)(mem, nbytes, cname)
	gs_memory_proc_alloc_string((*alloc_string));

		/* Resize a string. */

#define gs_memory_proc_resize_string(proc)\
  byte *proc(P5(gs_memory_t *mem, byte *data, uint old_num, uint new_num,\
    client_name_t cname))
#define gs_resize_string(mem, data, oldn, newn, cname)\
  (*(mem)->procs.resize_string)(mem, data, oldn, newn, cname)
	gs_memory_proc_resize_string((*resize_string));

		/* Free a string. */

#define gs_memory_proc_free_string(proc)\
  void proc(P4(gs_memory_t *mem, byte *data, uint nbytes,\
    client_name_t cname))
#define gs_free_string(mem, data, nbytes, cname)\
  (*(mem)->procs.free_string)(mem, data, nbytes, cname)
	gs_memory_proc_free_string((*free_string));

		/* Register a root for the garbage collector. */

#define gs_memory_proc_register_root(proc)\
  void proc(P5(gs_memory_t *mem, gs_gc_root_t *root, gs_ptr_type_t ptype,\
    void **pp, client_name_t cname))
#define gs_register_root(mem, root, ptype, pp, cname)\
  (*(mem)->procs.register_root)(mem, root, ptype, pp, cname)
	gs_memory_proc_register_root((*register_root));

		/* Unregister a root. */

#define gs_memory_proc_unregister_root(proc)\
  void proc(P3(gs_memory_t *mem, gs_gc_root_t *root, client_name_t cname))
#define gs_unregister_root(mem, root, cname)\
  (*(mem)->procs.unregister_root)(mem, root, cname)
	gs_memory_proc_unregister_root((*unregister_root));

		/* Report status (assigned, used) */

#define gs_memory_proc_status(proc)\
  void proc(P2(gs_memory_t *mem, gs_memory_status_t *status))
#define gs_memory_status(mem, pst)\
  (*(mem)->procs.status)(mem, pst)
	gs_memory_proc_status((*status));

} gs_memory_procs_t;

/*
 * An allocator instance.  Subclasses may have state as well.
 */
#define gs_memory_common\
	gs_memory_procs_t procs
struct gs_memory_s {
	gs_memory_common;
};

/*
 * gs_malloc and gs_free are historical artifacts, but we still need
 * a memory manager instance that allocates directly from the C heap.
 */
extern gs_memory_t gs_memory_default;
#define gs_malloc(nelts, esize, cname)\
  (void *)gs_alloc_byte_array(&gs_memory_default, nelts, esize, cname)
#define gs_free(data, nelts, esize, cname)\
  gs_free_object(&gs_memory_default, data, cname)

#endif					/* gsmemory_INCLUDED */
