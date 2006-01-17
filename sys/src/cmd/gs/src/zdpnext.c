/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zdpnext.c,v 1.8 2005/03/14 18:08:37 dan Exp $ */
/* NeXT Display PostScript extensions */
#include "math_.h"
#include "ghost.h"
#include "oper.h"
#include "gscoord.h"
#include "gscspace.h"		/* for iimage.h */
#include "gsdpnext.h"
#include "gsmatrix.h"
#include "gsiparam.h"		/* for iimage.h */
#include "gsiparm2.h"
#include "gspath2.h"
#include "gxcvalue.h"
#include "gxdevice.h"
#include "gxsample.h"
#include "ialloc.h"
#include "igstate.h"
#include "iimage.h"
#include "iimage2.h"
#include "store.h"

/* ------ alpha channel ------ */

/* - currentalpha <alpha> */
private int
zcurrentalpha(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    make_real(op, gs_currentalpha(igs));
    return 0;
}

/* <alpha> setalpha - */
private int
zsetalpha(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double alpha;
    int code;

    if (real_param(op, &alpha) < 0)
	return_op_typecheck(op);
    if ((code = gs_setalpha(igs, alpha)) < 0)
	return code;
    pop(1);
    return 0;
}

/* ------ Imaging/compositing ------ */

/*
 * Miscellaneous notes:
 *
 * composite / dissolve respect destination clipping (both clip & viewclip),
 *   but ignore source clipping.
 * composite / dissolve must handle overlapping source/destination correctly.
 * compositing converts the source to the destination's color model
 *   (including halftoning if needed).
 */

/*
 * Define the operand and bookeeping structure for a compositing operation.
 */
typedef struct alpha_composite_state_s {
    /* Compositing parameters */
    gs_composite_alpha_params_t params;
    /* Temporary structures */
    gs_composite_t *pcte;
    gx_device *cdev;
    gx_device *orig_dev;
} alpha_composite_state_t;

/* Forward references */
private int begin_composite(i_ctx_t *, alpha_composite_state_t *);
private void end_composite(i_ctx_t *, alpha_composite_state_t *);
private int xywh_param(os_ptr, double[4]);

/* <dict> .alphaimage - */
/* This is the dictionary version of the alphaimage operator, which is */
/* now a pseudo-operator (see gs_dpnxt.ps). */
private int
zalphaimage(i_ctx_t *i_ctx_p)
{
    return image1_setup(i_ctx_p, true);
}

/* <destx> <desty> <width> <height> <op> compositerect - */
private int
zcompositerect(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    double dest_rect[4];
    alpha_composite_state_t cstate;
    int code = xywh_param(op - 1, dest_rect);

    if (code < 0)
	return code;
    check_int_leu(*op, compositerect_last);
    cstate.params.op = (gs_composite_op_t) op->value.intval;
    code = begin_composite(i_ctx_p, &cstate);
    if (code < 0)
	return code;
    {
	gs_rect rect;

	rect.q.x = (rect.p.x = dest_rect[0]) + dest_rect[2];
	rect.q.y = (rect.p.y = dest_rect[1]) + dest_rect[3];
	code = gs_rectfill(igs, &rect, 1);
    }
    end_composite(i_ctx_p, &cstate);
    if (code >= 0)
	pop(5);
    return code;
}

/* Common code for composite and dissolve. */
private int
composite_image(i_ctx_t *i_ctx_p, const gs_composite_alpha_params_t * params)
{
    os_ptr op = osp;
    alpha_composite_state_t cstate;
    gs_image2_t image;
    double src_rect[4];
    double dest_pt[2];
    gs_matrix save_ctm;
    int code = xywh_param(op - 4, src_rect);

    cstate.params = *params;
    gs_image2_t_init(&image);
    if (code < 0 ||
	(code = num_params(op - 1, 2, dest_pt)) < 0
	)
	return code;
    if (r_has_type(op - 3, t_null))
	image.DataSource = igs;
    else {
	check_stype(op[-3], st_igstate_obj);
	check_read(op[-3]);
	image.DataSource = igstate_ptr(op - 3);
    }
    image.XOrigin = src_rect[0];
    image.YOrigin = src_rect[1];
    image.Width = src_rect[2];
    image.Height = src_rect[3];
    image.PixelCopy = true;
    /* Compute appropriate transformations. */
    gs_currentmatrix(igs, &save_ctm);
    gs_translate(igs, dest_pt[0], dest_pt[1]);
    gs_make_identity(&image.ImageMatrix);
    if (image.DataSource == igs) {
	image.XOrigin -= dest_pt[0];
	image.YOrigin -= dest_pt[1];
    }
    code = begin_composite(i_ctx_p, &cstate);
    if (code >= 0) {
	code = process_non_source_image(i_ctx_p,
					(const gs_image_common_t *)&image,
					"composite_image");
	end_composite(i_ctx_p, &cstate);
	if (code >= 0)
	    pop(8);
    }
    gs_setmatrix(igs, &save_ctm);
    return code;
}

/* <srcx> <srcy> <width> <height> <srcgstate|null> <destx> <desty> <op> */
/*   composite - */
private int
zcomposite(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_composite_alpha_params_t params;

    check_int_leu(*op, composite_last);
    params.op = (gs_composite_op_t) op->value.intval;
    return composite_image(i_ctx_p, &params);
}

/* <srcx> <srcy> <width> <height> <srcgstate|null> <destx> <desty> <delta> */
/*   dissolve - */
private int
zdissolve(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_composite_alpha_params_t params;
    double delta;
    int code = real_param(op, &delta);

    if (code < 0)
	return code;
    if (delta < 0 || delta > 1)
	return_error(e_rangecheck);
    params.op = composite_Dissolve;
    params.delta = delta;
    return composite_image(i_ctx_p, &params);
}

/* ------ Image reading ------ */

private int device_is_true_color(gx_device * dev);

/* <x> <y> <width> <height> <matrix> .sizeimagebox */
/*   <dev_x> <dev_y> <dev_width> <dev_height> <matrix> */
private void box_confine(int *pp, int *pq, int wh);
private int
zsizeimagebox(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const gx_device *dev = gs_currentdevice(igs);
    gs_rect srect, drect;
    gs_matrix mat;
    gs_int_rect rect;
    int w, h;
    int code;

    check_type(op[-4], t_integer);
    check_type(op[-3], t_integer);
    check_type(op[-2], t_integer);
    check_type(op[-1], t_integer);
    srect.p.x = op[-4].value.intval;
    srect.p.y = op[-3].value.intval;
    srect.q.x = srect.p.x + op[-2].value.intval;
    srect.q.y = srect.p.y + op[-1].value.intval;
    gs_currentmatrix(igs, &mat);
    gs_bbox_transform(&srect, &mat, &drect);
    /*
     * We want the dimensions of the image as a source, not a
     * destination, so we need to expand it rather than pixround.
     */
    rect.p.x = (int)floor(drect.p.x);
    rect.p.y = (int)floor(drect.p.y);
    rect.q.x = (int)ceil(drect.q.x);
    rect.q.y = (int)ceil(drect.q.y);
    /*
     * Clip the rectangle to the device boundaries, since that's what
     * the NeXT implementation does.
     */
    box_confine(&rect.p.x, &rect.q.x, dev->width);
    box_confine(&rect.p.y, &rect.q.y, dev->height);
    w = rect.q.x - rect.p.x;
    h = rect.q.y - rect.p.y;
    /*
     * The NeXT documentation doesn't specify very clearly what is
     * supposed to be in the matrix: the following produces results
     * that match testing on an actual NeXT system.
     */
    mat.tx -= rect.p.x;
    mat.ty -= rect.p.y;
    code = write_matrix(op, &mat);
    if (code < 0)
	return code;
    make_int(op - 4, rect.p.x);
    make_int(op - 3, rect.p.y);
    make_int(op - 2, w);
    make_int(op - 1, h);
    return 0;
}
private void
box_confine(int *pp, int *pq, int wh)
{
    if ( *pq <= 0 )
	*pp = *pq = 0;
    else if ( *pp >= wh )
	*pp = *pq = wh;
    else {
	if ( *pp < 0 )
	    *pp = 0;
	if ( *pq > wh )
	    *pq = wh;
    }
}

/* - .sizeimageparams <bits/sample> <multiproc> <ncolors> */
private int
zsizeimageparams(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gx_device *dev = gs_currentdevice(igs);
    int ncomp = dev->color_info.num_components;
    int bps;

    push(3);
    if (device_is_true_color(dev))
	bps = dev->color_info.depth / ncomp;
    else {
	/*
	 * Set bps to the smallest allowable number of bits that is
	 * sufficient to represent the number of different colors.
	 */
	gx_color_value max_value =
	    (dev->color_info.num_components == 1 ?
	     dev->color_info.max_gray :
	     max(dev->color_info.max_gray, dev->color_info.max_color));
	static const gx_color_value sizes[] = {
	    1, 2, 4, 8, 12, sizeof(gx_max_color_value) * 8
	};
	int i;

	for (i = 0;; ++i)
	    if (max_value <= ((ulong) 1 << sizes[i]) - 1)
		break;
	bps = sizes[i];
    }
    make_int(op - 2, bps);
    make_false(op - 1);
    make_int(op, ncomp);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zdpnext_op_defs[] =
{
    {"0currentalpha", zcurrentalpha},
    {"1setalpha", zsetalpha},
    {"1.alphaimage", zalphaimage},
    {"8composite", zcomposite},
    {"5compositerect", zcompositerect},
    {"8dissolve", zdissolve},
    {"5.sizeimagebox", zsizeimagebox},
    {"0.sizeimageparams", zsizeimageparams},
    op_def_end(0)
};

/* ------ Internal routines ------ */

/* Collect a rect operand. */
private int
xywh_param(os_ptr op, double rect[4])
{
    int code = num_params(op, 4, rect);

    if (code < 0)
	return code;
    if (rect[2] < 0)
	rect[0] += rect[2], rect[2] = -rect[2];
    if (rect[3] < 0)
	rect[1] += rect[3], rect[3] = -rect[3];
    return code;
}

/* Begin a compositing operation. */
private int
begin_composite(i_ctx_t *i_ctx_p, alpha_composite_state_t * pcp)
{
    gx_device *dev = gs_currentdevice(igs);
    int code =
	gs_create_composite_alpha(&pcp->pcte, &pcp->params, imemory);

    if (code < 0)
	return code;
    pcp->orig_dev = pcp->cdev = dev;	/* for end_composite */
    code = (*dev_proc(dev, create_compositor))
	(dev, &pcp->cdev, pcp->pcte, (gs_imager_state *)igs, imemory);
    if (code < 0) {
	end_composite(i_ctx_p, pcp);
	return code;
    }
    gs_setdevice_no_init(igs, pcp->cdev);
    return 0;
}

/* End a compositing operation. */
private void
end_composite(i_ctx_t *i_ctx_p, alpha_composite_state_t * pcp)
{
    /* Close and free the compositor and the compositing object. */
    if (pcp->cdev != pcp->orig_dev) {
	gs_closedevice(pcp->cdev);	/* also frees the device */
	gs_setdevice_no_init(igs, pcp->orig_dev);
    }
    ifree_object(pcp->pcte, "end_composite(gs_composite_t)");
}

/*
 * Determine whether a device has decomposed pixels with the components
 * in the standard PostScript order, and a 1-for-1 color map
 * (possibly inverted).  Return 0 if not true color, 1 if true color,
 * -1 if inverted true color.
 */
private int
device_is_true_color(gx_device * dev)
{
    int ncomp = dev->color_info.num_components;
    int depth = dev->color_info.depth;
    int i, max_v;

#define CV(i) (gx_color_value)((ulong)gx_max_color_value * i / max_v)
#define CV0 ((gx_color_value)0)

    /****** DOESN'T HANDLE INVERSION YET ******/
    switch (ncomp) {
	case 1:		/* gray-scale */
	    max_v = dev->color_info.max_gray;
	    if (max_v != (1 << depth) - 1)
		return 0;
	    for (i = 0; i <= max_v; ++i) {
		gx_color_value v[3];
                v[0] = v[1] = v[2] = CV(i);
		if ((*dev_proc(dev, map_rgb_color)) (dev, v) != i)
		    return 0;
	    }
	    return true;
	case 3:		/* RGB */
	    max_v = dev->color_info.max_color;
	    if (depth % 3 != 0 || max_v != (1 << (depth / 3)) - 1)
		return false;
	    {
		const int gs = depth / 3, rs = gs * 2;

		for (i = 0; i <= max_v; ++i) {
		    gx_color_value red[3];
                    gx_color_value green[3];
                    gx_color_value blue[3];
                    red[0] = CV(i); red[1] = CV0, red[2] = CV0;
                    green[0] = CV0; green[1] = CV(i); green[2] = CV0;
                    blue[0] = CV0; blue[1] = CV0; blue[2] = CV(i);
		    if ((*dev_proc(dev, map_rgb_color)) (dev, red) !=
			i << rs ||
			(*dev_proc(dev, map_rgb_color)) (dev, green) !=
			i << gs ||
			(*dev_proc(dev, map_rgb_color)) (dev, blue) !=
			i	/*<< bs */
			)
			return 0;
		}
	    }
	    return true;
	case 4:		/* CMYK */
	    max_v = dev->color_info.max_color;
	    if ((depth & 3) != 0 || max_v != (1 << (depth / 4)) - 1)
		return false;
	    {
		const int ys = depth / 4, ms = ys * 2, cs = ys * 3;

		for (i = 0; i <= max_v; ++i) {
                    
		    gx_color_value cyan[4];
                    gx_color_value magenta[4];
                    gx_color_value yellow[4];
                    gx_color_value black[4];
                    cyan[0] = CV(i); cyan[1] = cyan[2] = cyan[3] = CV0;
                    magenta[1] = CV(i); magenta[0] = magenta[2] = magenta[3] = CV0;
                    yellow[2] = CV(i); yellow[0] = yellow[1] = yellow[3] = CV0;
                    black[3] = CV(i); black[0] = black[1] = black[2] = CV0;
		    if ((*dev_proc(dev, map_cmyk_color)) (dev, cyan) !=
			i << cs ||
			(*dev_proc(dev, map_cmyk_color)) (dev, magenta) !=
			i << ms ||
			(*dev_proc(dev, map_cmyk_color)) (dev, yellow) !=
			i << ys ||
			(*dev_proc(dev, map_cmyk_color)) (dev, black) !=
			i	/*<< ks */
			)
			return 0;
		}
	    }
	    return 1;
	default:
	    return 0;		/* DeviceN */
    }
#undef CV
#undef CV0
}
