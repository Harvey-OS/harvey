/* Copyright (C) 1989, 1990, 1991, 1993 Aladdin Enterprises.  All rights reserved.
  
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

/* gsmatrix.c */
/* Matrix operators for Ghostscript library */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxfarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"

/* The identity matrix */
private gs_matrix gs_identity_matrix =
	{ identity_matrix_body };

/* Forward references */
typedef struct sincos_s { double sin, cos; } sincos_t;
private void rotation_coefficients(P2(floatp, sincos_t *));

/* ------ Matrix creation ------ */

/* Create an identity matrix */
void
gs_make_identity(gs_matrix *pmat)
{	*pmat = gs_identity_matrix;
}

/* Create a translation matrix */
int
gs_make_translation(floatp dx, floatp dy, register gs_matrix *pmat)
{	*pmat = gs_identity_matrix;
	pmat->tx = dx;
	pmat->ty = dy;
	return 0;
}

/* Create a scaling matrix */
int
gs_make_scaling(floatp sx, floatp sy, register gs_matrix *pmat)
{	*pmat = gs_identity_matrix;
	pmat->xx = sx;
	pmat->yy = sy;
	return 0;
}

/* Create a rotation matrix. */
/* The angle is in degrees. */
int
gs_make_rotation(floatp ang, register gs_matrix *pmat)
{	sincos_t sincos;
	rotation_coefficients(ang, &sincos);
	pmat->yy = pmat->xx = sincos.cos;
	pmat->xy = sincos.sin;
	pmat->yx = -sincos.sin;
	pmat->tx = pmat->ty = 0.0;
	return 0;
}

/* ------ Matrix arithmetic ------ */

/* Multiply two matrices.  We should check for floating exceptions, */
/* but for the moment it's just too awkward. */
/* Since this is used heavily, we check for shortcuts. */
int
gs_matrix_multiply(const gs_matrix *pm1, const gs_matrix *pm2, register gs_matrix *pmr)
{	double xx1 = pm1->xx, yy1 = pm1->yy;
	double tx1 = pm1->tx, ty1 = pm1->ty;
	double xx2 = pm2->xx, yy2 = pm2->yy;
	double xy2 = pm2->xy, yx2 = pm2->yx;
	if ( !is_skewed(pm1) )
	{	pmr->tx = tx1 * xx2 + pm2->tx;
		pmr->ty = ty1 * yy2 + pm2->ty;
		if ( is_fzero(xy2) )
			pmr->xy = 0;
		else
			pmr->xy = xx1 * xy2,
			pmr->ty += tx1 * xy2;
		pmr->xx = xx1 * xx2;
		if ( is_fzero(yx2) )
			pmr->yx = 0;
		else
			pmr->yx = yy1 * yx2,
			pmr->tx += ty1 * yx2;
		pmr->yy = yy1 * yy2;
	}
	else
	{	double xy1 = pm1->xy, yx1 = pm1->yx;
		pmr->xx = xx1 * xx2 + xy1 * yx2;
		pmr->xy = xx1 * xy2 + xy1 * yy2;
		pmr->yy = yx1 * xy2 + yy1 * yy2;
		pmr->yx = yx1 * xx2 + yy1 * yx2;
		pmr->tx = tx1 * xx2 + ty1 * yx2 + pm2->tx;
		pmr->ty = tx1 * xy2 + ty1 * yy2 + pm2->ty;
	}
	return 0;
}

/* Invert a matrix.  Return gs_error_undefinedresult if not invertible. */
int
gs_matrix_invert(register const gs_matrix *pm, register gs_matrix *pmr)
{	/* We have to be careful about fetch/store order, */
	/* because pm might be the same as pmr. */
	if ( !is_skewed(pm) )
	{	if ( is_fzero(pm->xx) || is_fzero(pm->yy) )
			return_error(gs_error_undefinedresult);
		pmr->tx = - (pmr->xx = 1.0 / pm->xx) * pm->tx;
		pmr->xy = 0.0;
		pmr->yx = 0.0;
		pmr->ty = - (pmr->yy = 1.0 / pm->yy) * pm->ty;
	}
	else
	{	double det = pm->xx * pm->yy - pm->xy * pm->yx;
		double mxx = pm->xx, mtx = pm->tx;
		if ( det == 0 ) return_error(gs_error_undefinedresult);
		pmr->xx = pm->yy / det;
		pmr->xy = - pm->xy / det;
		pmr->yx = - pm->yx / det;
		pmr->yy = mxx / det;	/* xx is already changed */
		pmr->tx = - (mtx * pmr->xx + pm->ty * pmr->yx);
		pmr->ty = - (mtx * pmr->xy + pm->ty * pmr->yy);	/* tx ditto */
	}
	return 0;
}

/* Rotate a matrix, possibly in place.  The angle is in degrees. */
int
gs_matrix_rotate(register const gs_matrix *pm, floatp ang, register gs_matrix *pmr)
{	double mxx, mxy;
	sincos_t sincos;
	rotation_coefficients(ang, &sincos);
	mxx = pm->xx, mxy = pm->xy;
	pmr->xx = sincos.cos * mxx + sincos.sin * pm->yx;
	pmr->xy = sincos.cos * mxy + sincos.sin * pm->yy;
	pmr->yx = sincos.cos * pm->yx - sincos.sin * mxx;
	pmr->yy = sincos.cos * pm->yy - sincos.sin * mxy;
	pmr->tx = pm->tx;
	pmr->ty = pm->ty;
	return 0;
}

/* ------ Coordinate transformations (floating point) ------ */

/* Note that all the transformation routines take separate */
/* x and y arguments, but return their result in a point. */

/* Transform a point. */
int
gs_point_transform(floatp x, floatp y, register const gs_matrix *pmat,
  register gs_point *ppt)
{	ppt->x = x * pmat->xx + pmat->tx;
	ppt->y = y * pmat->yy + pmat->ty;
	if ( !is_fzero(pmat->yx) )
		ppt->x += y * pmat->yx;
	if ( !is_fzero(pmat->xy) )
		ppt->y += x * pmat->xy;
	return 0;
}

/* Inverse-transform a point. */
/* Return gs_error_undefinedresult if the matrix is not invertible. */
int
gs_point_transform_inverse(floatp x, floatp y, register const gs_matrix *pmat,
  register gs_point *ppt)
{	if ( !is_skewed(pmat) )
	   {	if ( is_fzero(pmat->xx) || is_fzero(pmat->yy) )
			return_error(gs_error_undefinedresult);
		ppt->x = (x - pmat->tx) / pmat->xx;
		ppt->y = (y - pmat->ty) / pmat->yy;
		return 0;
	   }
	else
	   {	/* There are faster ways to do this, */
		/* but we won't implement one unless we have to. */
		gs_matrix imat;
		int code = gs_matrix_invert(pmat, &imat);
		if ( code < 0 ) return code;
		return gs_point_transform(x, y, &imat, ppt);
	   }
}

/* Transform a distance. */
int
gs_distance_transform(floatp dx, floatp dy, register const gs_matrix *pmat,
  register gs_point *pdpt)
{	pdpt->x = dx * pmat->xx;
	pdpt->y = dy * pmat->yy;
	if ( !is_fzero(pmat->yx) )
		pdpt->x += dy * pmat->yx;
	if ( !is_fzero(pmat->xy) )
		pdpt->y += dx * pmat->xy;
	return 0;
}

/* Inverse-transform a distance. */
/* Return gs_error_undefinedresult if the matrix is not invertible. */
int
gs_distance_transform_inverse(floatp dx, floatp dy,
  register const gs_matrix *pmat, register gs_point *pdpt)
{	if ( !is_skewed(pmat) )
	   {	if ( is_fzero(pmat->xx) || is_fzero(pmat->yy) )
			return_error(gs_error_undefinedresult);
		pdpt->x = dx / pmat->xx;
		pdpt->y = dy / pmat->yy;
	   }
	else
	   {	double det = pmat->xx * pmat->yy - pmat->xy * pmat->yx;
		if ( det == 0 ) return_error(gs_error_undefinedresult);
		pdpt->x = (dx * pmat->yy - dy * pmat->yx) / det;
		pdpt->y = (dy * pmat->xx - dx * pmat->xy) / det;
	   }
	return 0;
}

/* Transform or inverse-transform a bounding box. */
/* Return gs_error_undefinedresult if the matrix is not invertible. */
private int
bbox_transform_either(const gs_rect *pbox_in, const gs_matrix *pmat,
  gs_rect *pbox_out,
  int (*point_xform)(P4(floatp, floatp, const gs_matrix *, gs_point *)),
  int (*distance_xform)(P4(floatp, floatp, const gs_matrix *, gs_point *)))
{	int code;
	gs_point p, w, h;
	double xmin, ymin, xmax, ymax;	/* gs_point uses double */
	/* We must recompute the max and min after transforming, */
	/* since a rotation may be involved. */
	if (	(code = (*point_xform)(pbox_in->p.x, pbox_in->p.y, pmat, &p)) < 0 ||
		(code = (*distance_xform)(pbox_in->q.x - pbox_in->p.x, 0.0, pmat, &w)) < 0 ||
		(code = (*distance_xform)(0.0, pbox_in->q.y - pbox_in->p.y, pmat, &h)) < 0
	   )
		return code;
	xmin = xmax = p.x;
	if ( w.x < 0 )	xmin += w.x;
	else		xmax += w.x;
	if ( h.x < 0 )	xmin += h.x;
	else		xmax += h.x;
	ymin = ymax = p.y;
	if ( w.y < 0 )	ymin += w.y;
	else		ymax += w.y;
	if ( h.y < 0 )	ymin += h.y;
	else		ymax += h.y;
	pbox_out->p.x = xmin, pbox_out->p.y = ymin;
	pbox_out->q.x = xmax, pbox_out->q.y = ymax;
	return 0;
}
int
gs_bbox_transform(const gs_rect *pbox_in, const gs_matrix *pmat,
  gs_rect *pbox_out)
{	return bbox_transform_either(pbox_in, pmat, pbox_out,
		gs_point_transform, gs_distance_transform);
}
int
gs_bbox_transform_inverse(const gs_rect *pbox_in, const gs_matrix *pmat,
  gs_rect *pbox_out)
{	return bbox_transform_either(pbox_in, pmat, pbox_out,
		gs_point_transform_inverse, gs_distance_transform_inverse);
}

/* ------ Coordinate transformations (to fixed point) ------ */

/* Transform a point with a fixed-point result. */
int
gs_point_transform2fixed(register const gs_matrix_fixed *pmat,
  floatp x, floatp y, gs_fixed_point *ppt)
{	fixed px, py, t;
	double dtemp;
	int code;
	if ( !is_fzero(pmat->xy) )
	  {	/* Hope for 90 degree rotation */
		if ( (code = set_dfmul2fixed_vars(px, y, pmat->yx, dtemp)) < 0 ||
		     (code = set_dfmul2fixed_vars(py, x, pmat->xy, dtemp)) < 0
		   )
		  return code;
		if ( !is_fzero(pmat->xx) )
		  { if ( (code = set_dfmul2fixed_vars(t, x, pmat->xx, dtemp)) < 0 )
		      return code;
		    px += t;		/* should check for overflow */
		  }
		if ( !is_fzero(pmat->yy) )
		  { if ( (code = set_dfmul2fixed_vars(t, y, pmat->yy, dtemp)) < 0 )
		      return code;
		    py += t;		/* should check for overflow */
		  }
	  }
	else
	  {	if ( (code = set_dfmul2fixed_vars(px, x, pmat->xx, dtemp)) < 0 ||
		     (code = set_dfmul2fixed_vars(py, y, pmat->yy, dtemp)) < 0
		   )
		  return code;
		if ( !is_fzero(pmat->yx) )
		  { if ( (code = set_dfmul2fixed_vars(t, y, pmat->yx, dtemp)) < 0 )
		      return code;
		    px += t;		/* should check for overflow */
		  }
	  }
	ppt->x = px + pmat->tx_fixed;	/* should check for overflow */
	ppt->y = py + pmat->ty_fixed;	/* should check for overflow */
	return 0;
}

/* Transform a distance with a fixed-point result. */
int
gs_distance_transform2fixed(register const gs_matrix_fixed *pmat,
  floatp dx, floatp dy, gs_fixed_point *ppt)
{	fixed px, py, t;
	double dtemp;
	int code;
	if ( (code = set_dfmul2fixed_vars(px, dx, pmat->xx, dtemp)) < 0 ||
	     (code = set_dfmul2fixed_vars(py, dy, pmat->yy, dtemp)) < 0
	   )
	  return code;
	if ( !is_fzero(pmat->yx) )
	  {	if ( (code = set_dfmul2fixed_vars(t, dy, pmat->yx, dtemp)) < 0 )
		  return code;
		px += t;		/* should check for overflow */
	  }
	if ( !is_fzero(pmat->xy) )
	  {	if ( (code = set_dfmul2fixed_vars(t, dx, pmat->xy, dtemp)) < 0 )
		  return code;
		py += t;		/* should check for overflow */
	  }
	ppt->x = px;
	ppt->y = py;
	return 0;
}

/* ------ Private routines ------ */

/* Get the sine and cosine coefficients for rotation by an angle */
/* specified in degrees, with special checking so multiples of 90 */
/* produce exact results. */
private void
rotation_coefficients(floatp ang, sincos_t *psincos)
{	int quads;
	if ( ang >= -360 && ang <= 360 &&
	     ang == (quads = (int)ang / 90) * 90
	   )
	{	static const int isincos[5] = { 0, 1, 0, -1, 0 };
		quads &= 3;
		psincos->sin = isincos[quads];
		psincos->cos = isincos[quads + 1];
	}
	else
	{	double theta = ang * (M_PI / 180.0);
		psincos->sin = sin(theta);
		psincos->cos = cos(theta);
	}
}
