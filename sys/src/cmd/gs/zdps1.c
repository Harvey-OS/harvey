/* Copyright (C) 1990, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zdps1.c */
/* Display PostScript graphics extensions */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gsmatrix.h"
#include "gspath.h"
#include "gspath2.h"
#include "gsstate.h"
#include "ialloc.h"
#include "igstate.h"
#include "ivmspace.h"
#include "store.h"
#include "stream.h"
#include "ibnum.h"

/* Imported data */
extern op_proc_p zcopy_procs[t_next_index];

/* Forward references */
private int zcopy_gstate(P1(os_ptr));
private int gstate_unshare(P1(os_ptr));

/* Structure descriptors */
public_st_igstate_obj();

/* Initialize by adding an entry for gstates to the `copy' operator. */
/* This is done with a hack -- we know that gstates are the only */
/* t_astruct subtype that implements copy. */
private void
zdps1_init(void)
{	zcopy_procs[t_astruct] = zcopy_gstate;
}

/* ------ Graphics state ------ */

/* <bool> setstrokeadjust - */
int
zsetstrokeadjust(register os_ptr op)
{	check_type(*op, t_boolean);
	gs_setstrokeadjust(igs, op->value.boolval);
	pop(1);
	return 0;
}

/* - currentstrokeadjust <bool> */
int
zcurrentstrokeadjust(register os_ptr op)
{	push(1);
	make_bool(op, gs_currentstrokeadjust(igs));
	return 0;
}

/* <x> <y> sethalftonephase - */
int
zsethalftonephase(register os_ptr op)
{	int code;
	long x, y;
	check_type(op[-1], t_integer);
	check_type(*op, t_integer);
	x = op[-1].value.intval;
	y = op->value.intval;
	if ( x != (int)x || y != (int)y )
		return_error(e_rangecheck);
	code = gs_sethalftonephase(igs, (int)x, (int)y);
	if ( code >= 0 ) pop(2);
	return code;
}

/* - currenthalftonephase <x> <y> */
int
zcurrenthalftonephase(register os_ptr op)
{	gs_int_point phase;
	gs_currenthalftonephase(igs, &phase);
	push(2);
	make_int(op - 1, phase.x);
	make_int(op, phase.y);
	return 0;
}

/* ------ Graphics state objects ------ */

/* Check to make sure that all the elements of a graphics state */
/* can be stored in the given allocation space. */
/****** DOESN'T CHECK THE NON-REFS. ****** */
private int
gstate_check_space(int_gstate *isp, uint space)
{
#define gsref_check(p) store_check_space(space, p)
	int_gstate_map_refs(isp, gsref_check);
#undef gsref_check
	return 0;
}

/* - gstate <gstate> */
int
zgstate(register os_ptr op)
{	gs_state *pnew;
	igstate_obj *pigo;
	int_gstate *isp;
	gs_memory_t *mem;
	int code = gstate_check_space(istate, icurrent_space);
	if ( code < 0 ) return code;
	pigo = ialloc_struct(igstate_obj, &st_igstate_obj, "gstate");
	if ( pigo == 0 )
	  return_error(e_VMerror);
	mem = gs_state_swap_memory(igs, imemory);
	pnew = gs_gstate(igs);
	gs_state_swap_memory(igs, mem);
	if ( pnew == 0 )
	  {	ifree_object(pigo, "gstate");
		return_error(e_VMerror);
	  }
	isp = gs_int_gstate(pnew);
	int_gstate_map_refs(isp, ref_mark_new);
	push(1);
	/*
	 * Since igstate_obj isn't a ref, but only contains a ref,
	 * save won't clear its l_new bit automatically, and
	 * restore won't set it automatically; we have to make sure
	 * this ref is on the changes chain.
	 */
	make_iastruct(op, a_all, pigo);
	make_null(&pigo->gstate);
	ref_save(op, &pigo->gstate, "gstate");
	make_istruct_new(&pigo->gstate, 0, pnew);
	return 0;
}

/* copy for gstates */
private int
zcopy_gstate(register os_ptr op)
{	os_ptr op1 = op - 1;
	gs_state *pgs;
	gs_state *pgs1;
	int_gstate *pistate;
	gs_memory_t *mem;
	int code;
	check_stype(*op, st_igstate_obj);
	check_stype(*op1, st_igstate_obj);
	check_write(*op);
	code = gstate_unshare(op);
	if ( code < 0 )
	  return code;
	pgs = igstate_ptr(op);
	pgs1 = igstate_ptr(op1);
	pistate = gs_int_gstate(pgs);
	code = gstate_check_space(gs_int_gstate(pgs1), r_space(op));
	if ( code < 0 ) return code;
#define gsref_save(p) ref_save(op, p, "copygstate")
	int_gstate_map_refs(pistate, gsref_save);
#undef gsref_save
	mem = gs_state_swap_memory(pgs, imemory);
	code = gs_copygstate(pgs, pgs1);
	gs_state_swap_memory(pgs, mem);
	if ( code < 0 )
	  return code;
	int_gstate_map_refs(pistate, ref_mark_new);
	*op1 = *op;
	pop(1);
	return 0;
}

/* <gstate> currentgstate <gstate> */
int
zcurrentgstate(register os_ptr op)
{	gs_state *pgs;
	int_gstate *pistate;
	int code;
	gs_memory_t *mem;
	check_stype(*op, st_igstate_obj);
	check_write(*op);
	code = gstate_unshare(op);
	if ( code < 0 )
	  return code;
	pgs = igstate_ptr(op);
	pistate = gs_int_gstate(pgs);
	code = gstate_check_space(istate, r_space(op));
	if ( code < 0 ) return code;
#define gsref_save(p) ref_save(op, p, "currentgstate")
	int_gstate_map_refs(pistate, gsref_save);
#undef gsref_save
	mem = gs_state_swap_memory(pgs, imemory);
	code = gs_currentgstate(pgs, igs);
	gs_state_swap_memory(pgs, mem);
	if ( code < 0 )
	  return code;
	int_gstate_map_refs(pistate, ref_mark_new);
	return 0;
}

/* <gstate> setgstate - */
int
zsetgstate(register os_ptr op)
{	int code;
	check_stype(*op, st_igstate_obj);
	check_read(*op);
	code = gs_setgstate(igs, igstate_ptr(op));
	if ( code < 0 ) return code;
	pop(1);
	return 0;
}

/* ------ Rectangles ------- */

/* We preallocate a short list for rectangles, because */
/* the rectangle operators usually will involve very few rectangles. */
#define max_local_rect 5
typedef struct local_rects_s {
	gs_rect *pr;
	uint count;
	gs_rect rl[max_local_rect];
} local_rects;

/* Forward references */
private int rect_get(P2(local_rects *, os_ptr));
private void rect_release(P1(local_rects *));

/* <x> <y> <width> <height> rectappend - */
/* <numarray|numstring> rectappend - */
int
zrectappend(os_ptr op)
{	local_rects lr;
	int npop = rect_get(&lr, op);
	int code;
	if ( npop < 0 ) return npop;
	code = gs_rectappend(igs, lr.pr, lr.count);
	rect_release(&lr);
	if ( code < 0 ) return code;
	pop(npop);
	return 0;
}

/* <x> <y> <width> <height> rectclip - */
/* <numarray|numstring> rectclip - */
int
zrectclip(os_ptr op)
{	local_rects lr;
	int npop = rect_get(&lr, op);
	int code;
	if ( npop < 0 ) return npop;
	code = gs_rectclip(igs, lr.pr, lr.count);
	rect_release(&lr);
	if ( code < 0 ) return code;
	pop(npop);
	return 0;
}

/* <x> <y> <width> <height> rectfill - */
/* <numarray|numstring> rectfill - */
int
zrectfill(os_ptr op)
{	local_rects lr;
	int npop = rect_get(&lr, op);
	int code;
	if ( npop < 0 ) return npop;
	code = gs_rectfill(igs, lr.pr, lr.count);
	rect_release(&lr);
	if ( code < 0 ) return code;
	pop(npop);
	return 0;
}

/* <x> <y> <width> <height> rectstroke - */
/* <numarray|numstring> rectstroke - */
int
zrectstroke(os_ptr op)
{	gs_matrix mat;
	local_rects lr;
	int npop, code;
	if ( read_matrix(op, &mat) >= 0 )
	   {	/* Concatenate the matrix to the CTM just before */
		/* stroking the path. */
		npop = rect_get(&lr, op - 1);
		if ( npop < 0 ) return npop;
		code = gs_rectstroke(igs, lr.pr, lr.count, &mat);
		npop++;
	   }
	else
	   {	/* No matrix. */
		npop = rect_get(&lr, op);
		if ( npop < 0 ) return npop;
		code = gs_rectstroke(igs, lr.pr, lr.count, (gs_matrix *)0);
	   }
	rect_release(&lr);
	if ( code < 0 ) return code;
	pop(npop);
	return 0;
}

/* --- Internal routines --- */

/* Get rectangles from the stack. */
/* Return the number of elements to pop (>0) if OK, <0 if error. */
private int
rect_get(local_rects *plr, os_ptr op)
{	int format, code, npop;
	stream st;
	uint n, count;
	gs_rect *pr;
	switch ( r_type(op) )
	   {
	case t_array:
	case t_string:
		code = sread_num_array(&st, op);
		if ( code < 0 ) return code;
		format = code;
		count = scount_num_stream(&st, format);
		if ( count % 4 )
			return_error(e_rangecheck);
		count /= 4, npop = 1;
		break;
	default:			/* better be 4 numbers */
		sread_string(&st, (byte *)(op - 3), sizeof(ref) * 4);
		format = num_array;
		count = 1, npop = 4;
		break;
	   }
	plr->count = count;
	if ( count <= max_local_rect )
		pr = plr->rl;
	else
	{	pr = (gs_rect *)ialloc_byte_array(count, sizeof(gs_rect),
						  "rect_get");
		if ( pr == 0 )
			return_error(e_VMerror);
	}
	plr->pr = pr;
	for ( n = 0; n < count; n++, pr++ )
	   {	ref rnum;
		float rv[4];
		int i;
		for ( i = 0; i < 4; i++ )
		   {	switch ( code = sget_encoded_number(&st, format, &rnum) )
			   {
			case t_integer:
				rv[i] = rnum.value.intval;
				break;
			case t_real:
				rv[i] = rnum.value.realval;
				break;
			default:	/* code < 0 */
				return code;
			   }
		   }
		pr->q.x = (pr->p.x = rv[0]) + rv[2];
		pr->q.y = (pr->p.y = rv[1]) + rv[3];
	   }
	return npop;
}

/* Release the rectangle list if needed. */
private void
rect_release(local_rects *plr)
{	if ( plr->pr != plr->rl )
		ifree_object(plr->pr, "rect_release");
}

/* ------ Graphics state ------ */

/* <llx> <lly> <urx> <ury> setbbox - */
int
zsetbbox(register os_ptr op)
{	float box[4];
	int code = num_params(op, 4, box);
	if ( code < 0 ) return code;
	if ( (code = gs_setbbox(igs, box[0], box[1], box[2], box[3])) < 0 )
		return code;
	pop(4);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zdps1_l2_op_defs) {
		op_def_begin_level2(),
		/* Graphics state */
	{"0currenthalftonephase", zcurrenthalftonephase},
	{"0currentstrokeadjust", zcurrentstrokeadjust},
	{"2sethalftonephase", zsethalftonephase},
	{"1setstrokeadjust", zsetstrokeadjust},
		/* Graphics state objects */
	{"1currentgstate", zcurrentgstate},
	{"0gstate", zgstate},
	{"1setgstate", zsetgstate},
		/* Rectangles */
	{"1rectappend", zrectappend},
	{"1rectclip", zrectclip},
	{"1rectfill", zrectfill},
	{"1rectstroke", zrectstroke},
		/* Graphics state components */
	{"4setbbox", zsetbbox},
END_OP_DEFS(zdps1_init) }

/* ------ Internal routines ------ */

/* Ensure that a gstate is not shared with an outer save level. */
/* *op is of type t_astruct(igstate_obj). */
private int
gstate_unshare(os_ptr op)
{	ref *pgsref = &r_ptr(op, igstate_obj)->gstate;
	gs_state *pgs = r_ptr(pgsref, gs_state);
	gs_state *pnew;
	int_gstate *isp;
	if ( !ref_must_save(pgsref) )
	  return 0;
	/* Copy the gstate. */
	pnew = gs_gstate(pgs);
	if ( pnew == 0 )
	  return_error(e_VMerror);
	isp = gs_int_gstate(pnew);
	int_gstate_map_refs(isp, ref_mark_new);
	ref_do_save(op, pgsref, "gstate_unshare");
	make_istruct_new(pgsref, 0, pnew);
	return 0;
}
