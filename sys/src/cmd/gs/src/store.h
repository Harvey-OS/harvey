/* Copyright (C) 1989, 1995, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: store.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Assignment-related macros */

#ifndef store_INCLUDED
#  define store_INCLUDED

#include "ialloc.h"		/* for imemory masks & checks */
#include "idosave.h"

/*
 * Macros for storing a ref.  We use macros for storing into objects,
 * since the storage manager needs to be able to track stores for
 * save/restore and also for global/local checking.
 * We also use macros for other ref assignments, because (as it happens)
 * Turbo C generates pretty awful code for doing this.
 *
 * There are three cases that we need to distinguish:
 *      - Storing to a stack (no special action);
 *      - Storing into a newly created object (set l_new);
 *      - Storing into a slot of an existing object (check l_new in
 *          old value, set in new value).
 * The macros are called
 *      <make/store><new_type><case>(place_to_store, new_value)
 * where <case> is nothing for storing to the stack, _new for storing into
 * a new object, and _old for storing into an existing object.
 * (The _old macros also take a client name for tracing and debugging.)
 * <new_type> and new_value are chosen from the following alternatives:
 *      ref_assign      POINTER TO arbitrary ref
 *      make_t          type (only for null and mark)
 *      make_tv         type, value field name, value
 *                        (only for scalars, which don't have attributes)
 *      make_tav        type, attributes, value field name, value
 *      make_tasv       type, attributes, size, value field name, value
 * There are also specialized make_ macros for specific types:
 *      make_array, make_int, make_real, make_bool, make_false, make_true,
 *      make_mark, make_null, make_oper, make_[const_]string, make_struct.
 * Not all of the specialized make_ macros have _new and _old variants.
 *
 * For _tav and _tasv, we must store the value first, because sometimes
 * it depends on the contents of the place being stored into.
 *
 * Note that for composite objects (dictionary, file, array, string, device,
 * struct), we must set a_foreign if the contents are allocated statically
 * (e.g., for constant C strings) or not by the Ghostscript allocator
 * (e.g., with malloc).
 */

/*
 * Define the most efficient ref assignment macro for the platform.
 */
/*
 * Assigning the components individually is fastest on Turbo C,
 * and on Watcom C when one or both of the addresses are
 * already known or in a register.
 */
#define ref_assign_inline(pto,pfrom)\
	((pto)->value = (pfrom)->value,\
	 (pto)->tas = (pfrom)->tas)
#ifdef __TURBOC__
	/*
	 * Move the data in two 32-bit chunks, because
	 * otherwise the compiler calls SCOPY@.
	 * The cast to void is to discourage the compiler from
	 * wanting to deliver the value of the expression.
	 */
#  define ref_assign(pto,pfrom)\
	discard(ref_assign_inline(pto, pfrom))
#else
	/*
	 * Trust the compiler and hope for the best.
	 * The MIPS compiler doesn't like the cast to void.
	 */
#  define ref_assign(pto,pfrom)\
	(*(pto) = *(pfrom))
#endif

#define ialloc_new_mask (idmemory->new_mask)
     /*
      * The mmem argument may be either a gs_dual_memory_t or a
      * gs_ref_memory_t, since it is only used for accessing the masks.
      */
#define ref_saving_in(mmem)\
  ((mmem)->new_mask != 0)
#define ref_must_save_in(mmem,pto)\
  ((r_type_attrs(pto) & (mmem)->test_mask) == 0)
#define ref_must_save(pto) ref_must_save_in(idmemory, pto)
#define ref_do_save_in(mem, pcont, pto, cname)\
  alloc_save_change_in(mem, pcont, (ref_packed *)(pto), cname)
#define ref_do_save(pcont, pto, cname)\
  alloc_save_change(idmemory, pcont, (ref_packed *)(pto), cname)
#define ref_save_in(mem, pcont, pto, cname)\
  discard((ref_must_save_in(mem, pto) ?\
	   ref_do_save_in(mem, pcont, pto, cname) : 0))
#define ref_save(pcont, pto, cname)\
  discard((ref_must_save(pto) ? ref_do_save(pcont, pto, cname) : 0))
#define ref_mark_new_in(mmem,pto)\
  ((pto)->tas.type_attrs |= (mmem)->new_mask)
#define ref_mark_new(pto) ref_mark_new_in(idmemory, pto)
#define ref_assign_new_in(mem,pto,pfrom)\
  discard((ref_assign(pto,pfrom), ref_mark_new_in(mem,pto)))
#define ref_assign_new(pto,pfrom)\
  discard((ref_assign(pto,pfrom), ref_mark_new(pto)))
#define ref_assign_new_inline(pto,pfrom)\
  discard((ref_assign_inline(pto,pfrom), ref_mark_new(pto)))
#define ref_assign_old_in(mem,pcont,pto,pfrom,cname)\
  (ref_save_in(mem,pcont,pto,cname), ref_assign_new_in(mem,pto,pfrom))
#define ref_assign_old(pcont,pto,pfrom,cname)\
  (ref_save(pcont,pto,cname), ref_assign_new(pto,pfrom))
#define ref_assign_old_inline(pcont,pto,pfrom,cname)\
  (ref_save(pcont,pto,cname), ref_assign_new_inline(pto,pfrom))
/* ref_mark_old is only needed in very unusual situations, namely, */
/* when we want to do a ref_save just before a save instead of */
/* when the actual assignment occurs. */
#define ref_mark_old(pto) ((pto)->tas.type_attrs &= ~ialloc_new_mask)

/* Define macros for conditionally clearing the parts of a ref */
/* that aren't being set to anything useful. */

#ifdef DEBUG
#  define and_fill_s(pref)\
    , (gs_debug['$'] ? r_set_size(pref, 0xfeed) : 0)
/*
 * The following nonsense avoids compiler warnings about signed/unsigned
 * integer constants.
 */
#define DEADBEEF ((int)(((uint)0xdead << 16) | 0xbeef))
#  define and_fill_sv(pref)\
    , (gs_debug['$'] ? (r_set_size(pref, 0xfeed),\
			(pref)->value.intval = DEADBEEF) : 0)
#else /* !DEBUG */
#  define and_fill_s(pref)	/* */
#  define and_fill_sv(pref)	/* */
#endif

/* make_t must set the attributes to 0 to clear a_local! */
#define make_ta(pref,newtype,newattrs)\
  (r_set_type_attrs(pref, newtype, newattrs) and_fill_sv(pref))
#define make_t(pref,newtype)\
  make_ta(pref, newtype, 0)
#define make_t_new_in(mem,pref,newtype)\
  make_ta(pref, newtype, imemory_new_mask(mem))
#define make_t_new(pref,newtype)\
  make_ta(pref, newtype, ialloc_new_mask)
#define make_t_old_in(mem,pcont,pref,newtype,cname)\
  (ref_save_in(mem,pcont,pref,cname), make_t_new_in(mem,pref,newtype))
#define make_t_old(pcont,pref,newtype,cname)\
  (ref_save(pcont,pref,cname), make_t_new(pref,newtype))

#define make_tav(pref,newtype,newattrs,valfield,newvalue)\
  ((pref)->value.valfield = (newvalue),\
   r_set_type_attrs(pref, newtype, newattrs)\
   and_fill_s(pref))
#define make_tav_new(pref,t,a,vf,v)\
  make_tav(pref,t,(a)|ialloc_new_mask,vf,v)
#define make_tav_old(pcont,pref,t,a,vf,v,cname)\
  (ref_save(pcont,pref,cname), make_tav_new(pref,t,a,vf,v))

#define make_tv(pref,newtype,valfield,newvalue)\
  make_tav(pref,newtype,0,valfield,newvalue)
#define make_tv_new(pref,t,vf,v)\
  make_tav_new(pref,t,0,vf,v)
#define make_tv_old(pcont,pref,t,vf,v,cname)\
  make_tav_old(pcont,pref,t,0,vf,v,cname)

#define make_tasv(pref,newtype,newattrs,newsize,valfield,newvalue)\
  ((pref)->value.valfield = (newvalue),\
   r_set_type_attrs(pref, newtype, newattrs),\
   r_set_size(pref, newsize))
#define make_tasv_new(pref,t,a,s,vf,v)\
  make_tasv(pref,t,(a)|ialloc_new_mask,s,vf,v)
#define make_tasv_old(pcont,pref,t,a,s,vf,v,cname)\
  (ref_save(pcont,pref,cname), make_tasv_new(pref,t,a,s,vf,v))

/* Type-specific constructor macros for scalar (non-composite) types */

#define make_bool(pref,bval)\
  make_tv(pref, t_boolean, boolval, bval)
#define make_false(pref)\
  make_bool(pref, 0)
#define make_true(pref)\
  make_bool(pref, 1)

#define make_int(pref,ival)\
  make_tv(pref, t_integer, intval, ival)
#define make_int_new(pref,ival)\
  make_tv_new(pref, t_integer, intval, ival)

#define make_mark(pref)\
  make_t(pref, t_mark)

#define make_null(pref)\
  make_t(pref, t_null)
#define make_null_new(pref)\
  make_t_new(pref, t_null)
#define make_null_old_in(mem,pcont,pref,cname)\
  make_t_old_in(mem, pcont, pref, t_null, cname)
#define make_null_old(pcont,pref,cname)\
  make_t_old(pcont, pref, t_null, cname)

#define make_oper(pref,opidx,proc)\
  make_tasv(pref, t_operator, a_executable, opidx, opproc, proc)
#define make_oper_new(pref,opidx,proc)\
  make_tasv_new(pref, t_operator, a_executable, opidx, opproc, proc)

#define make_real(pref,rval)\
  make_tv(pref, t_real, realval, rval)
#define make_real_new(pref,rval)\
  make_tv_new(pref, t_real, realval, rval)

/* Type-specific constructor macros for composite types */

/* For composite types, the a_space field is relevant; however, */
/* as noted in ivmspace.h, a value of 0 designates the most static space, */
/* so for making empty composites, a space value of 0 is appropriate. */

#define make_array(pref,attrs,size,elts)\
  make_tasv(pref, t_array, attrs, size, refs, elts)
#define make_array_new(pref,attrs,size,elts)\
  make_tasv_new(pref, t_array, attrs, size, refs, elts)
#define make_const_array(pref,attrs,size,elts)\
  make_tasv(pref, t_array, attrs, size, const_refs, elts)
#define make_empty_array(pref,attrs)\
  make_array(pref, attrs, 0, (ref *)NULL)
#define make_empty_const_array(pref,attrs)\
  make_const_array(pref, attrs, 0, (const ref *)NULL)

#define make_string(pref,attrs,size,chars)\
  make_tasv(pref, t_string, attrs, size, bytes, chars)
#define make_const_string(pref,attrs,size,chars)\
  make_tasv(pref, t_string, attrs, size, const_bytes, chars)
#define make_empty_string(pref,attrs)\
  make_string(pref, attrs, 0, (byte *)NULL)
#define make_empty_const_string(pref,attrs)\
  make_const_string(pref, attrs, 0, (const byte *)NULL)

#define make_struct(pref,attrs,ptr)\
  make_tav(pref, t_struct, attrs, pstruct, (obj_header_t *)(ptr))
#define make_struct_new(pref,attrs,ptr)\
  make_tav_new(pref, t_struct, attrs, pstruct, (obj_header_t *)(ptr))

#define make_astruct(pref,attrs,ptr)\
  make_tav(pref, t_astruct, attrs, pstruct, (obj_header_t *)(ptr))
#define make_astruct_new(pref,attrs,ptr)\
  make_tav_new(pref, t_astruct, attrs, pstruct, (obj_header_t *)(ptr))

#endif /* store_INCLUDED */
