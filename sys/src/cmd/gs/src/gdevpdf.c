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

/*$Id: gdevpdf.c,v 1.2 2000/03/16 01:21:24 lpd Exp $ */
/* PDF-writing driver */
#include "memory_.h"
#include "string_.h"
#include "gx.h"
#include "gserrors.h"
#include "gscdefs.h"
#include "gxdevice.h"
#include "gdevpdfx.h"
#include "gdevpdff.h"
#include "gdevpdfo.h"

/* Define the default language level and PDF compatibility level. */
/* Acrobat 4 (PDF 1.3) is the default. */
#define PSDF_VERSION_INITIAL psdf_version_ll3
#define PDF_COMPATIBILITY_LEVEL_INITIAL 1.3

/* Define the names of the resource types. */
private const char *const resource_type_names[] = {
    pdf_resource_type_names
};

/* Define the size of internal stream buffers. */
/* (This is not a limitation, it only affects performance.) */
#define sbuf_size 512

/* GC descriptors */
private_st_pdf_page();
gs_private_st_element(st_pdf_page_element, pdf_page_t, "pdf_page_t[]",
		      pdf_page_elt_enum_ptrs, pdf_page_elt_reloc_ptrs,
		      st_pdf_page);
private_st_device_pdfwrite();

/* GC procedures */
private 
ENUM_PTRS_WITH(device_pdfwrite_enum_ptrs, gx_device_pdf *pdev)
{
    index -= gx_device_pdf_num_ptrs + gx_device_pdf_num_strings;
    if (index < PDF_NUM_STD_FONTS)
	ENUM_RETURN(pdev->std_fonts[index].font);
    index -= PDF_NUM_STD_FONTS;
    if (index < NUM_RESOURCE_TYPES * NUM_RESOURCE_CHAINS)
	ENUM_RETURN(pdev->resources[index / NUM_RESOURCE_CHAINS].chains[index % NUM_RESOURCE_CHAINS]);
    index -= NUM_RESOURCE_TYPES * NUM_RESOURCE_CHAINS;
    if (index <= pdev->outline_depth)
	ENUM_RETURN(pdev->outline_levels[index].first.action);
    index -= pdev->outline_depth + 1;
    if (index <= pdev->outline_depth)
	ENUM_RETURN(pdev->outline_levels[index].last.action);
    index -= pdev->outline_depth + 1;
    ENUM_PREFIX(st_device_psdf, 0);
}
#define e1(i,elt) ENUM_PTR(i, gx_device_pdf, elt);
gx_device_pdf_do_ptrs(e1)
#undef e1
#define e1(i,elt) ENUM_STRING_PTR(i + gx_device_pdf_num_ptrs, gx_device_pdf, elt);
gx_device_pdf_do_strings(e1)
#undef e1
ENUM_PTRS_END
private RELOC_PTRS_WITH(device_pdfwrite_reloc_ptrs, gx_device_pdf *pdev)
{
    RELOC_PREFIX(st_device_psdf);
#define r1(i,elt) RELOC_PTR(gx_device_pdf,elt);
    gx_device_pdf_do_ptrs(r1)
#undef r1
#define r1(i,elt) RELOC_STRING_PTR(gx_device_pdf,elt);
	gx_device_pdf_do_strings(r1)
#undef r1
    {
	int i, j;

	for (i = 0; i < PDF_NUM_STD_FONTS; ++i)
	    RELOC_PTR(gx_device_pdf, std_fonts[i].font);
	for (i = 0; i < NUM_RESOURCE_TYPES; ++i)
	    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j)
		RELOC_PTR(gx_device_pdf, resources[i].chains[j]);
	for (i = 0; i <= pdev->outline_depth; ++i) {
	    RELOC_PTR(gx_device_pdf, outline_levels[i].first.action);
	    RELOC_PTR(gx_device_pdf, outline_levels[i].last.action);
	}
    }
}
RELOC_PTRS_END
/* Even though device_pdfwrite_finalize is the same as gx_device_finalize, */
/* we need to implement it separately because st_composite_final */
/* declares all 3 procedures as private. */
private void
device_pdfwrite_finalize(void *vpdev)
{
    gx_device_finalize(vpdev);
}

/* Driver procedures */
private dev_proc_open_device(pdf_open);
private dev_proc_output_page(pdf_output_page);
private dev_proc_close_device(pdf_close);
/* Driver procedures defined in other files are declared in gdevpdfx.h. */

#ifndef X_DPI
#  define X_DPI 720
#endif
#ifndef Y_DPI
#  define Y_DPI 720
#endif

const gx_device_pdf gs_pdfwrite_device =
{std_device_dci_type_body(gx_device_pdf, 0, "pdfwrite",
			  &st_device_pdfwrite,
			  DEFAULT_WIDTH_10THS * X_DPI / 10,
			  DEFAULT_HEIGHT_10THS * Y_DPI / 10,
			  X_DPI, Y_DPI,
			  3, 24, 255, 255, 256, 256),
 {pdf_open,
  gx_upright_get_initial_matrix,
  NULL,				/* sync_output */
  pdf_output_page,
  pdf_close,
  gx_default_rgb_map_rgb_color,
  gx_default_rgb_map_color_rgb,
  gdev_pdf_fill_rectangle,
  NULL,				/* tile_rectangle */
  gdev_pdf_copy_mono,
  gdev_pdf_copy_color,
  NULL,				/* draw_line */
  NULL,				/* get_bits */
  gdev_pdf_get_params,
  gdev_pdf_put_params,
  NULL,				/* map_cmyk_color */
  NULL,				/* get_xfont_procs */
  NULL,				/* get_xfont_device */
  NULL,				/* map_rgb_alpha_color */
  gx_page_device_get_page_device,
  NULL,				/* get_alpha_bits */
  NULL,				/* copy_alpha */
  NULL,				/* get_band */
  NULL,				/* copy_rop */
  gdev_pdf_fill_path,
  gdev_pdf_stroke_path,
  gdev_pdf_fill_mask,
  NULL,				/* fill_trapezoid */
  NULL,				/* fill_parallelogram */
  NULL,				/* fill_triangle */
  NULL,				/* draw_thin_line */
  gdev_pdf_begin_image,
  NULL,				/* image_data */
  NULL,				/* end_image */
  gdev_pdf_strip_tile_rectangle,
  NULL,				/* strip_copy_rop */
  NULL,				/* get_clipping_box */
  NULL,				/* begin_typed_image */
  NULL,				/* get_bits_rectangle */
  NULL,				/* map_color_rgb_alpha */
  NULL,				/* create_compositor */
  NULL,				/* get_hardware_params */
  gdev_pdf_text_begin
 },
 psdf_initial_values(PSDF_VERSION_INITIAL, 0 /*false */ ),  /* (!ASCII85EncodePages) */
 PDF_COMPATIBILITY_LEVEL_INITIAL,  /* CompatibilityLevel */
#ifdef POST60
 0 /*false*/,			/* Optimize */
 0 /*false*/,			/* ParseDSCCommentsForDocInfo */
 0 /*false*/,			/* ParseDSCComments */
 0 /*false*/,			/* EmitDSCWarnings */
 0 /*false*/,			/* CreateJobTicket */
 0 /*false*/,			/* PreserveEPSInfo */
 0 /*false*/,			/* AutoPositionEPSFile */
 0 /*false*/,			/* PreserveCopyPage */
 0 /*false*/,			/* UsePrologue */
#endif
 1 /*true */ ,			/* ReAssignCharacters */
 1 /*true */ ,			/* ReEncodeCharacters */
 1,				/* FirstObjectNumber */
 pdf_compress_none,		/* compression */
 {{0}},				/* xref */
 {{0}},				/* asides */
 {{0}},				/* streams */
 {{0}},				/* pictures */
 0,				/* open_font */
 {0},				/* open_font_name */
 0,				/* embedded_encoding_id */
 0,				/* next_id */
 0,				/* Catalog */
 0,				/* Info */
 0,				/* Pages */
 0,				/* outlines_id */
 0,				/* next_page */
 0,				/* contents_id */
 PDF_IN_NONE,			/* context */
 0,				/* contents_length_id */
 0,				/* contents_pos */
 NoMarks,			/* procsets */
 {pdf_text_state_default},	/* text */
 {{0}},				/* std_fonts */
 {0},				/* space_char_ids */
 0,				/* pages */
 0,				/* num_pages */
 {
     {
	 {0}}},			/* resources */
 0,				/* cs_Pattern */
 0,				/* last_resource */
 {
     {
	 {0}}},			/* outline_levels */
 0,				/* outline_depth */
 0,				/* closed_outline_depth */
 0,				/* outlines_open */
 0,				/* articles */
 0,				/* Dests */
 0,				/* named_objects */
 0				/* open_graphics */
};

/* ---------------- Device open/close ---------------- */

/* Close and remove temporary files. */
private int
pdf_close_temp_file(gx_device_pdf *pdev, pdf_temp_file_t *ptf, int code)
{
    int err = 0;
    FILE *file = ptf->file;

    /*
     * ptf->strm == 0 or ptf->file == 0 is only possible if this procedure
     * is called to clean up during initialization failure, but ptf->strm
     * might not be open if it was finalized before the device was closed.
     */
    if (ptf->strm) {
	if (s_is_valid(ptf->strm)) {
	    sflush(ptf->strm);
	    /* Prevent freeing the stream from closing the file. */
	    ptf->strm->file = 0;
	} else
	    ptf->file = file = 0;	/* file was closed by finalization */
	gs_free_object(pdev->pdf_memory, ptf->strm_buf,
		       "pdf_close_temp_file(strm_buf)");
	ptf->strm_buf = 0;
	gs_free_object(pdev->pdf_memory, ptf->strm,
		       "pdf_close_temp_file(strm)");
	ptf->strm = 0;
    }
    if (file) {
	err = ferror(file) | fclose(file);
	unlink(ptf->file_name);
	ptf->file = 0;
    }
    ptf->save_strm = 0;
    return
	(code < 0 ? code : err != 0 ? gs_note_error(gs_error_ioerror) : code);
}
private int
pdf_close_files(gx_device_pdf * pdev, int code)
{
    code = pdf_close_temp_file(pdev, &pdev->pictures, code);
    code = pdf_close_temp_file(pdev, &pdev->streams, code);
    code = pdf_close_temp_file(pdev, &pdev->asides, code);
    return pdf_close_temp_file(pdev, &pdev->xref, code);
}

/* Reset the state of the current page. */
private void
pdf_reset_page(gx_device_pdf * pdev)
{
    pdev->contents_id = 0;
    pdf_reset_graphics(pdev);
    pdev->procsets = NoMarks;
    pdev->cs_Pattern = 0;	/* simplest to create one for each page */
    {
	static const pdf_text_state_t text_default = {
	    pdf_text_state_default
	};

	pdev->text = text_default;
    }
}

/* Open a temporary file, with or without a stream. */
private int
pdf_open_temp_file(gx_device_pdf *pdev, pdf_temp_file_t *ptf)
{
    char fmode[4];

    strcpy(fmode, "w+");
    strcat(fmode, gp_fmode_binary_suffix);
    ptf->file =
	gp_open_scratch_file(gp_scratch_file_name_prefix,
			     ptf->file_name, fmode);
    if (ptf->file == 0)
	return_error(gs_error_invalidfileaccess);
    return 0;
}
private int
pdf_open_temp_stream(gx_device_pdf *pdev, pdf_temp_file_t *ptf)
{
    int code = pdf_open_temp_file(pdev, ptf);

    if (code < 0)
	return code;
    ptf->strm = s_alloc(pdev->pdf_memory, "pdf_open_temp_stream(strm)");
    if (ptf->strm == 0)
	return_error(gs_error_VMerror);
    ptf->strm_buf = gs_alloc_bytes(pdev->pdf_memory, sbuf_size,
				   "pdf_open_temp_stream(strm_buf)");
    if (ptf->strm_buf == 0) {
	gs_free_object(pdev->pdf_memory, ptf->strm,
		       "pdf_open_temp_stream(strm)");
	ptf->strm = 0;
	return_error(gs_error_VMerror);
    }
    swrite_file(ptf->strm, ptf->file, ptf->strm_buf, sbuf_size);
    return 0;
}

/* Initialize the IDs allocated at startup. */
void
pdf_initialize_ids(gx_device_pdf * pdev)
{
    gs_param_string nstr;
    char buf[200];

    pdev->next_id = pdev->FirstObjectNumber;

    /* Initialize the Catalog. */

    param_string_from_string(nstr, "{Catalog}");
    pdf_create_named_dict(pdev, &nstr, &pdev->Catalog, 0L);

    /* Initialize the Info dictionary. */

    param_string_from_string(nstr, "{DocInfo}");
    pdf_create_named_dict(pdev, &nstr, &pdev->Info, 0L);
    sprintf(buf, ((gs_revision % 100) == 0 ? "(%s %1.1f)" : "(%s %1.2f)"),
	    gs_product, gs_revision / 100.0);
    cos_dict_put_c_strings(pdev->Info, pdev, "/Producer", buf);

    /* Allocate the root of the pages tree. */

    pdf_create_named_dict(pdev, NULL, &pdev->Pages, 0L);
}

/* Update the color mapping procedures after setting ProcessColorModel. */
void
pdf_set_process_color_model(gx_device_pdf * pdev)
{
    switch (pdev->color_info.num_components) {
    case 1:
	set_dev_proc(pdev, map_rgb_color, gx_default_gray_map_rgb_color);
	set_dev_proc(pdev, map_color_rgb, gx_default_gray_map_color_rgb);
	set_dev_proc(pdev, map_cmyk_color, NULL);
	break;
    case 3:
	set_dev_proc(pdev, map_rgb_color, gx_default_rgb_map_rgb_color);
	set_dev_proc(pdev, map_color_rgb, gx_default_rgb_map_color_rgb);
	set_dev_proc(pdev, map_cmyk_color, NULL);
	break;
    case 4:
	set_dev_proc(pdev, map_rgb_color, NULL);
	set_dev_proc(pdev, map_color_rgb, cmyk_8bit_map_color_rgb);
	set_dev_proc(pdev, map_cmyk_color, cmyk_8bit_map_cmyk_color);
	break;
    default:			/* can't happen */
	DO_NOTHING;
    }
}

/* Open the device. */
private int
pdf_open(gx_device * dev)
{
    gx_device_pdf *const pdev = (gx_device_pdf *) dev;
    gs_memory_t *mem = pdev->pdf_memory = gs_memory_stable(pdev->memory);
    int code;

    if ((code = pdf_open_temp_file(pdev, &pdev->xref)) < 0 ||
	(code = pdf_open_temp_stream(pdev, &pdev->asides)) < 0 ||
	(code = pdf_open_temp_stream(pdev, &pdev->streams)) < 0 ||
	(code = pdf_open_temp_stream(pdev, &pdev->pictures)) < 0
	)
	goto fail;
    code = gdev_vector_open_file((gx_device_vector *) pdev, sbuf_size);
    if (code < 0)
	goto fail;
    gdev_vector_init((gx_device_vector *) pdev);
    /* Set in_page so the vector routines won't try to call */
    /* any vector implementation procedures. */
    pdev->in_page = true;
    /*
     * pdf_initialize_ids allocates some named objects, so we must
     * initialize the named objects list now.
     */
    pdev->named_objects = cos_dict_alloc(mem, "pdf_open(named_objects)");
    pdf_initialize_ids(pdev);
    pdev->outlines_id = 0;
    pdev->next_page = 0;
    memset(pdev->space_char_ids, 0, sizeof(pdev->space_char_ids));
    pdev->pages =
	gs_alloc_struct_array(mem, initial_num_pages, pdf_page_t,
			      &st_pdf_page_element, "pdf_open(pages)");
    if (pdev->pages == 0) {
	code = gs_error_VMerror;
	goto fail;
    }
    memset(pdev->pages, 0, initial_num_pages * sizeof(pdf_page_t));
    pdev->num_pages = initial_num_pages;
    {
	int i, j;

	for (i = 0; i < NUM_RESOURCE_TYPES; ++i)
	    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j)
		pdev->resources[i].chains[j] = 0;
    }
    pdev->outline_levels[0].first.id = 0;
    pdev->outline_levels[0].left = max_int;
    pdev->outline_levels[0].first.action = 0;
    pdev->outline_levels[0].last.action = 0;
    pdev->outline_depth = 0;
    pdev->closed_outline_depth = 0;
    pdev->outlines_open = 0;
    pdev->articles = 0;
    pdev->Dests = 0;
    /* named_objects was initialized above */
    pdev->open_graphics = 0;
    pdf_reset_page(pdev);

    return 0;
  fail:
    return pdf_close_files(pdev, code);
}

/* Detect I/O errors. */
private int
pdf_ferror(gx_device_pdf *pdev)
{
    fflush(pdev->file);
    fflush(pdev->xref.file);
    sflush(pdev->strm);
    sflush(pdev->asides.strm);
    sflush(pdev->streams.strm);
    sflush(pdev->pictures.strm);
    return ferror(pdev->file) || ferror(pdev->xref.file) ||
	ferror(pdev->asides.file) || ferror(pdev->streams.file) ||
	ferror(pdev->pictures.file);
}

/* Close the current page. */
private int
pdf_close_page(gx_device_pdf * pdev)
{
    int page_num = ++(pdev->next_page);
    pdf_page_t *page;

    /*
     * If the very first page is blank, we need to open the document
     * before doing anything else.
     */
    pdf_open_document(pdev);
    pdf_close_contents(pdev, true);
    /*
     * We can't write the page object or the annotations array yet, because
     * later pdfmarks might add elements to them.  Write the other objects
     * that the page references, and record what we'll need later.
     *
     * Start by making sure the pages array element exists.
     */
    pdf_page_id(pdev, page_num);
    page = &pdev->pages[page_num - 1];
    page->MediaBox.x = (int)(pdev->MediaSize[0]);
    page->MediaBox.y = (int)(pdev->MediaSize[1]);
    page->procsets = pdev->procsets;
    page->contents_id = pdev->contents_id;
    /* Write out any resource dictionaries. */
    {
	int i;

	for (i = 0; i < resourceFont; ++i) {
	    bool any = false;
	    stream *s;
	    int j;

	    for (j = 0; j < NUM_RESOURCE_CHAINS; ++j) {
		pdf_resource_t **prev = &pdev->resources[i].chains[j];
		pdf_resource_t *pres;

		while ((pres = *prev) != 0) {
		    if (pres->used_on_page) {
			long id = pres->object->id;

			if (!any) {
			    page->resource_ids[i] = pdf_begin_obj(pdev);
			    s = pdev->strm;
			    pputs(s, "<<");
			    any = true;
			}
			pprintld2(s, "/R%ld\n%ld 0 R", id, id);
		    }
		    if (pres->named) {
			/* Named resource, might be used again. */
			prev = &pres->next;
		    } else
			*prev = pres->next;
		}
	    }
	    if (any) {
		pputs(s, ">>\n");
		pdf_end_obj(pdev);
	    }
	}
    }
    /* Record references to just those fonts used on this page. */
    {
	bool any = false;
	stream *s;
	int j;

	for (j = 0; j < NUM_RESOURCE_CHAINS; ++j) {
	    pdf_font_t **prev =
		(pdf_font_t **)&pdev->resources[resourceFont].chains[j];
	    pdf_font_t *font;

	    while ((font = *prev) != 0) {
		if (font->used_on_page) {
		    if (!any) {
			page->fonts_id = pdf_begin_obj(pdev);
			s = pdev->strm;
			pputs(s, "<<");
			any = true;
		    }
		    pprints1(s, "/%s", font->frname);
		    pprintld1(s, "\n%ld 0 R", font->object->id);
		    font->used_on_page = false;
		}
		if (font->skip) {
		    /* The font was already written and freed. */
		    *prev = font->next;
		} else
		    prev = &font->next;
	    }
	}
	if (any) {
	    pputs(s, ">>\n");
	    pdf_end_obj(pdev);
	}
    }
    /*
     * When Acrobat Reader 3 prints a file containing a Type 3 font with a
     * non-standard Encoding, it apparently only emits the subset of the
     * font actually used on the page.  Thus, if the "Download Fonts Once"
     * option is selected, characters not used on the page where the font
     * first appears will not be defined, and hence will print as blank if
     * used on subsequent pages.  Thus, we can't allow a Type 3 font to
     * add additional characters on subsequent pages.
     */
    if (pdev->CompatibilityLevel <= 1.2)
	pdev->open_font = 0;
    pdf_reset_page(pdev);
    return (pdf_ferror(pdev) ? gs_note_error(gs_error_ioerror) : 0);
}

/* Write the page object. */
private int
pdf_write_page(gx_device_pdf *pdev, int page_num)
{
    long page_id = pdf_page_id(pdev, page_num);
    pdf_page_t *page = &pdev->pages[page_num - 1];
    gs_memory_t *mem = pdev->pdf_memory;
    stream *s;

    pdf_open_obj(pdev, page_id);
    s = pdev->strm;
    pprintd2(s, "<</Type/Page/MediaBox [0 0 %d %d]\n",
	     page->MediaBox.x, page->MediaBox.y);
    pprintld1(s, "/Parent %ld 0 R\n", pdev->Pages->id);
    pputs(s, "/Resources<</ProcSet[/PDF");
    if (page->procsets & ImageB)
	pputs(s, " /ImageB");
    if (page->procsets & ImageC)
	pputs(s, " /ImageC");
    if (page->procsets & ImageI)
	pputs(s, " /ImageI");
    if (page->procsets & Text)
	pputs(s, " /Text");
    pputs(s, "]\n");
    {
	int i;

	for (i = 0; i < resourceFont; ++i)
	    if (page->resource_ids[i]) {
		pprints1(s, "/%s ", resource_type_names[i]);
		pprintld1(s, "%ld 0 R\n", page->resource_ids[i]);
	    }
    }
    if (page->fonts_id)
	pprintld1(s, "/Font %ld 0 R\n", page->fonts_id);
    pputs(s, ">>\n");

    /* Write out the annotations array if any. */

    if (page->Annots) {
	pputs(s, "/Annots");
	COS_WRITE(page->Annots, pdev);
	COS_FREE(page->Annots, mem, "pdf_write_page(Annots)");
	page->Annots = 0;
    }
    if (page->contents_id == 0)
	pputs(s, "/Contents []\n");
    else
	pprintld1(s, "/Contents %ld 0 R\n", page->contents_id);

    /* Write any elements stored by pdfmarks. */

    cos_dict_elements_write(page->Page, pdev);

    pputs(s, ">>\n");
    pdf_end_obj(pdev);
    return 0;
}

/* Wrap up ("output") a page. */
private int
pdf_output_page(gx_device * dev, int num_copies, int flush)
{
    gx_device_pdf *const pdev = (gx_device_pdf *) dev;
    int code = pdf_close_page(pdev);

    return (code < 0 ? code :
	    pdf_ferror(pdev) ? gs_note_error(gs_error_ioerror) :
	    gx_finish_output_page(dev, num_copies, flush));
}

/* Close the device. */
private int
pdf_close(gx_device * dev)
{
    gx_device_pdf *const pdev = (gx_device_pdf *) dev;
    gs_memory_t *mem = pdev->pdf_memory;
    stream *s;
    FILE *tfile = pdev->xref.file;
    long xref;
    long resource_pos;
    long Catalog_id = pdev->Catalog->id, Info_id = pdev->Info->id,
	Pages_id = pdev->Pages->id;
    long Threads_id = 0;
    bool partial_page = (pdev->contents_id != 0 && pdev->next_page != 0);

    /*
     * If this is an EPS file, or if the file didn't end with a showpage for
     * some other reason, or if the file has produced no marks at all, we
     * need to tidy up a little so as not to produce illegal PDF.  However,
     * if there is at least one complete page, we discard any leftover
     * marks.
     */
    if (pdev->next_page == 0)
	pdf_open_document(pdev);
    if (pdev->contents_id != 0)
	pdf_close_page(pdev);

    /* Write the page objects. */

    {
	int i;

	for (i = 1; i <= pdev->next_page; ++i)
	    pdf_write_page(pdev, i);
    }

    /* Write the font resources and related resources. */

    pdf_write_font_resources(pdev);

    /* Create the Pages tree. */

    pdf_open_obj(pdev, Pages_id);
    s = pdev->strm;
    pputs(s, "<< /Type /Pages /Kids [\n");
    /* Omit the last page if it was incomplete. */
    if (partial_page)
	--(pdev->next_page);
    {
	int i;

	for (i = 0; i < pdev->next_page; ++i)
	    pprintld1(s, "%ld 0 R\n", pdev->pages[i].Page->id);
    }
    pprintd1(s, "] /Count %d\n", pdev->next_page);
    cos_dict_elements_write(pdev->Pages, pdev);
    pputs(s, ">>\n");
    pdf_end_obj(pdev);

    /* Close outlines and articles. */

    if (pdev->outlines_id != 0) {
	pdfmark_close_outline(pdev);	/* depth must be zero! */
	pdf_open_obj(pdev, pdev->outlines_id);
	pprintd1(s, "<< /Count %d", pdev->outlines_open);
	pprintld2(s, " /First %ld 0 R /Last %ld 0 R >>\n",
		  pdev->outline_levels[0].first.id,
		  pdev->outline_levels[0].last.id);
	pdf_end_obj(pdev);
    }
    if (pdev->articles != 0) {
	pdf_article_t *part;

	/* Write the remaining information for each article. */
	for (part = pdev->articles; part != 0; part = part->next)
	    pdfmark_write_article(pdev, part);
    }

    /* Write named destinations.  (We can't free them yet.) */

    if (pdev->Dests)
	COS_WRITE_OBJECT(pdev->Dests, pdev);

    /* Write the Catalog. */

    /*
     * The PDF specification requires Threads to be an indirect object.
     * Write the threads now, if any.
     */
    if (pdev->articles != 0) {
	pdf_article_t *part;

	Threads_id = pdf_begin_obj(pdev);
	s = pdev->strm;
	pputs(s, "[ ");
	while ((part = pdev->articles) != 0) {
	    pdev->articles = part->next;
	    pprintld1(s, "%ld 0 R\n", part->contents->id);
	    COS_FREE(part->contents, mem, "pdf_close(article contents)");
	    gs_free_object(mem, part, "pdf_close(article)");
	}
	pputs(s, "]\n");
	pdf_end_obj(pdev);
    }
    pdf_open_obj(pdev, Catalog_id);
    s = pdev->strm;
    pputs(s, "<<");
    pprintld1(s, "/Type /Catalog /Pages %ld 0 R\n", Pages_id);
    if (pdev->outlines_id != 0)
	pprintld1(s, "/Outlines %ld 0 R\n", pdev->outlines_id);
    if (Threads_id)
	pprintld1(s, "/Threads %ld 0 R\n", Threads_id);
    if (pdev->Dests)
	pprintld1(s, "/Dests %ld 0 R\n", pdev->Dests->id);
    cos_dict_elements_write(pdev->Catalog, pdev);
    pputs(s, ">>\n");
    pdf_end_obj(pdev);
    if (pdev->Dests) {
	COS_FREE(pdev->Dests, mem, "pdf_close(Dests)");
	pdev->Dests = 0;
    }

    /* Prevent writing special named objects twice. */
    pdev->Catalog->id = 0;
    /*pdev->Info->id = 0;*/	/* Info should get written */
    pdev->Pages->id = 0;
    {
	int i;

	for (i = 0; i < pdev->num_pages; ++i)
	    if (pdev->pages[i].Page)
		pdev->pages[i].Page->id = 0;
    }

    /*
     * Write the definitions of the named objects.
     * Note that this includes Form XObjects created by BP/EP, named PS
     * XObjects, and eventually images named by NI.
     */

    {
	const cos_dict_element_t *pcde = pdev->named_objects->elements;

	for (; pcde; pcde = pcde->next)
	    if (pcde->value.contents.object->id)
		cos_write_object(pcde->value.contents.object, pdev);
    }

    /* Copy the resources into the main file. */

    s = pdev->strm;
    resource_pos = stell(s);
    sflush(pdev->asides.strm);
    {
	FILE *rfile = pdev->asides.file;
	long res_end = ftell(rfile);

	fseek(rfile, 0L, SEEK_SET);
	pdf_copy_data(s, rfile, res_end);
    }

    /* Write the cross-reference section. */

    xref = pdf_stell(pdev);
    if (pdev->FirstObjectNumber == 1)
	pprintld1(s, "xref\n0 %ld\n0000000000 65535 f \n",
		  pdev->next_id);
    else
	pprintld2(s, "xref\n0 1\n0000000000 65535 f \n%ld %ld\n",
		  pdev->FirstObjectNumber,
		  pdev->next_id - pdev->FirstObjectNumber);
    fseek(tfile, 0L, SEEK_SET);
    {
	long i;

	for (i = pdev->FirstObjectNumber; i < pdev->next_id; ++i) {
	    ulong pos;
	    char str[21];

	    fread(&pos, sizeof(pos), 1, tfile);
	    if (pos & ASIDES_BASE_POSITION)
		pos += resource_pos - ASIDES_BASE_POSITION;
	    sprintf(str, "%010ld 00000 n \n", pos);
	    pputs(s, str);
	}
    }

    /* Write the trailer. */

    pputs(s, "trailer\n");
    pprintld3(s, "<< /Size %ld /Root %ld 0 R /Info %ld 0 R\n",
	      pdev->next_id, Catalog_id, Info_id);
    pputs(s, ">>\n");
    pprintld1(s, "startxref\n%ld\n%%%%EOF\n", xref);

    /* Release the resource records. */

    {
	pdf_resource_t *pres;
	pdf_resource_t *prev;

	for (prev = pdev->last_resource; (pres = prev) != 0;) {
	    prev = pres->prev;
	    gs_free_object(mem, pres, "pdf_resource_t");
	}
	pdev->last_resource = 0;
    }

    /* Free named objects. */

    {
	cos_dict_element_t *pcde = pdev->named_objects->elements;

	/*
	 * Delete the objects' IDs so that freeing the dictionary will
	 * free them.
	 */
	for (; pcde; pcde = pcde->next)
	    pcde->value.contents.object->id = 0;
	COS_FREE(pdev->named_objects, mem, "pdf_close(named_objects)");
	pdev->named_objects = 0;
    }

    gs_free_object(mem, pdev->pages, "pages");
    pdev->pages = 0;
    pdev->num_pages = 0;

    {
	int code = gdev_vector_close_file((gx_device_vector *) pdev);

	return pdf_close_files(pdev, code);
    }
}
