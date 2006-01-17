/* Copyright (C) 1995, 1996, 1999, 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxobj.h,v 1.7 2005/10/12 10:45:21 leonardo Exp $ */
/* Memory manager implementation structures for Ghostscript */

#ifndef gxobj_INCLUDED
#  define gxobj_INCLUDED

#include "gxbitmap.h"

#ifdef DEBUG
#define IGC_PTR_STABILITY_CHECK 0
#else
#define IGC_PTR_STABILITY_CHECK 0
#endif

/* ================ Objects ================ */

/*
 * Object headers have the form:
	-l- -mark/back-
	-size-
	-type/reloc-
 * l (aLone) is a single bit.  Mark/back is 1 bit shorter than a uint.  We
 * round the header size up to the next multiple of the most severe
 * alignment restriction (4 or 8 bytes).
 *
 * The mark/back field is used for the mark during the marking phase of
 * garbage collection, and for a back pointer value during the compaction
 * phase.  Since we want to be able to collect local VM independently of
 * global VM, we need two different distinguished mark values:
 *      - For local objects that have not been traced and should be freed
 *      (compacted out), we use 1...11 in the mark field (o_unmarked).
 *      - For global objects that have not been traced but should be kept,
 *      we use 1...10 in the mark field (o_untraced).
 * Note that neither of these values is a possible real relocation value.
 *
 * The back pointer's meaning depends on whether the object is
 * free (unmarked) or in use (marked):
 *      - In free objects, the back pointer is an offset from the object
 * header back to a chunk_head_t structure that contains the location
 * to which all the data in this chunk will get moved; the reloc field
 * contains the amount by which the following run of useful objects
 * will be relocated downwards.
 *      - In useful objects, the back pointer is an offset from the object
 * back to the previous free object; the reloc field is not used (it
 * overlays the type field).
 * These two cases can be distinguished when scanning a chunk linearly,
 * but when simply examining an object via a pointer, the chunk pointer
 * is also needed.
 */
#define obj_flag_bits 1
#define obj_mb_bits (arch_sizeof_int * 8 - obj_flag_bits)
#define o_unmarked (((uint)1 << obj_mb_bits) - 1)
#define o_set_unmarked(pp)\
  ((pp)->o_smark = o_unmarked)
#define o_is_unmarked(pp)\
  ((pp)->o_smark == o_unmarked)
#define o_untraced (((uint)1 << obj_mb_bits) - 2)
#define o_set_untraced(pp)\
  ((pp)->o_smark = o_untraced)
#define o_is_untraced(pp)\
  ((pp)->o_smark == o_untraced)
#define o_marked 0
#define o_mark(pp)\
  ((pp)->o_smark = o_marked)
#define obj_back_shift obj_flag_bits
#define obj_back_scale (1 << obj_back_shift)
typedef struct obj_header_data_s {
    union _f {
	struct _h {
	    unsigned alone:1;
	} h;
	struct _m {
	    unsigned _:1, smark:obj_mb_bits;
	} m;
	struct _b {
	    unsigned _:1, back:obj_mb_bits;
	} b;
    } f;
    uint size;
    union _t {
	gs_memory_type_ptr_t type;
	uint reloc;
    } t;
#   if IGC_PTR_STABILITY_CHECK
    unsigned space_id:3; /* r_space_bits + 1 bit for "instability". */
#   endif
} obj_header_data_t;

/*
 * Define the alignment modulus for aligned objects.  We assume all
 * alignment values are powers of 2; we can avoid nested 'max'es that way.
 * The final | is because back pointer values are divided by obj_back_scale,
 * so objects must be aligned at least 0 mod obj_back_scale.
 *
 * Note: OBJECTS ARE NOT GUARANTEED to be aligned any more strictly than
 * required by the hardware, regardless of the value of obj_align_mod.
 * See gsmemraw.h for more information about this.
 */
#define obj_align_mod\
  (((arch_align_memory_mod - 1) |\
    (align_bitmap_mod - 1) |\
    (obj_back_scale - 1)) + 1)
/* The only possible values for obj_align_mod are 4, 8, or 16.... */
#if obj_align_mod == 4
#  define log2_obj_align_mod 2
#else
#if obj_align_mod == 8
#  define log2_obj_align_mod 3
#else
#if obj_align_mod == 16
#  define log2_obj_align_mod 4
#endif
#endif
#endif
#define obj_align_mask (obj_align_mod-1)
#define obj_align_round(siz)\
  (uint)(((siz) + obj_align_mask) & -obj_align_mod)
#define obj_size_round(siz)\
  obj_align_round((siz) + sizeof(obj_header_t))

/* Define the real object header type, taking alignment into account. */
struct obj_header_s {		/* must be a struct because of forward reference */
    union _d {
	obj_header_data_t o;
	byte _pad[ROUND_UP(sizeof(obj_header_data_t), obj_align_mod)];
    }
    d;
};

/* Define some reasonable abbreviations for the fields. */
#define o_alone d.o.f.h.alone
#define o_back d.o.f.b.back
#define o_smark d.o.f.m.smark
#define o_size d.o.size
#define o_type d.o.t.type
#define o_nreloc d.o.t.reloc

/*
 * The macros for getting the sizes of objects all take pointers to
 * the object header, for use when scanning storage linearly.
 */
#define pre_obj_contents_size(pp)\
  ((pp)->o_size)

#define pre_obj_rounded_size(pp)\
  obj_size_round(pre_obj_contents_size(pp))
#define pre_obj_next(pp)\
  ((obj_header_t *)((byte *)(pp) + obj_align_round(\
    pre_obj_contents_size(pp) + sizeof(obj_header_t) )))

/*
 * Define the header that free objects point back to when relocating.
 * Every chunk, including inner chunks, has one of these.
 */
typedef struct chunk_head_s {
    byte *dest;			/* destination for objects */
#if obj_align_mod > arch_sizeof_ptr
    byte *_pad[obj_align_mod / arch_sizeof_ptr - 1];
#endif
    obj_header_t free;		/* header for a free object, */
    /* in case the first real object */
    /* is in use */
} chunk_head_t;

#endif /* gxobj_INCLUDED */
