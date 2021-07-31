/* Copyright (C) 1991, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: iutil.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Interface to interpreter utilities */
/* Requires imemory.h, ostack.h */

#ifndef iutil_INCLUDED
#  define iutil_INCLUDED

/* ------ Object utilities ------ */

/* Copy refs from one place to another. */
/* (If we are copying to the stack, we can just use memcpy.) */
void refcpy_to_new(P4(ref * to, const ref * from, uint size,
		      gs_dual_memory_t *dmem));
int refcpy_to_old(P6(ref * aref, uint index, const ref * from, uint size,
		     gs_dual_memory_t *dmem, client_name_t cname));

/*
 * Fill an array with nulls.
 * For backward compatibility, we define the procedure with a new name.
 */
void refset_null_new(P3(ref * to, uint size, uint new_mask));
#define refset_null(to, size) refset_null_new(to, size, ialloc_new_mask)

/* Compare two objects for equality. */
bool obj_eq(P2(const ref *, const ref *));

/* Compare two objects for identity. */
/* (This is not a standard PostScript concept.) */
bool obj_ident_eq(P2(const ref *, const ref *));

/*
 * Set *pchars and *plen to point to the data of a name or string, and
 * return 0.  If the object isn't a name or string, return e_typecheck.
 * If the object is a string without read access, return e_invalidaccess.
 */
int obj_string_data(P3(const ref *op, const byte **pchars, uint *plen));

/*
 * Create a printable representation of an object, a la cvs and =
 * (full_print = 0), == (full_print = 1), or === (full_print = 2).  Return 0
 * if OK, 1 if the destination wasn't large enough, e_invalidaccess if the
 * object's contents weren't readable.  If the return value is 0 or 1,
 * *prlen contains the amount of data returned.  start_pos is the starting
 * output position -- the first start_pos bytes of output are discarded.
 *
 * The mem argument is only used for getting the type of structures,
 * not for allocating; if it is NULL and full_print != 0, structures will
 * print as --(struct)--.
 */
#define CVP_MAX_STRING 200  /* strings are truncated here if full_print = 1 */
int obj_cvp(P7(const ref * op, byte *str, uint len, uint * prlen,
	       int full_print, uint start_pos, gs_memory_t *mem));

/*
 * Create a printable representation of an object, a la cvs and =.  Return 0
 * if OK, e_rangecheck if the destination wasn't large enough,
 * e_invalidaccess if the object's contents weren't readable.  If pchars !=
 * NULL, then if the object was a string or name, store a pointer to its
 * characters in *pchars even if it was too large; otherwise, set *pchars =
 * str.  In any case, store the length in *prlen.
 *
 * obj_cvs is different from obj_cvp in two respects: if the printed
 * representation is too large, it returns e_rangecheck rather than 1;
 * and it can return a pointer to the data for names and strings, like
 * obj_string_data.
 */
int obj_cvs(P5(const ref * op, byte * str, uint len, uint * prlen,
	       const byte ** pchars));

/* Get an element from an array (packed or not). */
int array_get(P3(const ref *, long, ref *));

/* Get an element from a packed array. */
/* (This works for ordinary arrays too.) */
/* Source and destination are allowed to overlap if the source is packed, */
/* or if they are identical. */
void packed_get(P2(const ref_packed *, ref *));

/* Check to make sure an interval contains no object references */
/* to a space younger than a given one. */
/* Return 0 or e_invalidaccess. */
int refs_check_space(P3(const ref * refs, uint size, uint space));

/* ------ String utilities ------ */

/* Convert a C string to a string object. */
int string_to_ref(P4(const char *, ref *, gs_ref_memory_t *, client_name_t));

/* Convert a string object to a C string. */
/* Return 0 iff the buffer can't be allocated. */
char *ref_to_string(P3(const ref *, gs_memory_t *, client_name_t));

/* ------ Operand utilities ------ */

/* Get N numeric operands from the stack or an array. */
int num_params(P3(const ref *, int, double *));

/* float_params can lose accuracy for large integers. */
int float_params(P3(const ref *, int, float *));

/* Get a single real parameter. */
/* The only possible error is e_typecheck. */
int real_param(P2(const ref *, double *));

/* float_param can lose accuracy for large integers. */
int float_param(P2(const ref *, float *));

/* Get an integer parameter in a given range. */
int int_param(P3(const ref *, int, int *));

/* Make real values on the stack. */
/* Return e_limitcheck for infinities or double->float overflow. */
int make_reals(P3(ref *, const double *, int));
int make_floats(P3(ref *, const float *, int));

/* Define the gs_matrix type if necessary. */
#ifndef gs_matrix_DEFINED
#  define gs_matrix_DEFINED
typedef struct gs_matrix_s gs_matrix;
#endif

/* Read a matrix operand. */
int read_matrix(P2(const ref *, gs_matrix *));

/* Write a matrix operand. */
/* If dmem is NULL, the array is guaranteed newly allocated in imem. */
/* If dmem is not NULL, imem is ignored. */
int write_matrix_in(P4(ref *op, const gs_matrix *pmat, gs_dual_memory_t *dmem,
		       gs_ref_memory_t *imem));
#define write_matrix_new(op, pmat, imem)\
  write_matrix_in(op, pmat, NULL, imem)
#define write_matrix(op, pmat)\
  write_matrix_in(op, pmat, idmemory, NULL)

#endif /* iutil_INCLUDED */
