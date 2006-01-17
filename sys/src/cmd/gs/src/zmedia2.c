/* Copyright (C) 1993, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zmedia2.c,v 1.19 2005/08/30 06:38:44 igor Exp $ */
/* Media matching for setpagedevice */
#include "math_.h"
#include "memory_.h"
#include "ghost.h"
#include "gsmatrix.h"
#include "oper.h"
#include "idict.h"
#include "idparam.h"
#include "iname.h"
#include "store.h"

/* <pagedict> <attrdict> <policydict> <keys> .matchmedia <key> true */
/* <pagedict> <attrdict> <policydict> <keys> .matchmedia false */
/* <pagedict> null <policydict> <keys> .matchmedia null true */
private int zmatch_page_size(const gs_memory_t *mem,
			     const ref * pvreq, const ref * pvmed,
			     int policy, int orient, bool roll,
			     float *best_mismatch, gs_matrix * pmat,
			     gs_point * pmsize);
typedef struct match_record_s {
    ref best_key, match_key;
    uint priority, no_match_priority;
} match_record_t;
private void
reset_match(match_record_t *match)
{
    make_null(&match->best_key);
    make_null(&match->match_key);
    match->priority = match->no_match_priority;
}
private int
zmatchmedia(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr preq = op - 3;
    os_ptr pattr = op - 2;
    os_ptr ppol = op - 1;
    os_ptr pkeys = op;		/* *const */
    int policy_default;
    float best_mismatch = (float)max_long;	/* adhoc */
    float mepos_penalty;
    float mbest = best_mismatch;
    match_record_t match;
    ref no_priority;
    ref *ppriority;
    int mepos, orient;
    bool roll;
    int code;
    int ai;
    struct mkd_ {
	ref key, dict;
    } aelt;
    if (r_has_type(pattr, t_null)) {
	check_op(4);
	make_null(op - 3);
	make_true(op - 2);
	pop(2);
	return 0;
    }
    check_type(*preq, t_dictionary);
    check_dict_read(*preq);
    check_type(*pattr, t_dictionary);
    check_dict_read(*pattr);
    check_type(*ppol, t_dictionary);
    check_dict_read(*ppol);
    check_array(*pkeys);
    check_read(*pkeys);
    switch (code = dict_int_null_param(preq, "MediaPosition", 0, 0x7fff,
				       0, &mepos)) {
	default:
	    return code;
	case 2:
	case 1:
	    mepos = -1;
	case 0:;
    }
    switch (code = dict_int_null_param(preq, "Orientation", 0, 3,
				       0, &orient)) {
	default:
	    return code;
	case 2:
	case 1:
	    orient = -1;
	case 0:;
    }
    code = dict_bool_param(preq, "RollFedMedia", false, &roll);
    if (code < 0)
	return code;
    code = dict_int_param(ppol, "PolicyNotFound", 0, 7, 0,
			  &policy_default);
    if (code < 0)
	return code;
    if (dict_find_string(pattr, "Priority", &ppriority) > 0) {
	check_array_only(*ppriority);
	check_read(*ppriority);
    } else {
	make_empty_array(&no_priority, a_readonly);
	ppriority = &no_priority;
    }
    match.no_match_priority = r_size(ppriority);
    reset_match(&match);
    for (ai = dict_first(pattr);
	 (ai = dict_next(pattr, ai, (ref * /*[2]*/)&aelt)) >= 0;
	 ) {
	if (r_has_type(&aelt.dict, t_dictionary) &&
	    r_has_attr(dict_access_ref(&aelt.dict), a_read) &&
	    r_has_type(&aelt.key, t_integer)
	    ) {
	    bool match_all;
	    uint ki, pi;

	    code = dict_bool_param(&aelt.dict, "MatchAll", false,
				   &match_all);
	    if (code < 0)
		return code;
	    for (ki = 0; ki < r_size(pkeys); ki++) {
		ref key;
		ref kstr;
		ref *prvalue;
		ref *pmvalue;
		ref *ppvalue;
		int policy;

		array_get(imemory, pkeys, ki, &key);
		if (dict_find(&aelt.dict, &key, &pmvalue) <= 0)
		    continue;
		if (dict_find(preq, &key, &prvalue) <= 0 ||
		    r_has_type(prvalue, t_null)
		    ) {
		    if (match_all)
			goto no;
		    else
			continue;
		}
		/* Look for the Policies entry for this key. */
		if (dict_find(ppol, &key, &ppvalue) > 0) {
		    check_type_only(*ppvalue, t_integer);
		    policy = ppvalue->value.intval;
		} else
		    policy = policy_default;
	/*
	 * Match a requested attribute value with the attribute value in the
	 * description of a medium.  For all attributes except PageSize,
	 * matching means equality.  PageSize is special; see match_page_size
	 * below.
	 */
		if (r_has_type(&key, t_name) &&
		    (name_string_ref(imemory, &key, &kstr),
		     r_size(&kstr) == 8 &&
		     !memcmp(kstr.value.bytes, "PageSize", 8))
		    ) {
		    gs_matrix ignore_mat;
		    gs_point ignore_msize;

		    if (zmatch_page_size(imemory, prvalue, pmvalue,
					 policy, orient, roll,
					 &best_mismatch,
					 &ignore_mat,
					 &ignore_msize)
			<= 0)
			goto no;
		} else if (!obj_eq(imemory, prvalue, pmvalue))
		    goto no;
	    }

	    mepos_penalty = (mepos < 0 || aelt.key.value.intval == mepos) ?
		0 : .001;

	    /* We have a match. Save the match in case no better match is found */
	    if (r_has_type(&match.match_key, t_null)) 
		match.match_key = aelt.key;
	    /*
	     * If it is a better match than the current best it supersedes it 
	     * regardless of priority. If the match is the same, then update 
	     * to the current only if the key value is lower.
	     */
	    if (best_mismatch + mepos_penalty <= mbest) {
		if (best_mismatch + mepos_penalty < mbest  ||
		    (r_has_type(&match.match_key, t_integer) &&
		     match.match_key.value.intval > aelt.key.value.intval)) {
		    reset_match(&match);
		    match.match_key = aelt.key;
		    mbest = best_mismatch + mepos_penalty;
		}
	    }
	    /* In case of a tie, see if the new match has priority. */
	    for (pi = match.priority; pi > 0;) {
		ref pri;

		pi--;
		array_get(imemory, ppriority, pi, &pri);
		if (obj_eq(imemory, &aelt.key, &pri)) {	/* Yes, higher priority. */
		    match.best_key = aelt.key;
		    match.priority = pi;
		    break;
		}
	    }
no:;
	}
    }
    if (r_has_type(&match.match_key, t_null)) {
	make_false(op - 3);
	pop(3);
    } else {
	if (r_has_type(&match.best_key, t_null))
	    op[-3] = match.match_key;
	else
	    op[-3] = match.best_key;
	make_true(op - 2);
	pop(2);
    }
    return 0;
}

/* [<req_x> <req_y>] [<med_x0> <med_y0> (<med_x1> <med_y1> | )]
 *     <policy> <orient|null> <roll> <matrix|null> .matchpagesize
 *   <matrix|null> <med_x> <med_y> true   -or-  false
 */
private int
zmatchpagesize(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_matrix mat;
    float ignore_mismatch = (float)max_long;
    gs_point media_size;
    int orient;
    bool roll;
    int code;

    check_type(op[-3], t_integer);
    if (r_has_type(op - 2, t_null))
	orient = -1;
    else {
	check_int_leu(op[-2], 3);
	orient = (int)op[-2].value.intval;
    }
    check_type(op[-1], t_boolean);
    roll = op[-1].value.boolval;
    code = zmatch_page_size(imemory, 
			    op - 5, op - 4, (int)op[-3].value.intval,
			    orient, roll,
			    &ignore_mismatch, &mat, &media_size);
    switch (code) {
	default:
	    return code;
	case 0:
	    make_false(op - 5);
	    pop(5);
	    break;
	case 1:
	    code = write_matrix(op, &mat);
	    if (code < 0 && !r_has_type(op, t_null))
		return code;
	    op[-5] = *op;
	    make_real(op - 4, media_size.x);
	    make_real(op - 3, media_size.y);
	    make_true(op - 2);
	    pop(2);
	    break;
    }
    return 0;
}
/* Match the PageSize.  See below for details. */
private int
match_page_size(const gs_point * request,
			     const gs_rect * medium,
			     int policy, int orient, bool roll,
			     float *best_mismatch, gs_matrix * pmat,
			     gs_point * pmsize);
private int
zmatch_page_size(const gs_memory_t *mem, const ref * pvreq, const ref * pvmed,
		 int policy, int orient, bool roll,
		 float *best_mismatch, gs_matrix * pmat, gs_point * pmsize)
{
    uint nr, nm;
    int code;
    ref rv[6];

    /* array_get checks array types and size. */
    /* This allows normal or packed arrays to be used */
    if ((code = array_get(mem, pvreq, 1, &rv[1])) < 0)
        return_error(code);
    nr = r_size(pvreq);
    if ((code = array_get(mem, pvmed, 1, &rv[3])) < 0)
        return_error(code);
    nm = r_size(pvmed);
    if (!((nm == 2 || nm == 4) && (nr == 2 || nr == nm)))
	return_error(e_rangecheck);
    {
	uint i;
	double v[6];
	int code;

	array_get(mem, pvreq, 0, &rv[0]);
	for (i = 0; i < 4; ++i)
	    array_get(mem,pvmed, i % nm, &rv[i + 2]);
	if ((code = num_params(rv + 5, 6, v)) < 0)
	    return code;
	{
	    gs_point request;
	    gs_rect medium;

	    request.x = v[0], request.y = v[1];
	    medium.p.x = v[2], medium.p.y = v[3],
		medium.q.x = v[4], medium.q.y = v[5];
	    return match_page_size(&request, &medium, policy, orient,
				   roll, best_mismatch, pmat, pmsize);
	}
    }
}
/*
 * Match a requested PageSize with the PageSize of a medium.  The medium
 * may specify either a single value [mx my] or a range
 * [mxmin mymin mxmax mymax]; matching means equality or inclusion
 * to within a tolerance of 5, possibly swapping the requested X and Y.
 * Take the Policies value into account, keeping track of the discrepancy
 * if needed.  When a match is found, also return the matrix to be
 * concatenated after setting up the default matrix, and the actual
 * media size.
 *
 * NOTE: The algorithm here doesn't work properly for variable-size media
 * when the match isn't exact.  We'll fix it if we ever need to.
 */
private void make_adjustment_matrix(const gs_point * request,
				    const gs_rect * medium,
				    gs_matrix * pmat,
				    bool scale, int rotate);
private int
match_page_size(const gs_point * request, const gs_rect * medium, int policy,
		int orient, bool roll, float *best_mismatch, gs_matrix * pmat,
		gs_point * pmsize)
{
    double rx = request->x, ry = request->y;

    if ((rx <= 0) || (ry <= 0))
	return_error(e_rangecheck);
    if (policy == 7) {
		/* (Adobe) hack: just impose requested values */
	*best_mismatch = 0;
	gs_make_identity(pmat);
	*pmsize = *request;
    } else {
        int fit_direct  = rx - medium->p.x >= -5 && rx - medium->q.x <= 5 
                       && ry - medium->p.y >= -5 && ry - medium->q.y <= 5;
	int fit_rotated = rx - medium->p.y >= -5 && rx - medium->q.y <= 5 
                       && ry - medium->p.x >= -5 && ry - medium->q.x <= 5;
        
	/* Fudge matches from a non-standard page size match (4 element array) */
	/* as worse than an exact match from a standard (2 element array), but */
	/* better than for a rotated match to a standard pagesize. This should */
	/* prevent rotation unless we have to (particularly for raster file    */
	/* formats like TIFF, JPEG, PNG, PCX, BMP, etc. and also should allow  */
	/* exact page size specification when there is a range PageSize entry. */
	/* As the comment in gs_setpd.ps says "Devices that care will provide  */
	/* a real InputAttributes dictionary (most without a range pagesize)   */
        if ( fit_direct && fit_rotated) {
	    make_adjustment_matrix(request, medium, pmat, false, orient < 0 ? 0 : orient);
	    if (medium->p.x < medium->q.x || medium->p.y < medium->q.y)
		*best_mismatch = (float)0.001;		/* fudge a match to a range as a small number */
	    else	/* should be 0 for an exact match */
	        *best_mismatch = fabs((rx - medium->p.x) * (medium->q.x - rx)) +
	    			fabs((ry - medium->p.y) * (medium->q.y - ry));
        } else if ( fit_direct ) {
            int rotate = orient < 0 ? 0 : orient;

	    make_adjustment_matrix(request, medium, pmat, false, (rotate + 1) & 2);
	    *best_mismatch = fabs((medium->p.x - rx) * (medium->q.x - rx)) +
	    			fabs((medium->p.y - ry) * (medium->q.y - ry)) + 
            			    (pmat->xx == 0.0 || (rotate & 1) == 1 ? 0.01 : 0);	/* rotated */
        } else if ( fit_rotated ) {
            int rotate = (orient < 0 ? 1 : orient);

	    make_adjustment_matrix(request, medium, pmat, false, rotate | 1);
	    *best_mismatch = fabs((medium->p.y - rx) * (medium->q.y - rx)) +
	    			fabs((medium->p.x - ry) * (medium->q.x - ry)) + 
            			    (pmat->xx == 0.0 || (rotate & 1) == 1 ? 0.01 : 0);	/* rotated */
        } else {
	    int rotate =
		(orient >= 0 ? orient :
		 (rx < ry) ^ (medium->q.x < medium->q.y));
	    bool larger =
		(rotate & 1 ? medium->q.y >= rx && medium->q.x >= ry :
		 medium->q.x >= rx && medium->q.y >= ry);
	    bool adjust = false;
	    float mismatch = medium->q.x * medium->q.y - rx * ry;

	    switch (policy) {
	        default:		/* exact match only */
		    return 0;
	        case 3:		/* nearest match, adjust */
		    adjust = true;
	        case 5:		/* nearest match, don't adjust */
		    if (fabs(mismatch) >= fabs(*best_mismatch))
		        return 0;
		    break;
	        case 4:		/* next larger match, adjust */
		    adjust = true;
	        case 6:		/* next larger match, don't adjust */
		    if (!larger || mismatch >= *best_mismatch)
		        return 0;
		    break;
	    }
	    if (adjust)
	        make_adjustment_matrix(request, medium, pmat, !larger, rotate);
	    else {
	        gs_rect req_rect;
                if(rotate & 1) { 
                    req_rect.p.x = ry;
                    req_rect.p.y = rx;
                } else {
                    req_rect.p.x = rx;
                    req_rect.p.y = ry;
                }
	        req_rect.q = req_rect.p;
	        make_adjustment_matrix(request, &req_rect, pmat, false, rotate);
	    }
	    *best_mismatch = fabs(mismatch);
        }
        if (pmat->xx == 0) {	/* Swap request X and Y. */
	    double temp = rx;

	    rx = ry, ry = temp;
        }
#define ADJUST_INTO(req, mmin, mmax)\
      (req < mmin ? mmin : req > mmax ? mmax : req)
        pmsize->x = ADJUST_INTO(rx, medium->p.x, medium->q.x);
        pmsize->y = ADJUST_INTO(ry, medium->p.y, medium->q.y);
#undef ADJUST_INTO
    }
    return 1;
}
/*
 * Compute the adjustment matrix for scaling and/or rotating the page
 * to match the medium.  If the medium is completely flexible in a given
 * dimension (e.g., roll media in one dimension, or displays in both),
 * we must adjust its size in that dimension to match the request.
 * We recognize this by an unreasonably small medium->p.{x,y}.
 */
private void 
make_adjustment_matrix(const gs_point * request, const gs_rect * medium,
		       gs_matrix * pmat, bool scale, int rotate)
{
    double rx = request->x, ry = request->y;
    double mx = medium->q.x, my = medium->q.y;

    /* Rotate the request if necessary. */ 
    if (rotate & 1) {
	double temp = rx;

	rx = ry, ry = temp;
    }
    /* If 'medium' is flexible, adjust 'mx' and 'my' towards 'rx' and 'ry',
       respectively. Note that 'mx' and 'my' have just acquired the largest
       permissible value, medium->q. */
    if (medium->p.x < mx) {	/* non-empty width range */
	if (rx < medium->p.x)
	    mx = medium->p.x;	/* use minimum of the range */
	else if (rx < mx)
	    mx = rx;		/* fits */
		/* else leave mx == medium->q.x, i.e., the maximum */
    }
    if (medium->p.y < my) {	/* non-empty height range */
	if (ry < medium->p.y)
	    my = medium->p.y;	/* use minimum of the range */
	else if (ry < my)
	    my = ry;		/* fits */
	    /* else leave my == medium->q.y, i.e., the maximum */
    }

    /* Translate to align the centers. */ 
    gs_make_translation(mx / 2, my / 2, pmat);

    /* Rotate if needed. */ 
    if (rotate)
	gs_matrix_rotate(pmat, 90.0 * rotate, pmat);

    /* Scale if needed. */ 
    if (scale) {
	double xfactor = mx / rx;
	double yfactor = my / ry;
	double factor = min(xfactor, yfactor);

	if (factor < 1)
	    gs_matrix_scale(pmat, factor, factor, pmat);
    }
    /* Now translate the origin back, */ 
    /* using the original, unswapped request. */ 
    gs_matrix_translate(pmat, -request->x / 2, -request->y / 2, pmat);
}
#undef MIN_MEDIA_SIZE

/* ------ Initialization procedure ------ */

const op_def zmedia2_l2_op_defs[] =
{
    op_def_begin_level2(),
    {"4.matchmedia", zmatchmedia},
    {"6.matchpagesize", zmatchpagesize},
    op_def_end(0)
};
