/* Copyright (C) 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevclj.c,v 1.4 2002/02/21 22:24:51 giles Exp $ */
/*
 * H-P Color LaserJet 5/5M device; based on the PaintJet.
 */
#include "math_.h"
#include "gx.h"
#include "gsparam.h"
#include "gdevprn.h"
#include "gdevpcl.h"

typedef struct gx_device_clj_s gx_device_clj;
struct gx_device_clj_s {
	gx_device_common;
	gx_prn_device_common;
	bool rotated;
};

#define pclj ((gx_device_clj *)pdev)

/*
 * The HP Color LaserJet 5/5M provides a rather unexpected speed/performance
 * tradeoff.
 *
 * When generating rasters, only the fixed (simple) color spaces provide
 * reasonable performance (in this case, reasonable != good). However, in
 * these modes, certain of the fully-saturated primary colors (cyan, blue,
 * green, and red) are rendered differently as rasters as opposed to colored
 * geometric objects. Hence, the color of the output will be other than what
 * is expected.
 *
 * Alternatively, the direct color, 1-bit per pixel scheme can be used. This
 * will produce the expected colors, but performance will deteriorate
 * significantly (observed printing time will be about 3 times longer than
 * when using the simple color mode).
 *
 * Note that when using the latter mode to view output from the PCL
 * interpreter, geometric objects and raster rendered with other than
 * geometric color spaces will have the same appearance as if sent directly
 * to the CLJ, but rasters generated from simple color spaces will have a
 * different appearance. To make the latter rasters match in appearance, the
 * faster printing mode must be used (in which the case the other objects
 * will not have the same appearance).
 */
#define USE_FAST_MODE

/* X_DPI and Y_DPI must be the same */
#define X_DPI 300
#define Y_DPI 300

/*
 * Array of paper sizes, and the corresponding offsets.
 */
typedef struct clj_paper_size_s {
    uint        tag;                /* paper type tag */
    int         orient;             /* logical page orientation to use */
    float       width, height;      /* in pts; +- 5 pts */
    gs_point    offsets;            /* offsets in the given orientation */
} clj_paper_size;

/*
 * The Color LaserJet prints page sizes up to 11.8" wide (A4 size) in
 * long-edge-feed (landscape) orientation. Only executive, letter, and
 * A4 size are supported for color, so we don't bother to list the others.
 */
private const clj_paper_size    clj_paper_sizes[] = {
    /* U.S. letter size comes first so it will be the default. */
    {   2,  1, 11.00 * 72.0, 8.50 * 72.0, { .200 * 72.0, 0.0 } },
    {   1,  1, 10.50 * 72.0, 7.25 * 72.0, { .200 * 72.0, 0.0 } },
    {  26,  1, 11.69 * 72.0, 8.27 * 72.0, { .197 * 72.0, 0.0 } }
};

/*
 * The supported set of resolutions.
 *
 * The Color LaserJet 5/5M is actually a pseudo-contone device, with hardware
 * capable of providing about 16 levels of intensity. The current code does
 * not take advantage of this feature, because it is not readily controllable
 * via PCL. Rather, the device is modeled as a bi-level device in each of
 * three color planes. The maximum supported resolution for such an arrangement
 * is 300 dpi.
 *
 * The CLJ does support raster scaling, but to invoke that scaling, even for
 * integral factors, involves a large performance penalty. Hence, only those
 * resolutions that can be supported without invoking raster scaling are
 * included here. These resolutions are always the same in the fast and slow
 * scan directions, so only a single value is listed here.
 *
 * All valuse are in dots per inch.
 */
private const float supported_resolutions[] = { 75.0, 100.0, 150.0, 300.0 };


/* indicate the maximum supported resolution and scan-line length (pts) */
#define CLJ_MAX_RES        300.0
#define CLJ_MAX_SCANLINE   (12.0 * 72.0)


/*
 * Determine a requested resolution pair is supported.
 */
  private bool
is_supported_resolution(
    const float HWResolution[2]
)
{
    int     i;

    for (i = 0; i < countof(supported_resolutions); i++) {
        if (HWResolution[0] == supported_resolutions[i])
            return HWResolution[0] == HWResolution[1];
    }
    return false;
}

/* ---------------- Standard driver ---------------- */

/*
 * Find the paper size information corresponding to a given pair of dimensions.
 * If rotatep != 0, *rotatep is set to true if the page must be rotated 90
 * degrees to fit.
 *
 * A return value of 0 indicates the paper size is not supported.
 *
 * Note that for the standard driver, rotation is not allowed.
 */
  private const clj_paper_size *
get_paper_size(
    const float             MediaSize[2],
    bool *                  rotatep
)
{
    static const float      tolerance = 5.0;
    float                   width = MediaSize[0];
    float                   height = MediaSize[1];
    const clj_paper_size *  psize = 0;
    int                     i;

    for (i = 0, psize = clj_paper_sizes; i < countof(clj_paper_sizes); i++, psize++) {
        if ( (fabs(width - psize->width) <= tolerance)  &&
             (fabs(height - psize->height) <= tolerance)  ) {
            if (rotatep != 0)
                *rotatep = false;
            return psize;
        } else if ( (fabs(width - psize->height) <= tolerance) &&
                    (fabs(height - psize->width) <= tolerance)   ) {
            if (rotatep != 0)
                *rotatep = true;
            return psize;
        }
    }

    return 0;
}

/*
 * Get the (PostScript style) default matrix for the current page size.
 *
 * For all of the supported sizes, the page will be printed with long-edge
 * feed (the CLJ does support some additional sizes, but only for monochrome).
 * As will all HP laser printers, the printable region marin is 12 pts. from
 * the edge of the physical page.
 */
private void
clj_get_initial_matrix( gx_device *pdev, gs_matrix *pmat)
{
    floatp      	fs_res = pdev->HWResolution[0] / 72.0;
    floatp      	ss_res = pdev->HWResolution[1] / 72.0;
    const clj_paper_size *psize;

    psize = get_paper_size(pdev->MediaSize, NULL);
    /* if the paper size is not recognized, not much can be done */
    /* This shouldn't be possible since clj_put_params rejects   */
    /* unknown media sizes.					 */
    if (psize == 0) {
	pmat->xx = fs_res;
	pmat->xy = 0.0;
	pmat->yx = 0.0;
	pmat->yy = -ss_res;
	pmat->tx = 0.0;
	pmat->ty = pdev->MediaSize[1] * ss_res;
	return;
    }
  
    if (pclj->rotated) {
        pmat->xx = 0.0;
        pmat->xy = ss_res;
        pmat->yx = fs_res;
        pmat->yy = 0.0;
        pmat->tx = -psize->offsets.x * fs_res;
        pmat->ty = -psize->offsets.y * ss_res;
    } else {
        pmat->xx = fs_res;
        pmat->xy = 0.0;
        pmat->yx = 0.0;
        pmat->yy = -ss_res;
        pmat->tx = -psize->offsets.x * fs_res;
        pmat->ty = pdev->height + psize->offsets.y * ss_res;
    }
}

/*
 * Get parameters, including InputAttributes for all supported page sizes.
 * We associate each page size with a different "media source", since that
 * is currently the only way to register multiple page sizes.
 */
private int
clj_get_params(gx_device *pdev, gs_param_list *plist)
{
    gs_param_dict mdict;
    int code = gdev_prn_get_params(pdev, plist);
    int ecode = code;
    int i;

    code = gdev_begin_input_media(plist, &mdict, countof(clj_paper_sizes));
    if (code < 0)
	ecode = code;
    else {
	for (i = 0; i < countof(clj_paper_sizes); ++i) {
	    code = gdev_write_input_page_size(i, &mdict,
					      clj_paper_sizes[i].width,
					      clj_paper_sizes[i].height);
	    if (code < 0)
		ecode = code;
	}
	code = gdev_end_input_media(plist, &mdict);
	if (code < 0)
	    ecode = code;
    }
    return ecode;
}

/*
 * Get the media size being set by put_params, if any.  Return 0 if no media
 * size is being set, 1 (and set mediasize[]) if the size is being set, <0
 * on error.
 */
private int
clj_media_size(float mediasize[2], gs_param_list *plist)
{
    gs_param_float_array fres;
    gs_param_float_array fsize;
    gs_param_int_array hwsize;
    int have_pagesize = 0;

    if ( (param_read_float_array(plist, "HWResolution", &fres) == 0) &&
          !is_supported_resolution(fres.data) ) 
        return_error(gs_error_rangecheck);

    if ( (param_read_float_array(plist, "PageSize", &fsize) == 0) ||
         (param_read_float_array(plist, ".MediaSize", &fsize) == 0) ) {
	mediasize[0] = fsize.data[0];
	mediasize[1] = fsize.data[1];
	have_pagesize = 1;
    }

    if (param_read_int_array(plist, "HWSize", &hwsize) == 0) {
        mediasize[0] = ((float)hwsize.data[0]) / fres.data[0];
        mediasize[1] = ((float)hwsize.data[1]) / fres.data[1];
	have_pagesize = 1;
    }

    return have_pagesize;
}

/*
 * Special put_params routine, to make certain the desired MediaSize and
 * HWResolution are supported.
 */
  private int
clj_put_params(
    gx_device *             pdev,
    gs_param_list *         plist
)
{
    float		    mediasize[2];
    bool                    rotate = false;
    int                     have_pagesize = clj_media_size(mediasize, plist);

    if (have_pagesize < 0)
	return have_pagesize;
    if (have_pagesize) {
	if (get_paper_size(mediasize, &rotate) == 0 || rotate)
	    return_error(gs_error_rangecheck);
    }
    return gdev_prn_put_params(pdev, plist);
}

/*
 * Pack and then compress a scanline of data. Return the size of the compressed
 * data produced.
 *
 * Input is arranged with one byte per pixel, but only the three low-order bits
 * are used. These bits are in order ymc, with yellow being the highest order
 * bit.
 *
 * Output is arranged in three planes, with one bit per pixel per plane. The
 * Color LaserJet 5/5M does support more congenial pixel encodings, but use
 * of anything other than the fixed palettes seems to result in very poor
 * performance.
 *
 * Only compresion mode 2 is used. Compression mode 1 (pure run length) has
 * an advantage over compression mode 2 only in cases in which very long runs
 * occur (> 128 bytes). Since both methods provide good compression in that
 * case, it is not worth worrying about, and compression mode 2 provides much
 * better worst-case behavior. Compression mode 3 requires considerably more
 * effort to generate, so it is useful only when it is known a prior that
 * scanlines repeat frequently.
 */
  private void
pack_and_compress_scanline(
    const byte *        pin,
    int                 in_size,
    byte  *             pout[3],
    int                 out_size[3]
)
{
#define BUFF_SIZE                                                           \
    ( ((int)(CLJ_MAX_RES * CLJ_MAX_SCANLINE / 72.0) + sizeof(ulong) - 1)    \
         / sizeof(ulong) )

    ulong               buff[3 * BUFF_SIZE];
    byte *              p_c = (byte *)buff;
    byte *              p_m = (byte *)(buff + BUFF_SIZE);
    byte *              p_y = (byte *)(buff + 2 * BUFF_SIZE);
    ulong *             ptrs[3];
    byte                c_val = 0, m_val = 0, y_val = 0;
    ulong               mask = 0x80;
    int                 i;

    /* pack the input for 4-bits per index */
    for (i = 0; i < in_size; i++) {
        uint    ival = *pin++;

        if (ival != 0) {
            if ((ival & 0x4) != 0)
                y_val |= mask;
            if ((ival & 0x2) != 0)
                m_val |= mask;
            if ((ival & 0x1) != 0)
                c_val |= mask;
        }

        if ((mask >>= 1) == 0) {
            /* NB - write out in byte units */
            *p_c++ = c_val;
            c_val = 0L;
            *p_m++ = m_val;
            m_val = 0L;
            *p_y++ = y_val;
            y_val = 0L;
            mask = 0x80;
        }
    }
    if (mask != 0x80) {
        /* NB - write out in byte units */
        *p_c++ = c_val;
        *p_m++ = m_val;
        *p_y++ = y_val;
    }

    /* clear to up a longword boundary */
    while ((((ulong)p_c) & (sizeof(ulong) - 1)) != 0) {
        *p_c++ = 0;
        *p_m++ = 0;
        *p_y++ = 0;
    }

    ptrs[0] = (ulong *)p_c;
    ptrs[1] = (ulong *)p_m;
    ptrs[2] = (ulong *)p_y;

    for (i = 0; i < 3; i++) {
        ulong * p_start = buff + i * BUFF_SIZE;
        ulong * p_end = ptrs[i];

        /* eleminate trailing 0's */
        while ((p_end > p_start) && (p_end[-1] == 0))
            p_end--;

        if (p_start == p_end)
            out_size[i] = 0;
        else
            out_size[i] = gdev_pcl_mode2compress(p_start, p_end, pout[i]);
    }

#undef BUFF_SIZE
}

/*
 * Send the page to the printer.  Compress each scan line.
 */
  private int
clj_print_page(
    gx_device_printer *     pdev,
    FILE *                  prn_stream
)
{
    gs_memory_t *mem = pdev->memory;
    bool                    rotate;
    const clj_paper_size *  psize = get_paper_size(pdev->MediaSize, &rotate);
    int                     lsize = pdev->width;
    int                     clsize = (lsize + (lsize + 255) / 128) / 8;
    byte *                  data = 0;
    byte *                  cdata[3];
    int                     blank_lines = 0;
    int                     i;
    floatp                  fs_res = pdev->HWResolution[0] / 72.0;
    floatp                  ss_res = pdev->HWResolution[1] / 72.0;
    int			    imageable_width, imageable_height;

    /* no paper size at this point is a serious error */
    if (psize == 0)
        return_error(gs_error_unregistered);

    /* allocate memory for the raw and compressed data */
    if ((data = gs_alloc_bytes(mem, lsize, "clj_print_page(data)")) == 0)
        return_error(gs_error_VMerror);
    if ((cdata[0] = gs_alloc_bytes(mem, 3 * clsize, "clj_print_page(cdata)")) == 0) {
        gs_free_object(mem, data, "clj_print_page(data)");
        return_error(gs_error_VMerror);
    }
    cdata[1] = cdata[0] + clsize;
    cdata[2] = cdata[1] + clsize;


    /* Imageable area is without the margins. Note that the actual rotation
     * of page size into pdev->width & height has been done. We just use
     * rotate to access the correct offsets. */
    if (pclj->rotated) {
    	imageable_width = pdev->width - (2 * psize->offsets.x) * fs_res;
    	imageable_height = pdev->height - (2 * psize->offsets.y) * ss_res;
    }
    else {
    	imageable_width = pdev->width - (2 * psize->offsets.y) * ss_res;
    	imageable_height = pdev->height - (2 * psize->offsets.x) * fs_res;
    }

    /* start the page.  The pcl origin (0, 150 dots by default, y
       increasing down the long edge side of the page) needs to be
       offset such that it coincides with the offsets of the imageable
       area.  This calculation should be independant of rotation but
       only the rotated case has been tested with a real device. */
    fprintf( prn_stream,
             "\033E\033&u300D\033&l%da1x%dO\033*p0x0y+50x-100Y\033*t%dR"
#ifdef USE_FAST_MODE
	     "\033*r-3U"
#else
             "\033*v6W\001\002\003\001\001\001"
#endif
             "\033*r0f%ds%dt1A\033*b2M",
             psize->tag,
             pclj->rotated,
             (int)(pdev->HWResolution[0]),
             imageable_width,
             imageable_height
             );

    /* process each scanline */
    for (i = 0; i < imageable_height; i++) {
        int     clen[3];

        gdev_prn_copy_scan_lines(pdev, i, data, lsize);

	/* The 'lsize' bytes of data have the blank margin area at the end due	*/
	/* to the 'initial_matrix' offsets that are applied.			*/
        pack_and_compress_scanline(data, imageable_width, cdata, clen);
        if ((clen[0] == 0) && (clen[1] == 0) && (clen[2] == 0))
            ++blank_lines;
        else {
            if (blank_lines != 0) {
                fprintf(prn_stream, "\033*b%dY", blank_lines);
                blank_lines = 0;
            }
            fprintf(prn_stream, "\033*b%dV", clen[0]);
            fwrite(cdata[0], sizeof(byte), clen[0], prn_stream);
            fprintf(prn_stream, "\033*b%dV", clen[1]);
            fwrite(cdata[1], sizeof(byte), clen[1], prn_stream);
            fprintf(prn_stream, "\033*b%dW", clen[2]);
            fwrite(cdata[2], sizeof(byte), clen[2], prn_stream);
        }
    }

    /* PCL will take care of blank lines at the end */
    fputs("\033*rC\f", prn_stream);

    /* free the buffers used */
    gs_free_object(mem, cdata[0], "clj_print_page(cdata)");
    gs_free_object(mem, data, "clj_print_page(data)");

    return 0;
}

/* CLJ device methods */
#define CLJ_PROCS(get_params, put_params)\
    gdev_prn_open,                  /* open_device */\
    clj_get_initial_matrix,         /* get_initial matrix */\
    NULL,	                    /* sync_output */\
    gdev_prn_output_page,           /* output_page */\
    gdev_prn_close,                 /* close_device */\
    gdev_pcl_3bit_map_rgb_color,    /* map_rgb_color */\
    gdev_pcl_3bit_map_color_rgb,    /* map_color_rgb */\
    NULL,	                    /* fill_rectangle */\
    NULL,	                    /* tile_rectangle */\
    NULL,	                    /* copy_mono */\
    NULL,	                    /* copy_color */\
    NULL,	                    /* obsolete draw_line */\
    NULL,	                    /* get_bits */\
    get_params, 	            /* get_params */\
    put_params,                     /* put_params */\
    NULL,	                    /* map_cmyk_color */\
    NULL,	                    /* get_xfont_procs */\
    NULL,	                    /* get_xfont_device */\
    NULL,	                    /* map_rgb_alpha_color */\
    gx_page_device_get_page_device  /* get_page_device */

private gx_device_procs cljet5_procs = {
    CLJ_PROCS(clj_get_params, clj_put_params)
};

/* CLJ device structure */
#define CLJ_DEVICE_BODY(procs, dname, rotated)\
  prn_device_body(\
    gx_device_clj,\
    procs,                  /* procedures */\
    dname,                  /* device name */\
    110,                    /* width - will be overridden subsequently */\
    85,                     /* height - will be overridden subsequently */\
    X_DPI, Y_DPI,           /* resolutions - current must be the same */\
    0.167, 0.167,           /* margins (left, bottom, right, top */\
    0.167, 0.167,\
    3,                      /* num_components - 3 colors, 1 bit per pixel */\
    8,			    /* depth - pack into bytes */\
    1, 1, 		    /* max_gray=max_component=1 */\
    2, 2,		    /* dithered_grays=dithered_components=2 */ \
    clj_print_page          /* routine to output page */\
),\
    rotated		    /* rotated - may be overridden subsequently */

gx_device_clj gs_cljet5_device = {
    CLJ_DEVICE_BODY(cljet5_procs, "cljet5", 0 /*false*/)
};

/* ---------------- Driver with page rotation ---------------- */

/*
 * For use with certain PCL interpreters, which don't implement
 * setpagedevice, we provide a version of this driver that attempts to
 * handle page rotation at the driver level.  This version breaks an
 * invariant that all drivers must obey, namely, that drivers are not
 * allowed to change the parameters passed by put_params (they can only
 * accept or reject them).  Consequently, this driver must not be used in
 * any context other than these specific PCL interpreters.  We support this
 * hack only because these PCL interpreters can't be changed to handle page
 * rotation properly.
 */

/*
 * Special get_params routine, to fake MediaSize, width, and height if
 * we were in a 'rotated' state.
 */
private int
clj_pr_get_params( gx_device *pdev, gs_param_list *plist )
{
    int code;

    /* First un-rotate the MediaSize, etc. if we were in a rotated mode		*/
    if (pclj->rotated) {
        float ftmp;
	int   itmp;

	ftmp = pdev->MediaSize[0];
	pdev->MediaSize[0] = pdev->MediaSize[1];
	pdev->MediaSize[1] = ftmp;
	itmp = pdev->width;
	pdev->width = pdev->height;
	pdev->height = itmp;
    }

    /* process the parameter list */
    code = gdev_prn_get_params(pdev, plist);

    /* Now re-rotate the page size if needed */
    if (pclj->rotated) {
        float ftmp;
	int   itmp;

	ftmp = pdev->MediaSize[0];
	pdev->MediaSize[0] = pdev->MediaSize[1];
	pdev->MediaSize[1] = ftmp;
	itmp = pdev->width;
	pdev->width = pdev->height;
	pdev->height = itmp;
    }

    return code;
}

/*
 * Special put_params routine, to intercept changes in the MediaSize, and to
 * make certain the desired MediaSize and HWResolution are supported.
 *
 * This function will rotate MediaSize if it is needed by the device in
 * order to print this size page.
 */
  private int
clj_pr_put_params(
    gx_device *             pdev,
    gs_param_list *         plist
)
{
    float		    mediasize[2];
    int                     code = 0;
    bool                    rotate = false;
    int                     have_pagesize = clj_media_size(mediasize, plist);

    if (have_pagesize < 0)
	return have_pagesize;
    if (have_pagesize) {
	if (get_paper_size(mediasize, &rotate) == 0)
	    return_error(gs_error_rangecheck);
	if (rotate) {
	    /* We need to rotate the requested page size, so synthesize a new	*/
	    /* parameter list in front of the requestor's list to force the	*/
	    /* rotated page size.						*/
	    gs_param_float_array	pf_array;
	    gs_c_param_list		alist;
	    float			ftmp = mediasize[0];

	    mediasize[0] = mediasize[1];
	    mediasize[1] = ftmp;
	    pf_array.data = mediasize;
	    pf_array.size = 2;
	    pf_array.persistent = false;

	    gs_c_param_list_write(&alist, pdev->memory);
	    code = param_write_float_array((gs_param_list *)&alist, ".MediaSize", &pf_array);
	    gs_c_param_list_read(&alist);

	    /* stick this synthesized parameter on the front of the existing list */
	    gs_c_param_list_set_target(&alist, plist);
	    if ((code = gdev_prn_put_params(pdev, (gs_param_list *)&alist)) >= 0)
		pclj->rotated = true;
	    gs_c_param_list_release(&alist);
	} else {
	    if ((code = gdev_prn_put_params(pdev, plist)) >= 0)
		pclj->rotated = false;
	}
    } else 
	code = gdev_prn_put_params(pdev, plist);

    return code;
}

/* CLJ device methods -- se above for CLJ_PROCS */
private gx_device_procs cljet5pr_procs = {
    CLJ_PROCS(clj_pr_get_params, clj_pr_put_params)
};

/* CLJ device structure -- see above for CLJ_DEVICE_BODY */
gx_device_clj gs_cljet5pr_device = {
    CLJ_DEVICE_BODY(cljet5pr_procs, "cljet5pr", 1 /*true*/)
};
