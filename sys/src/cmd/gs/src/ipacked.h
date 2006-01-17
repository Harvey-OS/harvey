/* Copyright (C) 1991, 1992, 1993, 1994, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ipacked.h,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Packed array format for Ghostscript */

#ifndef ipacked_INCLUDED
#  define ipacked_INCLUDED

/*

   In a packed array, an element may either be a 2-byte ref_packed or a
   full-size ref (8 or 16 bytes).  We carefully arrange the first two bytes,
   which are either an entire ref_packed or the type_attrs member of a ref,
   so that we can distinguish the 2 forms.  The encoding:

   00tttttt exrwsfnm    full-size ref
   010mjjjj jjjjjjjj    executable operator (so bind can work)
   011mvvvv vvvvvvvv    integer (biased by packed_min_intval)
   100m---- --------    (not used)
   101m---- --------    (not used)
   110miiii iiiiiiii    literal name
   111miiii iiiiiiii    executable name

   The m bit is the mark bit for the garbage collector.

   ****** Note for the future: We could get packed tokens into the first-level
   ****** interpreter dispatch by changing to the following representation:

   000ttttt exrwsfnm    full-size ref
   m0100jjj jjjjjjjj    executable operator (so bind can work)
   m0101vvv vvvvvvvv    integer (biased by packed_min_intval)
   m011iiii iiiiiiii    literal name
   m100iiii iiiiiiii    executable name
   m101---- --------    (not used)
   m11----- --------    (not used)

   ****** We aren't going to do this for a while.

   The jjj index of executable operators is either the index of the operator
   in the op_def_table, if the index is less than op_def_count, or the index
   of the definition in the op_array_table (subtracting op_def_count first).

   The iii index of names is the one that the name machinery already
   maintains.  A name whose index is larger than will fit in the packed
   representation must be represented as a full-size ref.

   There are two packed array types, t_mixedarray and t_shortarray.  A
   t_mixedarray can have a mix of packed and full-size elements; a
   t_shortarray has all packed elements.  The 'size' of a packed array is the
   number of elements, not the number of bytes it occupies.

   Packed array elements can be distinguished from full-size elements, so we
   allow the interpreter to simply execute all the different kinds of arrays
   directly.  However, if we really allowed free mixing of packed and
   full-size elements, this could lead to unaligned placement of full-size
   refs; some machines can't handle unaligned accesses of this kind.  To
   guarantee that full-size elements in mixed arrays are always properly
   aligned, if a full-size ref must be aligned at an address which is 0 mod
   N, we convert up to N/2-1 preceding packed elements into full-size
   elements, when creating the array, so that the alignment is preserved.
   The only code this actually affects is in make_packed_array and in the
   code for compacting refs in the garbage collector.

   Note that code in zpacked.c and interp.c knows more about the
   representation of packed elements than the definitions in this file would
   imply.  Read the code carefully if you change the representation.

 */

#define r_packed_type_shift 13
#define r_packed_value_bits 12
typedef enum {
    pt_full_ref = 0,
#define pt_min_packed 2
    pt_executable_operator = 2,
    pt_integer = 3,
    pt_unused1 = 4,
    pt_unused2 = 5,
#define pt_min_name 6
    pt_literal_name = 6,
#define pt_min_exec_name 7
    pt_executable_name = 7
} packed_type;

#define packed_per_ref (sizeof(ref) / sizeof(ref_packed))
#define align_packed_per_ref\
  (arch_align_ref_mod / arch_align_short_mod)
#define pt_tag(pt) ((ref_packed)(pt) << r_packed_type_shift)
#define packed_value_mask ((1 << r_packed_value_bits) - 1)
#define packed_max_value packed_value_mask
#define r_is_packed(rp)  (*(const ref_packed *)(rp) >= pt_tag(pt_min_packed))
/* Names */
#define r_packed_is_name(prp) (*(prp) >= pt_tag(pt_min_name))
#define r_packed_is_exec_name(prp) (*(prp) >= pt_tag(pt_min_exec_name))
#define packed_name_max_index packed_max_value
#define packed_name_index(prp) (*(prp) & packed_value_mask)
/* Integers */
#define packed_min_intval (-(1 << (r_packed_value_bits - 1)))
#define packed_max_intval ((1 << (r_packed_value_bits - 1)) - 1)
#define packed_int_mask packed_value_mask

/* Packed ref marking */
#define lp_mark_shift 12
#define lp_mark (1 << lp_mark_shift)
#define r_has_pmark(rp) (*(rp) & lp_mark)
#define r_set_pmark(rp) (*(rp) |= lp_mark)
#define r_clear_pmark(rp) (*(rp) &= ~lp_mark)
#define r_store_pmark(rp,pm) (*(rp) = (*(rp) & ~lp_mark) | (pm))

/* Advance to the next element in a packed array. */
#define packed_next(prp)\
  (r_is_packed(prp) ? prp + 1 : prp + packed_per_ref)

/* Define the current array packing flag (setpacking/currentpacking) */
/* for operators. */
/* This is a ref so that it can be managed properly by save/restore. */
#define ref_array_packing_container i_ctx_p
#define ref_array_packing (ref_array_packing_container->array_packing)

#endif /* ipacked_INCLUDED */
