/* Copyright (C) 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdfk.c,v 1.10 2005/02/25 07:58:50 igor Exp $ */
/* Lab and ICCBased color space writing */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gxcspace.h"
#include "stream.h"
#include "gsicc.h"
#include "gserrors.h"
#include "gxcie.h"
#include "gdevpdfx.h"
#include "gdevpdfg.h"
#include "gdevpdfc.h"
#include "gdevpdfo.h"
#include "strimpl.h"

/* ------ CIE space synthesis ------ */

/* Add a /Range entry to a CIE-based color space dictionary. */
private int
pdf_cie_add_ranges(cos_dict_t *pcd, const gs_range *prange, int n, bool clamp)
{
    cos_array_t *pca = cos_array_alloc(pcd->pdev, "pdf_cie_add_ranges");
    int code = 0, i;

    if (pca == 0)
	return_error(gs_error_VMerror);
    for (i = 0; i < n; ++i) {
	floatp rmin = prange[i].rmin, rmax = prange[i].rmax;

	if (clamp) {
	    if (rmin < 0) rmin = 0;
	    if (rmax > 1) rmax = 1;
	}
	if ((code = cos_array_add_real(pca, rmin)) < 0 ||
	    (code = cos_array_add_real(pca, rmax)) < 0
	    )
	    break;
    }
    if (code >= 0)
	code = cos_dict_put_c_key_object(pcd, "/Range", COS_OBJECT(pca));
    if (code < 0)
	COS_FREE(pca, "pdf_cie_add_ranges");
    return code;
}

/* Transform a CIEBased color to XYZ. */
private int
cie_to_xyz(const double *in, double out[3], const gs_color_space *pcs,
	   const gs_imager_state *pis)
{
    gs_client_color cc;
    frac xyz[3];
    int ncomp = gs_color_space_num_components(pcs);
    int i;

    for (i = 0; i < ncomp; ++i)
	cc.paint.values[i] = in[i];
    cs_concretize_color(&cc, pcs, xyz, pis);
    out[0] = frac2float(xyz[0]);
    out[1] = frac2float(xyz[1]);
    out[2] = frac2float(xyz[2]);
    return 0;
}

/* ------ Lab space writing and synthesis ------ */

/* Transform XYZ values to Lab. */
private double
lab_g_inverse(double v)
{
    if (v >= (6.0 * 6.0 * 6.0) / (29 * 29 * 29))
	return pow(v, 1.0 / 3);	/* use cbrt if available? */
    else
	return (v * (841.0 / 108) + 4.0 / 29);
}
private void
xyz_to_lab(const double xyz[3], double lab[3], const gs_cie_common *pciec)
{
    const gs_vector3 *const pWhitePoint = &pciec->points.WhitePoint;
    double L, lunit;

    /* Calculate L* first. */
    L = lab_g_inverse(xyz[1] / pWhitePoint->v) * 116 - 16;
    /* Clamp L* to the PDF range [0..100]. */
    if (L < 0)
	L = 0;
    else if (L > 100)
	L = 100;
    lab[1] = L;
    lunit = (L + 16) / 116;

    /* Calculate a* and b*. */
    lab[0] = (lab_g_inverse(xyz[0] / pWhitePoint->u) - lunit) * 500;
    lab[2] = (lab_g_inverse(xyz[2] / pWhitePoint->w) - lunit) * -200;
}

/* Create a PDF Lab color space corresponding to a CIEBased color space. */
private int
lab_range(gs_range range_out[3] /* only [1] and [2] used */,
	  const gs_color_space *pcs, const gs_cie_common *pciec,
	  const gs_range *ranges, gs_memory_t *mem)
{
    /*
     * Determine the range of a* and b* by evaluating the color space
     * mapping at all of its extrema.
     */
    int ncomp = gs_color_space_num_components(pcs);
    gs_imager_state *pis;
    int code = gx_cie_to_xyz_alloc(&pis, pcs, mem);
    int i, j;

    if (code < 0)
	return code;
    for (j = 1; j < 3; ++j)
	range_out[j].rmin = 1000.0, range_out[j].rmax = -1000.0;
    for (i = 0; i < 1 << ncomp; ++i) {
	double in[4], xyz[3];

	for (j = 0; j < ncomp; ++j)
	    in[j] = (i & (1 << j) ? ranges[j].rmax : ranges[j].rmin);
	if (cie_to_xyz(in, xyz, pcs, pis) >= 0) {
	    double lab[3];

	    xyz_to_lab(xyz, lab, pciec);
	    for (j = 1; j < 3; ++j) {
		range_out[j].rmin = min(range_out[j].rmin, lab[j]);
		range_out[j].rmax = max(range_out[j].rmax, lab[j]);
	    }
	}
    }
    gx_cie_to_xyz_free(pis);
    return 0;
}
/*
 * Create a Lab color space object.
 * This procedure is exported for Lab color spaces in gdevpdfc.c.
 */
int
pdf_put_lab_color_space(cos_array_t *pca, cos_dict_t *pcd,
			const gs_range ranges[3] /* only [1] and [2] used */)
{
    int code;
    cos_value_t v;

    if ((code = cos_array_add(pca, cos_c_string_value(&v, "/Lab"))) >= 0)
	code = pdf_cie_add_ranges(pcd, ranges + 1, 2, false);
    return code;
}

/*
 * Create a Lab color space for a CIEBased space that can't be represented
 * directly as a Calxxx or Lab space.
 */
private int
pdf_convert_cie_to_lab(gx_device_pdf *pdev, cos_array_t *pca,
		       const gs_color_space *pcs,
		       const gs_cie_common *pciec, const gs_range *prange)
{
    cos_dict_t *pcd;
    gs_range ranges[3];
    int code;

    /****** NOT IMPLEMENTED YET, REQUIRES TRANSFORMING VALUES ******/
    if (1) return_error(gs_error_rangecheck);
    pcd = cos_dict_alloc(pdev, "pdf_convert_cie_to_lab(dict)");
    if (pcd == 0)
	return_error(gs_error_VMerror);
    if ((code = lab_range(ranges, pcs, pciec, prange, pdev->pdf_memory)) < 0 ||
	(code = pdf_put_lab_color_space(pca, pcd, ranges)) < 0 ||
	(code = pdf_finish_cie_space(pca, pcd, pciec)) < 0
	)
	COS_FREE(pcd, "pdf_convert_cie_to_lab(dict)");
    return code;
}

/* ------ ICCBased space writing and synthesis ------ */

/*
 * Create an ICCBased color space object (internal).  The client must write
 * the profile data on *ppcstrm.
 */
private int
pdf_make_iccbased(gx_device_pdf *pdev, cos_array_t *pca, int ncomps,
		  const gs_range *prange /*[4]*/,
		  const gs_color_space *pcs_alt,
		  cos_stream_t **ppcstrm,
		  const gs_range_t **pprange /* if scaling is needed */)

{
    cos_value_t v;
    int code;
    cos_stream_t * pcstrm = 0;
    cos_array_t * prngca = 0;
    bool std_ranges = true;
    bool scale_inputs = false;
    int i;

    /* Check the ranges. */
    if (pprange)
	*pprange = 0;
    for (i = 0; i < ncomps; ++i) {
	double rmin = prange[i].rmin, rmax = prange[i].rmax;

	if (rmin < 0.0 || rmax > 1.0) {
	    /* We'll have to scale the inputs.  :-( */
	    if (pprange == 0)
		return_error(gs_error_rangecheck); /* scaling not allowed */
	    *pprange = prange;
	    scale_inputs = true;
	}
	else if (rmin > 0.0 || rmax < 1.0)
	    std_ranges = false;
    }

    /* ICCBased color spaces are essentially copied to the output. */
    if ((code = cos_array_add(pca, cos_c_string_value(&v, "/ICCBased"))) < 0)
	return code;

    /* Create a stream for the output. */
    if ((pcstrm = cos_stream_alloc(pdev, "pdf_make_iccbased(stream)")) == 0) {
	code = gs_note_error(gs_error_VMerror);
	goto fail;
    }

    /* Indicate the number of components. */
    code = cos_dict_put_c_key_int(cos_stream_dict(pcstrm), "/N", ncomps);
    if (code < 0)
	goto fail;

    /* Indicate the range, if needed. */
    if (!std_ranges && !scale_inputs) {
	code = pdf_cie_add_ranges(cos_stream_dict(pcstrm), prange, ncomps, true);
	if (code < 0)
	    goto fail;
    }

    /* Output the alternate color space, if necessary. */
    switch (gs_color_space_get_index(pcs_alt)) {
    case gs_color_space_index_DeviceGray:
    case gs_color_space_index_DeviceRGB:
    case gs_color_space_index_DeviceCMYK:
	break;			/* implicit (default) */
    default:
	if ((code = pdf_color_space(pdev, &v, NULL, pcs_alt,
				    &pdf_color_space_names, false)) < 0 ||
	    (code = cos_dict_put_c_key(cos_stream_dict(pcstrm), "/Alternate",
				       &v)) < 0
	    )
	    goto fail;
    }

    /* Wrap up. */
    if ((code = cos_array_add_object(pca, COS_OBJECT(pcstrm))) < 0)
	goto fail;
    *ppcstrm = pcstrm;
    return code;
 fail:
    if (prngca)
	COS_FREE(prngca, "pdf_make_iccbased(Range)");
    if (pcstrm)
	COS_FREE(pcstrm, "pdf_make_iccbased(stream)");
    return code;
}
/*
 * Finish writing the data stream for an ICCBased color space object.
 */
private int
pdf_finish_iccbased(cos_stream_t *pcstrm)
{
    /*
     * The stream must be an indirect object.  Assign an ID, and write the
     * object out now.
     */
    gx_device_pdf *pdev = pcstrm->pdev;

    pcstrm->id = pdf_obj_ref(pdev);
    return cos_write_object(COS_OBJECT(pcstrm), pdev);
}

/*
 * Create an ICCBased color space for a CIEBased space that can't be
 * represented directly as a Calxxx or Lab space.
 */

typedef struct profile_table_s profile_table_t;
struct profile_table_s {
    const char *tag;
    const byte *data;
    uint length;
    uint data_length;		/* may be < length if write != 0 */
    int (*write)(cos_stream_t *, const profile_table_t *, gs_memory_t *);
    const void *write_data;
    const gs_range_t *ranges;
};
private profile_table_t *
add_table(profile_table_t **ppnt, const char *tag, const byte *data,
	  uint length)
{
    profile_table_t *pnt = (*ppnt)++;

    pnt->tag = tag, pnt->data = data, pnt->length = length;
    pnt->data_length = length;
    pnt->write = NULL;
    /* write_data not set */
    pnt->ranges = NULL;
    return pnt;
}
private void
set_uint32(byte bytes[4], uint value)
{
    bytes[0] = (byte)(value >> 24);
    bytes[1] = (byte)(value >> 16);
    bytes[2] = (byte)(value >> 8);
    bytes[3] = (byte)value;
}
private void
set_XYZ(byte bytes[4], floatp value)
{
    set_uint32(bytes, (uint)(int)(value * 65536));
}
private void
add_table_xyz3(profile_table_t **ppnt, const char *tag, byte bytes[20],
	       const gs_vector3 *pv)
{
    memcpy(bytes, "XYZ \000\000\000\000", 8);
    set_XYZ(bytes + 8, pv->u);
    set_XYZ(bytes + 12, pv->v);
    set_XYZ(bytes + 16, pv->w);
    DISCARD(add_table(ppnt, tag, bytes, 20));
}
private void
set_sample16(byte *p, floatp v)
{
    int value = (int)(v * 65535);

    if (value < 0)
	value = 0;
    else if (value > 65535)
	value = 65535;
    p[0] = (byte)(value >> 8);
    p[1] = (byte)value;
}
/* Create and write a TRC curve table. */
private int write_trc_abc(cos_stream_t *, const profile_table_t *, gs_memory_t *);
private int write_trc_lmn(cos_stream_t *, const profile_table_t *, gs_memory_t *);
private profile_table_t *
add_trc(profile_table_t **ppnt, const char *tag, byte bytes[12],
	const gs_cie_common *pciec, cie_cache_one_step_t one_step)
{
    const int count = gx_cie_cache_size;
    profile_table_t *pnt;

    memcpy(bytes, "curv\000\000\000\000", 8);
    set_uint32(bytes + 8, count);
    pnt = add_table(ppnt, tag, bytes, 12);
    pnt->length += count * 2;
    pnt->write = (one_step == ONE_STEP_ABC ? write_trc_abc : write_trc_lmn);
    pnt->write_data = (const gs_cie_abc *)pciec;
    return pnt;
}
private int
rgb_to_index(const profile_table_t *pnt)
{
    switch (pnt->tag[0]) {
    case 'r': return 0;
    case 'g': return 1;
    case 'b': default: /* others can't happen */ return 2;
    }
}
private double
cache_arg(int i, int denom, const gs_range_t *range)
{
    double arg = i / (double)denom;

    if (range) {
	/* Sample over the range [range->rmin .. range->rmax]. */
	arg = arg * (range->rmax - range->rmin) + range->rmin;
    }
    return arg;
}

private int
write_trc_abc(cos_stream_t *pcstrm, const profile_table_t *pnt,
	      gs_memory_t *ignore_mem)
{
    /* Write the curve table from DecodeABC. */
    const gs_cie_abc *pabc = pnt->write_data;
    int ci = rgb_to_index(pnt);
    gs_cie_abc_proc proc = pabc->DecodeABC.procs[ci];
    byte samples[gx_cie_cache_size * 2];
    byte *p = samples;
    int i;

    for (i = 0; i < gx_cie_cache_size; ++i, p += 2)
	set_sample16(p, proc(cache_arg(i, gx_cie_cache_size - 1, pnt->ranges),
			     pabc));
    return cos_stream_add_bytes(pcstrm, samples, gx_cie_cache_size * 2);
}
private int
write_trc_lmn(cos_stream_t *pcstrm, const profile_table_t *pnt,
	      gs_memory_t *ignore_mem)
{
    const gs_cie_common *pciec = pnt->write_data;
    int ci = rgb_to_index(pnt);
    gs_cie_common_proc proc = pciec->DecodeLMN.procs[ci];
    byte samples[gx_cie_cache_size * 2];
    byte *p = samples;
    int i;

    /* Write the curve table from DecodeLMN. */
    for (i = 0; i < gx_cie_cache_size; ++i, p += 2)
	set_sample16(p, proc(cache_arg(i, gx_cie_cache_size - 1, pnt->ranges),
			     pciec));
    return cos_stream_add_bytes(pcstrm, samples, gx_cie_cache_size * 2);
}
/* Create and write an a2b0 lookup table. */
#define NUM_IN_ENTRIES 2	/* assume linear interpolation */
#define NUM_OUT_ENTRIES 2	/* ibid. */
#define MAX_CLUT_ENTRIES 2500	/* enough for 7^4 */
typedef struct icc_a2b0_s {
    byte header[52];
    const gs_color_space *pcs;
    int num_points;		/* on each axis of LUT */
    int count;			/* total # of entries in LUT */
} icc_a2b0_t;
private int write_a2b0(cos_stream_t *, const profile_table_t *, gs_memory_t *);
private profile_table_t *
add_a2b0(profile_table_t **ppnt, icc_a2b0_t *pa2b, int ncomps,
	 const gs_color_space *pcs)
{
    static const byte a2b0_data[sizeof(pa2b->header)] = {
	'm', 'f', 't', '2',		/* type signature */
	0, 0, 0, 0,			/* reserved, 0 */
	0,				/* # of input channels **VARIABLE** */
	3,				/* # of output channels */
	0,				/* # of CLUT points **VARIABLE** */
	0,				/* reserved, padding */
	0, 1, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0, /* matrix column 0 */
	0, 0, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0, /* matrix column 1 */
	0, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0, /* matrix column 2 */
	0, NUM_IN_ENTRIES,		/* # of input table entries */
	0, NUM_OUT_ENTRIES		/* # of output table entries */
    };
    int num_points = (int)floor(pow(MAX_CLUT_ENTRIES, 1.0 / ncomps));
    profile_table_t *pnt;

    num_points = min(num_points, 255);
    memcpy(pa2b->header, a2b0_data, sizeof(a2b0_data));
    pa2b->header[8] = ncomps;
    pa2b->header[10] = num_points;
    pa2b->pcs = pcs;
    pa2b->num_points = num_points;
    pa2b->count = (int)pow(num_points, ncomps);
    pnt = add_table(ppnt, "A2B0", pa2b->header,
		    sizeof(pa2b->header) +
		    ncomps * 2 * NUM_IN_ENTRIES + /* in */
		    pa2b->count * (3 * 2) + /* clut: XYZ, 16-bit values */
		    3 * 2 * NUM_OUT_ENTRIES /* out */
		    );
    pnt->data_length = sizeof(pa2b->header); /* only write fixed part */
    pnt->write = write_a2b0;
    pnt->write_data = pa2b;
    return pnt;
}
private int
write_a2b0(cos_stream_t *pcstrm, const profile_table_t *pnt,
	   gs_memory_t *mem)
{
    const icc_a2b0_t *pa2b = pnt->write_data;
    const gs_color_space *pcs = pa2b->pcs;
    int ncomps = pa2b->header[8];
    int num_points = pa2b->num_points;
    int i;
#define MAX_NCOMPS 4		/* CIEBasedDEFG */
    static const byte v01[MAX_NCOMPS * 2 * 2] = {
	0,0, 255,255,   0,0, 255,255,   0,0, 255,255,   0,0, 255,255
    };
    gs_imager_state *pis;
    int code;

    /* Write the input table. */

    if ((code = cos_stream_add_bytes(pcstrm, v01, ncomps * 4)) < 0
	)
	return code;

    /* Write the lookup table. */

    code = gx_cie_to_xyz_alloc(&pis, pcs, mem);
    if (code < 0)
	return code;
    for (i = 0; i < pa2b->count; ++i) {
	double in[MAX_NCOMPS], xyz[3];
	byte entry[3 * 2];
	byte *p = entry;
	int n, j;

	for (n = i, j = ncomps - 1; j >= 0; --j, n /= num_points)
	    in[j] = cache_arg(n % num_points, num_points - 1,
			      (pnt->ranges ? pnt->ranges + j : NULL));
	cie_to_xyz(in, xyz, pcs, pis);
	/*
	 * NOTE: Due to an obscure provision of the ICC Profile
	 * specification, values in a2b0 lookup tables do *not* represent
	 * the range [0 .. 1], but rather the range [0
	 * .. MAX_ICC_XYZ_VALUE].  This caused us a lot of grief before we
	 * figured it out!
	 */
#define MAX_ICC_XYZ_VALUE (1 + 32767.0/32768)
	for (j = 0; j < 3; ++j, p += 2)
	    set_sample16(p, xyz[j] / MAX_ICC_XYZ_VALUE);
#undef MAX_ICC_XYZ_VALUE
	if ((code = cos_stream_add_bytes(pcstrm, entry, sizeof(entry))) < 0)
	    break;
    }
    gx_cie_to_xyz_free(pis);
    if (code < 0)
	return code;

    /* Write the output table. */

    return cos_stream_add_bytes(pcstrm, v01, 3 * 4);
}
private int
pdf_convert_cie_to_iccbased(gx_device_pdf *pdev, cos_array_t *pca,
			    const gs_color_space *pcs, const char *dcsname,
			    const gs_cie_common *pciec, const gs_range *prange,
			    cie_cache_one_step_t one_step,
			    const gs_matrix3 *pmat, const gs_range_t **pprange)
{
    /*
     * We have two options for creating an ICCBased color space to represent
     * a CIEBased space.  For CIEBasedABC spaces using only a single
     * Decode step followed by a single Matrix step, we can use [rgb]TRC
     * and [rgb]XYZ; for CIEBasedA spaces using only DecodeA, we could use
     * kTRC (but don't); otherwise, we must use a mft2 LUT.
     */
    int code;
    int ncomps = gs_color_space_num_components(pcs);
    gs_color_space alt_space;
    cos_stream_t *pcstrm;

    /*
     * Even though Ghostscript includes icclib, icclib is unusable here,
     * because it requires random access to the output stream.
     * Instead, we construct the ICC profile by hand.
     */
    /* Header */
    byte header[128];
    static const byte header_data[] = {
	0, 0, 0, 0,			/* profile size **VARIABLE** */
	0, 0, 0, 0,			/* CMM type signature */
	0x02, 0x20, 0, 0,		/* profile version number */
	's', 'c', 'n', 'r',		/* profile class signature */
	0, 0, 0, 0,			/* data color space **VARIABLE** */
	'X', 'Y', 'Z', ' ',		/* connection color space */
	2002 / 256, 2002 % 256, 0, 1, 0, 1, /* date (1/1/2002) */
	0, 0, 0, 0, 0, 0,		/* time */
	'a', 'c', 's', 'p',		/* profile file signature */
	0, 0, 0, 0,			/* primary platform signature */
	0, 0, 0, 3,			/* profile flags (embedded use only) */
	0, 0, 0, 0, 0, 0, 0, 0,		/* device manufacturer */
	0, 0, 0, 0,			/* device model */
	0, 0, 0, 0, 0, 0, 0, 2		/* device attributes */
	/* Remaining fields are zero or variable. */
	/* [4] */			/* rendering intent */
	/* 3 * [4] */			/* illuminant */
    };
    /* Description */
#define DESC_LENGTH 5		/* "adhoc" */
    byte desc[12 + DESC_LENGTH + 1 + 11 + 67];
    static const byte desc_data[] = {
	'd', 'e', 's', 'c',		/* type signature */
	0, 0, 0, 0,			/* reserved, 0 */
	0, 0, 0, DESC_LENGTH + 1,	/* ASCII description length */
	'a', 'd', 'h', 'o', 'c', 0,	/* ASCII description */
	/* Remaining fields are zero. */
    };
    /* White point */
    byte wtpt[20];
    /* Copyright (useless, but required by icclib) */
    static const byte cprt_data[] = {
	't', 'e', 'x', 't',	/* type signature */
	0, 0, 0, 0,		/* reserved, 0 */
	'n', 'o', 'n', 'e', 0	/* must be null-terminated (!) */
    };
    /* Lookup table */
    icc_a2b0_t a2b0;
    /* [rgb]TRC */
    byte rTRC[12], gTRC[12], bTRC[12];
    /* [rgb]XYZ */
    byte rXYZ[20], gXYZ[20], bXYZ[20];
    /* Table structures */
#define MAX_NUM_TABLES 9	/* desc, [rgb]TRC, [rgb]xYZ, wtpt, cprt */
    profile_table_t tables[MAX_NUM_TABLES];
    profile_table_t *next_table = tables;

    pdf_cspace_init_Device(pdev->memory, &alt_space, ncomps);	/* can't fail */
    code = pdf_make_iccbased(pdev, pca, ncomps, prange, &alt_space,
			     &pcstrm, pprange);
    if (code < 0)
	return code;

    /* Fill in most of the header, except for the total size. */

    memset(header, 0, sizeof(header));
    memcpy(header, header_data, sizeof(header_data));
    memcpy(header + 16, dcsname, 4);

    /* Construct the tables. */

    /* desc */
    memset(desc, 0, sizeof(desc));
    memcpy(desc, desc_data, sizeof(desc_data));
    DISCARD(add_table(&next_table, "desc", desc, sizeof(desc)));

    /* wtpt */
    add_table_xyz3(&next_table, "wtpt", wtpt, &pciec->points.WhitePoint);
    memcpy(header + 68, wtpt + 8, 12); /* illuminant = white point */

    /* cprt */
    /* (We have no use for this tag, but icclib requires it.) */
    DISCARD(add_table(&next_table, "cprt", cprt_data, sizeof(cprt_data)));

    /* Use TRC + XYZ if possible, otherwise AToB. */
    if ((one_step == ONE_STEP_ABC || one_step == ONE_STEP_LMN) && pmat != 0) {
	/* Use TRC + XYZ. */
	profile_table_t *tr =
	    add_trc(&next_table, "rTRC", rTRC, pciec, one_step);
	profile_table_t *tg =
	    add_trc(&next_table, "gTRC", gTRC, pciec, one_step);
	profile_table_t *tb =
	    add_trc(&next_table, "bTRC", bTRC, pciec, one_step);

	if (*pprange) {
	    tr->ranges = *pprange;
	    tg->ranges = *pprange + 1;
	    tb->ranges = *pprange + 2;
	}
	add_table_xyz3(&next_table, "rXYZ", rXYZ, &pmat->cu);
	add_table_xyz3(&next_table, "gXYZ", gXYZ, &pmat->cv);
	add_table_xyz3(&next_table, "bXYZ", bXYZ, &pmat->cw);
    } else {
	/* General case, use a lookup table. */
	/* AToB (mft2) */
	profile_table_t *pnt = add_a2b0(&next_table, &a2b0, ncomps, pcs);

	pnt->ranges = *pprange;
    }

    /* Write the profile. */
    {
	byte bytes[4 + MAX_NUM_TABLES * 12];
	int num_tables = next_table - tables;
	int i;
	byte *p;
	uint table_size = 4 + num_tables * 12;
	uint offset = sizeof(header) + table_size;

	set_uint32(bytes, next_table - tables);
	for (i = 0, p = bytes + 4; i < num_tables; ++i, p += 12) {
	    memcpy(p, tables[i].tag, 4);
	    set_uint32(p + 4, offset);
	    set_uint32(p + 8, tables[i].length);
	    offset += round_up(tables[i].length, 4);
	}
	set_uint32(header, offset);
	if ((code = cos_stream_add_bytes(pcstrm, header, sizeof(header))) < 0 ||
	    (code = cos_stream_add_bytes(pcstrm, bytes, table_size)) < 0
	    )
	    return code;
	for (i = 0; i < num_tables; ++i) {
	    uint len = tables[i].data_length;
	    static const byte pad[3] = {0, 0, 0};

	    if ((code = cos_stream_add_bytes(pcstrm, tables[i].data, len)) < 0 ||
		(tables[i].write != 0 &&
		 (code = tables[i].write(pcstrm, &tables[i], pdev->pdf_memory)) < 0) ||
		(code = cos_stream_add_bytes(pcstrm, pad, 
			-(int)(tables[i].length) & 3)) < 0
		)
		return code;
	}
    }

    return pdf_finish_iccbased(pcstrm);
}

/* ------ Entry points (from gdevpdfc.c) ------ */

/*
 * Create an ICCBased color space.  This is a single-use procedure,
 * broken out only for readability.
 */
int
pdf_iccbased_color_space(gx_device_pdf *pdev, cos_value_t *pvalue,
			 const gs_color_space *pcs, cos_array_t *pca)
{
    /*
     * This would arise only in a pdf ==> pdf translation, but we
     * should allow for it anyway.
     */
    const gs_icc_params * picc_params = &pcs->params.icc;
    const gs_cie_icc * picc_info = picc_params->picc_info;
    cos_stream_t * pcstrm;
    int code =
	pdf_make_iccbased(pdev, pca, picc_info->num_components,
			  picc_info->Range.ranges,
			  (const gs_color_space *)&picc_params->alt_space,
			  &pcstrm, NULL);

    if (code < 0)
	return code;

    /* Transfer the profile stream. */
    code = cos_stream_add_stream_contents(pcstrm, picc_info->instrp);
    if (code >= 0)
	code = pdf_finish_iccbased(pcstrm);
    /*
     * The stream has been added to the array: in case of failure, the
     * caller will free the array, so there is no need to free the stream
     * explicitly here.
     */
    return code;
}

/* Convert a CIEBased space to Lab or ICCBased. */
int
pdf_convert_cie_space(gx_device_pdf *pdev, cos_array_t *pca,
		      const gs_color_space *pcs, const char *dcsname,
		      const gs_cie_common *pciec, const gs_range *prange,
		      cie_cache_one_step_t one_step, const gs_matrix3 *pmat,
		      const gs_range_t **pprange)
{
    return (pdev->CompatibilityLevel < 1.3 ?
	    /* PDF 1.2 or earlier, use a Lab space. */
	    pdf_convert_cie_to_lab(pdev, pca, pcs, pciec, prange) :
	    /* PDF 1.3 or later, use an ICCBased space. */
	    pdf_convert_cie_to_iccbased(pdev, pca, pcs, dcsname, pciec, prange,
					one_step, pmat, pprange)
	    );
}
