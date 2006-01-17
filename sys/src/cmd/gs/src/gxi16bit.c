/* Copyright (C) 1994, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxi16bit.c,v 1.2 2005/06/08 14:00:32 igor Exp $ */
/* 16-bit image procedures */
#include "gx.h"
#include "memory_.h"
#include "gpcheck.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxfrac.h"
#include "gxarith.h"
#include "gxmatrix.h"
#include "gsccolor.h"
#include "gspaint.h"
#include "gxdevice.h"
#include "gxcmap.h"
#include "gxdcolor.h"
#include "gxistate.h"
#include "gxdevmem.h"
#include "gxcpath.h"
#include "gximage.h"

/* ---------------- Unpacking procedures ---------------- */

private const byte *
sample_unpack_16(byte * bptr, int *pdata_x, const byte * data,
		 int data_x, uint dsize, const sample_map *ignore_smap, int spread,
		 int ignore_num_components_per_plane)
{
    /* Assuming an identity map for all components. */
    register frac *bufp = (frac *) bptr;
    uint dskip = data_x << 1;
    const byte *psrc = data + dskip;
#define inc_bufp(bp, n) bp = (frac *)((byte *)(bp) + (n))
    uint sample;
    int left = dsize - dskip;

    while (left > 2) {
	sample = ((uint) psrc[0] << 8) + psrc[1];
	*bufp = (frac)((frac_1 * (sample + 1)) >> 16);
	inc_bufp(bufp, spread);
	psrc += 2;
	left -= 2;
    }
    *pdata_x = 0;
    return bptr;
}

const sample_unpack_proc_t sample_unpack_16_proc = sample_unpack_16;

/* ---------------- Rendering procedures ---------------- */

/* ------ Rendering for 16-bit samples ------ */

#define FRACS_PER_LONG (arch_sizeof_long / arch_sizeof_frac)
typedef union {
    frac v[GS_IMAGE_MAX_COLOR_COMPONENTS];
#define LONGS_PER_COLOR_FRACS\
  ((GS_IMAGE_MAX_COLOR_COMPONENTS + FRACS_PER_LONG - 1) / FRACS_PER_LONG)
    long all[LONGS_PER_COLOR_FRACS];	/* for fast comparison */
} color_fracs;

#define LONGS_PER_4_FRACS ((FRACS_PER_LONG + 3) / 4)
#if LONGS_PER_4_FRACS == 1
#  define COLOR_FRACS_4_EQ(f1, f2)\
     ((f1).all[0] == (f2).all[0])
#else
#if LONGS_PER_4_FRACS == 2
#  define COLOR_FRACS_4_EQ(f1, f2)\
     ((f1).all[0] == (f2).all[0] && (f1).all[1] == (f2).all[1])
#endif
#endif

/* Test whether a color is transparent. */
private bool
mask_color16_matches(const frac *v, const gx_image_enum *penum,
		   int num_components)
{
    int i;

    for (i = num_components * 2, v += num_components - 1; (i -= 2) >= 0; --v)
	if (*v < penum->mask_color.values[i] ||
	    *v > penum->mask_color.values[i + 1]
	    )
	    return false;
    return true;
}

/* Render an image with more than 8 bits per sample. */
/* The samples have been expanded into fracs. */
private int
image_render_frac(gx_image_enum * penum, const byte * buffer, int data_x,
		  uint w, int h, gx_device * dev)
{
    const gs_imager_state *pis = penum->pis;
    gs_logical_operation_t lop = penum->log_op;
    gx_dda_fixed_point pnext;
    image_posture posture = penum->posture;
    fixed xl, ytf;
    fixed pdyx, pdyy;		/* edge of parallelogram */
    int yt = penum->yci, iht = penum->hci;
    const gs_color_space *pcs = penum->pcs;
    cs_proc_remap_color((*remap_color)) = pcs->type->remap_color;
    gs_client_color cc;
    bool device_color = penum->device_color;
    const gx_color_map_procs *cmap_procs = gx_get_cmap_procs(pis, dev);
    cmap_proc_rgb((*map_rgb)) = cmap_procs->map_rgb;
    cmap_proc_cmyk((*map_cmyk)) = cmap_procs->map_cmyk;
    bool use_mask_color = penum->use_mask_color;
    gx_device_color devc1, devc2;
    gx_device_color *pdevc = &devc1;
    gx_device_color *pdevc_next = &devc2;
    int spp = penum->spp;
    const frac *psrc_initial = (const frac *)buffer + data_x * spp;
    const frac *psrc = psrc_initial;
    const frac *rsrc = psrc + spp; /* psrc + spp at start of run */
    fixed xrun;			/* x at start of run */
    int irun;			/* int xrun */
    fixed yrun;			/* y ditto */
    color_fracs run;		/* run value */
    color_fracs next;		/* next sample value */
    const frac *bufend = psrc + w;
    int code = 0, mcode = 0;

    if (h == 0)
	return 0;
    pnext = penum->dda.pixel0;
    xrun = xl = dda_current(pnext.x);
    irun = fixed2int_var_rounded(xrun);
    yrun = ytf = dda_current(pnext.y);
    pdyx = dda_current(penum->dda.row.x) - penum->cur.x;
    pdyy = dda_current(penum->dda.row.y) - penum->cur.y;
    if_debug5('b', "[b]y=%d data_x=%d w=%d xt=%f yt=%f\n",
	      penum->y, data_x, w, fixed2float(xl), fixed2float(ytf));
    memset(&run, 0, sizeof(run));
    memset(&next, 0, sizeof(next));
    /* Ensure that we don't get any false dev_color_eq hits. */
    set_nonclient_dev_color(&devc1, gx_no_color_index);
    set_nonclient_dev_color(&devc2, gx_no_color_index);
    cs_full_init_color(&cc, pcs);
    run.v[0] = ~psrc[0];	/* force remap */

    while (psrc < bufend) {
	next.v[0] = psrc[0];
	switch (spp) {
	    case 4:		/* may be CMYK */
		next.v[1] = psrc[1];
		next.v[2] = psrc[2];
		next.v[3] = psrc[3];
		psrc += 4;
		if (COLOR_FRACS_4_EQ(next, run))
		    goto inc;
		if (use_mask_color && mask_color16_matches(next.v, penum, 4)) {
		    color_set_null(pdevc_next);
		    goto f;
		}
		if (device_color) {
		    (*map_cmyk) (next.v[0], next.v[1],
				 next.v[2], next.v[3],
				 pdevc_next, pis, dev,
				 gs_color_select_source);
		    goto f;
		}
		decode_frac(next.v[0], cc, 0);
		decode_frac(next.v[1], cc, 1);
		decode_frac(next.v[2], cc, 2);
		decode_frac(next.v[3], cc, 3);
		if_debug4('B', "[B]cc[0..3]=%g,%g,%g,%g\n",
			  cc.paint.values[0], cc.paint.values[1],
			  cc.paint.values[2], cc.paint.values[3]);
		if_debug1('B', "[B]cc[3]=%g\n",
			  cc.paint.values[3]);
		break;
	    case 3:		/* may be RGB */
		next.v[1] = psrc[1];
		next.v[2] = psrc[2];
		psrc += 3;
		if (COLOR_FRACS_4_EQ(next, run))
		    goto inc;
		if (use_mask_color && mask_color16_matches(next.v, penum, 3)) {
		    color_set_null(pdevc_next);
		    goto f;
		}
		if (device_color) {
		    (*map_rgb) (next.v[0], next.v[1],
				next.v[2], pdevc_next, pis, dev,
				gs_color_select_source);
		    goto f;
		}
		decode_frac(next.v[0], cc, 0);
		decode_frac(next.v[1], cc, 1);
		decode_frac(next.v[2], cc, 2);
		if_debug3('B', "[B]cc[0..2]=%g,%g,%g\n",
			  cc.paint.values[0], cc.paint.values[1],
			  cc.paint.values[2]);
		break;
	    case 1:		/* may be Gray */
		psrc++;
		if (next.v[0] == run.v[0])
		    goto inc;
		if (use_mask_color && mask_color16_matches(next.v, penum, 1)) {
		    color_set_null(pdevc_next);
		    goto f;
		}
		if (device_color) {
		    (*map_rgb) (next.v[0], next.v[0],
				next.v[0], pdevc_next, pis, dev,
				gs_color_select_source);
		    goto f;
		}
		decode_frac(next.v[0], cc, 0);
		if_debug1('B', "[B]cc[0]=%g\n",
			  cc.paint.values[0]);
		break;
	    default:		/* DeviceN */
		{
		    int i;

		    for (i = 1; i < spp; ++i)
			next.v[i] = psrc[i];
		    psrc += spp;
		    if (!memcmp(next.v, run.v, spp * sizeof(next.v[0])))
			goto inc;
		    if (use_mask_color &&
			mask_color16_matches(next.v, penum, spp)
			) {
			color_set_null(pdevc_next);
			goto f;
		    }
		    for (i = 0; i < spp; ++i)
			decode_frac(next.v[i], cc, i);
#ifdef DEBUG
		    if (gs_debug_c('B')) {
			dprintf2("[B]cc[0..%d]=%g", spp - 1,
				 cc.paint.values[0]);
			for (i = 1; i < spp; ++i)
			    dprintf1(",%g", cc.paint.values[i]);
			dputs("\n");
		    }
#endif
		}
		break;
	}
	mcode = remap_color(&cc, pcs, pdevc_next, pis, dev,
			   gs_color_select_source);
	if (mcode < 0)
	    goto fill;
f:
	if_debug7('B', "[B]0x%x,0x%x,0x%x,0x%x -> %ld,%ld,0x%lx\n",
		  next.v[0], next.v[1], next.v[2], next.v[3],
		  pdevc_next->colors.binary.color[0],
		  pdevc_next->colors.binary.color[1],
		  (ulong) pdevc_next->type);
	/* Even though the supplied colors don't match, */
	/* the device colors might. */
	if (!dev_color_eq(devc1, devc2)) {
	    /* Fill the region between xrun/irun and xl */
	    gx_device_color *ptemp;

fill:
	    if (posture != image_portrait) {	/* Parallelogram */
		code = (*dev_proc(dev, fill_parallelogram))
		    (dev, xrun, yrun,
		     xl - xrun, ytf - yrun, pdyx, pdyy,
		     pdevc, lop);
	    } else {		/* Rectangle */
		int xi = irun;
		int wi = (irun = fixed2int_var_rounded(xl)) - xi;

		if (wi < 0)
		    xi += wi, wi = -wi;
		code = gx_fill_rectangle_device_rop(xi, yt,
						  wi, iht, pdevc, dev, lop);
	    }
	    if (code < 0)
		goto err;
	    rsrc = psrc;
	    if ((code = mcode) < 0)
		goto err;
	    ptemp = pdevc;
	    pdevc = pdevc_next;
	    pdevc_next = ptemp;
	    xrun = xl;
	    yrun = ytf;
	}
	run = next;
inc:
	xl = dda_next(pnext.x);
	ytf = dda_next(pnext.y);
    }
    /* Fill the final run. */
    code = (*dev_proc(dev, fill_parallelogram))
	(dev, xrun, yrun, xl - xrun, ytf - yrun, pdyx, pdyy, pdevc, lop);
    return (code < 0 ? code : 1);

    /* Save position if error, in case we resume. */
err:
    penum->used.x = (rsrc - spp - psrc_initial) / spp;
    penum->used.y = 0;
    return code;
}
