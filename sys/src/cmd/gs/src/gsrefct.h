/* Copyright (C) 1993, 1994, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsrefct.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Reference counting definitions */

#ifndef gsrefct_INCLUDED
#  define gsrefct_INCLUDED

/*
 * A reference-counted object must include the following header:
 *      rc_header rc;
 * The header need not be the first element of the object.
 *
 * Reference-counted objects have a freeing procedure that gets called when
 * the reference count reaches zero.  In retrospect, we probably should have
 * used finalization for this, but it's too difficult to change now.
 * Because of the interaction between these two features, the freeing
 * procedure for reference-counted objects that do use finalization must
 * free the object itself first, before decrementing the reference counts
 * of referenced objects (which of course requires saving pointers to those
 * objects before freeing the containing object).
 */
typedef struct rc_header_s rc_header;
struct rc_header_s {
    long ref_count;
    gs_memory_t *memory;
#define rc_free_proc(proc)\
  void proc(P3(gs_memory_t *, void *, client_name_t))
    rc_free_proc((*free));
};

#ifdef DEBUG
void rc_trace_init_free(P2(const void *vp, const rc_header *prc));
void rc_trace_free_struct(P3(const void *vp, const rc_header *prc,
			     client_name_t cname));
void rc_trace_increment(P2(const void *vp, const rc_header *prc));
void rc_trace_adjust(P3(const void *vp, const rc_header *prc, int delta));
#define IF_RC_DEBUG(call) if (gs_debug_c('^')) dlputs(""), call
#else
#define IF_RC_DEBUG(call) DO_NOTHING
#endif

/* ------ Allocate/free ------ */

rc_free_proc(rc_free_struct_only);
/* rc_init[_free] is only used to initialize stack-allocated structures. */
#define rc_init_free(vp, mem, rcinit, proc)\
  BEGIN\
    (vp)->rc.ref_count = rcinit;\
    (vp)->rc.memory = mem;\
    (vp)->rc.free = proc;\
    IF_RC_DEBUG(rc_trace_init_free(vp, &(vp)->rc));\
  END
#define rc_init(vp, mem, rcinit)\
  rc_init_free(vp, mem, rcinit, rc_free_struct_only)

#define rc_alloc_struct_n(vp, typ, pstyp, mem, errstat, cname, rcinit)\
  BEGIN\
    if ( ((vp) = gs_alloc_struct(mem, typ, pstyp, cname)) == 0 ) {\
      errstat;\
    } else {\
      rc_init(vp, mem, rcinit);\
    }\
  END
#define rc_alloc_struct_0(vp, typ, pstype, mem, errstat, cname)\
  rc_alloc_struct_n(vp, typ, pstype, mem, errstat, cname, 0)
#define rc_alloc_struct_1(vp, typ, pstype, mem, errstat, cname)\
  rc_alloc_struct_n(vp, typ, pstype, mem, errstat, cname, 1)

#define rc_free_struct(vp, cname)\
  BEGIN\
    IF_RC_DEBUG(rc_trace_free_struct(vp, &(vp)->rc, cname));\
    (vp)->rc.free((vp)->rc.memory, (void *)(vp), cname);\
  END

/* ------ Reference counting ------ */

/* Increment a reference count. */
#define RC_DO_INCREMENT(vp)\
  BEGIN\
    (vp)->rc.ref_count++;\
    IF_RC_DEBUG(rc_trace_increment(vp, &(vp)->rc));\
  END
#define rc_increment(vp)\
  BEGIN\
    if (vp) RC_DO_INCREMENT(vp);\
  END

/* Increment a reference count, allocating the structure if necessary. */
#define rc_allocate_struct(vp, typ, pstype, mem, errstat, cname)\
  BEGIN\
    if (vp)\
      RC_DO_INCREMENT(vp);\
    else\
      rc_alloc_struct_1(vp, typ, pstype, mem, errstat, cname);\
  END

/* Guarantee that a structure is allocated and is not shared. */
#define RC_DO_ADJUST(vp, delta)\
  BEGIN\
    IF_RC_DEBUG(rc_trace_adjust(vp, &(vp)->rc, delta));\
    (vp)->rc.ref_count += (delta);\
  END
#define rc_unshare_struct(vp, typ, pstype, mem, errstat, cname)\
  BEGIN\
    if ( (vp) == 0 || (vp)->rc.ref_count > 1 || (vp)->rc.memory != (mem) ) {\
      typ *new;\
      rc_alloc_struct_1(new, typ, pstype, mem, errstat, cname);\
      if ( vp ) RC_DO_ADJUST(vp, -1);\
      (vp) = new;\
    }\
  END

/* Adjust a reference count either up or down. */
#ifdef DEBUG
#  define rc_check_(vp)\
     BEGIN\
       if (gs_debug_c('?') && (vp)->rc.ref_count < 0)\
	 lprintf2("0x%lx has ref_count of %ld!\n", (ulong)(vp),\
		  (vp)->rc.ref_count);\
     END
#else
#  define rc_check_(vp) DO_NOTHING
#endif
#define rc_adjust_(vp, delta, cname, body)\
  BEGIN\
    if (vp) {\
      RC_DO_ADJUST(vp, delta);\
      if (!(vp)->rc.ref_count) {\
	rc_free_struct(vp, cname);\
	body;\
      } else\
	rc_check_(vp);\
    }\
  END
#define rc_adjust(vp, delta, cname)\
  rc_adjust_(vp, delta, cname, (vp) = 0)
#define rc_adjust_only(vp, delta, cname)\
  rc_adjust_(vp, delta, cname, DO_NOTHING)
#define rc_adjust_const(vp, delta, cname)\
  rc_adjust_only(vp, delta, cname)
#define rc_decrement(vp, cname)\
  rc_adjust(vp, -1, cname)
#define rc_decrement_only(vp, cname)\
  rc_adjust_only(vp, -1, cname)

/*
 * Assign a pointer, adjusting reference counts.  vpfrom might be a local
 * variable with a copy of the last reference to the object, and freeing
 * vpto might decrement the object's reference count and cause it to be
 * freed (incorrectly); for that reason, we do the increment first.
 */
#define rc_assign(vpto, vpfrom, cname)\
  BEGIN\
    if ((vpto) != (vpfrom)) {\
      rc_increment(vpfrom);\
      rc_decrement_only(vpto, cname);\
      (vpto) = (vpfrom);\
    }\
  END
/*
 * Adjust reference counts for assigning a pointer,
 * but don't do the assignment.  We use this before assigning
 * an entire structure containing reference-counted pointers.
 */
#define rc_pre_assign(vpto, vpfrom, cname)\
  BEGIN\
    if ((vpto) != (vpfrom)) {\
      rc_increment(vpfrom);\
      rc_decrement_only(vpto, cname);\
    }\
  END

#endif /* gsrefct_INCLUDED */
