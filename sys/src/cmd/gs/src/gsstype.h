/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsstype.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Definition of structure type descriptors and extern_st */

#ifndef gsstype_INCLUDED
#  define gsstype_INCLUDED

/* Define an opaque type for the garbage collector state. */
typedef struct gc_state_s gc_state_t;

/*
 * Define the structure used to return an enumerated pointer.  Ordinary
 * object pointers use only the ptr element; strings also use size.
 */
typedef struct enum_ptr_s {
    const void *ptr;
    uint size;
} enum_ptr_t;

/*
 * The first argument of enum_ptrs procedures formerly was not const *, and
 * EV_CONST was defined as empty.  Unfortunately, changing EV_CONST to const
 * produced many compiler warnings from places that cast this argument to a
 * non-const non-void pointer type.
 */
#define EV_CONST const

/* Define the procedures for structure types. */

		/* Clear the marks of a structure. */

#define struct_proc_clear_marks(proc)\
  void proc(P3(void /*obj_header_t*/ *pre, uint size,\
    const gs_memory_struct_type_t *pstype))

		/* Enumerate the pointers in a structure. */

#define struct_proc_enum_ptrs(proc)\
  gs_ptr_type_t proc(P6(EV_CONST void /*obj_header_t*/ *ptr, uint size,\
    int index, enum_ptr_t *pep, const gs_memory_struct_type_t *pstype,\
    gc_state_t *gcst))

		/* Relocate all the pointers in this structure. */

#define struct_proc_reloc_ptrs(proc)\
  void proc(P4(void /*obj_header_t*/ *ptr, uint size,\
    const gs_memory_struct_type_t *pstype, gc_state_t *gcst))

		/*
		 * Finalize this structure just before freeing it.
		 * Finalization procedures must not allocate or resize
		 * any objects in any space managed by the allocator,
		 * and must not assume that any objects in such spaces
		 * referenced by this structure still exist.  However,
		 * finalization procedures may free such objects, and
		 * may allocate, free, and reference objects allocated
		 * in other ways, such as objects allocated with malloc
		 * by libraries.
		 */

#define struct_proc_finalize(proc)\
  void proc(P1(void /*obj_header_t*/ *ptr))

/*
 * A descriptor for an object (structure) type.
 */
typedef struct struct_shared_procs_s struct_shared_procs_t;

struct gs_memory_struct_type_s {
	uint ssize;
	struct_name_t sname;

	/* ------ Procedures shared among many structure types. ------ */
	/* Note that this pointer is usually 0. */

	const struct_shared_procs_t *shared;

	/* ------ Procedures specific to this structure type. ------ */

	struct_proc_clear_marks((*clear_marks));
	struct_proc_enum_ptrs((*enum_ptrs));
	struct_proc_reloc_ptrs((*reloc_ptrs));
	struct_proc_finalize((*finalize));

	/* A pointer to additional data for the above procedures. */

	const void *proc_data;
};

/*
 * Because of bugs in some compilers' bookkeeping for undefined structure
 * types, any file that uses extern_st must include this file.
 * (If it weren't for these bugs, the definition of extern_st could
 * go in gsmemory.h.)
 */
#define extern_st(st) extern const gs_memory_struct_type_t st

#endif /* gsstype_INCLUDED */
