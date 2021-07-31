/* Copyright (C) 1989, 1992, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gscoord.c */
/* Coordinate system operators for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsccode.h"			/* for gxfont.h */
#include "gxfarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxfont.h"			/* for char_tm */
#include "gxpath.h"			/* for gx_path_translate */
#include "gzstate.h"
#include "gxcoord.h"			/* requires gsmatrix, gsstate */

/* Choose whether to enable the rounding code in update_ctm_fixed. */
#define ROUND_CTM_FIXED 1

/* Forward declarations */
#ifdef DEBUG
#define trace_ctm(pgs) trace_matrix_fixed(&(pgs)->ctm)
private void near trace_matrix_fixed(P1(const gs_matrix_fixed *));
private void near trace_matrix(P1(const gs_matrix *));
#endif

/* Macro for ensuring ctm_inverse is valid */
#ifdef DEBUG
#define print_inverse(pgs)\
if ( gs_debug_c('x') )\
	dprintf("[x]Inverting:\n"), trace_ctm(pgs), trace_matrix(&pgs->ctm_inverse)
#else
#define print_inverse(pgs) DO_NOTHING
#endif
#define ensure_inverse_valid(pgs)\
	if ( !pgs->inverse_valid )\
	   {	int code = ctm_set_inverse(pgs);\
		if ( code < 0 ) return code;\
	   }

private int
ctm_set_inverse(gs_state *pgs)
{	int code = gs_matrix_invert(&ctm_only(pgs), &pgs->ctm_inverse);
	print_inverse(pgs);
	if ( code < 0 ) return code;
	pgs->inverse_valid = true;
	return 0;
}

/* Machinery for updating fixed version of ctm. */
/*
 * We (conditionally) adjust the floating point translation
 * so that it exactly matches the (rounded) fixed translation.
 * This avoids certain unpleasant rounding anomalies, such as
 * 0 0 moveto currentpoint not returning 0 0, and () stringwidth
 * not returning 0 0.
 */
#if ROUND_CTM_FIXED
#  define update_t_fixed(mat, t, t_fixed)\
    (mat).t = fixed2float((mat).t_fixed = float2fixed((mat).t))
#else					/* !ROUND_CTM_FIXED */
#  define update_t_fixed(mat, t, t_fixed)\
    (mat).t_fixed = float2fixed((mat).t)
#endif					/* (!)ROUND_CTM_FIXED */
#define update_matrix_fixed(mat)\
  update_t_fixed(mat, tx, tx_fixed),\
  update_t_fixed(mat, ty, ty_fixed)
#define update_ctm(pgs)\
  update_matrix_fixed(pgs->ctm),\
  pgs->inverse_valid = false,\
  pgs->char_tm_valid = false

void
gs_update_matrix_fixed(gs_matrix_fixed *pmat)
{	update_matrix_fixed(*pmat);
}

/* ------ Coordinate system definition ------ */

int
gs_initmatrix(gs_state *pgs)
{	gs_defaultmatrix(pgs, &ctm_only(pgs));
	update_ctm(pgs);
#ifdef DEBUG
if ( gs_debug_c('x') )
	dprintf("[x]initmatrix:\n"), trace_ctm(pgs);
#endif
	return 0;
}

int
gs_defaultmatrix(const gs_state *pgs, gs_matrix *pmat)
{	gx_device *dev = gs_currentdevice_inline(pgs);
	gs_deviceinitialmatrix(dev, pmat);
	/* Add in the translation for the Margins. */
	pmat->tx += dev->Margins[0] *
	  dev->HWResolution[0] / dev->Margins_HWResolution[0];
	pmat->ty += dev->Margins[1] *
	  dev->HWResolution[1] / dev->Margins_HWResolution[1];
	return 0;
}

int
gs_currentmatrix(const gs_state *pgs, gs_matrix *pmat)
{	*pmat = ctm_only(pgs);
	return 0;
}

/* Set the current transformation matrix for rendering text. */
/* Note that this may be based on a font other than the current font. */
int
gs_setcharmatrix(gs_state *pgs, const gs_matrix *pmat)
{	int code = gs_matrix_multiply(pmat, &ctm_only(pgs),
				      &char_tm_only(pgs));
	if ( code < 0 )
		return code;
	gs_update_matrix_fixed(&pgs->char_tm);
#ifdef DEBUG
if ( gs_debug_c('x') )
	dprintf("[x]setting char_tm:"), trace_matrix_fixed(&pgs->char_tm);
#endif
	pgs->char_tm_valid = true;
	return 0;
}

/* Read (after possibly computing) the current transformation matrix */
/* for rendering text.  If force=1, update char_tm if it is invalid; */
/* if force=0, don't update char_tm, and return an error code. */
int
gs_currentcharmatrix(gs_state *pgs, gs_matrix *ptm, int force)
{	if ( !pgs->char_tm_valid )
	{	int code;
		if ( !force )
			return_error(gs_error_undefinedresult);
		code = gs_setcharmatrix(pgs, &pgs->font->FontMatrix);
		if ( code < 0 )
			return code;
	}
	if ( ptm != NULL )
		*ptm = char_tm_only(pgs);
	return 0;
}

int
gs_setmatrix(gs_state *pgs, const gs_matrix *pmat)
{	ctm_only(pgs) = *pmat;
	update_ctm(pgs);
#ifdef DEBUG
if ( gs_debug_c('x') )
	dprintf("[x]setmatrix:\n"), trace_ctm(pgs);
#endif
	return 0;
}

int
gs_settocharmatrix(gs_state *pgs)
{	if ( pgs->char_tm_valid )
	{	pgs->ctm = pgs->char_tm;
		pgs->inverse_valid = false;
		return 0;
	}
	else
		return_error(gs_error_undefinedresult);
}

int
gs_translate(gs_state *pgs, floatp dx, floatp dy)
{	gs_point pt;
	int code;
	if ( (code = gs_distance_transform(dx, dy, &ctm_only(pgs), &pt)) < 0 )
		return code;
	pgs->ctm.tx += pt.x;
	pgs->ctm.ty += pt.y;
	update_ctm(pgs);
#ifdef DEBUG
if ( gs_debug_c('x') )
	dprintf4("[x]translate: %f %f -> %f %f\n",
		 dx, dy, pt.x, pt.y),
	trace_ctm(pgs);
#endif
	return 0;
}

int
gs_scale(gs_state *pgs, floatp sx, floatp sy)
{	pgs->ctm.xx *= sx;
	pgs->ctm.xy *= sx;
	pgs->ctm.yx *= sy;
	pgs->ctm.yy *= sy;
	pgs->inverse_valid = false, pgs->char_tm_valid = false;
#ifdef DEBUG
if ( gs_debug_c('x') )
	dprintf2("[x]scale: %f %f\n", sx, sy), trace_ctm(pgs);
#endif
	return 0;
}

int
gs_rotate(gs_state *pgs, floatp ang)
{	int code = gs_matrix_rotate(&ctm_only(pgs), ang, &ctm_only(pgs));
	pgs->inverse_valid = false, pgs->char_tm_valid = false;
#ifdef DEBUG
if ( gs_debug_c('x') )
	dprintf1("[x]rotate: %f\n", ang), trace_ctm(pgs);
#endif
	return code;
}

int
gs_concat(gs_state *pgs, const gs_matrix *pmat)
{	int code = gs_matrix_multiply(pmat, &ctm_only(pgs), &ctm_only(pgs));
	update_ctm(pgs);
#ifdef DEBUG
if ( gs_debug_c('x') )
	dprintf("[x]concat:\n"), trace_matrix(pmat), trace_ctm(pgs);
#endif
	return code;
}

/* ------ Coordinate transformation ------ */

int
gs_transform(gs_state *pgs, floatp x, floatp y, gs_point *pt)
{	return gs_point_transform(x, y, &ctm_only(pgs), pt);
}

int
gs_dtransform(gs_state *pgs, floatp dx, floatp dy, gs_point *pt)
{	return gs_distance_transform(dx, dy, &ctm_only(pgs), pt);
}

int
gs_itransform(gs_state *pgs, floatp x, floatp y, gs_point *pt)
{	/* If the matrix isn't skewed, we get more accurate results */
	/* by using transform_inverse than by using the inverse matrix. */
	if ( !is_skewed(&pgs->ctm) )
	   {	return gs_point_transform_inverse(x, y, &ctm_only(pgs), pt);
	   }
	else
	   {	ensure_inverse_valid(pgs);
		return gs_point_transform(x, y, &pgs->ctm_inverse, pt);
	   }
}

int
gs_idtransform(gs_state *pgs, floatp dx, floatp dy, gs_point *pt)
{	/* If the matrix isn't skewed, we get more accurate results */
	/* by using transform_inverse than by using the inverse matrix. */
	if ( !is_skewed(&pgs->ctm) )
	   {	return gs_distance_transform_inverse(dx, dy,
						     &ctm_only(pgs), pt);
	   }
	else
	   {	ensure_inverse_valid(pgs);
		return gs_distance_transform(dx, dy, &pgs->ctm_inverse, pt);
	   }
}

/* ------ For internal use only ------ */

/* Set the translation to a fixed value, and translate any existing path. */
/* Used by gschar.c to prepare for a BuildChar or BuildGlyph procedure. */
int
gx_translate_to_fixed(register gs_state *pgs, fixed px, fixed py)
{	fixed dx = px - pgs->ctm.tx_fixed;
	fixed dy = py - pgs->ctm.ty_fixed;
	int result = gx_path_translate(pgs->path, dx, dy);
	pgs->ctm.tx = fixed2float(pgs->ctm.tx_fixed = px);
	pgs->ctm.ty = fixed2float(pgs->ctm.ty_fixed = py);
	pgs->inverse_valid = false;
	if ( pgs->char_tm_valid )
	{	/* Update char_tm now, leaving it valid. */
		pgs->char_tm.tx += fixed2float(dx);
		pgs->char_tm.ty += fixed2float(dy);
		pgs->char_tm.tx_fixed += dx;
		pgs->char_tm.ty_fixed += dy;
	}
#ifdef DEBUG
if ( gs_debug_c('x') )
	dprintf2("[x]translate_to_fixed %g, %g:\n",
		 fixed2float(px), fixed2float(py)),
	trace_ctm(pgs),
	dprintf("[x]   char_tm:\n"),
	trace_matrix_fixed(&pgs->char_tm);
#endif
	return result;
}

/* Scale the CTM and character matrix for oversampling. */
int
gx_scale_char_matrix(register gs_state *pgs, int sx, int sy)
{
#define scale_cxy(s, vx, vy)\
  if ( s != 1 )\
   {	pgs->ctm.vx *= s;\
	pgs->ctm.vy *= s;\
	pgs->inverse_valid = false;\
	if ( pgs->char_tm_valid )\
	{	pgs->char_tm.vx *= s;\
		pgs->char_tm.vy *= s;\
	}\
   }
	scale_cxy(sx, xx, yx);
	scale_cxy(sy, xy, yy);
#undef scale_cxy
	if_debug2('x', "[x]char scale: %d %d\n", sx, sy);
	return 0;
}

/* Compute the coefficients for fast fixed-point distance transformations */
/* from a transformation matrix. */
/* We should cache the coefficients with the ctm.... */
int
gx_matrix_to_fixed_coeff(const gs_matrix *pmat, register fixed_coeff *pfc,
  int max_bits)
{	gs_matrix ctm;
	int scale = -10000;
	int expt, shift;
	ctm = *pmat;
	pfc->skewed = 0;
	if ( !is_fzero(ctm.xx) )
	   {	discard(frexp(ctm.xx, &scale));
	   }
	if ( !is_fzero(ctm.xy) )
	   {	discard(frexp(ctm.xy, &expt));
		if ( expt > scale ) scale = expt;
		pfc->skewed = 1;
	   }
	if ( !is_fzero(ctm.yx) )
	   {	discard(frexp(ctm.yx, &expt));
		if ( expt > scale ) scale = expt;
		pfc->skewed = 1;
	   }
	if ( !is_fzero(ctm.yy) )
	   {	discard(frexp(ctm.yy, &expt));
		if ( expt > scale ) scale = expt;
	   }
	scale = sizeof(long) * 8 - 1 - max_bits - scale;
	shift = scale - _fixed_shift;
	if ( shift > 0 )
	   {	pfc->shift = shift;
		pfc->round = (fixed)1 << (shift - 1);
	   }
	else
	   {	pfc->shift = 0;
		pfc->round = 0;
		scale -= shift;
	   }
#define set_c(c)\
  if ( is_fzero(ctm.c) ) pfc->c.f = 0, pfc->c.l = 0;\
  else pfc->c.f = ldexp(ctm.c, _fixed_shift), pfc->c.l = (long)ldexp(ctm.c, scale)
	set_c(xx);
	set_c(xy);
	set_c(yx);
	set_c(yy);
#ifdef DEBUG
if ( gs_debug_c('x') )
   {	dprintf6("[x]ctm: [%6g %6g %6g %6g %6g %6g]\n",
		 ctm.xx, ctm.xy, ctm.yx, ctm.yy, ctm.tx, ctm.ty);
	dprintf6("   scale=%d fc: [0x%lx 0x%lx 0x%lx 0x%lx] shift=%d\n",
		 scale, pfc->xx.l, pfc->xy.l, pfc->yx.l, pfc->yy.l,
		 pfc->shift);
   }
#endif
	pfc->max_bits = max_bits;
	return 0;
}

/* ------ Debugging printout ------ */

#ifdef DEBUG

/* Print a matrix */
private void near
trace_matrix_fixed(const gs_matrix_fixed *pmat)
{	trace_matrix((const gs_matrix *)pmat);
	dprintf2("\t\tt_fixed: [%6g %6g]\n",
		 fixed2float(pmat->tx_fixed), fixed2float(pmat->ty_fixed));
}
private void near
trace_matrix(register const gs_matrix *pmat)
{	dprintf6("\t[%6g %6g %6g %6g %6g %6g]\n",
		 pmat->xx, pmat->xy, pmat->yx, pmat->yy, pmat->tx, pmat->ty);
}

#endif
