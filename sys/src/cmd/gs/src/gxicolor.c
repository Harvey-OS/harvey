/* Copyright (C) 1992, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxicolor.c,v 1.9 2003/08/18 21:21:57 dan Exp $ */
/* Color image rendering */
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
#include "gzstate.h"
#include "gxdevice.h"
#include "gxcmap.h"
#include "gxdcconv.h"
#include "gxdcolor.h"
#include "gxistate.h"
#include "gxdevmem.h"
#include "gxcpath.h"
#include "gximage.h"

typedef union {
    byte v[GS_IMAGE_MAX_COLOR_COMPONENTS];
#define BYTES_PER_BITS32 4
#define BITS32_PER_COLOR_SAMPLES\
  ((GS_IMAGE_MAX_COLOR_COMPONENTS + BYTES_PER_BITS32 - 1) / BYTES_PER_BITS32)
    bits32 all[BITS32_PER_COLOR_SAMPLES];	/* for fast comparison */
} color_samples;

/* ------ Strategy procedure ------ */

/* Check the prototype. */
iclass_proc(gs_image_class_4_color);

private irender_proc(image_render_color);
irender_proc_t
gs_image_class_4_color(gx_image_enum * penum)
{
    if (penum->use_mask_color) {
	/*
	 * Scale the mask colors to match the scaling of each sample to
	 * a full byte, and set up the quick-filter parameters.
	 */
	int i;
	color_samples mask, test;
	bool exact = penum->spp <= BYTES_PER_BITS32;

	memset(&mask, 0, sizeof(mask));
	memset(&test, 0, sizeof(test));
	for (i = 0; i < penum->spp; ++i) {
	    byte v0, v1;
	    byte match = 0xff;

	    gx_image_scale_mask_colors(penum, i);
	    v0 = (byte)penum->mask_color.values[2 * i];
	    v1 = (byte)penum->mask_color.values[2 * i + 1];
	    while ((v0 & match) != (v1 & match))
		match <<= 1;
	    mask.v[i] = match;
	    test.v[i] = v0 & match;
	    exact &= (v0 == match && (v1 | match) == 0xff);
	}
	penum->mask_color.mask = mask.all[0];
	penum->mask_color.test = test.all[0];
	penum->mask_color.exact = exact;
    } else {
	penum->mask_color.mask = 0;
	penum->mask_color.test = ~0;
    }
    return &image_render_color;
}

/* ------ Rendering procedures ------ */

/* Test whether a color is transparent. */
private bool
mask_color_matches(const byte *v, const gx_image_enum *penum,
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

/* Render a color image with 8 or fewer bits per sample. */
private int
image_render_color(gx_image_enum *penum_orig, const byte *buffer, int data_x,
		   uint w, int h, gx_device * dev)
{
    const gx_image_enum *const penum = penum_orig; /* const within proc */
    gx_image_clue *const clues = penum_orig->clues; /* not const */
    const gs_imager_state *pis = penum->pis;
    gs_logical_operation_t lop = penum->log_op;
    gx_dda_fixed_point pnext;
    image_posture posture = penum->posture;
    fixed xprev, yprev;
    fixed pdyx, pdyy;		/* edge of parallelogram */
    int vci, vdi;
    const gs_color_space *pcs = penum->pcs;
    cs_proc_remap_color((*remap_color)) = pcs->type->remap_color;
    cs_proc_remap_concrete_color((*remap_concrete_color)) =
	    pcs->type->remap_concrete_color;
    gs_client_color cc;
    bool device_color = penum->device_color;
    const gx_color_map_procs *cmap_procs = gx_get_cmap_procs(pis, dev);
    bits32 mask = penum->mask_color.mask;
    bits32 test = penum->mask_color.test;
    gx_image_clue *pic = &clues[0];
#define pdevc (&pic->dev_color)
    gx_image_clue *pic_next = &clues[1];
#define pdevc_next (&pic_next->dev_color)
    gx_image_clue empty_clue;
    gx_image_clue clue_temp;
    int spp = penum->spp;
    const byte *psrc_initial = buffer + data_x * spp;
    const byte *psrc = psrc_initial;
    const byte *rsrc = psrc + spp; /* psrc + spp at start of run */
    fixed xrun;			/* x ditto */
    fixed yrun;			/* y ditto */
    int irun;			/* int x/rrun */
    color_samples run;		/* run value */
    color_samples next;		/* next sample value */
    const byte *bufend = psrc + w;
    bool use_cache = spp * penum->bps <= 12;
    int code = 0, mcode = 0;

    if (h == 0)
	return 0;
    pnext = penum->dda.pixel0;
    xrun = xprev = dda_current(pnext.x);
    yrun = yprev = dda_current(pnext.y);
    pdyx = dda_current(penum->dda.row.x) - penum->cur.x;
    pdyy = dda_current(penum->dda.row.y) - penum->cur.y;
    switch (posture) {
	case image_portrait:
	    vci = penum->yci, vdi = penum->hci;
	    irun = fixed2int_var_rounded(xrun);
	    break;
	case image_landscape:
	default:    /* we don't handle skew -- treat as landscape */
	    vci = penum->xci, vdi = penum->wci;
	    irun = fixed2int_var_rounded(yrun);
	    break;
    }

    if_debug5('b', "[b]y=%d data_x=%d w=%d xt=%f yt=%f\n",
	      penum->y, data_x, w, fixed2float(xprev), fixed2float(yprev));
    memset(&run, 0, sizeof(run));
    memset(&next, 0, sizeof(next));
    /* Ensure that we don't get any false dev_color_eq hits. */
    if (use_cache) {
	set_nonclient_dev_color(&empty_clue.dev_color, gx_no_color_index);
	pic = &empty_clue;
    }
    cs_full_init_color(&cc, pcs);
    run.v[0] = ~psrc[0];	/* force remap */
    while (psrc < bufend) {
	dda_next(pnext.x);
	dda_next(pnext.y);
#define CLUE_HASH3(penum, next)\
  &clues[(next.v[0] + (next.v[1] << 2) + (next.v[2] << 4)) & 255];
#define CLUE_HASH4(penum, next)\
  &clues[(next.v[0] + (next.v[1] << 2) + (next.v[2] << 4) +\
		 (next.v[3] << 6)) & 255]

	if (spp == 4) {		/* may be CMYK or RGBA */
	    next.v[0] = psrc[0];
	    next.v[1] = psrc[1];
	    next.v[2] = psrc[2];
	    next.v[3] = psrc[3];
	    psrc += 4;
map4:	    if (next.all[0] == run.all[0])
		goto inc;
	    if (use_cache) {
		pic_next = CLUE_HASH4(penum, next);
		if (pic_next->key == next.all[0])
		    goto f;
		/*
		 * If we are really unlucky, pic_next == pic,
		 * so mapping this color would clobber the one
		 * we're about to use for filling the run.
		 */
		if (pic_next == pic) {
		    clue_temp = *pic;
		    pic = &clue_temp;
		}
		pic_next->key = next.all[0];
	    }
	    /* Check for transparent color. */
	    if ((next.all[0] & mask) == test &&
		(penum->mask_color.exact ||
		 mask_color_matches(next.v, penum, 4))
		) {
		color_set_null(pdevc_next);
		goto mapped;
	    }
	    if (device_color) {
		frac frac_color[4];

		if (penum->alpha) {
		    /*
		     * We do not have support for DeviceN color and alpha.
		     */
		    cmap_procs->map_rgb_alpha
			(byte2frac(next.v[0]), byte2frac(next.v[1]),
			 byte2frac(next.v[2]), byte2frac(next.v[3]),
			 pdevc_next, pis, dev,
			 gs_color_select_source);
		    goto mapped;
		}
		/*
		 * We can call the remap concrete_color for the colorspace
		 * directly since device_color is only true if the colorspace
		 * is concrete.
		 */
		frac_color[0] = byte2frac(next.v[0]);
		frac_color[1] = byte2frac(next.v[1]);
		frac_color[2] = byte2frac(next.v[2]);
		frac_color[3] = byte2frac(next.v[3]);
		remap_concrete_color(frac_color, pcs, pdevc_next, pis,
					    dev, gs_color_select_source);
		goto mapped;
	    }
	    decode_sample(next.v[3], cc, 3);
	    if_debug1('B', "[B]cc[3]=%g\n", cc.paint.values[3]);
do3:	    decode_sample(next.v[0], cc, 0);
	    decode_sample(next.v[1], cc, 1);
	    decode_sample(next.v[2], cc, 2);
	    if_debug3('B', "[B]cc[0..2]=%g,%g,%g\n",
		      cc.paint.values[0], cc.paint.values[1],
		      cc.paint.values[2]);
	} else if (spp == 3) {	    /* may be RGB */
	    next.v[0] = psrc[0];
	    next.v[1] = psrc[1];
	    next.v[2] = psrc[2];
	    psrc += 3;
	    if (next.all[0] == run.all[0])
		goto inc;
	    if (use_cache) {
		pic_next = CLUE_HASH3(penum, next);
		if (pic_next->key == next.all[0])
		    goto f;
		/* See above re the following check. */
		if (pic_next == pic) {
		    clue_temp = *pic;
		    pic = &clue_temp;
		}
		pic_next->key = next.all[0];
	    }
	    /* Check for transparent color. */
	    if ((next.all[0] & mask) == test &&
		(penum->mask_color.exact ||
		 mask_color_matches(next.v, penum, 3))
		) {
		color_set_null(pdevc_next);
		goto mapped;
	    }
	    if (device_color) {
		frac frac_color[3];
		/*
		 * We can call the remap concrete_color for the colorspace
		 * directly since device_color is only true if the colorspace
		 * is concrete.
		 */
		frac_color[0] = byte2frac(next.v[0]);
		frac_color[1] = byte2frac(next.v[1]);
		frac_color[2] = byte2frac(next.v[2]);
		remap_concrete_color(frac_color, pcs, pdevc_next, pis,
						dev, gs_color_select_source);
		goto mapped;
	    }
	    goto do3;
	} else if (penum->alpha) {
	    if (spp == 2) {	/* might be Gray + alpha */
		next.v[2] = next.v[1] = next.v[0] = psrc[0];
		next.v[3] = psrc[1];
		psrc += 2;
		goto map4;
	    } else if (spp == 5) {	/* might be CMYK + alpha */
		/* Convert CMYK to RGB. */
		frac rgb[3];

		color_cmyk_to_rgb(byte2frac(psrc[0]), byte2frac(psrc[1]),
				  byte2frac(psrc[2]), byte2frac(psrc[3]),
				  pis, rgb);
		/*
		 * It seems silly to do all this converting between
		 * fracs and bytes, but that's what the current
		 * APIs require.
		 */
		next.v[0] = frac2byte(rgb[0]);
		next.v[1] = frac2byte(rgb[1]);
		next.v[2] = frac2byte(rgb[2]);
		next.v[3] = psrc[4];
		psrc += 5;
		goto map4;
	    }
	} else {		/* DeviceN */
	    int i;

	    use_cache = false;	/* should do in initialization */
	    if (!memcmp(psrc, run.v, spp)) {
		psrc += spp;
		goto inc;
	    }
	    memcpy(next.v, psrc, spp);
	    psrc += spp;
	    if ((next.all[0] & mask) == test &&
		(penum->mask_color.exact ||
		 mask_color_matches(next.v, penum, spp))
		) {
		color_set_null(pdevc_next);
		goto mapped;
	    }
	    for (i = 0; i < spp; ++i)
		decode_sample(next.v[i], cc, i);
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
	mcode = remap_color(&cc, pcs, pdevc_next, pis, dev,
			   gs_color_select_source);
	if (mcode < 0)
	    goto fill;
mapped:	if (pic == pic_next)
	    goto fill;
f:	if_debug7('B', "[B]0x%x,0x%x,0x%x,0x%x -> %ld,%ld,0x%lx\n",
		  next.v[0], next.v[1], next.v[2], next.v[3],
		  pdevc_next->colors.binary.color[0],
		  pdevc_next->colors.binary.color[1],
		  (ulong) pdevc_next->type);
	/* Even though the supplied colors don't match, */
	/* the device colors might. */
	if (dev_color_eq(*pdevc, *pdevc_next))
	    goto set;
fill:	/* Fill the region between */
	/* xrun/irun and xprev */
        /*
	 * Note;  This section is nearly a copy of a simlar section below
         * for processing the last image pixel in the loop.  This would have been
         * made into a subroutine except for complications about the number of
         * variables that would have been needed to be passed to the routine.
	 */
	switch (posture) {
	case image_portrait:
	    {		/* Rectangle */
		int xi = irun;
		int wi = (irun = fixed2int_var_rounded(xprev)) - xi;

		if (wi < 0)
		    xi += wi, wi = -wi;
		if (wi > 0)
		    code = gx_fill_rectangle_device_rop(xi, vci, wi, vdi,
							pdevc, dev, lop);
	    }
	    break;
	case image_landscape:
	    {		/* 90 degree rotated rectangle */
		int yi = irun;
		int hi = (irun = fixed2int_var_rounded(yprev)) - yi;

		if (hi < 0)
		    yi += hi, hi = -hi;
		if (hi > 0)
		    code = gx_fill_rectangle_device_rop(vci, yi, vdi, hi,
							pdevc, dev, lop);
	    }
	    break;
	default:
	    {		/* Parallelogram */
		code = (*dev_proc(dev, fill_parallelogram))
		    (dev, xrun, yrun, xprev - xrun, yprev - yrun, pdyx, pdyy,
		     pdevc, lop);
		xrun = xprev;
		yrun = yprev;
	    }
	}
	if (code < 0)
	    goto err;
	rsrc = psrc;
	if ((code = mcode) < 0) {
	    /* Invalidate any partially built cache entry. */
	    if (use_cache)
		pic_next->key = ~next.all[0];
	    goto err;
	}
	if (use_cache)
	    pic = pic_next;
	else {
	    gx_image_clue *ptemp = pic;

	    pic = pic_next;
	    pic_next = ptemp;
	}
set:	run = next;
inc:	xprev = dda_current(pnext.x);
	yprev = dda_current(pnext.y);	/* harmless if no skew */
    }
    /* Fill the last run. */
    /*
     * Note;  This section is nearly a copy of a simlar section above
     * for processing an image pixel in the loop.  This would have been
     * made into a subroutine except for complications about the number
     * variables that would have been needed to be passed to the routine.
     */
    switch (posture) {
    	case image_portrait:
	    {		/* Rectangle */
		int xi = irun;
		int wi = (irun = fixed2int_var_rounded(xprev)) - xi;

		if (wi < 0)
		    xi += wi, wi = -wi;
		if (wi > 0)
		    code = gx_fill_rectangle_device_rop(xi, vci, wi, vdi,
							pdevc, dev, lop);
	    }
	    break;
	case image_landscape:
	    {		/* 90 degree rotated rectangle */
		int yi = irun;
		int hi = (irun = fixed2int_var_rounded(yprev)) - yi;

		if (hi < 0)
		    yi += hi, hi = -hi;
		if (hi > 0)
		    code = gx_fill_rectangle_device_rop(vci, yi, vdi, hi,
							pdevc, dev, lop);
	    }
	    break;
	default:
	    {		/* Parallelogram */
		code = (*dev_proc(dev, fill_parallelogram))
		    (dev, xrun, yrun, xprev - xrun, yprev - yrun, pdyx, pdyy,
		     pdevc, lop);
	    }
    }
    return (code < 0 ? code : 1);
    /* Save position if error, in case we resume. */
err:
    penum_orig->used.x = (rsrc - spp - psrc_initial) / spp;
    penum_orig->used.y = 0;
    return code;
}
