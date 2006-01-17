/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gxdcolor.c,v 1.13 2005/06/20 08:59:23 igor Exp $ */
/* Pure and null device color implementation */
#include "gx.h"
#include "memory_.h"
#include "gserrors.h"
#include "gsbittab.h"
#include "gxdcolor.h"
#include "gxdevice.h"
#include "gxdevcli.h"

/* Define the standard device color types. */

/* 'none' means the color is not defined. */
private dev_color_proc_save_dc(gx_dc_no_save_dc);
private dev_color_proc_get_dev_halftone(gx_dc_no_get_dev_halftone);
private dev_color_proc_load(gx_dc_no_load);
private dev_color_proc_fill_rectangle(gx_dc_no_fill_rectangle);
private dev_color_proc_fill_masked(gx_dc_no_fill_masked);
private dev_color_proc_equal(gx_dc_no_equal);
private dev_color_proc_write(gx_dc_no_write);
private dev_color_proc_read(gx_dc_no_read);
private dev_color_proc_get_nonzero_comps(gx_dc_no_get_nonzero_comps);
const gx_device_color_type_t gx_dc_type_data_none = {
    &st_bytes,
    gx_dc_no_save_dc, gx_dc_no_get_dev_halftone, gx_dc_no_get_phase,
    gx_dc_no_load, gx_dc_no_fill_rectangle, gx_dc_no_fill_masked,
    gx_dc_no_equal, gx_dc_no_write, gx_dc_no_read, gx_dc_no_get_nonzero_comps
};
#undef gx_dc_type_none
const gx_device_color_type_t *const gx_dc_type_none = &gx_dc_type_data_none;
#define gx_dc_type_none (&gx_dc_type_data_none)

/* 'null' means the color has no effect when used for drawing. */
private dev_color_proc_load(gx_dc_null_load);
private dev_color_proc_fill_rectangle(gx_dc_null_fill_rectangle);
private dev_color_proc_fill_masked(gx_dc_null_fill_masked);
private dev_color_proc_equal(gx_dc_null_equal);
private dev_color_proc_read(gx_dc_null_read);
const gx_device_color_type_t gx_dc_type_data_null = {
    &st_bytes,
    gx_dc_no_save_dc, gx_dc_no_get_dev_halftone, gx_dc_no_get_phase,
    gx_dc_null_load, gx_dc_null_fill_rectangle, gx_dc_null_fill_masked,
    gx_dc_null_equal, gx_dc_no_write, gx_dc_null_read, gx_dc_no_get_nonzero_comps
};
#undef gx_dc_type_null
const gx_device_color_type_t *const gx_dc_type_null = &gx_dc_type_data_null;
#define gx_dc_type_null (&gx_dc_type_data_null)

private dev_color_proc_save_dc(gx_dc_pure_save_dc);
private dev_color_proc_load(gx_dc_pure_load);
private dev_color_proc_fill_rectangle(gx_dc_pure_fill_rectangle);
private dev_color_proc_fill_masked(gx_dc_pure_fill_masked);
private dev_color_proc_equal(gx_dc_pure_equal);
private dev_color_proc_write(gx_dc_pure_write);
private dev_color_proc_read(gx_dc_pure_read);
const gx_device_color_type_t gx_dc_type_data_pure = {
    &st_bytes,
    gx_dc_pure_save_dc, gx_dc_no_get_dev_halftone, gx_dc_no_get_phase,
    gx_dc_pure_load, gx_dc_pure_fill_rectangle, gx_dc_pure_fill_masked,
    gx_dc_pure_equal, gx_dc_pure_write, gx_dc_pure_read,
    gx_dc_pure_get_nonzero_comps
};
#undef gx_dc_type_pure
const gx_device_color_type_t *const gx_dc_type_pure = &gx_dc_type_data_pure;
#define gx_dc_type_pure (&gx_dc_type_data_pure)

/*
 * Get the black and white pixel values of a device.
 */
gx_color_index
gx_device_black(gx_device *dev)
{
    if (dev->cached_colors.black == gx_no_color_index) {
	const gx_cm_color_map_procs * cm_procs = dev_proc(dev, get_color_mapping_procs)(dev);
        int i, ncomps = dev->color_info.num_components;
        frac cm_comps[GX_DEVICE_COLOR_MAX_COMPONENTS];
        gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];

    	/* Get color components for black (gray = 0) */
    	cm_procs->map_gray(dev, frac_0, cm_comps);

        for (i = 0; i < ncomps; i++)
            cv[i] = frac2cv(cm_comps[i]);

	dev->cached_colors.black = dev_proc(dev, encode_color)(dev, cv);
    }
    return dev->cached_colors.black;
}
gx_color_index
gx_device_white(gx_device *dev)
{
    if (dev->cached_colors.white == gx_no_color_index) {
	const gx_cm_color_map_procs * cm_procs = dev_proc(dev, get_color_mapping_procs)(dev);
        int i, ncomps = dev->color_info.num_components;
        frac cm_comps[GX_DEVICE_COLOR_MAX_COMPONENTS];
        gx_color_value cv[GX_DEVICE_COLOR_MAX_COMPONENTS];

    	/* Get color components for white (gray = 1) */
    	cm_procs->map_gray(dev, frac_1, cm_comps);

        for (i = 0; i < ncomps; i++)
            cv[i] = frac2cv(cm_comps[i]);

	dev->cached_colors.white = dev_proc(dev, encode_color)(dev, cv);
    }
    return dev->cached_colors.white;
}

/* Clear the color cache. */
void
gx_device_decache_colors(gx_device *dev)
{
    dev->cached_colors.black = dev->cached_colors.white = gx_no_color_index;
}

/* Set a null RasterOp source. */
private const gx_rop_source_t gx_rop_no_source_0 = {gx_rop_no_source_body(0)};
private const gx_rop_source_t gx_rop_no_source_1 = {gx_rop_no_source_body(1)};
void
gx_set_rop_no_source(const gx_rop_source_t **psource,
		     gx_rop_source_t *pno_source, gx_device *dev)
{
    gx_color_index black;

top:
    black = dev->cached_colors.black;
    if (black == 0)
	*psource = &gx_rop_no_source_0;
    else if (black == 1)
	*psource = &gx_rop_no_source_1;
    else if (black == gx_no_color_index) {	/* cache not loaded */
	discard(gx_device_black(dev));
	goto top;
    } else {
	*pno_source = gx_rop_no_source_0;
	gx_rop_source_set_color(pno_source, black);
	*psource = pno_source;
    }
}

/*
 * Test device colors for equality.  Testing for equality is done
 * for determining when cache values, etc. can be used.  Thus these
 * routines should err toward false responses if there is any question
 * about the equality of the two device colors.
 */
bool
gx_device_color_equal(const gx_device_color *pdevc1,
		      const gx_device_color *pdevc2)
{
    return pdevc1->type->equal(pdevc1, pdevc2);
}

/*
 * Return a device color type index. This index is used by the command
 * list processor to identify a device color type, as the type pointer
 * itself is meaningful only within a single address space.
 *
 * Currently, we ignore the pattern device colors as they cannot be
 * passed through the command list.
 *
 * Returns gs_error_unknownerror for an unrecognized type.
 */
private  const gx_device_color_type_t * dc_color_type_table[] = {
    gx_dc_type_none,            /* unset device color */
    gx_dc_type_null,            /* blank (transparent) device color */
    gx_dc_type_pure,            /* pure device color */
    /* gx_dc_type_pattern, */   /* patterns - not used in command list */
    gx_dc_type_ht_binary,       /* binary halftone device colors */
    gx_dc_type_ht_colored,      /* general halftone device colors */
    gx_dc_type_wts              /* well-tempered screen device colors */
};

int
gx_get_dc_type_index(const gx_device_color * pdevc)
{
    const gx_device_color_type_t *  type = pdevc->type;
    int                             num_types, i;

    num_types = sizeof(dc_color_type_table) / sizeof(dc_color_type_table[0]);
    for (i = 0; i < num_types && type != dc_color_type_table[i]; i++)
        ;

    return i < num_types ? i : gs_error_unknownerror;
}

/* map a device color type index into the associated method vector */
const gx_device_color_type_t *
gx_get_dc_type_from_index(int i)
{
    if ( i >= 0                                                          &&
         i < sizeof(dc_color_type_table) / sizeof(dc_color_type_table[0])  )
        return dc_color_type_table[i];
    else
        return 0;
}

/* ------ Canonical get_phase methods ------ */
bool
gx_dc_no_get_phase(const gx_device_color * pdevc, gs_int_point * pphase)
{
    return false;
}

bool
gx_dc_ht_get_phase(const gx_device_color * pdevc, gs_int_point * pphase)
{
    *pphase = pdevc->phase;
    return true;
}

/* ------ Undefined color ------ */
private void
gx_dc_no_save_dc(const gx_device_color * pdevc, gx_device_color_saved * psdc)
{
    psdc->type = pdevc->type;
}

private const gx_device_halftone *
gx_dc_no_get_dev_halftone(const gx_device_color * pdevc)
{
    return 0;
}

private int
gx_dc_no_load(gx_device_color *pdevc, const gs_imager_state *ignore_pis,
	      gx_device *ignore_dev, gs_color_select_t ignore_select)
{
    return 0;
}

private int
gx_dc_no_fill_rectangle(const gx_device_color *pdevc, int x, int y,
			int w, int h, gx_device *dev,
			gs_logical_operation_t lop,
			const gx_rop_source_t *source)
{
    gx_device_color filler;

    if (w <= 0 || h <= 0)
	return 0;
    if (lop_uses_T(lop))
	return_error(gs_error_Fatal);
    set_nonclient_dev_color(&filler, 0);   /* any valid value for dev will do */
    return gx_dc_pure_fill_rectangle(&filler, x, y, w, h, dev, lop, source);
}

private int
gx_dc_no_fill_masked(const gx_device_color *pdevc, const byte *data,
		     int data_x, int raster, gx_bitmap_id id,
		     int x, int y, int w, int h, gx_device *dev,
		     gs_logical_operation_t lop, bool invert)
{
    if (w <= 0 || h <= 0)
	return 0;
    return_error(gs_error_Fatal);
}

private bool
gx_dc_no_equal(const gx_device_color *pdevc1, const gx_device_color *pdevc2)
{
    return false;
}

private int
gx_dc_no_write(
    const gx_device_color *         pdevc,      /* ignored */
    const gx_device_color_saved *   psdc,       /* ignored */
    const gx_device *               dev,        /* ignored */
    byte *                          data,       /* ignored */
    uint *                          psize )
{
    *psize = 0;
    return psdc != 0 && psdc->type == pdevc->type ? 1 : 0;
}

private int
gx_dc_no_read(
    gx_device_color *       pdevc,
    const gs_imager_state * pis,                /* ignored */
    const gx_device_color * prior_devc,         /* ignored */
    const gx_device *       dev,                /* ignored */
    const byte *            pdata,              /* ignored */
    uint                    size,               /* ignored */
    gs_memory_t *           mem )               /* ignored */
{
    pdevc->type = gx_dc_type_none;
    return 0;
}

private int
gx_dc_no_get_nonzero_comps(
    const gx_device_color * pdevc_ignored,
    const gx_device *       dev_ignored,
    gx_color_index *        pcomp_bits_ignored )
{
    return 0;
}

/* ------ Null color ------ */

private int
gx_dc_null_load(gx_device_color *pdevc, const gs_imager_state *ignore_pis,
		gx_device *ignore_dev, gs_color_select_t ignore_select)
{
    return 0;
}

private int
gx_dc_null_fill_rectangle(const gx_device_color * pdevc, int x, int y,
			  int w, int h, gx_device * dev,
			  gs_logical_operation_t lop,
			  const gx_rop_source_t * source)
{
    return 0;
}

private int
gx_dc_null_fill_masked(const gx_device_color * pdevc, const byte * data,
		       int data_x, int raster, gx_bitmap_id id,
		       int x, int y, int w, int h, gx_device * dev,
		       gs_logical_operation_t lop, bool invert)
{
    return 0;
}

private bool
gx_dc_null_equal(const gx_device_color * pdevc1, const gx_device_color * pdevc2)
{
    return pdevc2->type == pdevc1->type;
}

private int
gx_dc_null_read(
    gx_device_color *       pdevc,
    const gs_imager_state * pis,                /* ignored */
    const gx_device_color * prior_devc,         /* ignored */
    const gx_device *       dev,                /* ignored */
    const byte *            pdata,              /* ignored */
    uint                    size,               /* ignored */
    gs_memory_t *           mem )               /* ignored */
{
    pdevc->type = gx_dc_type_null;
    return 0;
}

/* ------ Pure color ------ */

private void
gx_dc_pure_save_dc(const gx_device_color * pdevc, gx_device_color_saved * psdc)
{
    psdc->type = pdevc->type;
    psdc->colors.pure = pdevc->colors.pure;
}

private int
gx_dc_pure_load(gx_device_color * pdevc, const gs_imager_state * ignore_pis,
		gx_device * ignore_dev, gs_color_select_t ignore_select)
{
    return 0;
}

/* Fill a rectangle with a pure color. */
/* Note that we treat this as "texture" for RasterOp. */
private int
gx_dc_pure_fill_rectangle(const gx_device_color * pdevc, int x, int y,
		  int w, int h, gx_device * dev, gs_logical_operation_t lop,
			  const gx_rop_source_t * source)
{
    if (source == NULL && lop_no_S_is_T(lop))
	return (*dev_proc(dev, fill_rectangle)) (dev, x, y, w, h,
						 pdevc->colors.pure);
    {
	gx_color_index colors[2];
	gx_rop_source_t no_source;

	colors[0] = colors[1] = pdevc->colors.pure;
	if (source == NULL)
	    set_rop_no_source(source, no_source, dev);
	return (*dev_proc(dev, strip_copy_rop))
	    (dev, source->sdata, source->sourcex, source->sraster,
	     source->id, (source->use_scolors ? source->scolors : NULL),
	     NULL /*arbitrary */ , colors, x, y, w, h, 0, 0, lop);
    }
}

/* Fill a mask with a pure color. */
/* Note that there is no source in this case: the mask is the source. */
private int
gx_dc_pure_fill_masked(const gx_device_color * pdevc, const byte * data,
	int data_x, int raster, gx_bitmap_id id, int x, int y, int w, int h,
		   gx_device * dev, gs_logical_operation_t lop, bool invert)
{
    if (lop_no_S_is_T(lop)) {
	gx_color_index color0, color1;

	if (invert)
	    color0 = pdevc->colors.pure, color1 = gx_no_color_index;
	else
	    color1 = pdevc->colors.pure, color0 = gx_no_color_index;
	return (*dev_proc(dev, copy_mono))
	    (dev, data, data_x, raster, id, x, y, w, h, color0, color1);
    } {
	gx_color_index scolors[2];
	gx_color_index tcolors[2];

	scolors[0] = gx_device_black(dev);
	scolors[1] = gx_device_white(dev);
	tcolors[0] = tcolors[1] = pdevc->colors.pure;
	return (*dev_proc(dev, strip_copy_rop))
	    (dev, data, data_x, raster, id, scolors,
	     NULL, tcolors, x, y, w, h, 0, 0,
	     (invert ? rop3_invert_S(lop) : lop) | lop_S_transparent);
    }
}

private bool
gx_dc_pure_equal(const gx_device_color * pdevc1, const gx_device_color * pdevc2)
{
    return pdevc2->type == pdevc1->type &&
	gx_dc_pure_color(pdevc1) == gx_dc_pure_color(pdevc2);
}

/*
 * Serialize a pure color.
 *
 * Operands:
 *
 *  pdevc       pointer to device color to be serialized
 *
 *  psdc        pointer ot saved version of last serialized color (for
 *              this band); this is ignored
 *  
 *  dev         pointer to the current device, used to retrieve process
 *              color model information
 *
 *  pdata       pointer to buffer in which to write the data
 *
 *  psize       pointer to a location that, on entry, contains the size of
 *              the buffer pointed to by pdata; on return, the size of
 *              the data required or actually used will be written here.
 *
 * Returns:
 *
 *  1, with *psize set to 0, if *pdevc and *psdc represent the same color
 *
 *  0, with *psize set to the amount of data written, if everything OK
 *
 *  gs_error_rangecheck, with *psize set to the size of buffer required,
 *  if *psize was not large enough
 *
 *  < 0, != gs_error_rangecheck, in the event of some other error; in this
 *  case *psize is not changed.
 */
private int
gx_dc_pure_write(
    const gx_device_color *         pdevc,
    const gx_device_color_saved *   psdc,       /* ignored */
    const gx_device *               dev,
    byte *                          pdata,
    uint *                          psize )
{
    if ( psdc != 0                              &&
         psdc->type == pdevc->type              &&
         psdc->colors.pure == pdevc->colors.pure  ) {
        *psize = 0;
        return 1;
    } else
        return gx_dc_write_color(pdevc->colors.pure, dev, pdata, psize);
}

/*
 * Reconstruct a pure device color from its serial representation.
 *
 * Operands:
 *
 *  pdevc       pointer to the location in which to write the
 *              reconstructed device color
 *
 *  pis         pointer to the current imager state (ignored here)
 *
 *  prior_devc  pointer to the current device color (this is provided
 *              separately because the device color is not part of the
 *              imager state; it is ignored here)
 *
 *  dev         pointer to the current device, used to retrieve process
 *              color model information
 *
 *  pdata       pointer to the buffer to be read
 *
 *  size        size of the buffer to be read; this should be large
 *              enough to hold the entire color description
 *
 *  mem         pointer to the memory to be used for allocations
 *              (ignored here)
 *
 * Returns:
 *
 *  # of bytes read if everthing OK, < 0 in the event of an error
 */
private int
gx_dc_pure_read(
    gx_device_color *       pdevc,
    const gs_imager_state * pis,                /* ignored */
    const gx_device_color * prior_devc,         /* ignored */
    const gx_device *       dev,
    const byte *            pdata,
    uint                    size,
    gs_memory_t *           mem )               /* ignored */
{
    pdevc->type = gx_dc_type_pure;
    return gx_dc_read_color(&pdevc->colors.pure, dev, pdata, size);
}

int
gx_dc_pure_get_nonzero_comps(
    const gx_device_color * pdevc,
    const gx_device *       dev,
    gx_color_index *        pcomp_bits )
{
    int                     code;
    gx_color_value          cvals[GX_DEVICE_COLOR_MAX_COMPONENTS];

    code = dev_proc(dev, decode_color)( (gx_device *)dev,
                                         pdevc->colors.pure,
                                         cvals );
    if (code >= 0) {
        int             i, ncomps = dev->color_info.num_components;
        gx_color_index  mask = 0x1, comp_bits = 0;

        for (i = 0; i < ncomps; i++, mask <<= 1) {
            if (cvals[i] != 0)
                comp_bits |= mask;
        }
        *pcomp_bits = comp_bits;
        code = 0;
    }

    return code;
}

/* ------ Halftone color initialization ------ */

void
gx_complete_halftone(gx_device_color *pdevc, int num_comps, gx_device_halftone *pdht)
{
    int i, mask = 0;

    pdevc->type = gx_dc_type_ht_colored;
    pdevc->colors.colored.c_ht = pdht;
    pdevc->colors.colored.num_components = num_comps;
    pdevc->colors.colored.alpha = max_ushort;
    for (i = 0; i < num_comps; i++)
        mask |= ((pdevc->colors.colored.c_level[i] != 0 ? 1 : 0) << i);
    pdevc->colors.colored.plane_mask = mask;
}

/* ------ Default implementations ------ */

/* Fill a mask with a color by parsing the mask into rectangles. */
int
gx_dc_default_fill_masked(const gx_device_color * pdevc, const byte * data,
	int data_x, int raster, gx_bitmap_id id, int x, int y, int w, int h,
		   gx_device * dev, gs_logical_operation_t lop, bool invert)
{
    int lbit = data_x & 7;
    const byte *row = data + (data_x >> 3);
    uint one = (invert ? 0 : 0xff);
    uint zero = one ^ 0xff;
    int iy;

    for (iy = 0; iy < h; ++iy, row += raster) {
	const byte *p = row;
	int bit = lbit;
	int left = w;
	int l0;

	while (left) {
	    int run, code;

	    /* Skip a run of zeros. */
	    run = byte_bit_run_length[bit][*p ^ one];
	    if (run) {
		if (run < 8) {
		    if (run >= left)
			break;	/* end of row while skipping */
		    bit += run, left -= run;
		} else if ((run -= 8) >= left)
		    break;	/* end of row while skipping */
		else {
		    left -= run;
		    ++p;
		    while (left > 8 && *p == zero)
			left -= 8, ++p;
		    run = byte_bit_run_length_0[*p ^ one];
		    if (run >= left)	/* run < 8 unless very last byte */
			break;	/* end of row while skipping */
		    else
			bit = run & 7, left -= run;
		}
	    }
	    l0 = left;
	    /* Scan a run of ones, and then paint it. */
	    run = byte_bit_run_length[bit][*p ^ zero];
	    if (run < 8) {
		if (run >= left)
		    left = 0;
		else
		    bit += run, left -= run;
	    } else if ((run -= 8) >= left)
		left = 0;
	    else {
		left -= run;
		++p;
		while (left > 8 && *p == one)
		    left -= 8, ++p;
		run = byte_bit_run_length_0[*p ^ zero];
		if (run >= left)	/* run < 8 unless very last byte */
		    left = 0;
		else
		    bit = run & 7, left -= run;
	    }
	    code = gx_device_color_fill_rectangle(pdevc,
			  x + w - l0, y + iy, l0 - left, 1, dev, lop, NULL);
	    if (code < 0)
		return code;
	}
    }
    return 0;
}

/* ------ Serialization identification support ------ */

/*
 * Utility to write a color index.  Currently, a very simple mechanism
 * is used, much simpler than that used by other command-list writers. This
 * should be sufficient for most situations.
 *
 * Operands:
 *
 *  color       color to be serialized.
 *
 *  dev         pointer to the current device, used to retrieve process
 *              color model information
 *
 *  pdata       pointer to buffer in which to write the data
 *
 *  psize       pointer to a location that, on entry, contains the size of
 *              the buffer pointed to by pdata; on return, the size of
 *              the data required or actually used will be written here.
 *
 * Returns:
 *
 *  0, with *psize set to the amount of data written, if everything OK
 *
 *  gs_error_rangecheck, with *psize set to the size of buffer required,
 *  if *psize was not large enough
 *
 *  < 0, != gs_error_rangecheck, in the event of some other error; in this
 *  case *psize is not changed.
 */
int
gx_dc_write_color(
    gx_color_index      color,
    const gx_device *   dev,
    byte *              pdata,
    uint *              psize )
{
    int                 depth = dev->color_info.depth;
    int                 num_bytes = (depth + 8) >> 3;   /* NB: +8, not +7 */

    /* gx_no_color_index is encoded as a single byte */
    if (color == gx_no_color_index)
        num_bytes = 1;

    /* check for adequate space */
    if (*psize < num_bytes) {
        *psize = num_bytes;
        return gs_error_rangecheck;
    }
    *psize = num_bytes;

    /* gx_no_color_index is a single byte of 0xff */
    if (color == gx_no_color_index) {
        *psize = 1;
        *pdata = 0xff;
    } else {
        if (depth < 8 * arch_sizeof_color_index)
            color &= ((gx_color_index)1 << depth) - 1;
        while (--num_bytes >= 0) {
            pdata[num_bytes] = color & 0xff;
            color >>= 8;
        }
    }
    return 0;
}

/*
 * Utility to reconstruct device color from its serial representation.
 *
 * Operands:
 *
 *  pcolor      pointer to the location in which to write the
 *              reconstucted color
 *
 *  dev         pointer to the current device, used to retrieve process
 *              color model information
 *
 *  pdata       pointer to the buffer to be read
 *
 *  size        size of the buffer to be read; this is expected to be
 *              large enough for the full color
 *
 * Returns: # of bytes read, or < 0 in the event of an error
 */
int
gx_dc_read_color(
    gx_color_index *    pcolor,
    const gx_device *   dev,
    const byte *        pdata,
    int                 size )
{
    gx_color_index      color = 0;
    int                 depth = dev->color_info.depth;
    int                 i, num_bytes = (depth + 8) >> 3;   /* NB: +8, not +7 */

    /* check that enough data has been provided */
    if (size < 1 || (pdata[0] != 0xff && size < num_bytes))
        return gs_error_rangecheck;

    /* check of gx_no_color_index */
    if (pdata[0] == 0xff) {
        *pcolor = gx_no_color_index;
        return 1;
    }

    /* num_bytes > arch_sizeof_color_index, discard first byte */
    for (i = (num_bytes >= arch_sizeof_color_index ? 1 : 0); i < num_bytes; i++)
        color = (color << 8) | pdata[i];
    *pcolor = color;
    return num_bytes;
}
