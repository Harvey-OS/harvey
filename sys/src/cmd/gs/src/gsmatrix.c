/* Copyright (C) 1989, 1995, 1996, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsmatrix.c,v 1.8 2004/08/31 13:23:16 igor Exp $ */
/* Matrix operators for Ghostscript library */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxfarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "stream.h"

/* The identity matrix */
private const gs_matrix gs_identity_matrix =
{identity_matrix_body};

/* ------ Matrix creation ------ */

/* Create an identity matrix */
void
gs_make_identity(gs_matrix * pmat)
{
    *pmat = gs_identity_matrix;
}

/* Create a translation matrix */
int
gs_make_translation(floatp dx, floatp dy, gs_matrix * pmat)
{
    *pmat = gs_identity_matrix;
    pmat->tx = dx;
    pmat->ty = dy;
    return 0;
}

/* Create a scaling matrix */
int
gs_make_scaling(floatp sx, floatp sy, gs_matrix * pmat)
{
    *pmat = gs_identity_matrix;
    pmat->xx = sx;
    pmat->yy = sy;
    return 0;
}

/* Create a rotation matrix. */
/* The angle is in degrees. */
int
gs_make_rotation(floatp ang, gs_matrix * pmat)
{
    gs_sincos_t sincos;

    gs_sincos_degrees(ang, &sincos);
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
gs_matrix_multiply(const gs_matrix * pm1, const gs_matrix * pm2, gs_matrix * pmr)
{
    double xx1 = pm1->xx, yy1 = pm1->yy;
    double tx1 = pm1->tx, ty1 = pm1->ty;
    double xx2 = pm2->xx, yy2 = pm2->yy;
    double xy2 = pm2->xy, yx2 = pm2->yx;

    if (is_xxyy(pm1)) {
	pmr->tx = tx1 * xx2 + pm2->tx;
	pmr->ty = ty1 * yy2 + pm2->ty;
	if (is_fzero(xy2))
	    pmr->xy = 0;
	else
	    pmr->xy = xx1 * xy2,
		pmr->ty += tx1 * xy2;
	pmr->xx = xx1 * xx2;
	if (is_fzero(yx2))
	    pmr->yx = 0;
	else
	    pmr->yx = yy1 * yx2,
		pmr->tx += ty1 * yx2;
	pmr->yy = yy1 * yy2;
    } else {
	double xy1 = pm1->xy, yx1 = pm1->yx;

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
gs_matrix_invert(const gs_matrix * pm, gs_matrix * pmr)
{				/* We have to be careful about fetch/store order, */
    /* because pm might be the same as pmr. */
    if (is_xxyy(pm)) {
	if (is_fzero(pm->xx) || is_fzero(pm->yy))
	    return_error(gs_error_undefinedresult);
	pmr->tx = -(pmr->xx = 1.0 / pm->xx) * pm->tx;
	pmr->xy = 0.0;
	pmr->yx = 0.0;
	pmr->ty = -(pmr->yy = 1.0 / pm->yy) * pm->ty;
    } else {
	double det = pm->xx * pm->yy - pm->xy * pm->yx;
	double mxx = pm->xx, mtx = pm->tx;

	if (det == 0)
	    return_error(gs_error_undefinedresult);
	pmr->xx = pm->yy / det;
	pmr->xy = -pm->xy / det;
	pmr->yx = -pm->yx / det;
	pmr->yy = mxx / det;	/* xx is already changed */
	pmr->tx = -(mtx * pmr->xx + pm->ty * pmr->yx);
	pmr->ty = -(mtx * pmr->xy + pm->ty * pmr->yy);	/* tx ditto */
    }
    return 0;
}

/* Translate a matrix, possibly in place. */
int
gs_matrix_translate(const gs_matrix * pm, floatp dx, floatp dy, gs_matrix * pmr)
{
    gs_point trans;
    int code = gs_distance_transform(dx, dy, pm, &trans);

    if (code < 0)
	return code;
    if (pmr != pm)
	*pmr = *pm;
    pmr->tx += trans.x;
    pmr->ty += trans.y;
    return 0;
}

/* Scale a matrix, possibly in place. */
int
gs_matrix_scale(const gs_matrix * pm, floatp sx, floatp sy, gs_matrix * pmr)
{
    pmr->xx = pm->xx * sx;
    pmr->xy = pm->xy * sx;
    pmr->yx = pm->yx * sy;
    pmr->yy = pm->yy * sy;
    if (pmr != pm) {
	pmr->tx = pm->tx;
	pmr->ty = pm->ty;
    }
    return 0;
}

/* Rotate a matrix, possibly in place.  The angle is in degrees. */
int
gs_matrix_rotate(const gs_matrix * pm, floatp ang, gs_matrix * pmr)
{
    double mxx, mxy;
    gs_sincos_t sincos;

    gs_sincos_degrees(ang, &sincos);
    mxx = pm->xx, mxy = pm->xy;
    pmr->xx = sincos.cos * mxx + sincos.sin * pm->yx;
    pmr->xy = sincos.cos * mxy + sincos.sin * pm->yy;
    pmr->yx = sincos.cos * pm->yx - sincos.sin * mxx;
    pmr->yy = sincos.cos * pm->yy - sincos.sin * mxy;
    if (pmr != pm) {
	pmr->tx = pm->tx;
	pmr->ty = pm->ty;
    }
    return 0;
}

/* ------ Coordinate transformations (floating point) ------ */

/* Note that all the transformation routines take separate */
/* x and y arguments, but return their result in a point. */

/* Transform a point. */
int
gs_point_transform(floatp x, floatp y, const gs_matrix * pmat,
		   gs_point * ppt)
{
    ppt->x = x * pmat->xx + pmat->tx;
    ppt->y = y * pmat->yy + pmat->ty;
    if (!is_fzero(pmat->yx))
	ppt->x += y * pmat->yx;
    if (!is_fzero(pmat->xy))
	ppt->y += x * pmat->xy;
    return 0;
}

/* Inverse-transform a point. */
/* Return gs_error_undefinedresult if the matrix is not invertible. */
int
gs_point_transform_inverse(floatp x, floatp y, const gs_matrix * pmat,
			   gs_point * ppt)
{
    if (is_xxyy(pmat)) {
	if (is_fzero(pmat->xx) || is_fzero(pmat->yy))
	    return_error(gs_error_undefinedresult);
	ppt->x = (x - pmat->tx) / pmat->xx;
	ppt->y = (y - pmat->ty) / pmat->yy;
	return 0;
    } else if (is_xyyx(pmat)) {
	if (is_fzero(pmat->xy) || is_fzero(pmat->yx))
	    return_error(gs_error_undefinedresult);
	ppt->x = (y - pmat->ty) / pmat->xy;
	ppt->y = (x - pmat->tx) / pmat->yx;
	return 0;
    } else {			/* There are faster ways to do this, */
	/* but we won't implement one unless we have to. */
	gs_matrix imat;
	int code = gs_matrix_invert(pmat, &imat);

	if (code < 0)
	    return code;
	return gs_point_transform(x, y, &imat, ppt);
    }
}

/* Transform a distance. */
int
gs_distance_transform(floatp dx, floatp dy, const gs_matrix * pmat,
		      gs_point * pdpt)
{
    pdpt->x = dx * pmat->xx;
    pdpt->y = dy * pmat->yy;
    if (!is_fzero(pmat->yx))
	pdpt->x += dy * pmat->yx;
    if (!is_fzero(pmat->xy))
	pdpt->y += dx * pmat->xy;
    return 0;
}

/* Inverse-transform a distance. */
/* Return gs_error_undefinedresult if the matrix is not invertible. */
int
gs_distance_transform_inverse(floatp dx, floatp dy,
			      const gs_matrix * pmat, gs_point * pdpt)
{
    if (is_xxyy(pmat)) {
	if (is_fzero(pmat->xx) || is_fzero(pmat->yy))
	    return_error(gs_error_undefinedresult);
	pdpt->x = dx / pmat->xx;
	pdpt->y = dy / pmat->yy;
    } else if (is_xyyx(pmat)) {
	if (is_fzero(pmat->xy) || is_fzero(pmat->yx))
	    return_error(gs_error_undefinedresult);
	pdpt->x = dy / pmat->xy;
	pdpt->y = dx / pmat->yx;
    } else {
	double det = pmat->xx * pmat->yy - pmat->xy * pmat->yx;

	if (det == 0)
	    return_error(gs_error_undefinedresult);
	pdpt->x = (dx * pmat->yy - dy * pmat->yx) / det;
	pdpt->y = (dy * pmat->xx - dx * pmat->xy) / det;
    }
    return 0;
}

/* Compute the bounding box of 4 points. */
int
gs_points_bbox(const gs_point pts[4], gs_rect * pbox)
{
#define assign_min_max(vmin, vmax, v0, v1)\
  if ( v0 < v1 ) vmin = v0, vmax = v1; else vmin = v1, vmax = v0
#define assign_min_max_4(vmin, vmax, v0, v1, v2, v3)\
  { double min01, max01, min23, max23;\
    assign_min_max(min01, max01, v0, v1);\
    assign_min_max(min23, max23, v2, v3);\
    vmin = min(min01, min23);\
    vmax = max(max01, max23);\
  }
    assign_min_max_4(pbox->p.x, pbox->q.x,
		     pts[0].x, pts[1].x, pts[2].x, pts[3].x);
    assign_min_max_4(pbox->p.y, pbox->q.y,
		     pts[0].y, pts[1].y, pts[2].y, pts[3].y);
#undef assign_min_max
#undef assign_min_max_4
    return 0;
}

/* Transform or inverse-transform a bounding box. */
/* Return gs_error_undefinedresult if the matrix is not invertible. */
private int
bbox_transform_either_only(const gs_rect * pbox_in, const gs_matrix * pmat,
			   gs_point pts[4],
     int (*point_xform) (floatp, floatp, const gs_matrix *, gs_point *))
{
    int code;

    if ((code = (*point_xform) (pbox_in->p.x, pbox_in->p.y, pmat, &pts[0])) < 0 ||
	(code = (*point_xform) (pbox_in->p.x, pbox_in->q.y, pmat, &pts[1])) < 0 ||
	(code = (*point_xform) (pbox_in->q.x, pbox_in->p.y, pmat, &pts[2])) < 0 ||
     (code = (*point_xform) (pbox_in->q.x, pbox_in->q.y, pmat, &pts[3])) < 0
	)
	DO_NOTHING;
    return code;
}

private int
bbox_transform_either(const gs_rect * pbox_in, const gs_matrix * pmat,
		      gs_rect * pbox_out,
     int (*point_xform) (floatp, floatp, const gs_matrix *, gs_point *))
{
    int code;

    /*
     * In principle, we could transform only one point and two
     * distance vectors; however, because of rounding, we will only
     * get fully consistent results if we transform all 4 points.
     * We must compute the max and min after transforming,
     * since a rotation may be involved.
     */
    gs_point pts[4];

    if ((code = bbox_transform_either_only(pbox_in, pmat, pts, point_xform)) < 0)
	return code;
    return gs_points_bbox(pts, pbox_out);
}
int
gs_bbox_transform(const gs_rect * pbox_in, const gs_matrix * pmat,
		  gs_rect * pbox_out)
{
    return bbox_transform_either(pbox_in, pmat, pbox_out,
				 gs_point_transform);
}
int
gs_bbox_transform_only(const gs_rect * pbox_in, const gs_matrix * pmat,
		       gs_point points[4])
{
    return bbox_transform_either_only(pbox_in, pmat, points,
				      gs_point_transform);
}
int
gs_bbox_transform_inverse(const gs_rect * pbox_in, const gs_matrix * pmat,
			  gs_rect * pbox_out)
{
    return bbox_transform_either(pbox_in, pmat, pbox_out,
				 gs_point_transform_inverse);
}

/* ------ Coordinate transformations (to fixed point) ------ */

#define f_fits_in_fixed(f) f_fits_in_bits(f, fixed_int_bits)

/* Make a gs_matrix_fixed from a gs_matrix. */
int
gs_matrix_fixed_from_matrix(gs_matrix_fixed *pfmat, const gs_matrix *pmat)
{
    *(gs_matrix *)pfmat = *pmat;
    if (f_fits_in_fixed(pmat->tx) && f_fits_in_fixed(pmat->ty)) {
	pfmat->tx = fixed2float(pfmat->tx_fixed = float2fixed(pmat->tx));
	pfmat->ty = fixed2float(pfmat->ty_fixed = float2fixed(pmat->ty));
	pfmat->txy_fixed_valid = true;
    } else {
	pfmat->txy_fixed_valid = false;
    }
    return 0;
}

/* Transform a point with a fixed-point result. */
int
gs_point_transform2fixed(const gs_matrix_fixed * pmat,
			 floatp x, floatp y, gs_fixed_point * ppt)
{
    fixed px, py, t;
    double xtemp, ytemp;
    int code;

    if (!pmat->txy_fixed_valid) {	/* The translation is out of range.  Do the */
	/* computation in floating point, and convert to */
	/* fixed at the end. */
	gs_point fpt;

	gs_point_transform(x, y, (const gs_matrix *)pmat, &fpt);
	if (!(f_fits_in_fixed(fpt.x) && f_fits_in_fixed(fpt.y)))
	    return_error(gs_error_limitcheck);
	ppt->x = float2fixed(fpt.x);
	ppt->y = float2fixed(fpt.y);
	return 0;
    }
    if (!is_fzero(pmat->xy)) {	/* Hope for 90 degree rotation */
	if ((code = CHECK_DFMUL2FIXED_VARS(px, y, pmat->yx, xtemp)) < 0 ||
	    (code = CHECK_DFMUL2FIXED_VARS(py, x, pmat->xy, ytemp)) < 0
	    )
	    return code;
	FINISH_DFMUL2FIXED_VARS(px, xtemp);
	FINISH_DFMUL2FIXED_VARS(py, ytemp);
	if (!is_fzero(pmat->xx)) {
	    if ((code = CHECK_DFMUL2FIXED_VARS(t, x, pmat->xx, xtemp)) < 0)
		return code;
	    FINISH_DFMUL2FIXED_VARS(t, xtemp);
	    if ((code = CHECK_SET_FIXED_SUM(px, px, t)) < 0)
	        return code;
	}
	if (!is_fzero(pmat->yy)) {
	    if ((code = CHECK_DFMUL2FIXED_VARS(t, y, pmat->yy, ytemp)) < 0)
		return code;
	    FINISH_DFMUL2FIXED_VARS(t, ytemp);
	    if ((code = CHECK_SET_FIXED_SUM(py, py, t)) < 0)
	        return code;
	}
    } else {
	if ((code = CHECK_DFMUL2FIXED_VARS(px, x, pmat->xx, xtemp)) < 0 ||
	    (code = CHECK_DFMUL2FIXED_VARS(py, y, pmat->yy, ytemp)) < 0
	    )
	    return code;
	FINISH_DFMUL2FIXED_VARS(px, xtemp);
	FINISH_DFMUL2FIXED_VARS(py, ytemp);
	if (!is_fzero(pmat->yx)) {
	    if ((code = CHECK_DFMUL2FIXED_VARS(t, y, pmat->yx, ytemp)) < 0)
		return code;
	    FINISH_DFMUL2FIXED_VARS(t, ytemp);
	    if ((code = CHECK_SET_FIXED_SUM(px, px, t)) < 0)
	        return code;
	}
    }
    if (((code = CHECK_SET_FIXED_SUM(ppt->x, px, pmat->tx_fixed)) < 0) ||
        ((code = CHECK_SET_FIXED_SUM(ppt->y, py, pmat->ty_fixed)) < 0) )
        return code;
    return 0;
}

#if PRECISE_CURRENTPOINT
/* Transform a point with a fixed-point result. */
/* Used for the best precision of the current point,
   see comment in clamp_point_aux. */
int
gs_point_transform2fixed_rounding(const gs_matrix_fixed * pmat,
			 floatp x, floatp y, gs_fixed_point * ppt)
{
    gs_point fpt;

    gs_point_transform(x, y, (const gs_matrix *)pmat, &fpt);
    if (!(f_fits_in_fixed(fpt.x) && f_fits_in_fixed(fpt.y)))
	return_error(gs_error_limitcheck);
    ppt->x = float2fixed_rounded(fpt.x);
    ppt->y = float2fixed_rounded(fpt.y);
    return 0;
}
#endif

/* Transform a distance with a fixed-point result. */
int
gs_distance_transform2fixed(const gs_matrix_fixed * pmat,
			    floatp dx, floatp dy, gs_fixed_point * ppt)
{
    fixed px, py, t;
    double xtemp, ytemp;
    int code;

    if ((code = CHECK_DFMUL2FIXED_VARS(px, dx, pmat->xx, xtemp)) < 0 ||
	(code = CHECK_DFMUL2FIXED_VARS(py, dy, pmat->yy, ytemp)) < 0
	)
	return code;
    FINISH_DFMUL2FIXED_VARS(px, xtemp);
    FINISH_DFMUL2FIXED_VARS(py, ytemp);
    if (!is_fzero(pmat->yx)) {
	if ((code = CHECK_DFMUL2FIXED_VARS(t, dy, pmat->yx, ytemp)) < 0)
	    return code;
	FINISH_DFMUL2FIXED_VARS(t, ytemp);
	if ((code = CHECK_SET_FIXED_SUM(px, px, t)) < 0)
	    return code;
    }
    if (!is_fzero(pmat->xy)) {
	if ((code = CHECK_DFMUL2FIXED_VARS(t, dx, pmat->xy, xtemp)) < 0)
	    return code;
	FINISH_DFMUL2FIXED_VARS(t, xtemp);
	if ((code = CHECK_SET_FIXED_SUM(py, py, t)) < 0)
	    return code;
    }
    ppt->x = px;
    ppt->y = py;
    return 0;
}

/* ------ Serialization ------ */

/*
 * For maximum conciseness in band lists, we write a matrix as a control
 * byte followed by 0 to 6 values.  The control byte has the format
 * AABBCD00.  AA and BB control (xx,yy) and (xy,yx) as follows:
 *	00 = values are (0.0, 0.0)
 *	01 = values are (V, V) [1 value follows]
 *	10 = values are (V, -V) [1 value follows]
 *	11 = values are (U, V) [2 values follow]
 * C and D control tx and ty as follows:
 *	0 = value is 0.0
 *	1 = value follows
 * The following code is the only place that knows this representation.
 */

/* Put a matrix on a stream. */
int
sput_matrix(stream *s, const gs_matrix *pmat)
{
    byte buf[1 + 6 * sizeof(float)];
    byte *cp = buf + 1;
    byte b = 0;
    float coeff[6];
    int i;
    uint ignore;

    coeff[0] = pmat->xx;
    coeff[1] = pmat->xy;
    coeff[2] = pmat->yx;
    coeff[3] = pmat->yy;
    coeff[4] = pmat->tx;
    coeff[5] = pmat->ty;
    for (i = 0; i < 4; i += 2) {
	float u = coeff[i], v = coeff[i ^ 3];

	b <<= 2;
	if (u != 0 || v != 0) {
	    memcpy(cp, &u, sizeof(float));
	    cp += sizeof(float);

	    if (v == u)
		b += 1;
	    else if (v == -u)
		b += 2;
	    else {
		b += 3;
		memcpy(cp, &v, sizeof(float));
		cp += sizeof(float);
	    }
	}
    }
    for (; i < 6; ++i) {
	float v = coeff[i];

	b <<= 1;
	if (v != 0) {
	    ++b;
	    memcpy(cp, &v, sizeof(float));
	    cp += sizeof(float);
	}
    }
    buf[0] = b << 2;
    return sputs(s, buf, cp - buf, &ignore);
}

/* Get a matrix from a stream. */
int
sget_matrix(stream *s, gs_matrix *pmat)
{
    int b = sgetc(s);
    float coeff[6];
    int i;
    int status;
    uint nread;

    if (b < 0)
	return b;
    for (i = 0; i < 4; i += 2, b <<= 2)
	if (!(b & 0xc0))
	    coeff[i] = coeff[i ^ 3] = 0.0;
	else {
	    float value;

	    status = sgets(s, (byte *)&value, sizeof(value), &nread);
	    if (status < 0 && status != EOFC)
		return_error(gs_error_ioerror);
	    coeff[i] = value;
	    switch ((b >> 6) & 3) {
		case 1:
		    coeff[i ^ 3] = value;
		    break;
		case 2:
		    coeff[i ^ 3] = -value;
		    break;
		case 3:
		    status = sgets(s, (byte *)&coeff[i ^ 3],
				   sizeof(coeff[0]), &nread);
		    if (status < 0 && status != EOFC)
			return_error(gs_error_ioerror);
	    }
	}
    for (; i < 6; ++i, b <<= 1)
	if (b & 0x80) {
	    status = sgets(s, (byte *)&coeff[i], sizeof(coeff[0]), &nread);
	    if (status < 0 && status != EOFC)
		return_error(gs_error_ioerror);
	} else
	    coeff[i] = 0.0;
    pmat->xx = coeff[0];
    pmat->xy = coeff[1];
    pmat->yx = coeff[2];
    pmat->yy = coeff[3];
    pmat->tx = coeff[4];
    pmat->ty = coeff[5];
    return 0;
}
