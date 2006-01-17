/* Copyright (C) 2000-2003, Ghostgum Software Pty Ltd.  All rights reserved.
  
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

/* $Id: dscparse.h,v 1.12 2003/08/14 22:28:13 ghostgum Exp $*/
/* Interface for the DSC parser. */

#ifndef dscparse_INCLUDED
#  define dscparse_INCLUDED

/* Some local types that may need modification */
typedef int GSBOOL;
typedef unsigned long GSDWORD;	/* must be at least 32 bits */
typedef unsigned int GSWORD;	/* must be at least 16 bits */

#ifndef FALSE
# define FALSE ((GSBOOL)0)
# define TRUE ((GSBOOL)(!FALSE))
#endif

/* DSC_OFFSET is an unsigned integer which holds the offset
 * from the start of a file to a particular DSC comment,
 * or the length of a file.
 * Normally it is "unsigned long" which is commonly 32 bits. 
 * Change it if you need to handle larger files.
 */
#ifndef DSC_OFFSET
# define DSC_OFFSET unsigned long
#endif
#ifndef DSC_OFFSET_FORMAT
# define DSC_OFFSET_FORMAT "lu"	/* for printf */
#endif

#ifndef dsc_private
# ifdef private
#  define dsc_private private
# else
#  define dsc_private static
# endif
#endif

#ifndef min
# define min(a,b)  ((a) < (b) ? (a) : (b))
#endif
#ifndef max
# define max(a,b)  ((a) > (b) ? (a) : (b))
#endif

/* maximum legal length of lines in a DSC compliant file */
#define DSC_LINE_LENGTH 255

/* memory for strings is allocated in chunks of this length */
#define CDSC_STRING_CHUNK 4096

/* page array is allocated in chunks of this many pages */
#define CDSC_PAGE_CHUNK 128	

/* buffer length for storing lines passed to dsc_scan_data() */
/* must be at least 2 * DSC_LINE_LENGTH */
/* We choose 8192 as twice the length passed to us by GSview */
#define CDSC_DATA_LENGTH 8192

/* Return codes from dsc_scan_data()
 *  < 0     = error
 *  >=0     = OK
 *
 *  -1      = error, usually insufficient memory.
 *  0-9     = normal
 *  10-99   = internal codes, should not be seen.
 *  100-999 = identifier of last DSC comment processed.
 */

typedef enum CDSC_RETURN_CODE_e {
  CDSC_ERROR		= -1,	/* Fatal error, usually insufficient memory */

  CDSC_OK		= 0,	/* OK, no DSC comment found */
  CDSC_NOTDSC	 	= 1,	/* Not DSC, or DSC is being ignored */

/* Any section */
  CDSC_UNKNOWNDSC	= 100,	/* DSC comment not recognised */

/* Header section */
  CDSC_PSADOBE		= 200,	/* %!PS-Adobe- */
  CDSC_BEGINCOMMENTS	= 201,	/* %%BeginComments */
  CDSC_ENDCOMMENTS	= 202,	/* %%EndComments */
  CDSC_PAGES		= 203,	/* %%Pages: */
  CDSC_CREATOR		= 204,	/* %%Creator: */
  CDSC_CREATIONDATE	= 205,	/* %%CreationDate: */
  CDSC_TITLE		= 206,	/* %%Title: */
  CDSC_FOR		= 207,	/* %%For: */
  CDSC_LANGUAGELEVEL	= 208,	/* %%LanguageLevel: */
  CDSC_BOUNDINGBOX	= 209,	/* %%BoundingBox: */
  CDSC_ORIENTATION	= 210,	/* %%Orientation: */
  CDSC_PAGEORDER	= 211,	/* %%PageOrder: */
  CDSC_DOCUMENTMEDIA	= 212,	/* %%DocumentMedia: */
  CDSC_DOCUMENTPAPERSIZES    = 213,	/* %%DocumentPaperSizes: */
  CDSC_DOCUMENTPAPERFORMS    = 214,	/* %%DocumentPaperForms: */
  CDSC_DOCUMENTPAPERCOLORS   = 215,	/* %%DocumentPaperColors: */
  CDSC_DOCUMENTPAPERWEIGHTS  = 216,	/* %%DocumentPaperWeights: */
  CDSC_DOCUMENTDATA	     = 217,	/* %%DocumentData: */
  CDSC_REQUIREMENTS	     = 218,	/* IGNORED %%Requirements: */
  CDSC_DOCUMENTNEEDEDFONTS   = 219,	/* IGNORED %%DocumentNeededFonts: */
  CDSC_DOCUMENTSUPPLIEDFONTS = 220,	/* IGNORED %%DocumentSuppliedFonts: */
  CDSC_HIRESBOUNDINGBOX	     = 221,	/* %%HiResBoundingBox: */
  CDSC_CROPBOX	     	     = 222,	/* %%CropBox: */
  CDSC_PLATEFILE     	     = 223,	/* %%PlateFile: (DCS 2.0) */
  CDSC_DOCUMENTPROCESSCOLORS = 224,	/* %%DocumentProcessColors: */
  CDSC_DOCUMENTCUSTOMCOLORS  = 225,	/* %%DocumentCustomColors: */
  CDSC_CMYKCUSTOMCOLOR       = 226,	/* %%CMYKCustomColor: */
  CDSC_RGBCUSTOMCOLOR        = 227,	/* %%RGBCustomColor: */

/* Preview section */
  CDSC_BEGINPREVIEW	= 301,	/* %%BeginPreview */
  CDSC_ENDPREVIEW	= 302,	/* %%EndPreview */

/* Defaults section */
  CDSC_BEGINDEFAULTS	= 401,	/* %%BeginDefaults */
  CDSC_ENDDEFAULTS	= 402,	/* %%EndDefaults */
/* also %%PageMedia, %%PageOrientation, %%PageBoundingBox */

/* Prolog section */
  CDSC_BEGINPROLOG	= 501,	/* %%BeginProlog */
  CDSC_ENDPROLOG	= 502,	/* %%EndProlog */
  CDSC_BEGINFONT	= 503,	/* IGNORED %%BeginFont */
  CDSC_ENDFONT		= 504,	/* IGNORED %%EndFont */
  CDSC_BEGINFEATURE	= 505,	/* IGNORED %%BeginFeature */
  CDSC_ENDFEATURE	= 506,	/* IGNORED %%EndFeature */
  CDSC_BEGINRESOURCE	= 507,	/* IGNORED %%BeginResource */
  CDSC_ENDRESOURCE	= 508,	/* IGNORED %%EndResource */
  CDSC_BEGINPROCSET	= 509,	/* IGNORED %%BeginProcSet */
  CDSC_ENDPROCSET	= 510,	/* IGNORED %%EndProcSet */

/* Setup section */
  CDSC_BEGINSETUP	= 601,	/* %%BeginSetup */
  CDSC_ENDSETUP		= 602,	/* %%EndSetup */
  CDSC_FEATURE		= 603,	/* IGNORED %%Feature: */
  CDSC_PAPERCOLOR	= 604,	/* IGNORED %%PaperColor: */
  CDSC_PAPERFORM	= 605,	/* IGNORED %%PaperForm: */
  CDSC_PAPERWEIGHT	= 606,	/* IGNORED %%PaperWeight: */
  CDSC_PAPERSIZE	= 607,	/* %%PaperSize: */
/* also %%Begin/EndFeature, %%Begin/EndResource */

/* Page section */
  CDSC_PAGE		= 700,	/* %%Page: */
  CDSC_PAGETRAILER	= 701,	/* IGNORED %%PageTrailer */
  CDSC_BEGINPAGESETUP	= 702,	/* IGNORED %%BeginPageSetup */
  CDSC_ENDPAGESETUP	= 703,	/* IGNORED %%EndPageSetup */
  CDSC_PAGEMEDIA	= 704,	/* %%PageMedia: */
/* also %%PaperColor, %%PaperForm, %%PaperWeight, %%PaperSize */
  CDSC_PAGEORIENTATION	= 705,	/* %%PageOrientation: */
  CDSC_PAGEBOUNDINGBOX	= 706,	/* %%PageBoundingBox: */
/* also %%Begin/EndFont, %%Begin/EndFeature */
/* also %%Begin/EndResource, %%Begin/EndProcSet */
  CDSC_INCLUDEFONT	= 707,	/* IGNORED %%IncludeFont: */
  CDSC_VIEWINGORIENTATION = 708, /* %%ViewingOrientation: */
  CDSC_PAGECROPBOX	= 709,	/* %%PageCropBox: */

/* Trailer section */
  CDSC_TRAILER		= 800,	/* %%Trailer */
/* also %%Pages, %%BoundingBox, %%Orientation, %%PageOrder, %%DocumentMedia */ 
/* %%Page is recognised as an error */
/* also %%DocumentNeededFonts, %%DocumentSuppliedFonts */

/* End of File */
  CDSC_EOF		= 900	/* %%EOF */
} CDSC_RETURN_CODE;


/* stored in dsc->preview */ 
typedef enum CDSC_PREVIEW_TYPE_e {
    CDSC_NOPREVIEW = 0,
    CDSC_EPSI = 1,
    CDSC_TIFF = 2,
    CDSC_WMF = 3,
    CDSC_PICT = 4
} CDSC_PREVIEW_TYPE;

/* stored in dsc->page_order */ 
typedef enum CDSC_PAGE_ORDER_e {
    CDSC_ORDER_UNKNOWN = 0,
    CDSC_ASCEND = 1,
    CDSC_DESCEND = 2,
    CDSC_SPECIAL = 3
} CDSC_PAGE_ORDER;

/* stored in dsc->page_orientation and dsc->page[pagenum-1].orientation */ 
typedef enum CDSC_ORIENTATION_ENUM_e {
    CDSC_ORIENT_UNKNOWN = 0,
    CDSC_PORTRAIT = 1,
    CDSC_LANDSCAPE = 2,
    CDSC_UPSIDEDOWN = 3,
    CDSC_SEASCAPE = 4
} CDSC_ORIENTATION_ENUM;

/* stored in dsc->document_data */
typedef enum CDSC_DOCUMENT_DATA_e {
    CDSC_DATA_UNKNOWN = 0,
    CDSC_CLEAN7BIT = 1,
    CDSC_CLEAN8BIT = 2,
    CDSC_BINARY = 3
} CDSC_DOCUMENT_DATA ;

typedef struct CDSCBBOX_S {
    int llx;
    int lly;
    int urx;
    int ury;
} CDSCBBOX;

typedef struct CDSCFBBOX_S {
    float fllx;
    float flly;
    float furx;
    float fury;
} CDSCFBBOX;

typedef struct CDSCMEDIA_S {
    const char *name;
    float width;	/* PostScript points */
    float height;
    float weight;	/* GSM */
    const char *colour;
    const char *type;
    CDSCBBOX *mediabox;	/* Used by GSview for PDF MediaBox */
} CDSCMEDIA;

#define CDSC_KNOWN_MEDIA 11
extern const CDSCMEDIA dsc_known_media[CDSC_KNOWN_MEDIA];

typedef struct CDSCCTM_S { /* used for %%ViewingOrientation */
    float xx;
    float xy;
    float yx;
    float yy;
    /* float ty; */
    /* float ty; */
} CDSCCTM;

typedef struct CDSCPAGE_S {
    int ordinal;
    const char *label;
    DSC_OFFSET begin;
    DSC_OFFSET end;
    unsigned int orientation;
    const CDSCMEDIA *media;
    CDSCBBOX *bbox;  /* PageBoundingBox, also used by GSview for PDF CropBox */
    CDSCCTM *viewing_orientation;
    /* Added 2001-10-19 */
    CDSCFBBOX *crop_box;  /* CropBox */
} CDSCPAGE;

/* binary DOS EPS header */
typedef struct CDSCDOSEPS_S {
    GSDWORD ps_begin;
    GSDWORD ps_length;
    GSDWORD wmf_begin;
    GSDWORD wmf_length;
    GSDWORD tiff_begin;
    GSDWORD tiff_length;
    GSWORD checksum;
} CDSCDOSEPS;

/* macbinary header */
typedef struct CDSCMACBIN_S {
    GSDWORD data_begin;		/* EPS */
    GSDWORD data_length;
    GSDWORD resource_begin;	/* PICT */
    GSDWORD resource_length;
} CDSCMACBIN;

/* rather than allocated every string with malloc, we allocate
 * chunks of 4k and place the (usually) short strings in these
 * chunks.
 */
typedef struct CDSCSTRING_S CDSCSTRING;
struct CDSCSTRING_S {
    unsigned int index;
    unsigned int length;
    char *data;
    CDSCSTRING *next;
};

/* Desktop Color Separations - DCS 2.0 */
typedef struct CDCS2_S CDCS2;
struct CDCS2_S {
    char *colourname;
    char *filetype;	/* Usually EPS */
    /* For multiple file DCS, location and filename will be set */
    char *location;	/* Local or NULL */
    char *filename;
    /* For single file DCS, begin will be not equals to end */
    DSC_OFFSET begin;
    DSC_OFFSET end;
    /* We maintain the separations as a linked list */
    CDCS2 *next;
};

typedef enum CDSC_COLOUR_TYPE_e {
    CDSC_COLOUR_UNKNOWN=0,
    CDSC_COLOUR_PROCESS=1,		/* %%DocumentProcessColors: */
    CDSC_COLOUR_CUSTOM=2		/* %%DocumentCustomColors: */
} CDSC_COLOUR_TYPE;

typedef enum CDSC_CUSTOM_COLOUR_e {
    CDSC_CUSTOM_COLOUR_UNKNOWN=0,
    CDSC_CUSTOM_COLOUR_RGB=1,		/* %%RGBCustomColor: */
    CDSC_CUSTOM_COLOUR_CMYK=2		/* %%CMYKCustomColor: */
} CDSC_CUSTOM_COLOUR;

typedef struct CDSCCOLOUR_S CDSCCOLOUR;
struct CDSCCOLOUR_S {
    char *name;
    CDSC_COLOUR_TYPE type;
    CDSC_CUSTOM_COLOUR custom;
    /* If custom is CDSC_CUSTOM_COLOUR_RGB, the next three are correct */
    float red;
    float green;
    float blue;
    /* If colourtype is CDSC_CUSTOM_COLOUR_CMYK, the next four are correct */
    float cyan;
    float magenta;
    float yellow;
    float black;
    /* Next colour */
    CDSCCOLOUR *next;
};

/* DSC error reporting */

typedef enum CDSC_MESSAGE_ERROR_e {
  CDSC_MESSAGE_BBOX = 0,
  CDSC_MESSAGE_EARLY_TRAILER = 1,
  CDSC_MESSAGE_EARLY_EOF = 2,
  CDSC_MESSAGE_PAGE_IN_TRAILER = 3,
  CDSC_MESSAGE_PAGE_ORDINAL = 4,
  CDSC_MESSAGE_PAGES_WRONG = 5,
  CDSC_MESSAGE_EPS_NO_BBOX = 6,
  CDSC_MESSAGE_EPS_PAGES = 7,
  CDSC_MESSAGE_NO_MEDIA = 8,
  CDSC_MESSAGE_ATEND = 9,
  CDSC_MESSAGE_DUP_COMMENT = 10,
  CDSC_MESSAGE_DUP_TRAILER = 11,
  CDSC_MESSAGE_BEGIN_END = 12,
  CDSC_MESSAGE_BAD_SECTION = 13,
  CDSC_MESSAGE_LONG_LINE = 14,
  CDSC_MESSAGE_INCORRECT_USAGE = 15
} CDSC_MESSAGE_ERROR;

/* severity */
typedef enum CDSC_MESSAGE_SEVERITY_e {
  CDSC_ERROR_INFORM	= 0,	/* Not an error */
  CDSC_ERROR_WARN	= 1,	/* Not a DSC error itself,  */
  CDSC_ERROR_ERROR	= 2	/* DSC error */
} CDSC_MESSAGE_SEVERITY;

/* response */
typedef enum CDSC_RESPONSE_e {
  CDSC_RESPONSE_OK	= 0,
  CDSC_RESPONSE_CANCEL	= 1,
  CDSC_RESPONSE_IGNORE_ALL = 2
} CDSC_RESPONSE;

extern const char * const dsc_message[];

#ifndef CDSC_TYPEDEF
#define CDSC_TYPEDEF
typedef struct CDSC_s CDSC;
#endif

struct CDSC_s {
char dummy[1024];
    /* public data */
    GSBOOL dsc;			/* TRUE if DSC comments found */
    GSBOOL ctrld;		/* TRUE if has CTRLD at start of stream */
    GSBOOL pjl;			/* TRUE if has HP PJL at start of stream */
    GSBOOL epsf;		/* TRUE if EPSF */
    GSBOOL pdf;			/* TRUE if Portable Document Format */
    unsigned int preview;	/* enum CDSC_PREVIEW_TYPE */
    char *dsc_version;	/* first line of file */
    unsigned int language_level;
    unsigned int document_data;	/* Clean7Bit, Clean8Bit, Binary */
				/* enum CDSC_DOCUMENT_DATA */
    /* DSC sections */
    DSC_OFFSET begincomments;
    DSC_OFFSET endcomments;
    DSC_OFFSET beginpreview;
    DSC_OFFSET endpreview;
    DSC_OFFSET begindefaults;
    DSC_OFFSET enddefaults;
    DSC_OFFSET beginprolog;
    DSC_OFFSET endprolog;
    DSC_OFFSET beginsetup;
    DSC_OFFSET endsetup;
    DSC_OFFSET begintrailer;
    DSC_OFFSET endtrailer;
    CDSCPAGE *page;
    unsigned int page_count;	/* number of %%Page: pages in document */
    unsigned int page_pages;	/* number of pages in document from %%Pages: */
    unsigned int page_order;	/* enum CDSC_PAGE_ORDER */
    unsigned int page_orientation;  /* the default page orientation */
				/* enum CDSC_ORIENTATION */
    CDSCCTM *viewing_orientation;
    unsigned int media_count;	/* number of media items */
    CDSCMEDIA **media;		/* the array of media */
    const CDSCMEDIA *page_media;/* the default page media */
    CDSCBBOX *bbox;		/* the document bounding box */
    CDSCBBOX *page_bbox;	/* the default page bounding box */
    CDSCDOSEPS *doseps;		/* DOS binary header */
    char *dsc_title;
    char *dsc_creator;
    char *dsc_date;
    char *dsc_for;

    unsigned int max_error;	/* highest error number that will be reported */
    const int *severity;	/* array of severity values, one per error */


    /* private data */
    void *caller_data;		/* pointer to be provided when calling */
			        /* error and debug callbacks */
    int id;			/* last DSC comment found */
    int scan_section;		/* section currently being scanned */
				/* enum CDSC_SECTION */

    DSC_OFFSET doseps_end;	/* ps_begin+ps_length, otherwise 0 */
    unsigned int page_chunk_length; /* number of pages allocated */
    DSC_OFFSET file_length;	/* length of document */
		/* If provided we try to recognise %%Trailer and %%EOF */
		/* incorrectly embedded inside document. */
		/* We will not parse DSC comments beyond this point. */
		/* Can be left set to default value of 0 */
    int skip_document;		/* recursion level of %%BeginDocument: */
    int skip_bytes;		/* #bytes to ignore from BeginData: */
				/* or DOSEPS preview section */
    int skip_lines;		/* #lines to ignore from BeginData: */
    GSBOOL skip_pjl;		/* TRUE if skip PJL until first PS comment */ 
    int begin_font_count;	/* recursion level of %%BeginFont */
    int begin_feature_count;	/* recursion level of %%BeginFeature */
    int begin_resource_count;	/* recursion level of %%BeginResource */
    int begin_procset_count;	/* recursion level of %%BeginProcSet */

    /* buffer for input */
    char data[CDSC_DATA_LENGTH];/* start of buffer */
    unsigned int data_length; 	/* length of data in buffer */
    unsigned int data_index;	/* offset to next char in buffer */
    DSC_OFFSET data_offset;	/* offset from start of document */
			       	/* to byte in data[0] */
    GSBOOL eof;			/* TRUE if there is no more data */

    /* information about DSC line */
    char *line;			/* pointer to last read DSC line */
				/* not null terminated */
    unsigned int line_length; 	/* number of characters in line */
    GSBOOL eol;			/* TRUE if dsc_line contains EOL */
    GSBOOL last_cr;		/* TRUE if last line ended in \r */
				/* check next time for \n */
    unsigned int line_count;	/* line number */
    GSBOOL long_line;		/* TRUE if found a line longer than 255 characters */
    char last_line[256];	/* previous DSC line, used for %%+ */

    /* more efficient string storage (for short strings) than malloc */
    CDSCSTRING *string_head;	/* linked list head */
    CDSCSTRING *string;		/* current list item */

    /* memory allocation routines */
    void *(*memalloc)(size_t size, void *closure_data);
    void (*memfree)(void *ptr, void *closure_data);
    void *mem_closure_data;

    /* function for printing debug messages */
    void (*debug_print_fn)(void *caller_data, const char *str);

    /* function for reporting errors in DSC comments */
    int (*dsc_error_fn)(void *caller_data, CDSC *dsc, 
	unsigned int explanation, const char *line, unsigned int line_len);

    /* public data */
    /* Added 2001-10-01 */
    CDSCFBBOX *hires_bbox;	/* the hires document bounding box */
    CDSCFBBOX *crop_box;	/* the size of the trimmed page */
    CDCS2 *dcs2;		/* Desktop Color Separations 2.0 */
    CDSCCOLOUR *colours;		/* Process and custom colours */

    /* private data */
    /* Added 2002-03-30 */
    int ref_count;

    /* public data */
    /* Added 2003-07-15 */
    CDSCMACBIN *macbin;		/* Mac Binary header */
};


/* Public functions */

/* Create and initialise DSC parser */
CDSC *dsc_init(void *caller_data);

CDSC *dsc_init_with_alloc(
    void *caller_data,
    void *(*memalloc)(size_t size, void *closure_data),
    void (*memfree)(void *ptr, void *closure_data),
    void *closure_data);

/* Free the DSC parser */
void dsc_free(CDSC *dsc);

/* Reference counting for dsc structure */
/* dsc_new is the same as dsc_init */
/* dsc_ref is called by dsc_new */
/* If dsc_unref decrements to 0, dsc_free will be called */
CDSC *dsc_new(void *caller_data);
int dsc_ref(CDSC *dsc);
int dsc_unref(CDSC *dsc);

/* Tell DSC parser how long document will be, to allow ignoring
 * of early %%Trailer and %%EOF.  This is optional.
 */
void dsc_set_length(CDSC *dsc, DSC_OFFSET len);

/* Process a buffer containing DSC comments and PostScript */
int dsc_scan_data(CDSC *dsc, const char *data, int len);

/* All data has been processed, fixup any DSC errors */
int dsc_fixup(CDSC *dsc);

/* Install error query function */
void dsc_set_error_function(CDSC *dsc, 
    int (*dsc_error_fn)(void *caller_data, CDSC *dsc, 
	unsigned int explanation, const char *line, unsigned int line_len));

/* Install print function for debug messages */
void dsc_set_debug_function(CDSC *dsc, 
	void (*debug_fn)(void *caller_data, const char *str));

/* Print a message to debug output, if provided */
void dsc_debug_print(CDSC *dsc, const char *str);

/* Given a page number, find the filename for multi-file DCS 2.0 */
const char * dsc_find_platefile(CDSC *dsc, int page);

/* Compare two strings, case insensitive */
int dsc_stricmp(const char *s, const char *t);

/* should be internal only functions, but made available to 
 * GSview for handling PDF
 */
int dsc_add_page(CDSC *dsc, int ordinal, char *label);
int dsc_add_media(CDSC *dsc, CDSCMEDIA *media);
int dsc_set_page_bbox(CDSC *dsc, unsigned int page_number, 
    int llx, int lly, int urx, int ury);

/* in dscutil.c */
void dsc_display(CDSC *dsc, void (*dfn)(void *ptr, const char *str));
#endif /* dscparse_INCLUDED */
