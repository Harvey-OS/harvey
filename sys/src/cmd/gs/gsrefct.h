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

/* gsrefct.h */
/* Reference counting definitions */

#ifndef gsrefct_INCLUDED
#  define gsrefct_INCLUDED

/*
 * A reference-counted object must include the following header:
 *	rc_header rc;
 * The header need not be the first element of the object.
 */
typedef struct rc_header_s rc_header;
struct rc_header_s {
	long ref_count;
#define rc_free_proc(proc)\
  void proc(P3(gs_memory_t *, char *, client_name_t))
	rc_free_proc((*free));
};

/* ------ Allocate/free ------ */

rc_free_proc(rc_free_struct_only);
#define rc_alloc_struct_n(vp, typ, pstyp, mem, errstat, cname, rcinit)\
   {	if ( (vp = gs_alloc_struct(mem, typ, pstyp, cname)) == 0 )\
	   errstat;\
	vp->rc.ref_count = rcinit;\
	vp->rc.free = rc_free_struct_only;\
   }
#define rc_alloc_struct_0(vp, typ, pstype, mem, errstat, cname)\
  rc_alloc_struct_n(vp, typ, pstype, mem, errstat, cname, 0)
#define rc_alloc_struct_1(vp, typ, pstype, mem, errstat, cname)\
  rc_alloc_struct_n(vp, typ, pstype, mem, errstat, cname, 1)

#define rc_free_struct(vp, mem, cname)\
  (*vp->rc.free)(mem, (char *)vp, cname)

/* ------ Reference counting ------ */

/* Increment a reference count. */
#define rc_increment(vp)\
  if ( vp != 0 ) vp->rc.ref_count++

/* Increment a reference count, allocating the structure if necessary. */
#define rc_allocate_struct(vp, typ, pstype, mem, errstat, cname)\
  if ( vp != 0 )\
	vp->rc.ref_count++;\
  else\
   {	rc_alloc_struct_1(vp, typ, pstype, mem, errstat, cname);\
   }

/* Guarantee that a structure is allocated and is not shared. */
#define rc_unshare_struct(vp, typ, pstype, mem, errstat, cname)\
  if ( vp == 0 || vp->rc.ref_count > 1 )\
   {	typ *new;\
	rc_alloc_struct_1(new, typ, pstype, mem, errstat, cname);\
	if ( vp ) vp->rc.ref_count--;\
	vp = new;\
   }

/* Adjust a reference count either up or down. */
#define rc_adjust_(vp, delta, mem, cname, body)\
  if ( vp != 0 && !(vp->rc.ref_count += delta) )\
   {	rc_free_struct(vp, mem, cname);\
	body;\
   }
#define rc_adjust(vp, delta, mem, cname)\
  rc_adjust_(vp, delta, mem, cname, vp = 0)
#define rc_adjust_only(vp, delta, mem, cname)\
  rc_adjust_(vp, delta, mem, cname, DO_NOTHING)
#define rc_adjust_const(vp, delta, mem, cname)\
  rc_adjust_only(vp, delta, mem, cname)
#define rc_decrement(vp, mem, cname)\
  rc_adjust(vp, -1, mem, cname)
#define rc_decrement_only(vp, mem, cname)\
  rc_adjust_only(vp, -1, mem, cname)

/* Assign a pointer, adjusting reference counts. */
#define rc_assign(vpto, vpfrom, mem, cname)\
  if ( vpto != vpfrom )\
   {	rc_decrement_only(vpto, mem, cname);\
	vpto = vpfrom;\
	rc_increment(vpto);\
   }
/* Adjust reference counts for assigning a pointer, */
/* but don't do the assignment.  We use this before assigning */
/* an entire structure containing reference-counted pointers. */
#define rc_pre_assign(vpto, vpfrom, mem, cname)\
  if ( vpto != vpfrom )\
   {	rc_decrement_only(vpto, mem, cname);\
	rc_increment(vpfrom);\
   }

#endif					/* gsrefct_INCLUDED */
