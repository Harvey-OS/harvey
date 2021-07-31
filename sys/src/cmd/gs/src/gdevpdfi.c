/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gdevpdfi.c,v 1.3 2000/03/16 01:21:24 lpd Exp $ */
/* Image handling for PDF-writing driver */
#include "math_.h"
#include "memory_.h"
#include "string_.h"
#include "jpeglib_.h"		/* for sdct.h */
#include "gx.h"
#include "gserrors.h"
#include "gsflip.h"
#include "gdevpdfx.h"
#include "gdevpdff.h"		/* for synthesized bitmap fonts */
#include "gdevpdfo.h"		/* for data stream */
#include "gxcspace.h"
#include "gscolor2.h"		/* for gscie.h */
#include "gscie.h"		/* requires gscspace.h */
#include "gxistate.h"
#include "strimpl.h"
#include "sa85x.h"
#include "scfx.h"
#include "sdct.h"
#include "slzwx.h"
#include "spngpx.h"
#include "srlx.h"
#include "szlibx.h"

/* We need this color space type for constructing temporary color spaces. */
extern const gs_color_space_type gs_color_space_type_Indexed;

/* Import procedures for writing filter parameters. */
extern stream_state_proc_get_params(s_DCTE_get_params, stream_DCT_state);
extern stream_state_proc_get_params(s_CF_get_params, stream_CF_state);

#define MAX_INLINE_IMAGE_BYTES 4000

/* ---------------- Utilities ---------------- */

/* ------ Images ------ */

/* Test whether a cached CIE procedure is the identity function. */
#define cie_cache_is_identity(pc)\
  ((pc)->floats.params.is_identity)
#define cie_cache3_is_identity(pca)\
  (cie_cache_is_identity(&(pca)[0]) &&\
   cie_cache_is_identity(&(pca)[1]) &&\
   cie_cache_is_identity(&(pca)[2]))

/*
 * Test whether a cached CIE procedure is an exponential.  A cached
 * procedure is exponential iff f(x) = k*(x^p).  We make a very cursory
 * check for this: we require that f(0) = 0, set k = f(1), set p =
 * log[a](f(a)/k), and then require that f(b) = k*(b^p), where a and b are
 * two arbitrarily chosen values between 0 and 1.  Naturally all this is
 * done with some slop.
 */
#define ia (gx_cie_cache_size / 3)
#define ib (gx_cie_cache_size * 2 / 3)
#define iv(i) ((i) / (double)(gx_cie_cache_size - 1))
#define a iv(ia)
#define b iv(ib)

private bool
cie_values_are_exponential(floatp va, floatp vb, floatp k,
			   float *pexpt)
{
    double p;

    if (fabs(k) < 0.001)
	return false;
    if (va == 0 || (va > 0) != (k > 0))
	return false;
    p = log(va / k) / log(a);
    if (fabs(vb - k * pow(b, p)) >= 0.001)
	return false;
    *pexpt = p;
    return true;
}

private bool
cie_scalar_cache_is_exponential(const gx_cie_scalar_cache * pc, float *pexpt)
{
    double k, va, vb;

    if (fabs(pc->floats.values[0]) >= 0.001)
	return false;
    k = pc->floats.values[gx_cie_cache_size - 1];
    va = pc->floats.values[ia];
    vb = pc->floats.values[ib];
    return cie_values_are_exponential(va, vb, k, pexpt);
}
#define cie_scalar3_cache_is_exponential(pca, expts)\
  (cie_scalar_cache_is_exponential(&(pca)[0], &(expts)[0]) &&\
   cie_scalar_cache_is_exponential(&(pca)[1], &(expts)[1]) &&\
   cie_scalar_cache_is_exponential(&(pca)[2], &(expts)[2]))

private bool
cie_vector_cache_is_exponential(const gx_cie_vector_cache * pc, float *pexpt)
{
    double k, va, vb;

    if (fabs(pc->vecs.values[0].u) >= 0.001)
	return false;
    k = pc->vecs.values[gx_cie_cache_size - 1].u;
    va = pc->vecs.values[ia].u;
    vb = pc->vecs.values[ib].u;
    return cie_values_are_exponential(va, vb, k, pexpt);
}
#define cie_vector3_cache_is_exponential(pca, expts)\
  (cie_vector_cache_is_exponential(&(pca)[0], &(expts)[0]) &&\
   cie_vector_cache_is_exponential(&(pca)[1], &(expts)[1]) &&\
   cie_vector_cache_is_exponential(&(pca)[2], &(expts)[2]))

#undef ia
#undef ib
#undef iv
#undef a
#undef b

/* Define the long and short versions of the keys in an image dictionary, */
/* and other strings for images. */
typedef struct pdf_image_names_s {
    const char *ASCII85Decode;
    const char *ASCIIHexDecode;
    const char *BitsPerComponent;
    const char *CalCMYK;
    const char *CalGray;
    const char *CalRGB;
    const char *CCITTFaxDecode;
    const char *ColorSpace;
    const char *DCTDecode;
    const char *Decode;
    const char *DecodeParms;
    const char *DeviceCMYK;
    const char *DeviceGray;
    const char *DeviceRGB;
    const char *Filter;
    const char *FlateDecode;
    const char *Height;
    const char *ImageMask;
    const char *Indexed;
    const char *Interpolate;
    const char *LZWDecode;
    const char *RunLengthDecode;
    const char *Width;
} pdf_image_names;
private const pdf_image_names image_names_full =
{
    "/ASCII85Decode", "/ASCIIHexDecode", "/BitsPerComponent",
    "/CalCMYK", "/CalGray", "/CalRGB", "/CCITTFaxDecode", "/ColorSpace",
    "/DCTDecode", "/Decode", "/DecodeParms",
    "/DeviceCMYK", "/DeviceGray", "/DeviceRGB",
    "/Filter", "/FlateDecode", "/Height", "/ImageMask", "/Indexed",
    "/Interpolate", "/LZWDecode", "/RunLengthDecode", "/Width"
};
private const pdf_image_names image_names_short =
{
    "/A85", "/AHx", "/BPC",
	/*
	 * Based on Adobe's published PDF documentation, it appears that the
	 * abbreviations for the calibrated color spaces were introduced in
	 * PDF 1.1 (added to Table 7.3) and removed in PDF 1.2 (Table 8.1)!
	 */
"/CalCMYK" /*CC */ , "/CalGray" /*CG */ , "/CalRGB" /*CR */ , "/CCF", "/CS",
    "/DCT", "/D", "/DP",
    "/CMYK", "/G", "/RGB",
    "/F", "/Fl", "/H", "/IM", "/I",
    "/I", "/LZW", "/RL", "/W"
};

/* Write out the values of image parameters other than filters. */
/* pdev is used only for updating procsets. */
private int
pdf_write_image_values(stream *s, gx_device_pdf * pdev, const gs_image_t * pim,
		       const pdf_image_names * pin, long *phpos)
{
    const gs_color_space *pcs = pim->ColorSpace;
    const char *cs_name;
    int num_components;
    float indexed_decode[2];
    const float *default_decode = NULL;

    if (pim->ImageMask) {
	pprints1(s, "%s true", pin->ImageMask);
	pdev->procsets |= ImageB;
	num_components = 1;
    } else {
	const gs_color_space *pbcs = pcs;
	const gs_indexed_params *pip = 0;
	const gs_cie_common *pciec;

	pputs(s, pin->ColorSpace);
      csw:switch (gs_color_space_get_index(pbcs)) {
	    case gs_color_space_index_DeviceGray:
		pdev->procsets |= ImageB;
		cs_name = pin->DeviceGray;
		break;
	    case gs_color_space_index_DeviceRGB:
		pdev->procsets |= ImageC;
		cs_name = pin->DeviceRGB;
		break;
	    case gs_color_space_index_DeviceCMYK:
		pdev->procsets |= ImageC;
		cs_name = pin->DeviceCMYK;
		break;
	    case gs_color_space_index_CIEA:
		pdev->procsets |= ImageB;
		pprints1(s, "[%s<<", pin->CalGray);
		{
		    const gs_cie_a *pcie = pbcs->params.a;
		    float expts[3];

		    if (cie_cache3_is_identity(pcie->common.caches.DecodeLMN))
			cie_vector_cache_is_exponential(&pcie->caches.DecodeA, &expts[0]);
		    else
			discard(cie_scalar3_cache_is_exponential(pcie->common.caches.DecodeLMN, expts));
		    if (expts[0] != 1)
			pprintg1(s, "/Gamma %g", expts[0]);
		    pciec = (const gs_cie_common *)pcie;
		}
	      cal:pprintg3(s, "/WhitePoint[%g %g %g]",
			 pciec->points.WhitePoint.u,
			 pciec->points.WhitePoint.v,
			 pciec->points.WhitePoint.w);
		if (pciec->points.BlackPoint.u != 0 ||
		    pciec->points.BlackPoint.v != 0 ||
		    pciec->points.BlackPoint.w != 0
		    )
		    pprintg3(s, "/BlackPoint[%g %g %g]",
			     pciec->points.BlackPoint.u,
			     pciec->points.BlackPoint.v,
			     pciec->points.BlackPoint.w);
		pputs(s, ">>]");
		cs_name = 0;
		break;
	    case gs_color_space_index_CIEABC:
		pdev->procsets |= ImageC;
		pprints1(s, "[%s<<", pin->CalRGB);
		{
		    const gs_cie_abc *pcie = pbcs->params.abc;
		    const gs_matrix3 *pmat;
		    float expts[3];

		    if (pcie->common.MatrixLMN.is_identity &&
			cie_cache3_is_identity(pcie->common.caches.DecodeLMN)
			) {
			discard(cie_vector3_cache_is_exponential(pcie->caches.DecodeABC, expts));
			pmat = &pcie->MatrixABC;
		    } else {
			discard(cie_scalar3_cache_is_exponential(pcie->common.caches.DecodeLMN, expts));
			pmat = &pcie->common.MatrixLMN;
		    }
		    if (expts[0] != 1 || expts[1] != 1 || expts[2] != 1)
			pprintg3(s, "/Gamma[%g %g %g]", expts[0], expts[1],
				 expts[2]);
		    if (!pmat->is_identity) {
			pprintg3(s, "/Matrix[%g %g %g",
				 pmat->cu.u, pmat->cu.v, pmat->cu.w);
			pprintg6(s, " %g %g %g %g %g %g]",
				 pmat->cv.u, pmat->cv.v, pmat->cv.w,
				 pmat->cw.u, pmat->cw.v, pmat->cw.w);
		    }
		    pciec = (const gs_cie_common *)pcie;
		}
		goto cal;
	    case gs_color_space_index_Indexed:
		pdev->procsets |= ImageI;
		pprints1(s, "[%s", pin->Indexed);
		pip = &pcs->params.indexed;
		pbcs = (const gs_color_space *)&pip->base_space;
		indexed_decode[0] = 0;
		indexed_decode[1] = (1 << pim->BitsPerComponent) - 1;
		default_decode = indexed_decode;
		goto csw;
	    default:		/* shouldn't happen */
		return_error(gs_error_rangecheck);
	}
	if (cs_name)
	    pprints1(s, " %s", cs_name);
	num_components = gs_color_space_num_components(pbcs);
	if (pip) {
	    register const char *const hex_digits = "0123456789abcdef";
	    int i;

	    pprintd1(s, " %d\n<", pip->hival);
	    for (i = 0; i < (pip->hival + 1) * num_components; ++i) {
		byte b = pip->lookup.table.data[i];

		pputc(s, hex_digits[b >> 4]);
		pputc(s, hex_digits[b & 0xf]);
	    }
	    pputs(s, ">\n]");
	    num_components = 1;
	}
    }
    pputs(s, pin->Width);
    pprintd1(s, " %d", pim->Width);
    pputs(s, pin->Height);
    pputc(s, ' ');
    if (phpos)
	*phpos = stell(s);	/* save Height position for later update */
    pprintd1(s, "%d", pim->Height);
    pputs(s, pin->BitsPerComponent);
    pprintd1(s, " %d", pim->BitsPerComponent);
    {
	int i;

	for (i = 0; i < num_components * 2; ++i)
	    if (pim->Decode[i] !=
		(default_decode ? default_decode[i] : i & 1)
		)
		break;
	if (i < num_components * 2) {
	    char sepr = '[';

	    pputs(s, pin->Decode);
	    for (i = 0; i < num_components * 2; sepr = ' ', ++i) {
		pputc(s, sepr);
		pprintg1(s, "%g", pim->Decode[i]);
	    }
	    pputc(s, ']');
	}
    }
    if (pim->Interpolate)
	pprints1(s, "%s true", pin->Interpolate);
    return 0;
}

/* Write out filters for an image. */
/* Currently this only writes parameters for CCITTFaxDecode. */
private int
pdf_write_image_filters(stream *s, gs_memory_t *mem,
			const psdf_binary_writer * pbw,
			const pdf_image_names * pin)
{
    const char *filter_name = 0;
    bool binary_ok = true;
    stream *fs = pbw->strm;
    byte decode_parms[100];	/* must be large enough, see below */
    stream s_parms;
    gs_param_list *printer;

    swrite_string(&s_parms, decode_parms, sizeof(decode_parms));
    for (; fs != s; fs = fs->strm) {
	const stream_state *st = fs->state;
	const stream_template *template = st->template;

	if (template == &s_A85E_template)
	    binary_ok = false;
	else if (template == &s_CFE_template) {
	    param_printer_params_t ppp;
	    stream_CF_state cfs;
	    int code;

	    ppp = param_printer_params_default;
	    ppp.prefix = "<<";
	    ppp.suffix = ">>";
	    code = psdf_alloc_param_printer(&printer, &ppp, &s_parms, mem);
	    if (code < 0)
		return code;
	    /*
	     * If EndOfBlock is true, we mustn't write out a Rows value.
	     * This is a hack....
	     */
	    cfs = *(const stream_CF_state *)st;
	    if (cfs.EndOfBlock)
		cfs.Rows = 0;
	    code = s_CF_get_params(printer, &cfs, false);
	    psdf_free_param_printer(printer);
	    if (code < 0)
		return code;
	    filter_name = pin->CCITTFaxDecode;
	} else if (template == &s_DCTE_template)
	    filter_name = pin->DCTDecode;
	else if (template == &s_zlibE_template)
	    filter_name = pin->FlateDecode;
	else if (template == &s_LZWE_template)
	    filter_name = pin->LZWDecode;
	else if (template == &s_PNGPE_template) {
	    /* This is a predictor for FlateDecode or LZWEncode. */
	    const stream_PNGP_state *const ss =
	    (const stream_PNGP_state *)st;

	    pprintd1(&s_parms, "<</Predictor %d", ss->Predictor);
	    pprintld1(&s_parms, "/Columns %ld", (long)ss->Columns);
	    if (ss->Colors != 1)
		pprintd1(&s_parms, "/Colors %d", ss->Colors);
	    if (ss->BitsPerComponent != 8)
		pprintd1(&s_parms, "/BitsPerComponent %d", ss->BitsPerComponent);
	    pputs(&s_parms, ">>");
	} else if (template == &s_RLE_template)
	    filter_name = pin->RunLengthDecode;
    }
    spputc(&s_parms, 0);	/* null terminator */
    sclose(&s_parms);
    if (filter_name) {
	if (binary_ok)
	    pprints2(s, "%s%s", pin->Filter, filter_name);
	else
	    pprints3(s, "%s[%s%s]", pin->Filter, pin->ASCII85Decode,
		     filter_name);
	if (decode_parms[0])
	    pprints2(s,
		     (binary_ok ? "%s%s" : "%s[null%s]"),
		     pin->DecodeParms, (const char *)decode_parms);
    } else if (!binary_ok)
	pprints2(s, "%s%s", pin->Filter, pin->ASCII85Decode);
    return 0;
}

/* Write out image parameters for an in-line image or an image resource. */
/* decode_parms, if supplied, must start with /, [, or <<. */
private int
pdf_write_image_params(stream *s, gx_device_pdf * pdev, const gs_image_t * pim,
		       const psdf_binary_writer * pbw,
		       const pdf_image_names * pin, long *phpos)
{
    int code = pdf_write_image_values(s, pdev, pim, pin, phpos);

    if (code < 0)
	return code;
    return pdf_write_image_filters(s, pdev->pdf_memory, pbw, pin);
}

/* Fill in the image parameters for a device space bitmap. */
/* PDF images are always specified top-to-bottom. */
/* data_h is the actual number of data rows, which may be less than h. */
private void
pdf_make_bitmap_matrix(gs_matrix * pmat, int x, int y, int w, int h,
		       int h_actual)
{
    pmat->xx = w;
    pmat->xy = 0;
    pmat->yx = 0;
    pmat->yy = -h_actual;
    pmat->tx = x;
    pmat->ty = y + h;
}
private void
pdf_make_bitmap_image(gs_image_t * pim, int x, int y, int w, int h)
{
    pim->Width = w;
    pim->Height = h;
    pdf_make_bitmap_matrix(&pim->ImageMatrix, x, y, w, h, h);
}

/* Put out the gsave and matrix for an image. */
private void
pdf_put_image_matrix(gx_device_pdf * pdev, const gs_matrix * pmat,
		     floatp y_scale)
{
    gs_matrix imat;

    gs_matrix_scale(pmat, 1.0, y_scale, &imat);
    pdf_put_matrix(pdev, "q ", &imat, "cm\n");
}

/* ------ Image writing ------ */

/* Forward references */
private image_enum_proc_plane_data(pdf_image_plane_data);
private image_enum_proc_end_image(pdf_image_end_image);

private const gx_image_enum_procs_t pdf_image_enum_procs =
{
    pdf_image_plane_data,
    pdf_image_end_image
};

/* Define the structure for writing an image. */
typedef struct pdf_image_writer_s {
    psdf_binary_writer binary;
    const pdf_image_names *pin;
    const char *begin_data;
    pdf_resource_t *pres;	/* XObject resource iff not in-line */
    long length_id;		/* id of length object (forward reference) */
    long Height_pos;		/* position for overwriting Height if needed */
    int data_offset;		/* offset of data from start of object */
    int height;			/* initially specified image height */
    long start_pos;		/* start of data in final file */
    cos_stream_t *data;
} pdf_image_writer;
gs_private_st_suffix_add2(st_pdf_image_writer, pdf_image_writer,
  "pdf_image_writer", pdf_image_writer_enum_ptrs, pdf_image_writer_reloc_ptrs,
  st_psdf_binary_writer, pres, data);
#define pdf_image_writer_max_ptrs (psdf_binary_writer_max_ptrs + 2)

/* Begin writing an image. */
private int
pdf_begin_write_image(gx_device_pdf * pdev, pdf_image_writer * piw,
		      gx_bitmap_id id, bool in_line)
{
    if (in_line) {
	piw->pres = 0;
	piw->pin = &image_names_short;
	piw->begin_data = (pdev->binary_ok ? "ID " : "ID\n");
    } else {
	int code = pdf_begin_resource(pdev, resourceXObject, id, &piw->pres);
	stream *s = pdev->strm;

	if (code < 0)
	    return code;
	piw->pres->rid = id;
	piw->length_id = pdf_obj_ref(pdev);
	pprintld1(s, "/Subtype/Image/Length %ld 0 R\n",
		  piw->length_id);
	piw->pin = &image_names_full;
	piw->begin_data = ">>\nstream\n";
    }
    return 0;
}

/* Set up the binary writer for an image. */
private int
pdf_begin_image_binary(gx_device_pdf *pdev, pdf_image_writer * piw)
{
    psdf_binary_writer *const pbw = &piw->binary;
    /* Patch pdev->strm so the right stream gets into the writer. */
    stream *save_strm = pdev->strm;
    int code;
    
    pdev->strm = pdev->streams.strm;
    code = psdf_begin_binary((gx_device_psdf *) pdev, pbw);
    pdev->strm = save_strm;
    return code;
}

/* Begin writing the image data. */
private int
pdf_begin_image_data(gx_device_pdf * pdev, pdf_image_writer * piw,
		     const gs_image_t * pim, int h)
{
    gs_memory_t *mem = pdev->pdf_memory;
    stream *s = pdev->streams.strm;
    long start_pos;
    int code;

    if (piw->pres) {
	pdf_x_object_t *pxo = (pdf_x_object_t *)piw->pres;

	pxo->width = pim->Width;
	pxo->height = h;
    }
    piw->height = h;
    piw->data = cos_stream_alloc(mem, "pdf_begin_image_data");
    if (piw->data == 0)
	return_error(gs_error_VMerror);
    start_pos = stell(s);
    if (!piw->pres)
	pputs(s, "BI\n");
    code = pdf_write_image_params(s, pdev, pim, &piw->binary, piw->pin,
				  &piw->Height_pos);
    if (code >= 0) {
	pprints1(s, "\n%s", piw->begin_data);
	piw->data_offset = stell(s) - start_pos;
	code = cos_stream_add_since(piw->data, pdev, start_pos);
    }
    if (code < 0) {
	COS_FREE(piw->data, mem, "pdf_begin_image_data");
	piw->data = 0;
    }
    return code;
}

/* Finish writing the binary data. */
private int
pdf_end_image_binary(gx_device_pdf *pdev, pdf_image_writer *piw, int data_h)
{
    long pos = stell(pdev->streams.strm);  /* piw->binary.target */
    int code;

    psdf_end_binary(&piw->binary);
    code = cos_stream_add_since(piw->data, pdev, pos);
    if (code < 0)
	return code;
    if (data_h != piw->height) {
	/*
	 * Overwrite the length in the stream file.  Make sure we add
	 * enough blanks to match the previous length.
	 */
	stream *ds = pdev->streams.strm;
	long save_pos = stell(ds);
	char hstr[sizeof(int) * 3];
	int hlen;

	sprintf(hstr, "%d", piw->height);
	hlen = strlen(hstr);
	sprintf(hstr, "%d", data_h);
	memset(hstr + strlen(hstr), ' ', hlen - strlen(hstr));
	sseek(ds, piw->Height_pos);
	pputs(ds, hstr);
	sseek(ds, save_pos);
    }
    piw->start_pos = pdf_stell(pdev);
    code = cos_stream_contents_write(piw->data, pdev);
    COS_FREE(piw->data, pdev->pdf_memory, "pdf_end_write_image");
    return code;
}

/* Finish writing an image. */
/* Return 0 if resource, 1 if in-line, or an error code. */
private int
pdf_end_write_image(gx_device_pdf * pdev, pdf_image_writer * piw)
{
    stream *s = pdev->strm;

    if (piw->pres) {		/* image resource */
	long length;

	pputs(s, "\n");
	length = pdf_stell(pdev) - piw->start_pos - piw->data_offset;
	pputs(s, "endstream\n");
	pdf_end_resource(pdev);
	pdf_open_separate(pdev, piw->length_id);
	s = pdev->strm;
	pprintld1(s, "%ld\n", length);
	pdf_end_separate(pdev);
	return 0;
    } else {			/* in-line image */
	pputs(s, "\nEI Q\n");
	return 1;
    }
}

/* Put out a reference to an image resource. */
private int
pdf_do_image(gx_device_pdf * pdev, const pdf_resource_t * pres,
	     const gs_matrix * pimat, bool in_contents)
{
    if (in_contents) {
	int code = pdf_open_contents(pdev, PDF_IN_STREAM);

	if (code < 0)
	    return code;
    }
    if (pimat) {
	/* Scale the matrix to account for short images. */
	const pdf_x_object_t *const pxo = (const pdf_x_object_t *)pres;
	double scale = (double)pxo->data_height / pxo->height;

	pdf_put_image_matrix(pdev, pimat, scale);
    }
    pprintld1(pdev->strm, "/R%ld Do\nQ\n", pdf_resource_id(pres));
    return 0;
}

/* ---------------- Driver procedures ---------------- */

/* ------ Low-level calls ------ */

/* Copy a monochrome bitmap or mask. */
private int
pdf_copy_mono(gx_device_pdf *pdev,
	      const byte *base, int sourcex, int raster, gx_bitmap_id id,
	      int x, int y, int w, int h, gx_color_index zero,
	      gx_color_index one, const gx_clip_path *pcpath)
{
    int code;
    gs_color_space cs;
    byte palette[sizeof(gx_color_index) * 2];
    gs_image_t image;
    int yi;
    pdf_image_writer writer;
    pdf_stream_position_t ipos;
    pdf_resource_t *pres = 0;
    byte invert = 0;
    long pos;

    /* Update clipping. */
    if (pdf_must_put_clip_path(pdev, pcpath)) {
	code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
	pdf_put_clip_path(pdev, pcpath);
    }
    /* We have 3 cases: mask, inverse mask, and solid. */
    if (zero == gx_no_color_index) {
	if (one == gx_no_color_index)
	    return 0;
	/* If a mask has an id, assume it's a character. */
	if (id != gx_no_bitmap_id && sourcex == 0) {
	    pdf_set_color(pdev, one, &pdev->fill_color,
			  &psdf_set_fill_color_commands);
	    pres = pdf_find_resource_by_gs_id(pdev, resourceCharProc, id);
	    if (pres == 0) {	/* Define the character in an embedded font. */
		pdf_char_proc_t *pcp;
		int y_offset;
		int max_y_offset =
		(pdev->open_font == 0 ? 0 :
		 pdev->open_font->max_y_offset);

		gs_image_t_init_mask(&image, false);
		invert = 0xff;
		pdf_make_bitmap_image(&image, x, y, w, h);
		y_offset =
		    image.ImageMatrix.ty - (int)(pdev->text.current.y + 0.5);
		if (x < pdev->text.current.x ||
		    y_offset < -max_y_offset || y_offset > max_y_offset
		    )
		    y_offset = 0;
		/*
		 * The Y axis of the text matrix is inverted,
		 * so we need to negate the Y offset appropriately.
		 */
		code = pdf_begin_char_proc(pdev, w, h, 0, y_offset, id,
					   &pcp, &ipos);
		if (code < 0)
		    return code;
		y_offset = -y_offset;
		pprintd3(pdev->strm, "0 0 0 %d %d %d d1\n", y_offset,
			 w, h + y_offset);
		pprintd3(pdev->strm, "%d 0 0 %d 0 %d cm\n", w, h,
			 y_offset);
		code = pdf_begin_write_image(pdev, &writer, gs_no_id, true);
		if (code < 0)
		    return code;
		pcp->rid = id;
		pres = (pdf_resource_t *) pcp;
		goto wr;
	    }
	    pdf_make_bitmap_matrix(&image.ImageMatrix, x, y, w, h, h);
	    goto rx;
	}
	pdf_set_color(pdev, one, &pdev->fill_color,
		      &psdf_set_fill_color_commands);
	gs_image_t_init_mask(&image, false);
	invert = 0xff;
    } else if (one == gx_no_color_index) {
	gs_image_t_init_mask(&image, false);
	pdf_set_color(pdev, zero, &pdev->fill_color,
		      &psdf_set_fill_color_commands);
    } else if (zero == pdev->black && one == pdev->white) {
	gs_cspace_init_DeviceGray(&cs);
	gs_image_t_init(&image, &cs);
    } else if (zero == pdev->white && one == pdev->black) {
	gs_cspace_init_DeviceGray(&cs);
	gs_image_t_init(&image, &cs);
	invert = 0xff;
    } else {
	/*
	 * We think this code is never executed when interpreting PostScript
	 * or PDF: the library never uses monobit non-mask images
	 * internally, and high-level images don't go through this code.
	 * However, we still want the code to work.
	 */
	gs_color_space cs_base;
	gx_color_index c[2];
	int i, j;
	int ncomp = pdev->color_info.num_components;
	byte *p;

	switch (ncomp) {
	case 1: gs_cspace_init_DeviceGray(&cs_base); break;
	case 3: gs_cspace_init_DeviceRGB(&cs_base); break;
	case 4: gs_cspace_init_DeviceCMYK(&cs_base); break;
	default: return_error(gs_error_rangecheck);
	}
	c[0] = psdf_adjust_color_index((gx_device_vector *)pdev, zero);
	c[1] = psdf_adjust_color_index((gx_device_vector *)pdev, one);
	gs_cspace_init(&cs, &gs_color_space_type_Indexed, NULL);
	cs.params.indexed.base_space = *(gs_direct_color_space *)&cs_base;
	cs.params.indexed.hival = 1;
	p = palette;
	for (i = 0; i < 2; ++i)
	    for (j = ncomp - 1; j >= 0; --j)
		*p++ = (byte)(c[i] >> (j * 8));
	cs.params.indexed.lookup.table.data = palette;
	cs.params.indexed.lookup.table.size = p - palette;
	cs.params.indexed.use_proc = false;
	gs_image_t_init(&image, &cs);
	image.BitsPerComponent = 1;
    }
    pdf_make_bitmap_image(&image, x, y, w, h);
    {
	ulong nbytes = (ulong) ((w + 7) >> 3) * h;
	bool in_line = nbytes <= MAX_INLINE_IMAGE_BYTES;

	if (in_line)
	    pdf_put_image_matrix(pdev, &image.ImageMatrix, 1.0);
	code = pdf_open_page(pdev, PDF_IN_STREAM);
	if (code < 0)
	    return code;
	code = pdf_begin_write_image(pdev, &writer, gs_no_id, in_line);
	if (code < 0)
	    return code;
    }
  wr:
    /*
     * There are 3 different cases at this point:
     *      - Writing an in-line image (pres == 0, writer.pres == 0);
     *      - Writing an XObject image (pres == 0, writer.pres != 0);
     *      - Writing the image for a CharProc (pres != 0).
     * We handle them with in-line code followed by a switch,
     * rather than making the shared code into a procedure,
     * simply because there would be an awful lot of parameters
     * that would need to be passed.
     */
    pdf_begin_image_binary(pdev, &writer);
    if (pres) {
	/* Always use CCITTFax 2-D for character bitmaps. */
	psdf_CFE_binary(&writer.binary, image.Width, image.Height, false);
    } else {
	/* Use the Distiller compression parameters. */
	psdf_setup_image_filters((gx_device_psdf *) pdev, &writer.binary,
				 &image, NULL, NULL);
    }
    pdf_begin_image_data(pdev, &writer, &image, h);
    pos = stell(pdev->streams.strm);
    for (yi = 0; yi < h; ++yi) {
	const byte *data = base + yi * raster + (sourcex >> 3);
	int sbit = sourcex & 7;

	if (sbit == 0) {
	    int nbytes = (w + 7) >> 3;
	    int i;

	    for (i = 0; i < nbytes; ++data, ++i)
		sputc(writer.binary.strm, *data ^ invert);
	} else {
	    int wleft = w;
	    int rbit = 8 - sbit;

	    for (; wleft + sbit > 8; ++data, wleft -= 8)
		sputc(writer.binary.strm,
		      ((*data << sbit) + (data[1] >> rbit)) ^ invert);
	    if (wleft > 0)
		sputc(writer.binary.strm,
		      ((*data << sbit) ^ invert) &
		      (byte) (0xff00 >> wleft));
	}
    }
    code = cos_stream_add_since(writer.data, pdev, pos);
    pdf_end_image_binary(pdev, &writer, writer.height);
    if (!pres) {
	switch (pdf_end_write_image(pdev, &writer)) {
	    default:		/* error */
		return code;
	    case 1:
		return 0;
	    case 0:
		return pdf_do_image(pdev, writer.pres, &image.ImageMatrix,
				    true);
	}
    }
    pputs(pdev->strm, "\nEI\n");
    code = pdf_end_char_proc(pdev, &ipos);
    if (code < 0)
	return code;
  rx:{
	gs_matrix imat;

	imat = image.ImageMatrix;
	imat.xx /= w;
	imat.xy /= h;
	imat.yx /= w;
	imat.yy /= h;
	return pdf_do_char_image(pdev, (const pdf_char_proc_t *)pres, &imat);
    }
}
int
gdev_pdf_copy_mono(gx_device * dev,
		   const byte * base, int sourcex, int raster, gx_bitmap_id id,
		   int x, int y, int w, int h, gx_color_index zero,
		   gx_color_index one)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;

    if (w <= 0 || h <= 0)
	return 0;
    return pdf_copy_mono(pdev, base, sourcex, raster, id, x, y, w, h,
			 zero, one, NULL);
}

/* Copy a color bitmap.  for_pattern = -1 means put the image in-line, */
/* 1 means put the image in a resource. */
private int
pdf_copy_color_data(gx_device_pdf * pdev, const byte * base, int sourcex,
		    int raster, gx_bitmap_id id, int x, int y, int w, int h,
		    gs_image_t *pim, pdf_image_writer *piw,
		    int for_pattern)
{
    int depth = pdev->color_info.depth;
    int bytes_per_pixel = depth >> 3;
    int yi;
    gs_color_space cs;
    ulong nbytes;
    int code;
    const byte *row_base;
    int row_step;
    long pos;
    bool in_line;

    switch(bytes_per_pixel) {
    case 3: gs_cspace_init_DeviceRGB(&cs); break;
    case 4: gs_cspace_init_DeviceCMYK(&cs); break;
    default: gs_cspace_init_DeviceGray(&cs);
    }
    gs_image_t_init(pim, &cs);
    pdf_make_bitmap_image(pim, x, y, w, h);
    pim->BitsPerComponent = 8;
    nbytes = (ulong)w * bytes_per_pixel * h;

    if (for_pattern) {
	/*
	 * Patterns must be emitted in order of increasing user Y, i.e.,
	 * the opposite of PDF's standard image order.
	 */
	row_base = base + (h - 1) * raster;
	row_step = -raster;
	in_line = for_pattern < 0;
    } else {
	row_base = base;
	row_step = raster;
	in_line = nbytes <= MAX_INLINE_IMAGE_BYTES;
	pdf_put_image_matrix(pdev, &pim->ImageMatrix, 1.0);
	/*
	 * Check whether we've already made an XObject resource for this
	 * image.
	 */
	if (id != gx_no_bitmap_id) {
	    piw->pres = pdf_find_resource_by_gs_id(pdev, resourceXObject, id);
	    if (piw->pres)
		return 0;
	}
    }
    /*
     * We have to be able to control whether to put Pattern images in line,
     * to avoid trying to create an XObject resource while we're in the
     * middle of writing a Pattern resource.
     */
    if (for_pattern < 0)
	pputs(pdev->strm, "q ");
    code = pdf_begin_write_image(pdev, piw, id, in_line);
    if (code < 0)
	return code;
    pdf_begin_image_binary(pdev, piw);
    code = psdf_setup_image_filters((gx_device_psdf *) pdev,
				    &piw->binary, pim, NULL, NULL);
    if (code < 0)
	return code;
    code = pdf_begin_image_data(pdev, piw, pim, h);
    if (code < 0)
	return code;
    pos = stell(pdev->streams.strm);
    for (yi = 0; yi < h; ++yi) {
	uint ignore;

	sputs(piw->binary.strm,
	      row_base + sourcex * bytes_per_pixel + yi * row_step,
	      w * bytes_per_pixel, &ignore);
    }
    cos_stream_add_since(piw->data, pdev, pos);
    pdf_end_image_binary(pdev, piw, piw->height);
    return pdf_end_write_image(pdev, piw);
}

int
gdev_pdf_copy_color(gx_device * dev, const byte * base, int sourcex,
		    int raster, gx_bitmap_id id, int x, int y, int w, int h)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    gs_image_t image;
    pdf_image_writer writer;
    int code;
    
    if (w <= 0 || h <= 0)
	return 0;
    code = pdf_open_page(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    /* Make sure we aren't being clipped. */
    pdf_put_clip_path(pdev, NULL);
    code = pdf_copy_color_data(pdev, base, sourcex, raster, id, x, y, w, h,
			       &image, &writer, 0);
    switch (code) {
	default:
	    return code;	/* error */
	case 1:
	    return 0;
	case 0:
	    return pdf_do_image(pdev, writer.pres, NULL, true);
    }
}

/* Fill a mask. */
int
gdev_pdf_fill_mask(gx_device * dev,
		 const byte * data, int data_x, int raster, gx_bitmap_id id,
		   int x, int y, int width, int height,
		   const gx_drawing_color * pdcolor, int depth,
		   gs_logical_operation_t lop, const gx_clip_path * pcpath)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;

    if (width <= 0 || height <= 0)
	return 0;
    if (depth > 1 || !gx_dc_is_pure(pdcolor) != 0)
	return gx_default_fill_mask(dev, data, data_x, raster, id,
				    x, y, width, height, pdcolor, depth, lop,
				    pcpath);
    return pdf_copy_mono(pdev, data, data_x, raster, id, x, y, width, height,
			 gx_no_color_index, gx_dc_pure_color(pdcolor),
			 pcpath);
}

/* Tile with a bitmap.  This is important for pattern fills. */
int
gdev_pdf_strip_tile_rectangle(gx_device * dev, const gx_strip_bitmap * tiles,
			      int x, int y, int w, int h,
			      gx_color_index color0, gx_color_index color1,
			      int px, int py)
{
    gx_device_pdf *const pdev = (gx_device_pdf *) dev;
    int tw = tiles->rep_width, th = tiles->rep_height;
    double xscale = pdev->HWResolution[0] / 72.0,
	yscale = pdev->HWResolution[1] / 72.0;
    pdf_resource_t *pres;

    if (tiles->id == gx_no_bitmap_id || tiles->shift != 0 ||
	(w < tw && h < th) ||
	/* We only handle full colored patterns right now. */
	color0 != gx_no_color_index || color1 != gx_no_color_index ||
	/* Pattern fills are only available starting in PDF 1.2. */
	pdev->CompatibilityLevel < 1.2)
	goto use_default;
    if (!pdev->cs_Pattern) {
	/* Create a single Pattern color space resource named Pattern. */
	int code = pdf_begin_resource_body(pdev, resourceColorSpace, gs_no_id,
					   &pdev->cs_Pattern);

	if (code < 0)
	    goto use_default;
	pputs(pdev->strm, "[/Pattern]\n");
	pdf_end_resource(pdev);
    }
    pres = pdf_find_resource_by_gs_id(pdev, resourcePattern, tiles->id);
    if (!pres) {
	/* Create the Pattern resource. */
	int code;
	long image_id, length_id, start, end;
	stream *s;
	gs_image_t image;
	pdf_image_writer writer;
	long image_bytes = (long)tw * th * pdev->color_info.depth;
	bool in_line = image_bytes <= MAX_INLINE_IMAGE_BYTES;
	ulong tile_id =
	    (tw == tiles->size.x && th == tiles->size.y ? tiles->id :
	     gx_no_bitmap_id);

	if (in_line)
	    image_id = 0;
	else if (image_bytes > 65500) {
	    /*
	     * Acrobat Reader can't handle image Patterns with more than
	     * 64K of data.  :-(
	     */
	    goto use_default;
	} else {
	    /* Write out the image as an XObject resource now. */
	    code = pdf_copy_color_data(pdev, tiles->data, 0, tiles->raster,
				       tile_id, 0, 0, tw, th, &image, &writer,
				       1);
	    if (code < 0)
		goto use_default;
	    image_id = pdf_resource_id(writer.pres);
	}
	code = pdf_begin_resource(pdev, resourcePattern, tiles->id, &pres);
	if (code < 0)
	    goto use_default;
	s = pdev->strm;
	length_id = pdf_obj_ref(pdev);
	pputs(s, "/PatternType 1/PaintType 1/TilingType 1/Resources<<\n");
	if (image_id)
	    pprintld2(s, "/XObject<</R%ld %ld 0 R>>", image_id, image_id);
	pputs(s, "/ProcSet[/PDF/ImageC]>>\n");
	/*
	 * Because of bugs in Acrobat Reader's Print function, we can't use
	 * the natural BBox and Step here: they have to be 1.
	 */
	pprintg2(s, "/Matrix[%g 0 0 %g 0 0]", tw / xscale, th / yscale);
	pprintld1(s, "/BBox[0 0 1 1]/XStep 1/YStep 1/Length %ld 0 R>>stream\n",
		  length_id);
	start = pdf_stell(pdev);
	if (image_id)
	    pprintld1(s, "/R%ld Do\n", image_id);
	else {
	    code = pdf_copy_color_data(pdev, tiles->data, 0, tiles->raster,
				       tile_id, 0, 0, tw, th, &image, &writer,
				       -1);
	    switch (code) {
	    default:
		return code;	/* error */
	    case 1:
		break;
	    case 0:			/* not possible */
		return_error(gs_error_Fatal);
	    }
	}
	end = pdf_stell(pdev);
	pputs(s, "endstream\n");
	pdf_end_resource(pdev);
	pdf_open_separate(pdev, length_id);
	pprintld1(pdev->strm, "%ld\n", end - start);
	pdf_end_separate(pdev);
    }
    /* Fill the rectangle with the Pattern. */
    {
	int code = pdf_open_page(pdev, PDF_IN_STREAM);
	stream *s;

	if (code < 0)
	    goto use_default;
	/* Make sure we aren't being clipped. */
	pdf_put_clip_path(pdev, NULL);
	s = pdev->strm;
	/*
	 * Because of bugs in Acrobat Reader's Print function, we can't
	 * leave the CTM alone here: we have to reset it to the default.
	 */
	pprintg2(s, "q %g 0 0 %g 0 0 cm", xscale, yscale);
	pprintld2(s, "/R%ld cs/R%ld scn ", pdf_resource_id(pdev->cs_Pattern),
		  pdf_resource_id(pres));
	pprintg4(s, "%g %g %g %g re f Q\n",
		 x / xscale, y / yscale, w / xscale, h / xscale);
    }
    return 0;
use_default:
    return gx_default_strip_tile_rectangle(dev, tiles, x, y, w, h,
					   color0, color1, px, py);
}

/* ------ High-level calls ------ */

/* Define the structure for keeping track of progress through an image. */
typedef struct pdf_image_enum_s {
    gx_image_enum_common;
    gs_memory_t *memory;
    int width;
    int bits_per_pixel;		/* bits per pixel (per plane) */
    int rows_left;
    pdf_image_writer writer;
    gs_matrix mat;
} pdf_image_enum;
gs_private_st_composite(st_pdf_image_enum, pdf_image_enum, "pdf_image_enum",
  pdf_image_enum_enum_ptrs, pdf_image_enum_reloc_ptrs);
/* GC procedures */
private ENUM_PTRS_WITH(pdf_image_enum_enum_ptrs, pdf_image_enum *pie)
    if (index < psdf_binary_writer_max_ptrs) {
	gs_ptr_type_t ret =
	    ENUM_USING(st_pdf_image_writer, &pie->writer, sizeof(pie->writer),
		       index);

	if (ret == 0)		/* don't stop early */
	    ENUM_RETURN(0);
	return ret;
    }
    return ENUM_USING_PREFIX(st_gx_image_enum_common,
			     psdf_binary_writer_max_ptrs);
ENUM_PTRS_END
private RELOC_PTRS_WITH(pdf_image_enum_reloc_ptrs, pdf_image_enum *pie)
{
    RELOC_USING(st_pdf_image_writer, &pie->writer, sizeof(pie->writer));
    RELOC_USING(st_gx_image_enum_common, vptr, size);
}
RELOC_PTRS_END

/* Test whether we can handle a given color space. */
private bool
pdf_can_handle_color_space(const gs_color_space * pcs)
{
    gs_color_space_index index = gs_color_space_get_index(pcs);

    if (index == gs_color_space_index_Indexed) {
	if (pcs->params.indexed.use_proc)
	    return false;
	index =
	    gs_color_space_get_index(gs_color_space_indexed_base_space(pcs));
    }
    switch (index) {
	case gs_color_space_index_DeviceGray:
	case gs_color_space_index_DeviceRGB:
	case gs_color_space_index_DeviceCMYK:
	    return true;
	case gs_color_space_index_Separation:
	case gs_color_space_index_Pattern:
	    return false;
/****** OK in PDF 1.2 ******/
	case gs_color_space_index_CIEA:
	    {			/* Check that we can represent this as a CalGray space. */
		const gs_cie_a *pcie = pcs->params.a;
		float expts[3];

		return (pcie->MatrixA.u == 1 && pcie->MatrixA.v == 1 &&
			pcie->MatrixA.w == 1 &&
			pcie->common.MatrixLMN.is_identity &&
			((cie_cache_is_identity(&pcie->caches.DecodeA) &&
			  cie_scalar3_cache_is_exponential(pcie->common.caches.DecodeLMN, expts) &&
			  expts[1] == expts[0] && expts[2] == expts[0]) ||
			 (cie_vector_cache_is_exponential(&pcie->caches.DecodeA, &expts[0]) &&
		     cie_cache3_is_identity(pcie->common.caches.DecodeLMN)))
		    );
	    }
	case gs_color_space_index_CIEABC:
	    {			/* Check that we can represent this as a CalRGB space. */
		const gs_cie_abc *pcie = pcs->params.abc;
		float expts[3];

		return ((cie_cache3_is_identity(pcie->caches.DecodeABC) &&
			 pcie->MatrixABC.is_identity &&
			 cie_scalar3_cache_is_exponential(pcie->common.caches.DecodeLMN, expts)) ||
			(cie_vector3_cache_is_exponential(pcie->caches.DecodeABC, expts) &&
		    cie_cache3_is_identity(pcie->common.caches.DecodeLMN) &&
			 pcie->common.MatrixLMN.is_identity)
		    );
	    }
	default:		/* CIEBasedDEF[G], LL3 spaces */
	    return false;
    }
}

/* Start processing an image. */
int
gdev_pdf_begin_image(gx_device * dev,
		     const gs_imager_state * pis, const gs_image_t * pim,
		     gs_image_format_t format, const gs_int_rect * prect,
	      const gx_drawing_color * pdcolor, const gx_clip_path * pcpath,
		     gs_memory_t * mem, gx_image_enum_common_t ** pinfo)
{
    gx_device_pdf *pdev = (gx_device_pdf *) dev;
    int code;
    pdf_image_enum *pie;
    const gs_color_space *pcs = pim->ColorSpace;
    int num_components =
	(pim->ImageMask ? 1 : gs_color_space_num_components(pcs));
    gs_int_rect rect;
    gs_image_t image;		/* we may change some components */
    ulong nbytes;
    int height;

    switch (format) {
    case gs_image_format_chunky:
    case gs_image_format_component_planar:
	break;
    default:
	return gx_default_begin_image(dev, pis, pim, format, prect,
				      pdcolor, pcpath, mem, pinfo);
    }
    code = pdf_open_page(pdev, PDF_IN_STREAM);
    if (code < 0)
	return code;
    if (prect)
	rect = *prect;
    else {
	rect.p.x = rect.p.y = 0;
	rect.q.x = pim->Width, rect.q.y = pim->Height;
    }
    image = *pim;
#define pim (&image)
    pie = gs_alloc_struct(mem, pdf_image_enum, &st_pdf_image_enum,
			  "pdf_begin_image");
    if (pie == 0)
	return_error(gs_error_VMerror);
    *pinfo = (gx_image_enum_common_t *) pie;
    gx_image_enum_common_init(*pinfo, (gs_data_image_t *) pim,
			      &pdf_image_enum_procs, dev,
			      num_components, format);
    pie->memory = mem;
    if ((pim->ImageMask ?
	 (!gx_dc_is_pure(pdcolor) || pim->CombineWithColor) :
	 !pdf_can_handle_color_space(pim->ColorSpace)) ||
	prect != 0
	) {
	gs_free_object(mem, pie, "pdf_begin_image");
	return gx_default_begin_image(dev, pis, pim, format, prect,
				      pdcolor, pcpath, mem, pinfo);
    }
    pie->width = rect.q.x - rect.p.x;
    height = rect.q.y - rect.p.y;
    pie->bits_per_pixel =
	pim->BitsPerComponent * num_components / pie->num_planes;
    pie->rows_left = rect.q.y - rect.p.y;
    pdf_put_clip_path(pdev, pcpath);
    if (pim->ImageMask)
	pdf_set_color(pdev, gx_dc_pure_color(pdcolor), &pdev->fill_color,
		      &psdf_set_fill_color_commands);
    {
	gs_matrix mat;
	gs_matrix bmat;
	int code;

	pdf_make_bitmap_matrix(&bmat, -rect.p.x, -rect.p.y,
			       pim->Width, pim->Height, height);
	if ((code = gs_matrix_invert(&pim->ImageMatrix, &mat)) < 0 ||
	    (code = gs_matrix_multiply(&bmat, &mat, &mat)) < 0 ||
	    (code = gs_matrix_multiply(&mat, &ctm_only(pis), &pie->mat)) < 0
	    ) {
	    gs_free_object(mem, pie, "pdf_begin_image");
	    return code;
	}
    }
    nbytes = (((ulong) pie->width * pie->bits_per_pixel + 7) >> 3) *
	pie->num_planes * pie->rows_left;
    code = pdf_begin_write_image(pdev, &pie->writer, gs_no_id,
				 nbytes <= MAX_INLINE_IMAGE_BYTES);
    if (code < 0)
	return code;
    pdf_begin_image_binary(pdev, &pie->writer);
/****** pctm IS WRONG ******/
    code = psdf_setup_image_filters((gx_device_psdf *) pdev,
				    &pie->writer.binary, pim,
				    &ctm_only(pis), pis);
    if (code < 0)
	return code;
    code = pdf_begin_image_data(pdev, &pie->writer, pim, height);
    if (code < 0)
	return code;
    return 0;
#undef pim
}

/* Process the next piece of an image. */
private int
pdf_image_plane_data(gx_image_enum_common_t * info,
		     const gx_image_plane_t * planes, int height,
		     int *rows_used)
{
    gx_device_pdf *pdev = (gx_device_pdf *)info->dev;
    pdf_image_enum *pie = (pdf_image_enum *) info;
    int h = height;
    int y;
    /****** DOESN'T HANDLE IMAGES WITH VARYING WIDTH PER PLANE ******/
    uint width_bits = pie->width * pie->plane_depths[0];
    /****** DOESN'T HANDLE NON-ZERO data_x CORRECTLY ******/
    uint bcount = (width_bits + 7) >> 3;
    uint ignore;
    int nplanes = pie->num_planes;
    stream *s = pdev->streams.strm;
    long pos = stell(s);
    int code;

    if (h > pie->rows_left)
	h = pie->rows_left;
    pie->rows_left -= h;
    for (y = 0; y < h; ++y) {
	if (nplanes > 1) {
	    /*
	     * We flip images in blocks, and each block except the last one
	     * must contain an integral number of pixels.  The easiest way
	     * to meet this condition is for all blocks except the last to
	     * be a multiple of 3 source bytes (guaranteeing an integral
	     * number of 1/2/4/8/12-bit samples), i.e., 3*nplanes flipped
	     * bytes.  This requires a buffer of at least
	     * 3*GS_IMAGE_MAX_COMPONENTS bytes.
	     */
	    int pi;
	    uint count = bcount;
	    uint offset = 0;
#define ROW_BYTES max(200 /*arbitrary*/, 3 * GS_IMAGE_MAX_COMPONENTS)
	    const byte *bit_planes[GS_IMAGE_MAX_COMPONENTS];
	    int block_bytes = ROW_BYTES / (3 * nplanes) * 3;
	    byte row[ROW_BYTES];

	    for (pi = 0; pi < nplanes; ++pi)
		bit_planes[pi] = planes[pi].data + planes[pi].raster * y;
	    while (count) {
		uint flip_count;
		uint flipped_count;

		if (count >= block_bytes) {
		    flip_count = block_bytes;
		    flipped_count = block_bytes * nplanes;
		} else {
		    flip_count = count;
		    flipped_count =
			(width_bits % (block_bytes * 8) * nplanes + 7) >> 3;
		}
		image_flip_planes(row, bit_planes, offset, flip_count,
				  nplanes, pie->plane_depths[0]);
		sputs(pie->writer.binary.strm, row, flipped_count, &ignore);
		offset += flip_count;
		count -= flip_count;
	    }
	} else {
	    sputs(pie->writer.binary.strm,
		  planes[0].data + planes[0].raster * y, bcount, &ignore);
	}
    }
    *rows_used = h;
    code = cos_stream_add_since(pie->writer.data, pdev, pos);
    return (code < 0 ? code : !pie->rows_left);
#undef ROW_BYTES
}

/* Clean up by releasing the buffers. */
private int
pdf_image_end_image(gx_image_enum_common_t * info, bool draw_last)
{
    gx_device_pdf *pdev = (gx_device_pdf *)info->dev;
    pdf_image_enum *pie = (pdf_image_enum *)info;
    int height = pie->writer.height;
    int data_height = height - pie->rows_left;
    int code;

    if (pie->writer.pres)
	((pdf_x_object_t *)pie->writer.pres)->data_height = data_height;
    else
	pdf_put_image_matrix(pdev, &pie->mat,
			     (height == 0 || data_height == 0 ? 1.0 :
			      (double)data_height / height));
    code = pdf_end_image_binary(pdev, &pie->writer, data_height);
    if (code < 0)
	return code;
    code = pdf_end_write_image(pdev, &pie->writer);
    switch (code) {
    default:
	return code;	/* error */
    case 1:
	return 0;
    case 0:;
    }
    code = pdf_do_image(pdev, pie->writer.pres, &pie->mat, true);
    gs_free_object(pie->memory, pie, "pdf_end_image");
    return code;
}
