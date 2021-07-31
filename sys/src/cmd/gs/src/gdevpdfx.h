/* Copyright (C) 1996, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: gdevpdfx.h,v 1.34 2001/10/07 05:33:11 lpd Exp $ */
/* Internal definitions for PDF-writing driver. */

#ifndef gdevpdfx_INCLUDED
#  define gdevpdfx_INCLUDED

#include "gsparam.h"
#include "gsuid.h"
#include "gxdevice.h"
#include "gxfont.h"
#include "gxline.h"
#include "stream.h"
#include "spprint.h"
#include "gdevpsdf.h"

/* ---------------- Acrobat limitations ---------------- */

/*
 * The PDF reference manual, 2nd ed., claims that the limit for coordinates
 * is +/- 32767. However, testing indicates that Acrobat Reader 4 for
 * Windows and Linux fail with coordinates outside +/- 16383. Hence, we
 * limit coordinates to 16k, with a little slop.
 */
#define MAX_USER_COORD 16300

/* ---------------- Statically allocated sizes ---------------- */
/* These should really be dynamic.... */

/* Define the maximum depth of an outline tree. */
/* Note that there is no limit on the breadth of the tree. */
#define MAX_OUTLINE_DEPTH 32

/* Define the maximum size of a destination array string. */
#define MAX_DEST_STRING 80

/* ================ Types and structures ================ */

/* Define the possible contexts for the output stream. */
typedef enum {
    PDF_IN_NONE,
    PDF_IN_STREAM,
    PDF_IN_TEXT,
    PDF_IN_STRING
} pdf_context_t;

/* ---------------- Cos objects ---------------- */

/*
 * These are abstract types for cos objects.  The concrete types are in
 * gdevpdfo.h.
 */
typedef struct cos_object_s cos_object_t;
typedef struct cos_stream_s cos_stream_t;
typedef struct cos_dict_s cos_dict_t;
typedef struct cos_array_s cos_array_t;
typedef struct cos_value_s cos_value_t;
typedef struct cos_object_procs_s cos_object_procs_t;
typedef const cos_object_procs_t *cos_type_t;
#define cos_types_DEFINED

/* ---------------- Resources ---------------- */

typedef enum {
    /*
     * Standard PDF resources.  Font must be last, because resources
     * up to but not including Font are written page-by-page.
     */
    resourceColorSpace,
    resourceExtGState,
    resourcePattern,
    resourceShading,
    resourceXObject,
    resourceFont,
    /*
     * Internally used (pseudo-)resources.
     */
    resourceCharProc,
    resourceCIDFont,
    resourceCMap,
    resourceFontDescriptor,
    resourceFunction,
    NUM_RESOURCE_TYPES
} pdf_resource_type_t;

#define PDF_RESOURCE_TYPE_NAMES\
  "/ColorSpace", "/ExtGState", "/Pattern", "/Shading", "/XObject", "/Font",\
  0, "/Font", "/CMap", "/FontDescriptor", 0
#define PDF_RESOURCE_TYPE_STRUCTS\
  &st_pdf_resource,		/* see below */\
  &st_pdf_resource,\
  &st_pdf_resource,\
  &st_pdf_resource,\
  &st_pdf_x_object,		/* see below */\
  &st_pdf_font,			/* gdevpdff.h / gdevpdff.c */\
  &st_pdf_char_proc,		/* gdevpdff.h / gdevpdff.c */\
  &st_pdf_font,			/* gdevpdff.h / gdevpdff.c */\
  &st_pdf_resource,\
  &st_pdf_font_descriptor,	/* gdevpdff.h / gdevpdff.c */\
  &st_pdf_resource

/*
 * rname is currently R<id> for all resources other than synthesized fonts;
 * for synthesized fonts, rname is A, B, ....  The string is null-terminated.
 */

#define pdf_resource_common(typ)\
    typ *next;			/* next resource of this type */\
    pdf_resource_t *prev;	/* previously allocated resource */\
    gs_id rid;			/* optional ID key */\
    bool named;\
    char rname[1/*R*/ + (sizeof(long) * 8 / 3 + 1) + 1/*\0*/];\
    ulong where_used;		/* 1 bit per level of content stream */\
    cos_object_t *object
typedef struct pdf_resource_s pdf_resource_t;
struct pdf_resource_s {
    pdf_resource_common(pdf_resource_t);
};

/* The descriptor is public for subclassing. */
extern_st(st_pdf_resource);
#define public_st_pdf_resource()  /* in gdevpdfu.c */\
  gs_public_st_ptrs3(st_pdf_resource, pdf_resource_t, "pdf_resource_t",\
    pdf_resource_enum_ptrs, pdf_resource_reloc_ptrs, next, prev, object)

/*
 * We define XObject resources here because they are used for Image,
 * Form, and PS XObjects, which are implemented in different files.
 */
typedef struct pdf_x_object_s pdf_x_object_t;
struct pdf_x_object_s {
    pdf_resource_common(pdf_x_object_t);
    int width, height;		/* specified width and height for images */
    int data_height;		/* actual data height for images */
};
#define private_st_pdf_x_object()  /* in gdevpdfu.c */\
  gs_private_st_suffix_add0(st_pdf_x_object, pdf_x_object_t,\
    "pdf_x_object_t", pdf_x_object_enum_ptrs, pdf_x_object_reloc_ptrs,\
    st_pdf_resource)

/* Define the mask for which procsets have been used on a page. */
typedef enum {
    NoMarks = 0,
    ImageB = 1,
    ImageC = 2,
    ImageI = 4,
    Text = 8
} pdf_procset_t;

/* ------ Fonts ------ */

/* Define the standard fonts. */
#define PDF_NUM_STD_FONTS 14
#define pdf_do_std_fonts(m)\
  m("Courier", ENCODING_INDEX_STANDARD)\
  m("Courier-Bold", ENCODING_INDEX_STANDARD)\
  m("Courier-Oblique", ENCODING_INDEX_STANDARD)\
  m("Courier-BoldOblique", ENCODING_INDEX_STANDARD)\
  m("Helvetica", ENCODING_INDEX_STANDARD)\
  m("Helvetica-Bold", ENCODING_INDEX_STANDARD)\
  m("Helvetica-Oblique", ENCODING_INDEX_STANDARD)\
  m("Helvetica-BoldOblique", ENCODING_INDEX_STANDARD)\
  m("Symbol", ENCODING_INDEX_SYMBOL)\
  m("Times-Roman", ENCODING_INDEX_STANDARD)\
  m("Times-Bold", ENCODING_INDEX_STANDARD)\
  m("Times-Italic", ENCODING_INDEX_STANDARD)\
  m("Times-BoldItalic", ENCODING_INDEX_STANDARD)\
  m("ZapfDingbats", ENCODING_INDEX_DINGBATS)

/* Define the range of synthesized space characters. */
#define X_SPACE_MIN 24
#define X_SPACE_MAX 150

/* Define abstract types.  The concrete types are in gdevpdff.h. */
typedef struct pdf_font_s pdf_font_t;

/* ------ Named objects ------ */

/* Define an element of the graphics object accumulation (BP/EP) stack. */
typedef struct pdf_graphics_save_s pdf_graphics_save_t;
struct pdf_graphics_save_s {
    pdf_graphics_save_t *prev;
    cos_stream_t *object;
    long position;
    pdf_context_t save_context;
    pdf_procset_t save_procsets;
    long save_contents_id;
};

#define private_st_pdf_graphics_save()	/* in gdevpdfm.c */\
  gs_private_st_ptrs2(st_pdf_graphics_save, pdf_graphics_save_t,\
    "pdf_graphics_save_t", pdf_graphics_save_enum_ptrs,\
    pdf_graphics_save_reloc_ptrs, prev, object)

/* ---------------- Other auxiliary structures ---------------- */

/* Outline nodes and levels */
typedef struct pdf_outline_node_s {
    long id, parent_id, prev_id, first_id, last_id;
    int count;
    cos_dict_t *action;
} pdf_outline_node_t;
typedef struct pdf_outline_level_s {
    pdf_outline_node_t first;
    pdf_outline_node_t last;
    int left;
} pdf_outline_level_t;
/*
 * The GC descriptor is implicit, since outline levels occur only in an
 * embedded array in the gx_device_pdf structure.
 */

/* Articles */
typedef struct pdf_bead_s {
    long id, article_id, prev_id, next_id, page_id;
    gs_rect rect;
} pdf_bead_t;
typedef struct pdf_article_s pdf_article_t;
struct pdf_article_s {
    pdf_article_t *next;
    cos_dict_t *contents;
    pdf_bead_t first;
    pdf_bead_t last;
};

#define private_st_pdf_article()\
  gs_private_st_ptrs2(st_pdf_article, pdf_article_t,\
    "pdf_article_t", pdf_article_enum_ptrs, pdf_article_reloc_ptrs,\
    next, contents)

/* ---------------- The device structure ---------------- */

/* Text state */
#ifndef pdf_font_descriptor_DEFINED
#  define pdf_font_descriptor_DEFINED
typedef struct pdf_font_descriptor_s pdf_font_descriptor_t;
#endif
typedef struct pdf_std_font_s {
    gs_font *font;		/* weak pointer, may be 0 */
    pdf_font_descriptor_t *pfd;  /* *not* a weak pointer */
    gs_matrix orig_matrix;
    gs_uid uid;			/* UniqueID, not XUID */
} pdf_std_font_t;
typedef struct pdf_text_state_s {
    /* State parameters */
    float character_spacing;	/* Tc */
    pdf_font_t *font;		/* for Tf */
    floatp size;		/* for Tf */
    float word_spacing;		/* Tw */
    float leading;		/* TL */
    bool use_leading;		/* true => use ', false => use Tj */
    int render_mode;		/* Tr */
    /* Bookkeeping */
    gs_matrix matrix;		/* relative to device space, not user space */
    gs_point line_start;
    gs_point current;
#define max_text_buffer 200	/* arbitrary, but overflow costs 5 chars */
    byte buffer[max_text_buffer];
    int buffer_count;
} pdf_text_state_t;

#define pdf_text_state_default\
  0, NULL, 0, 0, 0, 0 /*false*/, 0,\
  { identity_matrix_body }, { 0, 0 }, { 0, 0 }, { 0 }, 0

/* Resource lists */
#define NUM_RESOURCE_CHAINS 16
typedef struct pdf_resource_list_s {
    pdf_resource_t *chains[NUM_RESOURCE_CHAINS];
} pdf_resource_list_t;

/* Define the hash function for gs_ids. */
#define gs_id_hash(rid) ((rid) + ((rid) / NUM_RESOURCE_CHAINS))
/* Define the accessor for the proper hash chain. */
#define PDF_RESOURCE_CHAIN(pdev, type, rid)\
  (&(pdev)->resources[type].chains[gs_id_hash(rid) % NUM_RESOURCE_CHAINS])

/* Define the bookkeeping for an open stream. */
typedef struct pdf_stream_position_s {
    long length_id;
    long start_pos;
} pdf_stream_position_t;

/*
 * Define the structure for keeping track of text rotation.
 * There is one for the current page (for AutoRotate /PageByPage)
 * and one for the whole document (for AutoRotate /All).
 */
typedef struct pdf_text_rotation_s {
    long counts[5];		/* 0, 90, 180, 270, other */
    int Rotate;			/* computed rotation, -1 means none */
} pdf_text_rotation_t;
#define pdf_text_rotation_angle_values 0, 90, 180, 270, -1

/*
 * Define document and page information derived from DSC comments.
 */
typedef struct pdf_page_dsc_info_s {
    int orientation;		/* -1 .. 3 */
    int viewing_orientation;	/* -1 .. 3 */
    gs_rect bounding_box;
} pdf_page_dsc_info_t;

/*
 * Define the stored information for a page.  Because pdfmarks may add
 * information to any page anywhere in the document, we have to wait
 * until the end to write out the page dictionaries.
 */
typedef struct pdf_page_s {
    cos_dict_t *Page;
    gs_point MediaBox;
    pdf_procset_t procsets;
    long contents_id;
    long resource_ids[resourceFont + 1]; /* resources thru Font, see above */
    cos_array_t *Annots;
    pdf_text_rotation_t text_rotation;
    pdf_page_dsc_info_t dsc_info;
} pdf_page_t;
#define private_st_pdf_page()	/* in gdevpdf.c */\
  gs_private_st_ptrs2(st_pdf_page, pdf_page_t, "pdf_page_t",\
    pdf_page_enum_ptrs, pdf_page_reloc_ptrs, Page, Annots)

/*
 * Define the structure for the temporary files used while writing.
 * There are 4 of these, described below.
 */
typedef struct pdf_temp_file_s {
    char file_name[gp_file_name_sizeof];
    FILE *file;
    stream *strm;
    byte *strm_buf;
    stream *save_strm;		/* save pdev->strm while writing here */
} pdf_temp_file_t;

/* Define the device structure. */
#ifndef gx_device_pdf_DEFINED
#  define gx_device_pdf_DEFINED
typedef struct gx_device_pdf_s gx_device_pdf;
#endif
struct gx_device_pdf_s {
    gx_device_psdf_common;
    /* PDF-specific distiller parameters */
    double CompatibilityLevel;
    int EndPage;
    int StartPage;
    bool Optimize;
    bool ParseDSCCommentsForDocInfo;
    bool ParseDSCComments;
    bool EmitDSCWarnings;
    bool CreateJobTicket;
    bool PreserveEPSInfo;
    bool AutoPositionEPSFiles;
    bool PreserveCopyPage;
    bool UsePrologue;
    /* End of distiller parameters */
    /* Other parameters */
    bool ReAssignCharacters;
    bool ReEncodeCharacters;
    long FirstObjectNumber;
    /* End of parameters */
    /* Values derived from DSC comments */
    bool is_EPS;
    pdf_page_dsc_info_t doc_dsc_info; /* document default */
    pdf_page_dsc_info_t page_dsc_info; /* current page */
    /* Additional graphics state */
    bool fill_overprint, stroke_overprint;
    int overprint_mode;
    gs_id halftone_id;
    gs_id transfer_ids[4];
    int transfer_not_identity;	/* bitmask */
    gs_id black_generation_id, undercolor_removal_id;
    /* Following are set when device is opened. */
    enum {
	pdf_compress_none,
	pdf_compress_LZW,	/* not currently used, thanks to Unisys */
	pdf_compress_Flate
    } compression;
#define pdf_memory v_memory
    /*
     * The xref temporary file is logically an array of longs.
     * xref[id - FirstObjectNumber] is the position in the output file
     * of the object with the given id.
     *
     * Note that xref, unlike the other temporary files, does not have
     * an associated stream or stream buffer.
     */
    pdf_temp_file_t xref;
    /*
     * asides holds resources and other "aside" objects.  It is
     * copied verbatim to the output file at the end of the document.
     */
    pdf_temp_file_t asides;
    /*
     * streams holds data for stream-type Cos objects.  The data is
     * copied to the output file at the end of the document.
     *
     * Note that streams.save_strm is not used, since we don't interrupt
     * normal output when saving stream data.
     */
    pdf_temp_file_t streams;
    /*
     * pictures holds graphic objects being accumulated between BP and EP.
     * The object is moved to streams when the EP is reached: since BP and
     * EP nest, we delete the object from the pictures file at that time.
     */
    pdf_temp_file_t pictures;
    pdf_font_t *open_font;	/* current Type 3 synthesized font */
    bool use_open_font;		/* if false, start new open_font */
    long embedded_encoding_id;
    int max_embedded_code;	/* max Type 3 code used */
    long random_offset;		/* for generating subset prefixes */
    /* ................ */
    long next_id;
    /* The following 3 objects, and only these, are allocated */
    /* when the file is opened. */
    cos_dict_t *Catalog;
    cos_dict_t *Info;
    cos_dict_t *Pages;
#define pdf_num_initial_ids 3
    long outlines_id;
    int next_page;
    long contents_id;
    pdf_context_t context;
    long contents_length_id;
    long contents_pos;
    pdf_procset_t procsets;	/* used on this page */
    pdf_text_state_t text;
    pdf_std_font_t std_fonts[PDF_NUM_STD_FONTS];
    long space_char_ids[X_SPACE_MAX - X_SPACE_MIN + 1];
    pdf_text_rotation_t text_rotation;
#define initial_num_pages 50
    pdf_page_t *pages;
    int num_pages;
    ulong used_mask;		/* for where_used: page level = 1 */
    pdf_resource_list_t resources[NUM_RESOURCE_TYPES];
    /* cs_Patterns[0] is colored; 1,3,4 are uncolored + Gray,RGB,CMYK */
    pdf_resource_t *cs_Patterns[5];
    pdf_resource_t *last_resource;
    pdf_outline_level_t outline_levels[MAX_OUTLINE_DEPTH];
    int outline_depth;
    int closed_outline_depth;
    int outlines_open;
    pdf_article_t *articles;
    cos_dict_t *Dests;
    cos_dict_t *named_objects;
    pdf_graphics_save_t *open_graphics;
};

#define is_in_page(pdev)\
  ((pdev)->contents_id != 0)
#define is_in_document(pdev)\
  (is_in_page(pdev) || (pdev)->last_resource != 0)

/* Enumerate the individual pointers in a gx_device_pdf. */
#define gx_device_pdf_do_ptrs(m)\
 m(0,asides.strm) m(1,asides.strm_buf) m(2,asides.save_strm)\
 m(3,streams.strm) m(4,streams.strm_buf)\
 m(5,pictures.strm) m(6,pictures.strm_buf) m(7,pictures.save_strm)\
 m(8,open_font)\
 m(9,Catalog) m(10,Info) m(11,Pages)\
 m(12,text.font) m(13,pages)\
 m(14,cs_Patterns[0])\
 m(15,cs_Patterns[1]) m(16,cs_Patterns[3]) m(17,cs_Patterns[4])\
 m(18,last_resource)\
 m(19,articles) m(20,Dests) m(21,named_objects) m(22,open_graphics)
#define gx_device_pdf_num_ptrs 23
#define gx_device_pdf_do_strings(m) /* do nothing */
#define gx_device_pdf_num_strings 0
#define st_device_pdf_max_ptrs\
  (st_device_psdf_max_ptrs + gx_device_pdf_num_ptrs +\
   gx_device_pdf_num_strings +\
   PDF_NUM_STD_FONTS * 2 /* std_fonts[].{font,pfd} */ +\
   NUM_RESOURCE_TYPES * NUM_RESOURCE_CHAINS /* resources[].chains[] */ +\
   MAX_OUTLINE_DEPTH * 2 /* outline_levels[].{first,last}.action */

#define private_st_device_pdfwrite()	/* in gdevpdf.c */\
  gs_private_st_composite_final(st_device_pdfwrite, gx_device_pdf,\
    "gx_device_pdf", device_pdfwrite_enum_ptrs, device_pdfwrite_reloc_ptrs,\
    device_pdfwrite_finalize)

/* ================ Driver procedures ================ */

    /* In gdevpdfb.c */
dev_proc_copy_mono(gdev_pdf_copy_mono);
dev_proc_copy_color(gdev_pdf_copy_color);
dev_proc_fill_mask(gdev_pdf_fill_mask);
dev_proc_strip_tile_rectangle(gdev_pdf_strip_tile_rectangle);
    /* In gdevpdfd.c */
extern const gx_device_vector_procs pdf_vector_procs;
dev_proc_fill_rectangle(gdev_pdf_fill_rectangle);
dev_proc_fill_path(gdev_pdf_fill_path);
dev_proc_stroke_path(gdev_pdf_stroke_path);
    /* In gdevpdfi.c */
dev_proc_begin_typed_image(gdev_pdf_begin_typed_image);
    /* In gdevpdfp.c */
dev_proc_get_params(gdev_pdf_get_params);
dev_proc_put_params(gdev_pdf_put_params);
    /* In gdevpdft.c */
dev_proc_text_begin(gdev_pdf_text_begin);

/* ================ Utility procedures ================ */

/* ---------------- Exported by gdevpdf.c ---------------- */

/* Initialize the IDs allocated at startup. */
void pdf_initialize_ids(P1(gx_device_pdf * pdev));

/* Update the color mapping procedures after setting ProcessColorModel. */
void pdf_set_process_color_model(P1(gx_device_pdf * pdev));

/* Reset the text state parameters to initial values. */
void pdf_reset_text(P1(gx_device_pdf *pdev));

/* ---------------- Exported by gdevpdfu.c ---------------- */

/* ------ Document ------ */

/* Open the document if necessary. */
void pdf_open_document(P1(gx_device_pdf * pdev));

/* ------ Objects ------ */

/* Allocate an ID for a future object. */
long pdf_obj_ref(P1(gx_device_pdf * pdev));

/* Read the current position in the output stream. */
long pdf_stell(P1(gx_device_pdf * pdev));

/* Begin an object, optionally allocating an ID. */
long pdf_open_obj(P2(gx_device_pdf * pdev, long id));
long pdf_begin_obj(P1(gx_device_pdf * pdev));

/* End an object. */
int pdf_end_obj(P1(gx_device_pdf * pdev));

/* ------ Page contents ------ */

/* Open a page contents part. */
/* Return an error if the page has too many contents parts. */
int pdf_open_contents(P2(gx_device_pdf * pdev, pdf_context_t context));

/* Close the current contents part if we are in one. */
int pdf_close_contents(P2(gx_device_pdf * pdev, bool last));

/* ------ Resources et al ------ */

extern const char *const pdf_resource_type_names[];
extern const gs_memory_struct_type_t *const pdf_resource_type_structs[];

/*
 * Define the offset that indicates that a file position is in the
 * asides file rather than the main (contents) file.
 * Must be a power of 2, and larger than the largest possible output file.
 */
#define ASIDES_BASE_POSITION min_long

/* Begin an object logically separate from the contents. */
/* (I.e., an object in the resource file.) */
long pdf_open_separate(P2(gx_device_pdf * pdev, long id));
long pdf_begin_separate(P1(gx_device_pdf * pdev));

/* Begin an aside (resource, annotation, ...). */
int pdf_begin_aside(P4(gx_device_pdf * pdev, pdf_resource_t **plist,
		       const gs_memory_struct_type_t * pst,
		       pdf_resource_t **ppres));

/* Begin a resource of a given type. */
int pdf_begin_resource(P4(gx_device_pdf * pdev, pdf_resource_type_t rtype,
			  gs_id rid, pdf_resource_t **ppres));

/* Begin a resource body of a given type. */
int pdf_begin_resource_body(P4(gx_device_pdf * pdev, pdf_resource_type_t rtype,
			       gs_id rid, pdf_resource_t **ppres));

/* Allocate a resource, but don't open the stream. */
int pdf_alloc_resource(P5(gx_device_pdf * pdev, pdf_resource_type_t rtype,
			  gs_id rid, pdf_resource_t **ppres, long id));

/* Find a resource of a given type by gs_id. */
pdf_resource_t *pdf_find_resource_by_gs_id(P3(gx_device_pdf * pdev,
					      pdf_resource_type_t rtype,
					      gs_id rid));

/* Get the object id of a resource. */
long pdf_resource_id(P1(const pdf_resource_t *pres));

/* End a separate object. */
int pdf_end_separate(P1(gx_device_pdf * pdev));

/* End an aside. */
int pdf_end_aside(P1(gx_device_pdf * pdev));

/* End a resource. */
int pdf_end_resource(P1(gx_device_pdf * pdev));

/*
 * Write and release the Cos objects for resources local to a content stream.
 * We must write all the objects before freeing any of them, because
 * they might refer to each other.
 */
int pdf_write_resource_objects(P2(gx_device_pdf *pdev,
				  pdf_resource_type_t rtype));

/*
 * Store the resource sets for a content stream (page or XObject).
 * Sets page->{procsets, resource_ids[], fonts_id}.
 */
int pdf_store_page_resources(P2(gx_device_pdf *pdev, pdf_page_t *page));

/* Copy data from a temporary file to a stream. */
void pdf_copy_data(P3(stream *s, FILE *file, long count));

/* ------ Pages ------ */

/* Get or assign the ID for a page. */
/* Returns 0 if the page number is out of range. */
long pdf_page_id(P2(gx_device_pdf * pdev, int page_num));

/* Get the page structure for the current page. */
pdf_page_t *pdf_current_page(P1(gx_device_pdf *pdev));

/* Get the dictionary object for the current page. */
cos_dict_t *pdf_current_page_dict(P1(gx_device_pdf *pdev));

/* Open a page for writing. */
int pdf_open_page(P2(gx_device_pdf * pdev, pdf_context_t context));

/* Write saved page- or document-level information. */
int pdf_write_saved_string(P2(gx_device_pdf * pdev, gs_string * pstr));

/* ------ Path drawing ------ */

/* Test whether the clip path needs updating. */
bool pdf_must_put_clip_path(P2(gx_device_pdf * pdev, const gx_clip_path * pcpath));

/* Write and update the clip path. */
int pdf_put_clip_path(P2(gx_device_pdf * pdev, const gx_clip_path * pcpath));

/* ------ Miscellaneous output ------ */

#define PDF_MAX_PRODUCER 200	/* adhoc */
/* Generate the default Producer string. */
void pdf_store_default_Producer(P1(char buf[PDF_MAX_PRODUCER]));

/* Define the strings for filter names and parameters. */
typedef struct pdf_filter_names_s {
    const char *ASCII85Decode;
    const char *ASCIIHexDecode;
    const char *CCITTFaxDecode;
    const char *DCTDecode;
    const char *DecodeParms;
    const char *Filter;
    const char *FlateDecode;
    const char *LZWDecode;
    const char *RunLengthDecode;
} pdf_filter_names_t;
#define PDF_FILTER_NAMES\
  "/ASCII85Decode", "/ASCIIHexDecode", "/CCITTFaxDecode",\
  "/DCTDecode",  "/DecodeParms", "/Filter", "/FlateDecode",\
  "/LZWDecode", "/RunLengthDecode"
#define PDF_FILTER_NAMES_SHORT\
  "/A85", "/AHx", "/CCF", "/DCT", "/DP", "/F", "/Fl", "/LZW", "/RL"

/* Write matrix values. */
void pdf_put_matrix(P4(gx_device_pdf *pdev, const char *before,
		       const gs_matrix *pmat, const char *after));

/* Write a name, with escapes for unusual characters. */
typedef int (*pdf_put_name_chars_proc_t)(P3(stream *, const byte *, uint));
pdf_put_name_chars_proc_t
    pdf_put_name_chars_proc(P1(const gx_device_pdf *pdev));
void pdf_put_name_chars(P3(const gx_device_pdf *pdev, const byte *nstr,
			   uint size));
void pdf_put_name(P3(const gx_device_pdf *pdev, const byte *nstr, uint size));

/* Write a string in its shortest form ( () or <> ). */
void pdf_put_string(P3(const gx_device_pdf *pdev, const byte *str, uint size));

/* Write a value, treating names specially. */
void pdf_write_value(P3(const gx_device_pdf *pdev, const byte *vstr, uint size));

/* Store filters for a stream. */
int pdf_put_filters(P4(cos_dict_t *pcd, gx_device_pdf *pdev, stream *s,
		       const pdf_filter_names_t *pfn));

/* Define a possibly encoded and compressed data stream. */
typedef struct pdf_data_writer_s {
    psdf_binary_writer binary;
    long start;
    long length_id;
} pdf_data_writer_t;
/*
 * Begin a data stream.  The client has opened the object and written
 * the << and any desired dictionary keys.
 */
int pdf_begin_data_binary(P3(gx_device_pdf *pdev, pdf_data_writer_t *pdw,
			     bool data_is_binary));
#define pdf_begin_data(pdev, pdw) pdf_begin_data_binary(pdev, pdw, true)

/* End a data stream. */
int pdf_end_data(P1(pdf_data_writer_t *pdw));

/* Define the maximum size of a Function reference. */
#define MAX_REF_CHARS ((sizeof(long) * 8 + 2) / 3)

/* Create a Function object. */
#ifndef gs_function_DEFINED
typedef struct gs_function_s gs_function_t;
#  define gs_function_DEFINED
#endif
int pdf_function(P3(gx_device_pdf *pdev, const gs_function_t *pfn,
		    cos_value_t *pvalue));

/* Write a Function object, returning its object ID. */
int pdf_write_function(P3(gx_device_pdf *pdev, const gs_function_t *pfn,
			  long *pid));

/* ---------------- Exported by gdevpdfm.c ---------------- */

/*
 * Define the type for a pdfmark-processing procedure.
 * If nameable is false, the objname argument is always NULL.
 */
#define pdfmark_proc(proc)\
  int proc(P5(gx_device_pdf *pdev, gs_param_string *pairs, uint count,\
	      const gs_matrix *pctm, const gs_param_string *objname))

/* Compare a C string and a gs_param_string. */
bool pdf_key_eq(P2(const gs_param_string * pcs, const char *str));

/* Scan an integer out of a parameter string. */
int pdfmark_scan_int(P2(const gs_param_string * pstr, int *pvalue));

/* Process a pdfmark (called from pdf_put_params). */
int pdfmark_process(P2(gx_device_pdf * pdev, const gs_param_string_array * pma));

/* Close the current level of the outline tree. */
int pdfmark_close_outline(P1(gx_device_pdf * pdev));

/* Finish writing an article. */
int pdfmark_write_article(P2(gx_device_pdf * pdev, const pdf_article_t * part));

/* ---------------- Exported by gdevpdfr.c ---------------- */

/* Test whether an object name has valid syntax, {name}. */
bool pdf_objname_is_valid(P2(const byte *data, uint size));

/*
 * Look up a named object.  Return e_rangecheck if the syntax is invalid,
 * e_undefined if no object by that name exists.
 */
int pdf_find_named(P3(gx_device_pdf * pdev, const gs_param_string * pname,
		      cos_object_t **ppco));

/*
 * Create a named object.  id = -1L means do not assign an id.  pname = 0
 * means just create the object, do not name it.
 */
int pdf_create_named(P5(gx_device_pdf *pdev, const gs_param_string *pname,
			cos_type_t cotype, cos_object_t **ppco, long id));
int pdf_create_named_dict(P4(gx_device_pdf *pdev, const gs_param_string *pname,
			     cos_dict_t **ppcd, long id));

/*
 * Look up a named object as for pdf_find_named.  If the object does not
 * exist, create it (as a dictionary if it is one of the predefined names
 * {ThisPage}, {NextPage}, {PrevPage}, or {Page<#>}, otherwise as a
 * generic object) and return 1.
 */
int pdf_refer_named(P3(gx_device_pdf *pdev, const gs_param_string *pname,
		       cos_object_t **ppco));

/*
 * Look up a named object as for pdf_refer_named.  If the object already
 * exists and is not simply a forward reference, return e_rangecheck;
 * if it exists as a forward reference, set its type and return 0;
 * otherwise, create the object with the given type and return 1.
 * pname = 0 is allowed: in this case, simply create the object.
 */
int pdf_make_named(P5(gx_device_pdf * pdev, const gs_param_string * pname,
		      cos_type_t cotype, cos_object_t **ppco, bool assign_id));
int pdf_make_named_dict(P4(gx_device_pdf * pdev, const gs_param_string * pname,
			   cos_dict_t **ppcd, bool assign_id));

/*
 * Look up a named object as for pdf_refer_named.  If the object does not
 * exist, or is a forward reference, return e_undefined; if the object
 * exists has the wrong type, return e_typecheck.
 */
int pdf_get_named(P4(gx_device_pdf * pdev, const gs_param_string * pname,
		     cos_type_t cotype, cos_object_t **ppco));

/*
 * Scan a string for a token.  <<, >>, [, and ] are treated as tokens.
 * Return 1 if a token was scanned, 0 if we reached the end of the string,
 * or an error.  On a successful return, the token extends from *ptoken up
 * to but not including *pscan.
 */
int pdf_scan_token(P3(const byte **pscan, const byte * end,
		      const byte **ptoken));

/*
 * Scan a possibly composite token: arrays and dictionaries are treated as
 * single tokens.
 */
int pdf_scan_token_composite(P3(const byte **pscan, const byte * end,
				const byte **ptoken));

/* Replace object names with object references in a (parameter) string. */
int pdf_replace_names(P3(gx_device_pdf *pdev, const gs_param_string *from,
			 gs_param_string *to));

#endif /* gdevpdfx_INCLUDED */
