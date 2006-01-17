/* Copyright (C) 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpdtw.h,v 1.6 2005/04/04 08:53:07 igor Exp $ */
/* Font and CMap resource writing API for pdfwrite */

#ifndef gdevpdtw_INCLUDED
#  define gdevpdtw_INCLUDED

/*
 * The procedures declared here are called only from gdevpdtf.c: they are
 * not intended to be called from anywhere else.
 */

/* ---------------- Font resource writing ---------------- */

/*
 * Each of these procedures is referenced only from a single place in
 * gdevpdtf.c.  Their prototype and functionality must match the definition
 * of pdf_font_write_contents_proc_t in gdevpdtf.h.
 */
int
  pdf_write_contents_type0(gx_device_pdf *pdev, pdf_font_resource_t *pdfont),
  pdf_finish_write_contents_type3(gx_device_pdf *pdev,
				  pdf_font_resource_t *pdfont),
  pdf_write_contents_std(gx_device_pdf *pdev, pdf_font_resource_t *pdfont),
  pdf_write_contents_simple(gx_device_pdf *pdev, pdf_font_resource_t *pdfont),
  pdf_write_contents_cid0(gx_device_pdf *pdev, pdf_font_resource_t *pdfont),
  pdf_write_contents_cid2(gx_device_pdf *pdev, pdf_font_resource_t *pdfont),
  pdf_different_encoding_index(const pdf_font_resource_t *pdfont, int ch0),
  pdf_write_encoding(gx_device_pdf *pdev, const pdf_font_resource_t *pdfont, long id, int ch),
  pdf_write_encoding_ref(gx_device_pdf *pdev, const pdf_font_resource_t *pdfont, long id);


/* ---------------- CMap resource writing ---------------- */

#ifndef gs_cid_system_info_DEFINED
#  define gs_cid_system_info_DEFINED
typedef struct gs_cid_system_info_s gs_cid_system_info_t;
#endif
#ifndef gs_cmap_DEFINED
#  define gs_cmap_DEFINED
typedef struct gs_cmap_s gs_cmap_t;
#endif

/*
 * Write the CIDSystemInfo for a CIDFont or a CMap.
 */
int pdf_write_cid_system_info(gx_device_pdf *pdev,
			      const gs_cid_system_info_t *pcidsi, gs_id object_id);

/*
 * Write a CMap resource.  We pass the CMap object as well as the resource,
 * because we write CMaps when they are created.
 */
int pdf_write_cmap(gx_device_pdf *pdev, const gs_cmap_t *pcmap,
		   pdf_resource_t **ppres, int font_index_only);

#endif /* gdevpdtw_INCLUDED */
