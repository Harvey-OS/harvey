/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

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

/* $Id: dscparse.c,v 1.14 2003/09/13 02:04:15 ghostgum Exp $ */

/*
 * This is a DSC parser, based on the DSC 3.0 spec, 
 * with a few DSC 2.1 additions for page size.
 *
 * Current limitations:
 * %%+ may be used after any comment in the comment or trailer, 
 * but is currently only supported by
 *   %%DocumentMedia
 *
 * DSC 2.1 additions (discontinued in DSC 3.0):
 * %%DocumentPaperColors: 
 * %%DocumentPaperForms: 
 * %%DocumentPaperSizes: 
 * %%DocumentPaperWeights: 
 * %%PaperColor:   (ignored)
 * %%PaperForm:    (ignored)
 * %%PaperSize: 
 * %%PaperWeight:  (ignored)
 *
 * Other additions for defaults or page section
 % %%ViewingOrientation: xx xy yx yy
*/

#include <stdio.h> /* for sprintf(), not file I/O */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXSTR 256

#include "dscparse.h"

/* Macros for comparing string literals
 * For maximum speed, the length of the second macro argument is
 * computed at compile time.
 * THE SECOND MACRO ARGUMENT MUST BE A STRING LITERAL.
 */
#define COMPARE(p, str) (strncmp((const char *)(p), (str), sizeof(str) - 1) == 0)
#define IS_DSC(line, str) (COMPARE((line), (str)))

/* Macros for comparing the first one or two characters */
#define IS_WHITE(ch) (((ch) == ' ') || ((ch) == '\t'))
#define IS_EOL(ch) (((ch) == '\r') || ((ch) == '\n'))
#define IS_WHITE_OR_EOL(ch) (IS_WHITE(ch) || IS_EOL(ch))
#define IS_BLANK(str) (IS_EOL(str[0]))
#define NOT_DSC_LINE(str) (((str)[0] != '%') || ((str)[1] != '%'))

/* Macros for document offset to start and end of line */
#define DSC_START(dsc) ((dsc)->data_offset + (dsc)->data_index - (dsc)->line_length)
#define DSC_END(dsc) ((dsc)->data_offset + (dsc)->data_index)

/* dsc_scan_SECTION() functions return one of 
 * CDSC_ERROR, CDSC_OK, CDSC_NOTDSC 
 * or one of the following
 */
/* The line should be passed on to the next section parser. */
#define CDSC_PROPAGATE 10

/* If document is DOS EPS and we haven't read 30 bytes, ask for more. */
#define CDSC_NEEDMORE 11

/* local prototypes */
dsc_private void *dsc_memalloc(CDSC *dsc, size_t size);
dsc_private void dsc_memfree(CDSC *dsc, void *ptr);
dsc_private CDSC *dsc_init2(CDSC *dsc);
dsc_private void dsc_reset(CDSC *dsc);
dsc_private void dsc_section_join(DSC_OFFSET begin, DSC_OFFSET *pend, DSC_OFFSET **pplast);
dsc_private int dsc_read_line(CDSC *dsc);
dsc_private int dsc_read_doseps(CDSC *dsc);
dsc_private int dsc_read_macbin(CDSC *dsc);
dsc_private int dsc_read_applesingle(CDSC *dsc);
dsc_private char *dsc_alloc_string(CDSC *dsc, const char *str, int len);
dsc_private char *dsc_add_line(CDSC *dsc, const char *line, unsigned int len);
dsc_private char *dsc_copy_string(char *str, unsigned int slen,
				  char *line, unsigned int len, unsigned int *offset);
dsc_private GSDWORD dsc_get_dword(const unsigned char *buf);
dsc_private GSWORD dsc_get_word(const unsigned char *buf);
dsc_private GSDWORD dsc_get_bigendian_dword(const unsigned char *buf);
dsc_private GSWORD dsc_get_bigendian_word(const unsigned char *buf);
dsc_private int dsc_get_int(const char *line, unsigned int len, unsigned int *offset);
dsc_private float dsc_get_real(const char *line, unsigned int len,
			       unsigned int *offset);
dsc_private void dsc_unknown(CDSC *dsc);
dsc_private GSBOOL dsc_is_section(char *line);
dsc_private int dsc_parse_pages(CDSC *dsc);
dsc_private int dsc_parse_bounding_box(CDSC *dsc, CDSCBBOX **pbbox, int offset);
dsc_private int dsc_parse_float_bounding_box(CDSC *dsc, CDSCFBBOX **pfbbox, int offset);
dsc_private int dsc_parse_orientation(CDSC *dsc, unsigned int *porientation,
				      int offset);
dsc_private int dsc_parse_order(CDSC *dsc);
dsc_private int dsc_parse_media(CDSC *dsc, const CDSCMEDIA **page_media);
dsc_private int dsc_parse_document_media(CDSC *dsc);
dsc_private int dsc_parse_viewing_orientation(CDSC *dsc, CDSCCTM **pctm);
dsc_private int dsc_parse_page(CDSC *dsc);
dsc_private void dsc_save_line(CDSC *dsc);
dsc_private int dsc_scan_type(CDSC *dsc);
dsc_private int dsc_scan_comments(CDSC *dsc);
dsc_private int dsc_scan_preview(CDSC *dsc);
dsc_private int dsc_scan_defaults(CDSC *dsc);
dsc_private int dsc_scan_prolog(CDSC *dsc);
dsc_private int dsc_scan_setup(CDSC *dsc);
dsc_private int dsc_scan_page(CDSC *dsc);
dsc_private int dsc_scan_trailer(CDSC *dsc);
dsc_private int dsc_error(CDSC *dsc, unsigned int explanation,
			  char *line, unsigned int line_len);
dsc_private int dsc_dcs2_fixup(CDSC *dsc);
dsc_private int dsc_parse_platefile(CDSC *dsc);
dsc_private int dsc_parse_dcs1plate(CDSC *dsc);
dsc_private CDSCCOLOUR *dsc_find_colour(CDSC *dsc, const char *colourname);
dsc_private int dsc_parse_process_colours(CDSC *dsc);
dsc_private int dsc_parse_custom_colours(CDSC *dsc);
dsc_private int dsc_parse_cmyk_custom_colour(CDSC *dsc);
dsc_private int dsc_parse_rgb_custom_colour(CDSC *dsc);

/* DSC error reporting */
dsc_private const int dsc_severity[] = {
    CDSC_ERROR_WARN,   /* CDSC_MESSAGE_BBOX */
    CDSC_ERROR_WARN,   /* CDSC_MESSAGE_EARLY_TRAILER */
    CDSC_ERROR_WARN,   /* CDSC_MESSAGE_EARLY_EOF */
    CDSC_ERROR_ERROR,  /* CDSC_MESSAGE_PAGE_IN_TRAILER */
    CDSC_ERROR_ERROR,  /* CDSC_MESSAGE_PAGE_ORDINAL */
    CDSC_ERROR_ERROR,  /* CDSC_MESSAGE_PAGES_WRONG */
    CDSC_ERROR_ERROR,  /* CDSC_MESSAGE_EPS_NO_BBOX */
    CDSC_ERROR_ERROR,  /* CDSC_MESSAGE_EPS_PAGES */
    CDSC_ERROR_WARN,   /* CDSC_MESSAGE_NO_MEDIA */
    CDSC_ERROR_WARN,   /* CDSC_MESSAGE_ATEND */
    CDSC_ERROR_INFORM, /* CDSC_MESSAGE_DUP_COMMENT */
    CDSC_ERROR_INFORM, /* CDSC_MESSAGE_DUP_TRAILER */
    CDSC_ERROR_WARN,   /* CDSC_MESSAGE_BEGIN_END */
    CDSC_ERROR_INFORM, /* CDSC_MESSAGE_BAD_SECTION */
    CDSC_ERROR_INFORM, /* CDSC_MESSAGE_LONG_LINE */
    CDSC_ERROR_WARN,   /* CDSC_MESSAGE_INCORRECT_USAGE */
    0};

#define DSC_MAX_ERROR ((sizeof(dsc_severity) / sizeof(int)) - 2)

const CDSCMEDIA dsc_known_media[CDSC_KNOWN_MEDIA] = {
    /* These sizes taken from Ghostscript gs_statd.ps */
    {"11x17", 792, 1224, 0, NULL, NULL},
    {"A3", 842, 1190, 0, NULL, NULL},
    {"A4", 595, 842, 0, NULL, NULL},
    {"A5", 421, 595, 0, NULL, NULL},
    {"B4", 709, 1002, 0, NULL, NULL}, /* ISO, but not Adobe standard */
    {"B5", 501, 709, 0, NULL, NULL},  /* ISO, but not Adobe standard */
    {"Ledger", 1224, 792, 0, NULL, NULL},
    {"Legal", 612, 1008, 0, NULL, NULL},
    {"Letter", 612, 792, 0, NULL, NULL},
    {"Note", 612, 792, 0, NULL, NULL},
    {NULL, 0, 0, 0, NULL, NULL}};

/* parser state */
enum CDSC_SCAN_SECTION {
	scan_none = 0,
	scan_comments = 1,
	scan_pre_preview = 2,
	scan_preview = 3,
	scan_pre_defaults = 4,
	scan_defaults = 5,
	scan_pre_prolog = 6,
	scan_prolog = 7,
	scan_pre_setup = 8,
	scan_setup = 9,
	scan_pre_pages = 10,
	scan_pages = 11,
	scan_pre_trailer = 12,
	scan_trailer = 13,
	scan_eof = 14
};

static const char *const dsc_scan_section_name[15] = {
    "Type", "Comments",
    "pre-Preview", "Preview",
    "pre-Defaults", "Defaults",
    "pre-Prolog", "Prolog",
    "pre-Setup", "Setup",
    "pre-Page", "Page",
    "pre-Trailer", "Trailer",
    "EOF"};

/******************************************************************/
/* Public functions                                               */
/******************************************************************/

/* constructor */
CDSC *
dsc_init(void *caller_data)
{
	CDSC *dsc = (CDSC *)malloc(sizeof(CDSC));
	if(dsc == NULL)
		return NULL;
	memset(dsc, 0, sizeof(CDSC));
	dsc->caller_data = caller_data;
	dsc->ref_count = 0;
	dsc_ref(dsc);

	return dsc_init2(dsc);
}

/* constructor, with caller supplied memalloc */
CDSC *
dsc_init_with_alloc(
    void *caller_data,
    void *(*memalloc)(size_t size, void *closure_data),
    void (*memfree)(void *ptr, void *closure_data),
    void *closure_data)
{
	CDSC *dsc = (CDSC *)memalloc(sizeof(CDSC), closure_data);
	if(dsc == NULL)
		return NULL;
	memset(dsc, 0, sizeof(CDSC));
	dsc->caller_data = caller_data;

	dsc->memalloc = memalloc;
	dsc->memfree = memfree;
	dsc->mem_closure_data = closure_data;
	dsc->ref_count = 0;
	dsc_ref(dsc);

	return dsc_init2(dsc);
}

/* destructor */
void
dsc_free(CDSC *dsc)
{
	if(dsc == NULL)
		return;
	dsc_reset(dsc);
	dsc_memfree(dsc, dsc);
}

CDSC *
dsc_new(void *caller_data)
{
	return dsc_init(caller_data);
}

int
dsc_ref(CDSC *dsc)
{
	return ++(dsc->ref_count);
}

int
dsc_unref(CDSC *dsc)
{
	if(dsc->ref_count <= 0)
		return -1;
	dsc->ref_count--;
	if(dsc->ref_count == 0) {
		dsc_free(dsc);
		return 0;
	}
	return dsc->ref_count;
}

/* Tell DSC parser how long document will be, to allow ignoring
 * of early %%Trailer and %%EOF.  This is optional.
 */
void
dsc_set_length(CDSC *dsc, DSC_OFFSET len)
{
	dsc->file_length = len;
}

/* Process a buffer containing DSC comments and PostScript */
/* Return value is < 0 for error, >=0 for OK.
 *  CDSC_ERROR
 *  CDSC_OK
 *  CDSC_NOTDSC (DSC will be ignored)
 *  other values indicate the last DSC comment read
 */
int
dsc_scan_data(CDSC *dsc, const char *data, int length)
{
	int bytes_read;
	int code = 0;

	if(dsc == NULL)
		return CDSC_ERROR;

	if(dsc->id == CDSC_NOTDSC)
		return CDSC_NOTDSC;
	dsc->id = CDSC_OK;
	if(dsc->eof)
		return CDSC_OK; /* ignore */

	if(length == 0) {
		/* EOF, so process what remains */
		dsc->eof = TRUE;
	}

	do {
		if(dsc->id == CDSC_NOTDSC)
			break;

		if(length != 0) {
			/* move existing data if needed */
			if(dsc->data_length > CDSC_DATA_LENGTH / 2) {
				memmove(dsc->data, dsc->data + dsc->data_index,
					dsc->data_length - dsc->data_index);
				dsc->data_offset += dsc->data_index;
				dsc->data_length -= dsc->data_index;
				dsc->data_index = 0;
			}
			/* append to buffer */
			bytes_read = min(length, (int)(CDSC_DATA_LENGTH - dsc->data_length));
			memcpy(dsc->data + dsc->data_length, data, bytes_read);
			dsc->data_length += bytes_read;
			data += bytes_read;
			length -= bytes_read;
		}
		if(dsc->scan_section == scan_none) {
			code = dsc_scan_type(dsc);
			if(code == CDSC_NEEDMORE) {
				/* need more characters before we can identify type */
				code = CDSC_OK;
				break;
			}
			dsc->id = code;
		}

		if(code == CDSC_NOTDSC) {
			dsc->id = CDSC_NOTDSC;
			break;
		}

		while((code = dsc_read_line(dsc)) > 0) {
			if(dsc->id == CDSC_NOTDSC)
				break;
			if(dsc->file_length &&
			   (dsc->data_offset + dsc->data_index > dsc->file_length)) {
				/* have read past end of where we need to parse. */
				return CDSC_OK; /* ignore */
			}
			if(dsc->doseps_end &&
			   (dsc->data_offset + dsc->data_index > dsc->doseps_end)) {
				/* have read past end of DOS EPS or Mac Binary 
		 * PostScript section
		 */
				return CDSC_OK; /* ignore */
			}
			if(dsc->eof)
				return CDSC_OK;
			if(dsc->skip_document)
				continue; /* embedded document */
			if(dsc->skip_lines)
				continue; /* embedded lines */
			if(IS_DSC(dsc->line, "%%BeginData:"))
				continue;
			if(IS_DSC(dsc->line, "%%BeginBinary:"))
				continue;
			if(IS_DSC(dsc->line, "%%EndDocument"))
				continue;
			if(IS_DSC(dsc->line, "%%EndData"))
				continue;
			if(IS_DSC(dsc->line, "%%EndBinary"))
				continue;

			do {
				switch(dsc->scan_section) {
				case scan_comments:
					code = dsc_scan_comments(dsc);
					break;
				case scan_pre_preview:
				case scan_preview:
					code = dsc_scan_preview(dsc);
					break;
				case scan_pre_defaults:
				case scan_defaults:
					code = dsc_scan_defaults(dsc);
					break;
				case scan_pre_prolog:
				case scan_prolog:
					code = dsc_scan_prolog(dsc);
					break;
				case scan_pre_setup:
				case scan_setup:
					code = dsc_scan_setup(dsc);
					break;
				case scan_pre_pages:
				case scan_pages:
					code = dsc_scan_page(dsc);
					break;
				case scan_pre_trailer:
				case scan_trailer:
					code = dsc_scan_trailer(dsc);
					break;
				case scan_eof:
					code = CDSC_OK;
					break;
				default:
					/* invalid state */
					code = CDSC_ERROR;
				}
				/* repeat if line is start of next section */
			} while(code == CDSC_PROPAGATE);

			/* if DOS EPS header not complete, ask for more */
			if(code == CDSC_NEEDMORE) {
				code = CDSC_OK;
				break;
			}
			if(code == CDSC_NOTDSC) {
				dsc->id = CDSC_NOTDSC;
				break;
			}
		}
	} while(length != 0);

	return (code < 0) ? code : dsc->id;
}

/* Tidy up from incorrect DSC comments */
int
dsc_fixup(CDSC *dsc)
{
	unsigned int i;
	char buf[32];
	DSC_OFFSET *last;

	if(dsc->id == CDSC_NOTDSC)
		return 0;

	/* flush last partial line */
	dsc_scan_data(dsc, NULL, 0);

	/* Fix DSC error: EOF before end of %%BeginData */
	if(dsc->eof &&
	   (dsc->skip_lines || dsc->skip_bytes || dsc->skip_document)) {
		switch(dsc->scan_section) {
		case scan_comments:
			dsc->endcomments = DSC_END(dsc);
			break;
		case scan_preview:
			dsc->endpreview = DSC_END(dsc);
			break;
		case scan_defaults:
			dsc->enddefaults = DSC_END(dsc);
			break;
		case scan_prolog:
			dsc->endprolog = DSC_END(dsc);
			break;
		case scan_setup:
			dsc->endsetup = DSC_END(dsc);
			break;
		case scan_pages:
			if(dsc->page_count)
				dsc->page[dsc->page_count - 1].end = DSC_END(dsc);
			break;
		case scan_trailer:
		case scan_eof:
			dsc->endtrailer = DSC_END(dsc);
			break;
		}
	}

	/* Fix DSC error: code between %%EndSetup and %%Page */
	if(dsc->page_count && (dsc->page[0].begin != dsc->endsetup) && (dsc->endsetup != dsc->beginsetup)) {
		dsc->endsetup = dsc->page[0].begin;
		dsc_debug_print(dsc, "Warning: code included between setup and first page\n");
	}

	/* Last page contained a false trailer, */
	/* so extend last page to start of trailer */
	if(dsc->page_count && (dsc->begintrailer != 0) &&
	   (dsc->page[dsc->page_count - 1].end != dsc->begintrailer)) {
		dsc_debug_print(dsc, "Ignoring earlier misplaced trailer\n");
		dsc_debug_print(dsc, "and extending last page to start of trailer\n");
		dsc->page[dsc->page_count - 1].end = dsc->begintrailer;
	}

	/* 
     * Join up all sections.
     * There might be extra code between them, or we might have
     * missed including the \n which followed \r.
     */
	last = &dsc->endcomments;
	dsc_section_join(dsc->beginpreview, &dsc->endpreview, &last);
	dsc_section_join(dsc->begindefaults, &dsc->enddefaults, &last);
	dsc_section_join(dsc->beginprolog, &dsc->endprolog, &last);
	dsc_section_join(dsc->beginsetup, &dsc->endsetup, &last);
	for(i = 0; i < dsc->page_count; i++)
		dsc_section_join(dsc->page[i].begin, &dsc->page[i].end, &last);
	if(dsc->begintrailer)
		*last = dsc->begintrailer;

	if((dsc->page_pages == 0) && (dsc->page_count == 1)) {
		/* don't flag an error if %%Pages absent but one %%Page found */
		/* adjust incorrect page count */
		dsc->page_pages = dsc->page_count;
	}

	/* Warnings and Errors that we can now identify */
	if((dsc->page_count != dsc->page_pages)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_PAGES_WRONG, NULL, 0);
		switch(rc) {
		case CDSC_RESPONSE_OK:
			/* adjust incorrect page count */
			dsc->page_pages = dsc->page_count;
			break;
		case CDSC_RESPONSE_CANCEL:
			break;
			;
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}

	if(dsc->epsf && (dsc->bbox == (CDSCBBOX *)NULL)) {
		/* EPS files MUST include a BoundingBox */
		int rc = dsc_error(dsc, CDSC_MESSAGE_EPS_NO_BBOX, NULL, 0);
		switch(rc) {
		case CDSC_RESPONSE_OK:
			/* Assume that it is EPS */
			break;
		case CDSC_RESPONSE_CANCEL:
			/* Is NOT an EPS file */
			dsc->epsf = FALSE;
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}

	if(dsc->epsf && ((dsc->page_count > 1) || (dsc->page_pages > 1))) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_EPS_PAGES, NULL, 0);
		switch(rc) {
		case CDSC_RESPONSE_OK:
			/* Is an EPS file */
			break;
		case CDSC_RESPONSE_CANCEL:
			/* Is NOT an EPS file */
			dsc->epsf = FALSE;
			break;
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}

	/* convert single file DSC 2.0 into multiple pages */
	dsc_dcs2_fixup(dsc);

	if((dsc->media_count == 1) && (dsc->page_media == NULL)) {
		/* if one only media was specified, and default page media */
		/* was not specified, assume that default is the only media. */
		dsc->page_media = dsc->media[0];
	}

	if((dsc->media_count != 0) && (dsc->page_media == NULL)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_NO_MEDIA, NULL, 0);
		switch(rc) {
		case CDSC_RESPONSE_OK:
			/* default media is first listed */
			dsc->page_media = dsc->media[0];
			break;
		case CDSC_RESPONSE_CANCEL:
			/* No default media */
			break;
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}

	/* make sure all pages have a label */
	for(i = 0; i < dsc->page_count; i++) {
		if(strlen(dsc->page[i].label) == 0) {
			sprintf(buf, "%d", i + 1);
			if((dsc->page[i].label = dsc_alloc_string(dsc, buf, (int)strlen(buf))) == (char *)NULL)
				return CDSC_ERROR; /* no memory */
		}
	}
	return CDSC_OK;
}

/* Install a function to be used for displaying messages about 
 * DSC errors and warnings, and to request advice from user.
 * Installing an error function is optional.
 */
void
dsc_set_error_function(CDSC *dsc,
		       int (*fn)(void *caller_data, CDSC *dsc,
				 unsigned int explanation, const char *line, unsigned int line_len))
{
	dsc->dsc_error_fn = fn;
}

/* Install a function for printing debug messages */
/* This is optional */
void
dsc_set_debug_function(CDSC *dsc,
		       void (*debug_fn)(void *caller_data, const char *str))
{
	dsc->debug_print_fn = debug_fn;
}

/* Doesn't need to be public for PostScript documents */
/* Made public so GSview can add pages when processing PDF files */
int
dsc_add_page(CDSC *dsc, int ordinal, char *label)
{
	dsc->page[dsc->page_count].ordinal = ordinal;
	dsc->page[dsc->page_count].label =
	    dsc_alloc_string(dsc, label, (int)strlen(label) + 1);
	dsc->page[dsc->page_count].begin = 0;
	dsc->page[dsc->page_count].end = 0;
	dsc->page[dsc->page_count].orientation = CDSC_ORIENT_UNKNOWN;
	dsc->page[dsc->page_count].media = NULL;
	dsc->page[dsc->page_count].bbox = NULL;
	dsc->page[dsc->page_count].viewing_orientation = NULL;
	dsc->page[dsc->page_count].crop_box = NULL;

	dsc->page_count++;
	if(dsc->page_count >= dsc->page_chunk_length) {
		CDSCPAGE *new_page = (CDSCPAGE *)dsc_memalloc(dsc,
							      (CDSC_PAGE_CHUNK + dsc->page_count) * sizeof(CDSCPAGE));
		if(new_page == NULL)
			return CDSC_ERROR; /* out of memory */
		memcpy(new_page, dsc->page,
		       dsc->page_count * sizeof(CDSCPAGE));
		dsc_memfree(dsc, dsc->page);
		dsc->page = new_page;
		dsc->page_chunk_length = CDSC_PAGE_CHUNK + dsc->page_count;
	}
	return CDSC_OK;
}

/* Doesn't need to be public for PostScript documents */
/* Made public so GSview can store PDF MediaBox */
int
dsc_add_media(CDSC *dsc, CDSCMEDIA *media)
{
	CDSCMEDIA **newmedia_array;
	CDSCMEDIA *newmedia;

	/* extend media array  */
	newmedia_array = (CDSCMEDIA **)dsc_memalloc(dsc,
						    (dsc->media_count + 1) * sizeof(CDSCMEDIA *));
	if(newmedia_array == NULL)
		return CDSC_ERROR; /* out of memory */
	if(dsc->media != NULL) {
		memcpy(newmedia_array, dsc->media,
		       dsc->media_count * sizeof(CDSCMEDIA *));
		dsc_memfree(dsc, dsc->media);
	}
	dsc->media = newmedia_array;

	/* allocate new media */
	newmedia = dsc->media[dsc->media_count] =
	    (CDSCMEDIA *)dsc_memalloc(dsc, sizeof(CDSCMEDIA));
	if(newmedia == NULL)
		return CDSC_ERROR; /* out of memory */
	newmedia->name = NULL;
	newmedia->width = 595.0;
	newmedia->height = 842.0;
	newmedia->weight = 80.0;
	newmedia->colour = NULL;
	newmedia->type = NULL;
	newmedia->mediabox = NULL;

	dsc->media_count++;

	if(media->name) {
		newmedia->name = dsc_alloc_string(dsc, media->name,
						  (int)strlen(media->name));
		if(newmedia->name == NULL)
			return CDSC_ERROR; /* no memory */
	}
	newmedia->width = media->width;
	newmedia->height = media->height;
	newmedia->weight = media->weight;
	if(media->colour) {
		newmedia->colour = dsc_alloc_string(dsc, media->colour,
						    (int)strlen(media->colour));
		if(newmedia->colour == NULL)
			return CDSC_ERROR; /* no memory */
	}
	if(media->type) {
		newmedia->type = dsc_alloc_string(dsc, media->type,
						  (int)strlen(media->type));
		if(newmedia->type == NULL)
			return CDSC_ERROR; /* no memory */
	}
	newmedia->mediabox = NULL;

	if(media->mediabox) {
		newmedia->mediabox = (CDSCBBOX *)dsc_memalloc(dsc, sizeof(CDSCBBOX));
		if(newmedia->mediabox == NULL)
			return CDSC_ERROR; /* no memory */
		*newmedia->mediabox = *media->mediabox;
	}
	return CDSC_OK;
}

/* Doesn't need to be public for PostScript documents */
/* Made public so GSview can store PDF CropBox */
int
dsc_set_page_bbox(CDSC *dsc, unsigned int page_number,
		  int llx, int lly, int urx, int ury)
{
	CDSCBBOX *bbox;
	if(page_number >= dsc->page_count)
		return CDSC_ERROR;
	bbox = dsc->page[page_number].bbox;
	if(bbox == NULL)
		dsc->page[page_number].bbox = bbox =
		    (CDSCBBOX *)dsc_memalloc(dsc, sizeof(CDSCBBOX));
	if(bbox == NULL)
		return CDSC_ERROR;
	bbox->llx = llx;
	bbox->lly = lly;
	bbox->urx = urx;
	bbox->ury = ury;
	return CDSC_OK;
}

/******************************************************************/
/* Private functions below here.                                  */
/******************************************************************/

dsc_private void *
dsc_memalloc(CDSC *dsc, size_t size)
{
	if(dsc->memalloc)
		return dsc->memalloc(size, dsc->mem_closure_data);
	return malloc(size);
}

dsc_private void
dsc_memfree(CDSC *dsc, void *ptr)
{
	if(dsc->memfree)
		dsc->memfree(ptr, dsc->mem_closure_data);
	else
		free(ptr);
}

/* private constructor */
dsc_private CDSC *
dsc_init2(CDSC *dsc)
{
	dsc_reset(dsc);

	dsc->string_head = (CDSCSTRING *)dsc_memalloc(dsc, sizeof(CDSCSTRING));
	if(dsc->string_head == NULL) {
		dsc_free(dsc);
		return NULL; /* no memory */
	}
	dsc->string = dsc->string_head;
	dsc->string->next = NULL;
	dsc->string->data = (char *)dsc_memalloc(dsc, CDSC_STRING_CHUNK);
	if(dsc->string->data == NULL) {
		dsc_free(dsc);
		return NULL; /* no memory */
	}
	dsc->string->index = 0;
	dsc->string->length = CDSC_STRING_CHUNK;

	dsc->page = (CDSCPAGE *)dsc_memalloc(dsc, CDSC_PAGE_CHUNK * sizeof(CDSCPAGE));
	if(dsc->page == NULL) {
		dsc_free(dsc);
		return NULL; /* no memory */
	}
	dsc->page_chunk_length = CDSC_PAGE_CHUNK;
	dsc->page_count = 0;

	dsc->line = NULL;
	dsc->data_length = 0;
	dsc->data_index = dsc->data_length;

	return dsc;
}

dsc_private void
dsc_reset(CDSC *dsc)
{
	unsigned int i;
	/* Clear public members */
	dsc->dsc = FALSE;
	dsc->ctrld = FALSE;
	dsc->pjl = FALSE;
	dsc->epsf = FALSE;
	dsc->pdf = FALSE;
	dsc->epsf = FALSE;
	dsc->preview = CDSC_NOPREVIEW;
	dsc->dsc_version = NULL; /* stored in dsc->string */
	dsc->language_level = 0;
	dsc->document_data = CDSC_DATA_UNKNOWN;
	dsc->begincomments = 0;
	dsc->endcomments = 0;
	dsc->beginpreview = 0;
	dsc->endpreview = 0;
	dsc->begindefaults = 0;
	dsc->enddefaults = 0;
	dsc->beginprolog = 0;
	dsc->endprolog = 0;
	dsc->beginsetup = 0;
	dsc->endsetup = 0;
	dsc->begintrailer = 0;
	dsc->endtrailer = 0;

	for(i = 0; i < dsc->page_count; i++) {
		/* page media is pointer to an element of media or dsc_known_media */
		/* do not free it. */

		if(dsc->page[i].bbox)
			dsc_memfree(dsc, dsc->page[i].bbox);
		if(dsc->page[i].viewing_orientation)
			dsc_memfree(dsc, dsc->page[i].viewing_orientation);
		if(dsc->page[i].crop_box)
			dsc_memfree(dsc, dsc->page[i].crop_box);
	}
	if(dsc->page)
		dsc_memfree(dsc, dsc->page);
	dsc->page = NULL;

	dsc->page_count = 0;
	dsc->page_pages = 0;
	dsc->page_order = CDSC_ORDER_UNKNOWN;
	dsc->page_orientation = CDSC_ORIENT_UNKNOWN;
	if(dsc->viewing_orientation)
		dsc_memfree(dsc, dsc->viewing_orientation);
	dsc->viewing_orientation = NULL;

	if(dsc->media) {
		for(i = 0; i < dsc->media_count; i++) {
			if(dsc->media[i]) {
				if(dsc->media[i]->mediabox)
					dsc_memfree(dsc, dsc->media[i]->mediabox);
				dsc_memfree(dsc, dsc->media[i]);
			}
		}
		dsc_memfree(dsc, dsc->media);
	}
	dsc->media_count = 0;
	dsc->media = NULL;

	/* page_media is pointer to an element of media or dsc_known_media */
	/* do not free it. */
	dsc->page_media = NULL;

	if(dsc->bbox)
		dsc_memfree(dsc, dsc->bbox);
	dsc->bbox = NULL;
	if(dsc->page_bbox)
		dsc_memfree(dsc, dsc->page_bbox);
	dsc->page_bbox = NULL;
	if(dsc->doseps)
		dsc_memfree(dsc, dsc->doseps);
	dsc->doseps = NULL;

	dsc->dsc_title = NULL;
	dsc->dsc_creator = NULL;
	dsc->dsc_date = NULL;
	dsc->dsc_for = NULL;

	dsc->max_error = DSC_MAX_ERROR;
	dsc->severity = dsc_severity;

	/* Clear private members */
	/* Don't touch dsc->caller_data */
	dsc->id = CDSC_OK;
	dsc->scan_section = scan_none;
	dsc->doseps_end = 0;
	dsc->page_chunk_length = 0;
	dsc->file_length = 0;
	dsc->skip_document = 0;
	dsc->skip_bytes = 0;
	dsc->skip_lines = 0;
	dsc->skip_pjl = 0;
	dsc->begin_font_count = 0;
	dsc->begin_feature_count = 0;
	dsc->begin_resource_count = 0;
	dsc->begin_procset_count = 0;

	dsc->data_length = 0;
	dsc->data_index = 0;
	dsc->data_offset = 0;

	dsc->eof = 0;

	dsc->line = 0;
	dsc->line_length = 0;
	dsc->eol = 0;
	dsc->last_cr = FALSE;
	dsc->line_count = 1;
	dsc->long_line = FALSE;
	memset(dsc->last_line, 0, sizeof(dsc->last_line));

	dsc->string = dsc->string_head;
	while(dsc->string != (CDSCSTRING *)NULL) {
		if(dsc->string->data)
			dsc_memfree(dsc, dsc->string->data);
		dsc->string_head = dsc->string;
		dsc->string = dsc->string->next;
		dsc_memfree(dsc, dsc->string_head);
	}
	dsc->string_head = NULL;
	dsc->string = NULL;

	/* don't touch caller functions */

	/* public data */
	if(dsc->hires_bbox)
		dsc_memfree(dsc, dsc->hires_bbox);
	dsc->hires_bbox = NULL;
	if(dsc->crop_box)
		dsc_memfree(dsc, dsc->crop_box);
	dsc->crop_box = NULL;

	if(dsc->dcs2) {
		CDCS2 *this_dcs, *next_dcs;
		this_dcs = dsc->dcs2;
		while(this_dcs) {
			next_dcs = this_dcs->next;
			/* strings have already been freed */
			dsc_memfree(dsc, this_dcs);
			this_dcs = next_dcs;
		}
		dsc->dcs2 = NULL;
	}
	if(dsc->colours) {
		CDSCCOLOUR *this_colour, *next_colour;
		this_colour = dsc->colours;
		while(this_colour) {
			next_colour = this_colour->next;
			/* strings have already been freed */
			dsc_memfree(dsc, this_colour);
			this_colour = next_colour;
		}
		dsc->colours = NULL;
	}

	if(dsc->macbin)
		dsc_memfree(dsc, dsc->macbin);
	dsc->macbin = NULL;
}

/* 
* Join up all sections.
* There might be extra code between them, or we might have
* missed including the \n which followed \r.
* begin is the start of this section
* pend is a pointer to the end of this section
* pplast is a pointer to a pointer of the end of the previous section
*/
dsc_private void
dsc_section_join(DSC_OFFSET begin, DSC_OFFSET *pend, DSC_OFFSET **pplast)
{
	if(begin)
		**pplast = begin;
	if(*pend > begin)
		*pplast = pend;
}

/* return value is 0 if no line available, or length of line */
dsc_private int
dsc_read_line(CDSC *dsc)
{
	char *p, *last;
	dsc->line = NULL;

	if(dsc->eof) {
		/* return all that remains, even if line incomplete */
		dsc->line = dsc->data + dsc->data_index;
		dsc->line_length = dsc->data_length - dsc->data_index;
		dsc->data_index = dsc->data_length;
		return dsc->line_length;
	}

	if(dsc->file_length &&
	   (dsc->data_offset + dsc->data_index >= dsc->file_length)) {
		/* Have read past where we need to parse. */
		/* Ignore all that remains. */
		dsc->line = dsc->data + dsc->data_index;
		dsc->line_length = dsc->data_length - dsc->data_index;
		dsc->data_index = dsc->data_length;
		return dsc->line_length;
	}
	if(dsc->doseps_end &&
	   (dsc->data_offset + dsc->data_index >= dsc->doseps_end)) {
		/* Have read past end of DOS EPS PostScript section. */
		/* Ignore all that remains. */
		dsc->line = dsc->data + dsc->data_index;
		dsc->line_length = dsc->data_length - dsc->data_index;
		dsc->data_index = dsc->data_length;
		return dsc->line_length;
	}

	/* ignore embedded bytes */
	if(dsc->skip_bytes) {
		int cnt = min(dsc->skip_bytes,
			      (int)(dsc->data_length - dsc->data_index));
		dsc->skip_bytes -= cnt;
		dsc->data_index += cnt;
		if(dsc->skip_bytes != 0)
			return 0;
	}

	do {
		dsc->line = dsc->data + dsc->data_index;
		last = dsc->data + dsc->data_length;
		if(dsc->data_index == dsc->data_length) {
			dsc->line_length = 0;
			return 0;
		}
		if(dsc->eol) {
			/* if previous line was complete, increment line count */
			dsc->line_count++;
			if(dsc->skip_lines)
				dsc->skip_lines--;
		}

		/* skip over \n which followed \r */
		if(dsc->last_cr && dsc->line[0] == '\n') {
			dsc->data_index++;
			dsc->line++;
		}
		dsc->last_cr = FALSE;

		/* look for EOL */
		dsc->eol = FALSE;
		for(p = dsc->line; p < last; p++) {
			if(*p == '\r') {
				p++;
				if((p < last) && (*p == '\n'))
					p++; /* include line feed also */
				else
					dsc->last_cr = TRUE; /* we might need to skip \n */
				dsc->eol = TRUE;	     /* dsc->line is a complete line */
				break;
			}
			if(*p == '\n') {
				p++;
				dsc->eol = TRUE; /* dsc->line is a complete line */
				break;
			}
			if(*p == '\032') { /* MS-DOS Ctrl+Z */
				dsc->eol = TRUE;
			}
		}
		if(dsc->eol == FALSE) {
			/* we haven't got a complete line yet */
			if(dsc->data_length - dsc->data_index < sizeof(dsc->data) / 2) {
				/* buffer is less than half full, ask for some more */
				dsc->line_length = 0;
				return 0;
			}
		}
		dsc->data_index += dsc->line_length = (int)(p - dsc->line);
	} while(dsc->skip_lines && dsc->line_length);

	if(dsc->line_length == 0)
		return 0;

	if((dsc->line[0] == '%') && (dsc->line[1] == '%')) {
		/* handle recursive %%BeginDocument */
		if((dsc->skip_document) && dsc->line_length &&
		   COMPARE(dsc->line, "%%EndDocument")) {
			dsc->skip_document--;
		}

		/* handle embedded lines or binary data */
		if(COMPARE(dsc->line, "%%BeginData:")) {
			/* %%BeginData: <numberof>[ <type> [ <bytesorlines> ] ] 
	     * <numberof> ::= <uint> (Lines or physical bytes) 
	     * <type> ::= Hex | Binary | ASCII (Type of data) 
	     * <bytesorlines> ::= Bytes | Lines (Read in bytes or lines) 
	     */
			char begindata[MAXSTR + 1];
			int cnt;
			const char *numberof, *bytesorlines;
			cnt = dsc->line_length;
			if(dsc->line_length > sizeof(begindata) - 1)
				cnt = sizeof(begindata) - 1;
			memcpy(begindata, dsc->line, cnt);
			begindata[cnt] = '\0';
			numberof = strtok(begindata + 12, " \r\n");
			strtok(NULL, " \r\n"); /* dump type */
			bytesorlines = strtok(NULL, " \r\n");
			if(bytesorlines == NULL)
				bytesorlines = "Bytes";

			if((numberof == NULL) || (bytesorlines == NULL)) {
				/* invalid usage of %%BeginData */
				/* ignore that we ever saw it */
				int rc = dsc_error(dsc, CDSC_MESSAGE_INCORRECT_USAGE,
						   dsc->line, dsc->line_length);
				switch(rc) {
				case CDSC_RESPONSE_OK:
				case CDSC_RESPONSE_CANCEL:
					break;
				case CDSC_RESPONSE_IGNORE_ALL:
					return 0;
				}
			} else {
				cnt = atoi(numberof);
				if(cnt) {
					if(bytesorlines && (dsc_stricmp(bytesorlines, "Lines") == 0)) {
						/* skip cnt lines */
						if(dsc->skip_lines == 0) {
							/* we are not already skipping lines */
							dsc->skip_lines = cnt + 1;
						}
					} else {
						/* byte count doesn't includes \n or \r\n  */
						/* or \r of %%BeginData: */
						/* skip cnt bytes */
						if(dsc->skip_bytes == 0) {
							/* we are not already skipping lines */
							dsc->skip_bytes = cnt;
						}
					}
				}
			}
		} else if(COMPARE(dsc->line, "%%BeginBinary:")) {
			/* byte count doesn't includes \n or \r\n or \r of %%BeginBinary:*/
			int cnt = dsc_get_int(dsc->line + 14,
					      dsc->line_length - 14, NULL);
			if(dsc->skip_bytes == 0) {
				/* we are not already skipping lines */
				dsc->skip_bytes = cnt;
			}
		}
	}

	if((dsc->line[0] == '%') && (dsc->line[1] == '%') &&
	   COMPARE(dsc->line, "%%BeginDocument:")) {
		/* Skip over embedded document, recursively */
		dsc->skip_document++;
	}

	if(!dsc->long_line && (dsc->line_length > DSC_LINE_LENGTH)) {
		dsc_error(dsc, CDSC_MESSAGE_LONG_LINE, dsc->line, dsc->line_length);
		dsc->long_line = TRUE;
	}

	return dsc->line_length;
}

/* Save last DSC line, for use with %%+ */
dsc_private void
dsc_save_line(CDSC *dsc)
{
	int len = min(sizeof(dsc->last_line), dsc->line_length);
	memcpy(dsc->last_line, dsc->line, len);
}

/* display unknown DSC line */
dsc_private void
dsc_unknown(CDSC *dsc)
{
	if(dsc->debug_print_fn) {
		char line[DSC_LINE_LENGTH];
		unsigned int length = min(DSC_LINE_LENGTH - 1, dsc->line_length);
		sprintf(line, "Unknown in %s section at line %d:\n  ",
			dsc_scan_section_name[dsc->scan_section], dsc->line_count);
		dsc_debug_print(dsc, line);
		strncpy(line, dsc->line, length);
		line[length] = '\0';
		dsc_debug_print(dsc, line);
		dsc_debug_print(dsc, "\n");
	}
}

dsc_private GSBOOL
dsc_is_section(char *line)
{
	if(!((line[0] == '%') && (line[1] == '%')))
		return FALSE;
	if(IS_DSC(line, "%%BeginPreview"))
		return TRUE;
	if(IS_DSC(line, "%%BeginDefaults"))
		return TRUE;
	if(IS_DSC(line, "%%BeginProlog"))
		return TRUE;
	if(IS_DSC(line, "%%BeginSetup"))
		return TRUE;
	if(IS_DSC(line, "%%Page:"))
		return TRUE;
	if(IS_DSC(line, "%%Trailer"))
		return TRUE;
	if(IS_DSC(line, "%%EOF"))
		return TRUE;
	return FALSE;
}

/* Get little-endian DWORD, used for DOS EPS files */
dsc_private GSDWORD
dsc_get_dword(const unsigned char *buf)
{
	GSDWORD dw;
	dw = (GSDWORD)buf[0];
	dw += ((GSDWORD)buf[1]) << 8;
	dw += ((GSDWORD)buf[2]) << 16;
	dw += ((GSDWORD)buf[3]) << 24;
	return dw;
}

dsc_private GSWORD
dsc_get_word(const unsigned char *buf)
{
	GSWORD w;
	w = (GSWORD)buf[0];
	w |= (GSWORD)(buf[1] << 8);
	return w;
}

/* Get big-endian DWORD, used for Mac Binary files */
dsc_private GSDWORD
dsc_get_bigendian_dword(const unsigned char *buf)
{
	GSDWORD dw;
	dw = (GSDWORD)buf[3];
	dw += ((GSDWORD)buf[2]) << 8;
	dw += ((GSDWORD)buf[1]) << 16;
	dw += ((GSDWORD)buf[0]) << 24;
	return dw;
}

dsc_private GSWORD
dsc_get_bigendian_word(const unsigned char *buf)
{
	GSWORD w;
	w = (GSWORD)buf[1];
	w |= (GSWORD)(buf[0] << 8);
	return w;
}

dsc_private int
dsc_read_doseps(CDSC *dsc)
{
	unsigned char *line = (unsigned char *)dsc->line;
	if((dsc->doseps = (CDSCDOSEPS *)dsc_memalloc(dsc, sizeof(CDSCDOSEPS))) == NULL)
		return CDSC_ERROR; /* no memory */

	dsc->doseps->ps_begin = dsc_get_dword(line + 4);
	dsc->doseps->ps_length = dsc_get_dword(line + 8);
	dsc->doseps->wmf_begin = dsc_get_dword(line + 12);
	dsc->doseps->wmf_length = dsc_get_dword(line + 16);
	dsc->doseps->tiff_begin = dsc_get_dword(line + 20);
	dsc->doseps->tiff_length = dsc_get_dword(line + 24);
	dsc->doseps->checksum = dsc_get_word(line + 28);

	if(dsc->file_length &&
	   (dsc->doseps->ps_begin + dsc->doseps->ps_length > dsc->file_length)) {
		/* Error in DOS EPS header.
	 * Some files have been seen with a fixed large value as 
	 * the length of the PostScript section.
	 * Correct for these erroneous files.
	 */
		dsc->doseps->ps_length =
		    (GSDWORD)(dsc->file_length - dsc->doseps->ps_begin);
	}

	dsc->doseps_end = dsc->doseps->ps_begin + dsc->doseps->ps_length;

	/* move data_index backwards to byte after doseps header */
	dsc->data_index -= dsc->line_length - 30;
	/* we haven't read a line of PostScript code yet */
	dsc->line_count = 0;
	/* skip from current position to start of PostScript section */
	dsc->skip_bytes = dsc->doseps->ps_begin - 30;

	if(dsc->doseps->tiff_begin)
		dsc->preview = CDSC_TIFF;
	if(dsc->doseps->wmf_begin)
		dsc->preview = CDSC_WMF;

	return CDSC_OK;
}

dsc_private int
dsc_read_macbin(CDSC *dsc)
{
	unsigned char *line = (unsigned char *)dsc->line;
	if((dsc->macbin =
		(CDSCMACBIN *)dsc_memalloc(dsc, sizeof(CDSCMACBIN))) == NULL)
		return CDSC_ERROR; /* no memory */

	dsc->macbin->data_begin = 128;
	dsc->macbin->data_length = dsc_get_bigendian_dword(line + 83);
	dsc->macbin->resource_begin =
	    (dsc->macbin->data_begin + dsc->macbin->data_length + 127) & ~127;
	dsc->macbin->resource_length = dsc_get_bigendian_dword(line + 87);

	if(dsc->file_length &&
	   (((dsc->macbin->resource_begin + dsc->macbin->resource_length + 127) & ~127) > dsc->file_length)) {
		return CDSC_ERROR;
	}

	dsc->doseps_end = dsc->macbin->data_begin + dsc->macbin->data_length;

	/* move data_index to byte after Mac Binary header */
	dsc->data_index -= dsc->line_length - 128;
	/* we haven't read a line of PostScript code yet */
	dsc->line_count = 0;

	dsc->preview = CDSC_PICT;

	return CDSC_OK;
}

dsc_private int
dsc_read_applesingle(CDSC *dsc)
{
	GSDWORD EntryID;
	GSDWORD Offset;
	GSDWORD Length;
	GSWORD entries;
	int index;
	int header;
	int i;

	unsigned char *line = (unsigned char *)dsc->line;
	if((dsc->macbin =
		(CDSCMACBIN *)dsc_memalloc(dsc, sizeof(CDSCMACBIN))) == NULL)
		return CDSC_ERROR; /* no memory */
	entries = dsc_get_bigendian_word(line + 24);
	for(i = 0; i < (int)entries; i++) {
		index = 26 + i * 12;
		EntryID = dsc_get_bigendian_dword(line + index);
		Offset = dsc_get_bigendian_dword(line + index + 4);
		Length = dsc_get_bigendian_dword(line + index + 8);
		if(EntryID == 1) {
			/* data fork */
			dsc->macbin->data_begin = Offset;
			dsc->macbin->data_length = Length;
		} else if(EntryID == 2) {
			/* resource fork */
			dsc->macbin->resource_begin = Offset;
			dsc->macbin->resource_length = Length;
		}
	}

	if(dsc->file_length &&
	   (dsc->macbin->resource_begin + dsc->macbin->resource_length > dsc->file_length)) {
		return CDSC_ERROR;
	}
	if(dsc->file_length &&
	   (dsc->macbin->data_begin + dsc->macbin->data_length > dsc->file_length)) {
		return CDSC_ERROR;
	}

	dsc->doseps_end = dsc->macbin->data_begin + dsc->macbin->data_length;

	header = 26 + entries * 12;
	/* move data_index to byte after AppleSingle/AppleDouble header */
	dsc->data_index -= dsc->line_length - header;
	/* we haven't read a line of PostScript code yet */
	dsc->line_count = 0;
	/* skip from current position to start of PostScript section */
	dsc->skip_bytes = dsc->macbin->data_begin - header;

	dsc->preview = CDSC_PICT;

	return CDSC_OK;
}

dsc_private int
dsc_parse_pages(CDSC *dsc)
{
	int ip, io;
	unsigned int i;
	char *p;
	int n;
	if((dsc->page_pages != 0) && (dsc->scan_section == scan_comments)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_COMMENT, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			return CDSC_OK; /* ignore duplicate comments in header */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if((dsc->page_pages != 0) && (dsc->scan_section == scan_trailer)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_TRAILER, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			break; /* use duplicate comments in header */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}

	n = IS_DSC(dsc->line, "%%+") ? 3 : 8;
	while(IS_WHITE(dsc->line[n]))
		n++;
	p = dsc->line + n;
	if(COMPARE(p, "atend")) {
		if(dsc->scan_section != scan_comments)
			dsc_unknown(dsc);
		else {
			int rc = dsc_error(dsc, CDSC_MESSAGE_ATEND,
					   dsc->line, dsc->line_length);
			switch(rc) {
			case CDSC_RESPONSE_OK:
				/* assume (atend) */
				/* we should mark it as deferred */
				break;
			case CDSC_RESPONSE_CANCEL:
				/* ignore it */
				break;
			case CDSC_RESPONSE_IGNORE_ALL:
				return CDSC_NOTDSC;
			}
		}
	} else if(COMPARE(p, "(atend)")) {
		if(dsc->scan_section != scan_comments)
			dsc_unknown(dsc);
		/* do nothing */
		/* we should mark it as deferred */
	} else {
		ip = dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
		if(i) {
			n += i;
			dsc->page_pages = ip;
			io = dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
			if(i) {
				/* DSC 2 uses extra integer to indicate page order */
				/* DSC 3 uses %%PageOrder: */
				if(dsc->page_order == CDSC_ORDER_UNKNOWN)
					switch(io) {
					case -1:
						dsc->page_order = CDSC_DESCEND;
						break;
					case 0:
						dsc->page_order = CDSC_SPECIAL;
						break;
					case 1:
						dsc->page_order = CDSC_ASCEND;
						break;
					}
			}
		} else {
			int rc = dsc_error(dsc, CDSC_MESSAGE_INCORRECT_USAGE, dsc->line,
					   dsc->line_length);
			switch(rc) {
			case CDSC_RESPONSE_OK:
			case CDSC_RESPONSE_CANCEL:
				/* ignore it */
				break;
			case CDSC_RESPONSE_IGNORE_ALL:
				return CDSC_NOTDSC;
			}
		}
	}
	return CDSC_OK;
}

dsc_private int
dsc_parse_bounding_box(CDSC *dsc, CDSCBBOX **pbbox, int offset)
{
	unsigned int i, n;
	int llx, lly, urx, ury;
	float fllx, flly, furx, fury;
	char *p;
	/* Process first %%BoundingBox: in comments, and last in trailer */
	if((*pbbox != NULL) && (dsc->scan_section == scan_comments)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_COMMENT, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			return CDSC_OK; /* ignore duplicate comments in header */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if((*pbbox != NULL) && (dsc->scan_section == scan_pages)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_COMMENT, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			return CDSC_OK; /* ignore duplicate comments in header */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if((*pbbox != NULL) && (dsc->scan_section == scan_trailer)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_TRAILER, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			break; /* use duplicate comments in trailer */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if(*pbbox != NULL) {
		dsc_memfree(dsc, *pbbox);
		*pbbox = NULL;
	}

	/* should only process first %%BoundingBox: */

	while(IS_WHITE(dsc->line[offset]))
		offset++;
	p = dsc->line + offset;

	if(COMPARE(p, "atend")) {
		if(dsc->scan_section == scan_trailer)
			dsc_unknown(dsc);
		else {
			int rc = dsc_error(dsc, CDSC_MESSAGE_ATEND, dsc->line,
					   dsc->line_length);
			switch(rc) {
			case CDSC_RESPONSE_OK:
				/* assume (atend) */
				/* we should mark it as deferred */
				break;
			case CDSC_RESPONSE_CANCEL:
				/* ignore it */
				break;
			case CDSC_RESPONSE_IGNORE_ALL:
				return CDSC_NOTDSC;
			}
		}
	} else if(COMPARE(p, "(atend)")) {
		if(dsc->scan_section == scan_trailer)
			dsc_unknown(dsc);
		/* do nothing */
		/* we should mark it as deferred */
	} else {
		/* llx = */ lly = urx = ury = 0;
		n = offset;
		llx = dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			lly = dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			urx = dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			ury = dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
		if(i) {
			*pbbox = (CDSCBBOX *)dsc_memalloc(dsc, sizeof(CDSCBBOX));
			if(*pbbox == NULL)
				return CDSC_ERROR; /* no memory */
			(*pbbox)->llx = llx;
			(*pbbox)->lly = lly;
			(*pbbox)->urx = urx;
			(*pbbox)->ury = ury;
		} else {
			int rc = dsc_error(dsc, CDSC_MESSAGE_BBOX, dsc->line,
					   dsc->line_length);
			switch(rc) {
			case CDSC_RESPONSE_OK:
				/* fllx = */ flly = furx = fury = 0.0;
				n = offset;
				n += i;
				fllx = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
				n += i;
				if(i)
					flly = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
				n += i;
				if(i)
					furx = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
				n += i;
				if(i)
					fury = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
				if(i) {
					*pbbox = (CDSCBBOX *)dsc_memalloc(dsc, sizeof(CDSCBBOX));
					if(*pbbox == NULL)
						return CDSC_ERROR; /* no memory */
					(*pbbox)->llx = (int)fllx;
					(*pbbox)->lly = (int)flly;
					(*pbbox)->urx = (int)(furx + 0.999);
					(*pbbox)->ury = (int)(fury + 0.999);
				}
				return CDSC_OK;
			case CDSC_RESPONSE_CANCEL:
				return CDSC_OK;
			case CDSC_RESPONSE_IGNORE_ALL:
				return CDSC_NOTDSC;
			}
		}
	}
	return CDSC_OK;
}

dsc_private int
dsc_parse_float_bounding_box(CDSC *dsc, CDSCFBBOX **pbbox, int offset)
{
	unsigned int i, n;
	float fllx, flly, furx, fury;
	char *p;
	/* Process first %%HiResBoundingBox: or %%CropBox: in comments, 
     * and last in trailer.
     */
	if((*pbbox != NULL) && (dsc->scan_section == scan_comments)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_COMMENT, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			return CDSC_OK; /* ignore duplicate comments in header */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if((*pbbox != NULL) && (dsc->scan_section == scan_pages)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_COMMENT, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			return CDSC_OK; /* ignore duplicate comments in header */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if((*pbbox != NULL) && (dsc->scan_section == scan_trailer)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_TRAILER, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			break; /* use duplicate comments in trailer */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if(*pbbox != NULL) {
		dsc_memfree(dsc, *pbbox);
		*pbbox = NULL;
	}

	/* should only process first %%BoundingBox: */

	while(IS_WHITE(dsc->line[offset]))
		offset++;
	p = dsc->line + offset;

	if(COMPARE(p, "atend")) {
		if(dsc->scan_section == scan_trailer)
			dsc_unknown(dsc);
		else {
			int rc = dsc_error(dsc, CDSC_MESSAGE_ATEND, dsc->line,
					   dsc->line_length);
			switch(rc) {
			case CDSC_RESPONSE_OK:
				/* assume (atend) */
				/* we should mark it as deferred */
				break;
			case CDSC_RESPONSE_CANCEL:
				/* ignore it */
				break;
			case CDSC_RESPONSE_IGNORE_ALL:
				return CDSC_NOTDSC;
			}
		}
	} else if(COMPARE(p, "(atend)")) {
		if(dsc->scan_section == scan_trailer)
			dsc_unknown(dsc);
		/* do nothing */
		/* we should mark it as deferred */
	} else {
		/* fllx = */ flly = furx = fury = 0.0;
		n = offset;
		fllx = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			flly = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			furx = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			fury = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
		if(i) {
			*pbbox = (CDSCFBBOX *)dsc_memalloc(dsc, sizeof(CDSCFBBOX));
			if(*pbbox == NULL)
				return CDSC_ERROR; /* no memory */
			(*pbbox)->fllx = fllx;
			(*pbbox)->flly = flly;
			(*pbbox)->furx = furx;
			(*pbbox)->fury = fury;
		}
	}
	return CDSC_OK;
}

dsc_private int
dsc_parse_orientation(CDSC *dsc, unsigned int *porientation, int offset)
{
	char *p;
	if((dsc->page_orientation != CDSC_ORIENT_UNKNOWN) &&
	   (dsc->scan_section == scan_comments)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_COMMENT, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			return CDSC_OK; /* ignore duplicate comments in header */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if((dsc->page_orientation != CDSC_ORIENT_UNKNOWN) &&
	   (dsc->scan_section == scan_trailer)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_TRAILER, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			break; /* use duplicate comments in header; */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	p = dsc->line + offset;
	while(IS_WHITE(*p))
		p++;
	if(COMPARE(p, "atend")) {
		if(dsc->scan_section == scan_trailer)
			dsc_unknown(dsc);
		else {
			int rc = dsc_error(dsc, CDSC_MESSAGE_ATEND,
					   dsc->line, dsc->line_length);
			switch(rc) {
			case CDSC_RESPONSE_OK:
				/* assume (atend) */
				/* we should mark it as deferred */
				break;
			case CDSC_RESPONSE_CANCEL:
				/* ignore it */
				break;
			case CDSC_RESPONSE_IGNORE_ALL:
				return CDSC_NOTDSC;
			}
		}
	} else if(COMPARE(p, "(atend)")) {
		if(dsc->scan_section == scan_trailer)
			dsc_unknown(dsc);
		/* do nothing */
		/* we should mark it as deferred */
	} else if(COMPARE(p, "Portrait")) {
		*porientation = CDSC_PORTRAIT;
	} else if(COMPARE(p, "Landscape")) {
		*porientation = CDSC_LANDSCAPE;
	} else {
		dsc_unknown(dsc);
	}
	return CDSC_OK;
}

dsc_private int
dsc_parse_order(CDSC *dsc)
{
	char *p;
	if((dsc->page_order != CDSC_ORDER_UNKNOWN) &&
	   (dsc->scan_section == scan_comments)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_COMMENT, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			return CDSC_OK; /* ignore duplicate comments in header */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}
	if((dsc->page_order != CDSC_ORDER_UNKNOWN) &&
	   (dsc->scan_section == scan_trailer)) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_DUP_TRAILER, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
		case CDSC_RESPONSE_CANCEL:
			break; /* use duplicate comments in trailer */
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}

	p = dsc->line + (IS_DSC(dsc->line, "%%+") ? 3 : 13);
	while(IS_WHITE(*p))
		p++;
	if(COMPARE(p, "atend")) {
		if(dsc->scan_section == scan_trailer)
			dsc_unknown(dsc);
		else {
			int rc = dsc_error(dsc, CDSC_MESSAGE_ATEND, dsc->line,
					   dsc->line_length);
			switch(rc) {
			case CDSC_RESPONSE_OK:
				/* assume (atend) */
				/* we should mark it as deferred */
				break;
			case CDSC_RESPONSE_CANCEL:
				/* ignore it */
				break;
			case CDSC_RESPONSE_IGNORE_ALL:
				return CDSC_NOTDSC;
			}
		}
	} else if(COMPARE(p, "(atend)")) {
		if(dsc->scan_section == scan_trailer)
			dsc_unknown(dsc);
		/* do nothing */
		/* we should mark it as deferred */
	} else if(COMPARE(p, "Ascend")) {
		dsc->page_order = CDSC_ASCEND;
	} else if(COMPARE(p, "Descend")) {
		dsc->page_order = CDSC_DESCEND;
	} else if(COMPARE(p, "Special")) {
		dsc->page_order = CDSC_SPECIAL;
	} else {
		dsc_unknown(dsc);
	}
	return CDSC_OK;
}

dsc_private int
dsc_parse_media(CDSC *dsc, const CDSCMEDIA **page_media)
{
	char media_name[MAXSTR];
	int n = IS_DSC(dsc->line, "%%+") ? 3 : 12; /* %%PageMedia: */
	unsigned int i;

	if(dsc_copy_string(media_name, sizeof(media_name) - 1,
			   dsc->line + n, dsc->line_length - n, NULL)) {
		for(i = 0; i < dsc->media_count; i++) {
			if(dsc->media[i]->name &&
			   (dsc_stricmp(media_name, dsc->media[i]->name) == 0)) {
				*page_media = dsc->media[i];
				return CDSC_OK;
			}
		}
	}
	dsc_unknown(dsc);

	return CDSC_OK;
}

dsc_private int
dsc_parse_document_media(CDSC *dsc)
{
	unsigned int i, n;
	CDSCMEDIA lmedia;
	GSBOOL blank_line;

	if(IS_DSC(dsc->line, "%%DocumentMedia:"))
		n = 16;
	else if(IS_DSC(dsc->line, "%%+"))
		n = 3;
	else
		return CDSC_ERROR; /* error */

	/* check for blank remainder of line */
	blank_line = TRUE;
	for(i = n; i < dsc->line_length; i++) {
		if(!IS_WHITE_OR_EOL(dsc->line[i])) {
			blank_line = FALSE;
			break;
		}
	}

	if(!blank_line) {
		char name[MAXSTR];
		char colour[MAXSTR];
		char type[MAXSTR];
		lmedia.name = lmedia.colour = lmedia.type = (char *)NULL;
		lmedia.width = lmedia.height = lmedia.weight = 0;
		lmedia.mediabox = (CDSCBBOX *)NULL;
		lmedia.name = dsc_copy_string(name, sizeof(name),
					      dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			lmedia.width = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			lmedia.height = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			lmedia.weight = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			lmedia.colour = dsc_copy_string(colour, sizeof(colour),
							dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			lmedia.type = dsc_copy_string(type, sizeof(type),
						      dsc->line + n, dsc->line_length - n, &i);

		if(i == 0)
			dsc_unknown(dsc); /* we didn't get all fields */
		else {
			if(dsc_add_media(dsc, &lmedia))
				return CDSC_ERROR; /* out of memory */
		}
	}
	return CDSC_OK;
}

/* viewing orientation is believed to be the first four elements of
 * a CTM matrix
 */
dsc_private int
dsc_parse_viewing_orientation(CDSC *dsc, CDSCCTM **pctm)
{
	CDSCCTM ctm;
	unsigned int i, n;

	if(*pctm != NULL) {
		dsc_memfree(dsc, *pctm);
		*pctm = NULL;
	}

	n = IS_DSC(dsc->line, "%%+") ? 3 : 21; /* %%ViewingOrientation: */
	while(IS_WHITE(dsc->line[n]))
		n++;

	/* ctm.xx = */ ctm.xy = ctm.yx = ctm.yy = 0.0;
	ctm.xx = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
	n += i;
	if(i)
		ctm.xy = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
	n += i;
	if(i)
		ctm.yx = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
	n += i;
	if(i)
		ctm.yy = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
	if(i == 0) {
		dsc_unknown(dsc); /* we didn't get all fields */
	} else {
		*pctm = (CDSCCTM *)dsc_memalloc(dsc, sizeof(CDSCCTM));
		if(*pctm == NULL)
			return CDSC_ERROR; /* no memory */
		**pctm = ctm;
	}
	return CDSC_OK;
}

/* This is called before dsc_read_line(), since we may
 * need to skip a binary header which contains a new line
 * character
 */
dsc_private int
dsc_scan_type(CDSC *dsc)
{
	unsigned char *p;
	unsigned char *line = (unsigned char *)(dsc->data + dsc->data_index);
	int length = dsc->data_length - dsc->data_index;

	/* Types that should be known:
     *   DSC
     *   EPSF
     *   PJL + any of above
     *   ^D + any of above
     *   DOS EPS
     *   PDF
     *   non-DSC
     */

	/* First process any non PostScript headers */
	/* At this stage we do not have a complete line */

	if(length == 0)
		return CDSC_NEEDMORE;

	/* If we have already found a DOS EPS header, */
	/* ignore all until the PostScript section */
	if(dsc->skip_bytes) {
		int cnt = min(dsc->skip_bytes,
			      (int)(dsc->data_length - dsc->data_index));
		dsc->skip_bytes -= cnt;
		dsc->data_index += cnt;
		length -= cnt;
		line += cnt;
		if(dsc->skip_bytes != 0)
			return CDSC_NEEDMORE;
	}

	if(dsc->skip_pjl) {
		/* skip until first PostScript comment */
		while(length >= 2) {
			while(length && !IS_EOL(line[0])) {
				/* skip until EOL character */
				line++;
				dsc->data_index++;
				length--;
			}
			while((length >= 2) && IS_EOL(line[0]) && IS_EOL(line[1])) {
				/* skip until EOL followed by non-EOL */
				line++;
				dsc->data_index++;
				length--;
			}
			if(length < 2)
				return CDSC_NEEDMORE;

			if(IS_EOL(line[0]) && line[1] == '%') {
				line++;
				dsc->data_index++;
				length--;
				dsc->skip_pjl = FALSE;
				break;
			} else {
				line++;
				dsc->data_index++;
				length--;
			}
		}
		if(dsc->skip_pjl)
			return CDSC_NEEDMORE;
	}

	if(length == 0)
		return CDSC_NEEDMORE;

	if(line[0] == '\004') {
		line++;
		dsc->data_index++;
		length--;
		dsc->ctrld = TRUE;
	}

	if(line[0] == '\033') {
		/* possibly PJL */
		if(length < 9)
			return CDSC_NEEDMORE;
		if(COMPARE(line, "\033%-12345X")) {
			dsc->skip_pjl = TRUE; /* skip until first PostScript comment */
			dsc->pjl = TRUE;
			dsc->data_index += 9;
			return dsc_scan_type(dsc);
		}
	}

	if((line[0] == 0x0) && (length < 2))
		return CDSC_NEEDMORE; /* Could be Mac Binary EPSF */
	if((line[0] == 0x0) && (line[1] >= 1) && (line[1] <= 63) && (length < 128))
		return CDSC_NEEDMORE; /* Could be Mac Binary EPSF */
	if((line[0] == 0x0) && (line[1] == 0x5) && (length < 4))
		return CDSC_NEEDMORE; /* Could be Mac AppleSingle/AppleDouble */
	if((line[0] == 0xc5) && (length < 4))
		return CDSC_NEEDMORE; /* Could be DOS EPS */

	if((line[0] == 0xc5) && (line[1] == 0xd0) &&
	   (line[2] == 0xd3) && (line[3] == 0xc6)) {
		/* id is "EPSF" with bit 7 set */
		/* read DOS EPS header, then ignore all bytes until the PS section */
		if(length < 30)
			return CDSC_NEEDMORE;
		dsc->line = (char *)line;
		if(dsc_read_doseps(dsc))
			return CDSC_ERROR;
	} else if((line[0] == 0x0) && (line[1] == 0x05) &&
		  (line[2] == 0x16) && ((line[3] == 0x0) || (line[3] == 0x07))) {
		/* Mac AppleSingle or AppleDouble */
		GSDWORD version;
		GSWORD entries;
		if(length < 26)
			return CDSC_NEEDMORE;
		version = dsc_get_bigendian_dword(line + 4);
		entries = dsc_get_bigendian_word(line + 24);
		if((version == 0x00010000) || (version == 0x00020000)) {
			if(length < (int)(26 + entries * 12))
				return CDSC_NEEDMORE;
			dsc->line = (char *)line;
			if(dsc_read_applesingle(dsc))
				return CDSC_ERROR;
		}
	} else if((line[0] == 0x0) &&
		  (line[1] >= 1) && (line[1] <= 63) &&
		  (line[74] == 0x0) &&
		  (line[65] == 'E') && (line[66] == 'P') &&
		  (line[67] == 'S') && (line[68] == 'F')) {
		/* Mac Binary EPSF */
		dsc->line = (char *)line;
		if(dsc_read_macbin(dsc))
			return CDSC_ERROR;
	} else {
		if(length < 2)
			return CDSC_NEEDMORE;
		if((line[0] == '%') && (line[1] == 'P')) {
			if(length < 5)
				return CDSC_NEEDMORE;
			if(COMPARE(line, "%PDF-")) {
				dsc->pdf = TRUE;
				dsc->scan_section = scan_comments;
				return CDSC_OK;
			}
		}
	}

	/* Finally process PostScript headers */

	if(dsc_read_line(dsc) <= 0)
		return CDSC_NEEDMORE;

	dsc->dsc_version = dsc_add_line(dsc, dsc->line, dsc->line_length);
	if(COMPARE(dsc->line, "%!PS-Adobe")) {
		dsc->dsc = TRUE;
		dsc->begincomments = DSC_START(dsc);
		if(dsc->dsc_version == NULL)
			return CDSC_ERROR; /* no memory */
		p = (unsigned char *)dsc->line + 14;
		while(IS_WHITE(*p))
			p++;
		if(COMPARE(p, "EPSF-"))
			dsc->epsf = TRUE;
		dsc->scan_section = scan_comments;
		return CDSC_PSADOBE;
	}
	if(COMPARE(dsc->line, "%!")) {
		dsc->scan_section = scan_comments;
		return CDSC_NOTDSC;
	}

	dsc->scan_section = scan_comments;
	return CDSC_NOTDSC; /* unrecognised */
}

dsc_private int
dsc_scan_comments(CDSC *dsc)
{
	/* Comments section ends at */
	/*  %%EndComments */
	/*  another section */
	/*  line that does not start with %% */
	/* Save a few important lines */

	char *line = dsc->line;
	GSBOOL continued = FALSE;
	dsc->id = CDSC_OK;
	if(IS_DSC(line, "%%EndComments")) {
		dsc->id = CDSC_ENDCOMMENTS;
		dsc->endcomments = DSC_END(dsc);
		dsc->scan_section = scan_pre_preview;
		return CDSC_OK;
	} else if(IS_DSC(line, "%%BeginComments")) {
		/* ignore because we are in this section */
		dsc->id = CDSC_BEGINCOMMENTS;
	} else if(dsc_is_section(line)) {
		dsc->endcomments = DSC_START(dsc);
		dsc->scan_section = scan_pre_preview;
		return CDSC_PROPAGATE;
	} else if(line[0] == '%' && IS_WHITE_OR_EOL(line[1])) {
		dsc->endcomments = DSC_START(dsc);
		dsc->scan_section = scan_pre_preview;
		return CDSC_PROPAGATE;
	} else if(line[0] != '%') {
		dsc->id = CDSC_OK;
		dsc->endcomments = DSC_START(dsc);
		dsc->scan_section = scan_pre_preview;
		return CDSC_PROPAGATE;
	} else if(IS_DSC(line, "%%Begin")) {
		dsc->endcomments = DSC_START(dsc);
		dsc->scan_section = scan_pre_preview;
		return CDSC_PROPAGATE;
	}

	/* Handle continuation lines.
     * To simply processing, we assume that contination lines 
     * will only occur if repeat parameters are allowed and that 
     * a complete set of these parameters appears on each line.  
     * This is more restrictive than the DSC specification, but
     * is valid for the DSC comments understood by this parser
     * for all documents that we have seen.
     */
	if(IS_DSC(line, "%%+")) {
		line = dsc->last_line;
		continued = TRUE;
	} else
		dsc_save_line(dsc);

	if(IS_DSC(line, "%%Pages:")) {
		dsc->id = CDSC_PAGES;
		if(dsc_parse_pages(dsc) != 0)
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%Creator:")) {
		dsc->id = CDSC_CREATOR;
		dsc->dsc_creator = dsc_add_line(dsc, dsc->line + 10, dsc->line_length - 10);
		if(dsc->dsc_creator == NULL)
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%CreationDate:")) {
		dsc->id = CDSC_CREATIONDATE;
		dsc->dsc_date = dsc_add_line(dsc, dsc->line + 15, dsc->line_length - 15);
		if(dsc->dsc_date == NULL)
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%Title:")) {
		dsc->id = CDSC_TITLE;
		dsc->dsc_title = dsc_add_line(dsc, dsc->line + 8, dsc->line_length - 8);
		if(dsc->dsc_title == NULL)
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%For:")) {
		dsc->id = CDSC_FOR;
		dsc->dsc_for = dsc_add_line(dsc, dsc->line + 6, dsc->line_length - 6);
		if(dsc->dsc_for == NULL)
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%LanguageLevel:")) {
		unsigned int n = continued ? 3 : 16;
		unsigned int i;
		int ll;
		dsc->id = CDSC_LANGUAGELEVEL;
		ll = dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
		if(i) {
			if((ll == 1) || (ll == 2) || (ll == 3))
				dsc->language_level = ll;
			else {
				dsc_unknown(dsc);
			}
		} else
			dsc_unknown(dsc);
	} else if(IS_DSC(line, "%%BoundingBox:")) {
		dsc->id = CDSC_BOUNDINGBOX;
		if(dsc_parse_bounding_box(dsc, &(dsc->bbox), continued ? 3 : 14))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%HiResBoundingBox:")) {
		dsc->id = CDSC_HIRESBOUNDINGBOX;
		if(dsc_parse_float_bounding_box(dsc, &(dsc->hires_bbox),
						continued ? 3 : 19))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%CropBox:")) {
		dsc->id = CDSC_CROPBOX;
		if(dsc_parse_float_bounding_box(dsc, &(dsc->crop_box),
						continued ? 3 : 10))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%Orientation:")) {
		dsc->id = CDSC_ORIENTATION;
		if(dsc_parse_orientation(dsc, &(dsc->page_orientation),
					 continued ? 3 : 14))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%PageOrder:")) {
		dsc->id = CDSC_PAGEORDER;
		if(dsc_parse_order(dsc))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%DocumentMedia:")) {
		dsc->id = CDSC_DOCUMENTMEDIA;
		if(dsc_parse_document_media(dsc))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%DocumentPaperSizes:")) {
		/* DSC 2.1 */
		unsigned int n = continued ? 3 : 21;
		unsigned int count = 0;
		unsigned int i = 1;
		char name[MAXSTR];
		char *p;
		dsc->id = CDSC_DOCUMENTPAPERSIZES;
		while(i && (dsc->line[n] != '\r') && (dsc->line[n] != '\n')) {
			p = dsc_copy_string(name, sizeof(name) - 1,
					    dsc->line + n, dsc->line_length - n, &i);
			if(i && p) {
				const CDSCMEDIA *m = dsc_known_media;
				if(count >= dsc->media_count) {
					/* set some default values */
					CDSCMEDIA lmedia;
					lmedia.name = p;
					lmedia.width = 595.0;
					lmedia.height = 842.0;
					lmedia.weight = 80.0;
					lmedia.colour = NULL;
					lmedia.type = NULL;
					lmedia.mediabox = NULL;
					if(dsc_add_media(dsc, &lmedia))
						return CDSC_ERROR;
				} else
					dsc->media[count]->name =
					    dsc_alloc_string(dsc, p, (int)strlen(p));
				/* find in list of known media */
				while(m && m->name) {
					if(dsc_stricmp(p, m->name) == 0) {
						dsc->media[count]->width = m->width;
						dsc->media[count]->height = m->height;
						break;
					}
					m++;
				}
			}
			n += i;
			count++;
		}
	} else if(IS_DSC(line, "%%DocumentPaperForms:")) {
		/* DSC 2.1 */
		unsigned int n = continued ? 3 : 21;
		unsigned int count = 0;
		unsigned int i = 1;
		char type[MAXSTR];
		char *p;
		dsc->id = CDSC_DOCUMENTPAPERFORMS;
		while(i && (dsc->line[n] != '\r') && (dsc->line[n] != '\n')) {
			p = dsc_copy_string(type, sizeof(type) - 1,
					    dsc->line + n, dsc->line_length - n, &i);
			if(i && p) {
				if(count >= dsc->media_count) {
					/* set some default values */
					CDSCMEDIA lmedia;
					lmedia.name = NULL;
					lmedia.width = 595.0;
					lmedia.height = 842.0;
					lmedia.weight = 80.0;
					lmedia.colour = NULL;
					lmedia.type = p;
					lmedia.mediabox = NULL;
					if(dsc_add_media(dsc, &lmedia))
						return CDSC_ERROR;
				} else
					dsc->media[count]->type =
					    dsc_alloc_string(dsc, p, (int)strlen(p));
			}
			n += i;
			count++;
		}
	} else if(IS_DSC(line, "%%DocumentPaperColors:")) {
		/* DSC 2.1 */
		unsigned int n = continued ? 3 : 22;
		unsigned int count = 0;
		unsigned int i = 1;
		char colour[MAXSTR];
		char *p;
		dsc->id = CDSC_DOCUMENTPAPERCOLORS;
		while(i && (dsc->line[n] != '\r') && (dsc->line[n] != '\n')) {
			p = dsc_copy_string(colour, sizeof(colour) - 1,
					    dsc->line + n, dsc->line_length - n, &i);
			if(i && p) {
				if(count >= dsc->media_count) {
					/* set some default values */
					CDSCMEDIA lmedia;
					lmedia.name = NULL;
					lmedia.width = 595.0;
					lmedia.height = 842.0;
					lmedia.weight = 80.0;
					lmedia.colour = p;
					lmedia.type = NULL;
					lmedia.mediabox = NULL;
					if(dsc_add_media(dsc, &lmedia))
						return CDSC_ERROR;
				} else
					dsc->media[count]->colour =
					    dsc_alloc_string(dsc, p, (int)strlen(p));
			}
			n += i;
			count++;
		}
	} else if(IS_DSC(line, "%%DocumentPaperWeights:")) {
		/* DSC 2.1 */
		unsigned int n = continued ? 3 : 23;
		unsigned int count = 0;
		unsigned int i = 1;
		float w;
		dsc->id = CDSC_DOCUMENTPAPERWEIGHTS;
		while(i && (dsc->line[n] != '\r') && (dsc->line[n] != '\n')) {
			w = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
			if(i) {
				if(count >= dsc->media_count) {
					/* set some default values */
					CDSCMEDIA lmedia;
					lmedia.name = NULL;
					lmedia.width = 595.0;
					lmedia.height = 842.0;
					lmedia.weight = w;
					lmedia.colour = NULL;
					lmedia.type = NULL;
					lmedia.mediabox = NULL;
					if(dsc_add_media(dsc, &lmedia))
						return CDSC_ERROR;
				} else
					dsc->media[count]->weight = w;
			}
			n += i;
			count++;
		}
	} else if(IS_DSC(line, "%%DocumentData:")) {
		unsigned int n = continued ? 3 : 15;
		char *p = dsc->line + n;
		while(IS_WHITE(*p))
			p++;
		dsc->id = CDSC_DOCUMENTDATA;
		if(COMPARE(p, "Clean7Bit"))
			dsc->document_data = CDSC_CLEAN7BIT;
		else if(COMPARE(p, "Clean8Bit"))
			dsc->document_data = CDSC_CLEAN8BIT;
		else if(COMPARE(p, "Binary"))
			dsc->document_data = CDSC_BINARY;
		else
			dsc_unknown(dsc);
	} else if(IS_DSC(line, "%%Requirements:")) {
		dsc->id = CDSC_REQUIREMENTS;
		/* ignore */
	} else if(IS_DSC(line, "%%DocumentNeededFonts:")) {
		dsc->id = CDSC_DOCUMENTNEEDEDFONTS;
		/* ignore */
	} else if(IS_DSC(line, "%%DocumentSuppliedFonts:")) {
		dsc->id = CDSC_DOCUMENTSUPPLIEDFONTS;
		/* ignore */
	} else if(IS_DSC(line, "%%PlateFile:")) {
		dsc->id = CDSC_PLATEFILE;
		if(dsc_parse_platefile(dsc) != CDSC_OK)
			dsc->id = CDSC_UNKNOWNDSC;
	} else if(IS_DSC(line, "%%CyanPlate:") ||
		  IS_DSC(line, "%%MagentaPlate:") ||
		  IS_DSC(line, "%%YellowPlate:") ||
		  IS_DSC(line, "%%BlackPlate:")) {
		dsc->id = CDSC_PLATEFILE;
		if(dsc_parse_dcs1plate(dsc) != CDSC_OK)
			dsc->id = CDSC_UNKNOWNDSC;
	} else if(IS_DSC(line, "%%DocumentProcessColors:")) {
		dsc->id = CDSC_DOCUMENTPROCESSCOLORS;
		if(dsc_parse_process_colours(dsc) != CDSC_OK)
			dsc->id = CDSC_UNKNOWNDSC;
	} else if(IS_DSC(line, "%%DocumentCustomColors:")) {
		dsc->id = CDSC_DOCUMENTCUSTOMCOLORS;
		if(dsc_parse_custom_colours(dsc) != CDSC_OK)
			dsc->id = CDSC_UNKNOWNDSC;
	} else if(IS_DSC(line, "%%CMYKCustomColor:")) {
		dsc->id = CDSC_CMYKCUSTOMCOLOR;
		if(dsc_parse_cmyk_custom_colour(dsc) != CDSC_OK)
			dsc->id = CDSC_UNKNOWNDSC;
	} else if(IS_DSC(line, "%%RGBCustomColor:")) {
		dsc->id = CDSC_RGBCUSTOMCOLOR;
		if(dsc_parse_rgb_custom_colour(dsc) != CDSC_OK)
			dsc->id = CDSC_UNKNOWNDSC;
	} else if(dsc->line[0] == '%' && IS_WHITE_OR_EOL(dsc->line[1])) {
		dsc->id = CDSC_OK;
		/* ignore */
	} else {
		dsc->id = CDSC_UNKNOWNDSC;
		dsc_unknown(dsc);
	}

	dsc->endcomments = DSC_END(dsc);
	return CDSC_OK;
}

dsc_private int
dsc_scan_preview(CDSC *dsc)
{
	/* Preview section ends at */
	/*  %%EndPreview */
	/*  another section */
	/* Preview section must start with %%BeginPreview */
	char *line = dsc->line;
	dsc->id = CDSC_OK;

	if(dsc->scan_section == scan_pre_preview) {
		if(IS_BLANK(line))
			return CDSC_OK; /* ignore blank lines before preview */
		else if(IS_DSC(line, "%%BeginPreview")) {
			dsc->id = CDSC_BEGINPREVIEW;
			dsc->beginpreview = DSC_START(dsc);
			dsc->endpreview = DSC_END(dsc);
			dsc->scan_section = scan_preview;
			/* Don't mark the preview as EPSI if a DOS EPS header is present */
			if(dsc->preview == CDSC_NOPREVIEW)
				dsc->preview = CDSC_EPSI;
			return CDSC_OK;
		} else {
			dsc->scan_section = scan_pre_defaults;
			return CDSC_PROPAGATE;
		}
	}

	if(IS_DSC(line, "%%BeginPreview")) {
		/* ignore because we are in this section */
	} else if(dsc_is_section(line)) {
		dsc->endpreview = DSC_START(dsc);
		dsc->scan_section = scan_pre_defaults;
		return CDSC_PROPAGATE;
	} else if(IS_DSC(line, "%%EndPreview")) {
		dsc->id = CDSC_ENDPREVIEW;
		dsc->endpreview = DSC_END(dsc);
		dsc->scan_section = scan_pre_defaults;
		return CDSC_OK;
	} else if(line[0] == '%' && line[1] != '%') {
		/* Ordinary comments are OK */
	} else {
		dsc->id = CDSC_UNKNOWNDSC;
		/* DSC comments should not occur in preview */
		dsc_unknown(dsc);
	}

	dsc->endpreview = DSC_END(dsc);
	return CDSC_OK;
}

dsc_private int
dsc_scan_defaults(CDSC *dsc)
{
	/* Defaults section ends at */
	/*  %%EndDefaults */
	/*  another section */
	/* Defaults section must start with %%BeginDefaults */
	char *line = dsc->line;
	dsc->id = CDSC_OK;

	if(dsc->scan_section == scan_pre_defaults) {
		if(IS_BLANK(line))
			return CDSC_OK; /* ignore blank lines before defaults */
		else if(IS_DSC(line, "%%BeginDefaults")) {
			dsc->id = CDSC_BEGINDEFAULTS;
			dsc->begindefaults = DSC_START(dsc);
			dsc->enddefaults = DSC_END(dsc);
			dsc->scan_section = scan_defaults;
			return CDSC_OK;
		} else {
			dsc->scan_section = scan_pre_prolog;
			return CDSC_PROPAGATE;
		}
	}

	if(NOT_DSC_LINE(line)) {
		/* ignore */
	} else if(IS_DSC(line, "%%BeginPreview")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginDefaults")) {
		/* ignore because we are in this section */
	} else if(dsc_is_section(line)) {
		dsc->enddefaults = DSC_START(dsc);
		dsc->scan_section = scan_pre_prolog;
		return CDSC_PROPAGATE;
	} else if(IS_DSC(line, "%%EndDefaults")) {
		dsc->id = CDSC_ENDDEFAULTS;
		dsc->enddefaults = DSC_END(dsc);
		dsc->scan_section = scan_pre_prolog;
		return CDSC_OK;
	} else if(IS_DSC(line, "%%PageMedia:")) {
		dsc->id = CDSC_PAGEMEDIA;
		dsc_parse_media(dsc, &dsc->page_media);
	} else if(IS_DSC(line, "%%PageOrientation:")) {
		dsc->id = CDSC_PAGEORIENTATION;
		/* This can override %%Orientation:  */
		if(dsc_parse_orientation(dsc, &(dsc->page_orientation), 18))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%PageBoundingBox:")) {
		dsc->id = CDSC_PAGEBOUNDINGBOX;
		if(dsc_parse_bounding_box(dsc, &(dsc->page_bbox), 18))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%ViewingOrientation:")) {
		dsc->id = CDSC_VIEWINGORIENTATION;
		if(dsc_parse_viewing_orientation(dsc, &dsc->viewing_orientation))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%PageCropBox:")) {
		dsc->id = CDSC_PAGECROPBOX;
		if(dsc_parse_float_bounding_box(dsc, &dsc->crop_box, 14))
			return CDSC_ERROR;
	} else {
		dsc->id = CDSC_UNKNOWNDSC;
		/* All other DSC comments are unknown, but not an error */
		dsc_unknown(dsc);
	}
	dsc->enddefaults = DSC_END(dsc);
	return CDSC_OK;
}

/* CDSC_RESPONSE_OK and CDSC_RESPONSE_CANCEL mean ignore the 
 * mismatch (default) */
dsc_private int
dsc_check_match_prompt(CDSC *dsc, const char *str, int count)
{
	if(count != 0) {
		char buf[MAXSTR + MAXSTR];
		if(dsc->line_length < (unsigned int)(sizeof(buf) / 2 - 1)) {
			strncpy(buf, dsc->line, dsc->line_length);
			buf[dsc->line_length] = '\0';
		}
		sprintf(buf + strlen(buf), "\n%%%%Begin%.40s: / %%%%End%.40s\n", str, str);
		return dsc_error(dsc, CDSC_MESSAGE_BEGIN_END, buf, (int)strlen(buf));
	}
	return CDSC_RESPONSE_CANCEL;
}

dsc_private int
dsc_check_match_type(CDSC *dsc, const char *str, int count)
{
	if(dsc_check_match_prompt(dsc, str, count) == CDSC_RESPONSE_IGNORE_ALL)
		return CDSC_NOTDSC;
	return CDSC_OK;
}

/* complain if Begin/End blocks didn't match */
/* return non-zero if we should ignore all DSC */
dsc_private int
dsc_check_match(CDSC *dsc)
{
	int rc = 0;
	const char *font = "Font";
	const char *feature = "Feature";
	const char *resource = "Resource";
	const char *procset = "ProcSet";

	if(!rc)
		rc = dsc_check_match_type(dsc, font, dsc->begin_font_count);
	if(!rc)
		rc = dsc_check_match_type(dsc, feature, dsc->begin_feature_count);
	if(!rc)
		rc = dsc_check_match_type(dsc, resource, dsc->begin_resource_count);
	if(!rc)
		rc = dsc_check_match_type(dsc, procset, dsc->begin_procset_count);

	dsc->begin_font_count = 0;
	dsc->begin_feature_count = 0;
	dsc->begin_resource_count = 0;
	dsc->begin_procset_count = 0;
	return rc;
}

dsc_private int
dsc_scan_prolog(CDSC *dsc)
{
	/* Prolog section ends at */
	/*  %%EndProlog */
	/*  another section */
	/* Prolog section may start with %%BeginProlog or non-dsc line */
	char *line = dsc->line;
	dsc->id = CDSC_OK;

	if(dsc->scan_section == scan_pre_prolog) {
		if(dsc_is_section(line) && (!IS_DSC(line, "%%BeginProlog"))) {
			dsc->scan_section = scan_pre_setup;
			return CDSC_PROPAGATE;
		}
		dsc->id = CDSC_BEGINPROLOG;
		dsc->beginprolog = DSC_START(dsc);
		dsc->endprolog = DSC_END(dsc);
		dsc->scan_section = scan_prolog;
		if(IS_DSC(line, "%%BeginProlog"))
			return CDSC_OK;
	}

	if(NOT_DSC_LINE(line)) {
		/* ignore */
	} else if(IS_DSC(line, "%%BeginPreview")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginDefaults")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginProlog")) {
		/* ignore because we are in this section */
	} else if(dsc_is_section(line)) {
		dsc->endprolog = DSC_START(dsc);
		dsc->scan_section = scan_pre_setup;
		if(dsc_check_match(dsc))
			return CDSC_NOTDSC;
		return CDSC_PROPAGATE;
	} else if(IS_DSC(line, "%%EndProlog")) {
		dsc->id = CDSC_ENDPROLOG;
		dsc->endprolog = DSC_END(dsc);
		dsc->scan_section = scan_pre_setup;
		if(dsc_check_match(dsc))
			return CDSC_NOTDSC;
		return CDSC_OK;
	} else if(IS_DSC(line, "%%BeginFont:")) {
		dsc->id = CDSC_BEGINFONT;
		/* ignore Begin/EndFont, apart form making sure */
		/* that they are matched. */
		dsc->begin_font_count++;
	} else if(IS_DSC(line, "%%EndFont")) {
		dsc->id = CDSC_ENDFONT;
		dsc->begin_font_count--;
	} else if(IS_DSC(line, "%%BeginFeature:")) {
		dsc->id = CDSC_BEGINFEATURE;
		/* ignore Begin/EndFeature, apart form making sure */
		/* that they are matched. */
		dsc->begin_feature_count++;
	} else if(IS_DSC(line, "%%EndFeature")) {
		dsc->id = CDSC_ENDFEATURE;
		dsc->begin_feature_count--;
	} else if(IS_DSC(line, "%%BeginResource:")) {
		dsc->id = CDSC_BEGINRESOURCE;
		/* ignore Begin/EndResource, apart form making sure */
		/* that they are matched. */
		dsc->begin_resource_count++;
	} else if(IS_DSC(line, "%%EndResource")) {
		dsc->id = CDSC_ENDRESOURCE;
		dsc->begin_resource_count--;
	} else if(IS_DSC(line, "%%BeginProcSet:")) {
		dsc->id = CDSC_BEGINPROCSET;
		/* ignore Begin/EndProcSet, apart form making sure */
		/* that they are matched. */
		dsc->begin_procset_count++;
	} else if(IS_DSC(line, "%%EndProcSet")) {
		dsc->id = CDSC_ENDPROCSET;
		dsc->begin_procset_count--;
	} else {
		/* All other DSC comments are unknown, but not an error */
		dsc->id = CDSC_UNKNOWNDSC;
		dsc_unknown(dsc);
	}

	dsc->endprolog = DSC_END(dsc);
	return CDSC_OK;
}

dsc_private int
dsc_scan_setup(CDSC *dsc)
{
	/* Setup section ends at */
	/*  %%EndSetup */
	/*  another section */
	/* Setup section must start with %%BeginSetup */

	char *line = dsc->line;
	dsc->id = CDSC_OK;

	if(dsc->scan_section == scan_pre_setup) {
		if(IS_BLANK(line))
			return CDSC_OK; /* ignore blank lines before setup */
		else if(IS_DSC(line, "%%BeginSetup")) {
			dsc->id = CDSC_BEGINSETUP;
			dsc->beginsetup = DSC_START(dsc);
			dsc->endsetup = DSC_END(dsc);
			dsc->scan_section = scan_setup;
			return CDSC_OK;
		} else {
			dsc->scan_section = scan_pre_pages;
			return CDSC_PROPAGATE;
		}
	}

	if(NOT_DSC_LINE(line)) {
		/* ignore */
	} else if(IS_DSC(line, "%%BeginPreview")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginDefaults")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginProlog")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginSetup")) {
		/* ignore because we are in this section */
	} else if(dsc_is_section(line)) {
		dsc->endsetup = DSC_START(dsc);
		dsc->scan_section = scan_pre_pages;
		if(dsc_check_match(dsc))
			return CDSC_NOTDSC;
		return CDSC_PROPAGATE;
	} else if(IS_DSC(line, "%%EndSetup")) {
		dsc->id = CDSC_ENDSETUP;
		dsc->endsetup = DSC_END(dsc);
		dsc->scan_section = scan_pre_pages;
		if(dsc_check_match(dsc))
			return CDSC_NOTDSC;
		return CDSC_OK;
	} else if(IS_DSC(line, "%%BeginFeature:")) {
		dsc->id = CDSC_BEGINFEATURE;
		/* ignore Begin/EndFeature, apart form making sure */
		/* that they are matched. */
		dsc->begin_feature_count++;
	} else if(IS_DSC(line, "%%EndFeature")) {
		dsc->id = CDSC_ENDFEATURE;
		dsc->begin_feature_count--;
	} else if(IS_DSC(line, "%%Feature:")) {
		dsc->id = CDSC_FEATURE;
		/* ignore */
	} else if(IS_DSC(line, "%%BeginResource:")) {
		dsc->id = CDSC_BEGINRESOURCE;
		/* ignore Begin/EndResource, apart form making sure */
		/* that they are matched. */
		dsc->begin_resource_count++;
	} else if(IS_DSC(line, "%%EndResource")) {
		dsc->id = CDSC_ENDRESOURCE;
		dsc->begin_resource_count--;
	} else if(IS_DSC(line, "%%PaperColor:")) {
		dsc->id = CDSC_PAPERCOLOR;
		/* ignore */
	} else if(IS_DSC(line, "%%PaperForm:")) {
		dsc->id = CDSC_PAPERFORM;
		/* ignore */
	} else if(IS_DSC(line, "%%PaperWeight:")) {
		dsc->id = CDSC_PAPERWEIGHT;
		/* ignore */
	} else if(IS_DSC(line, "%%PaperSize:")) {
		/* DSC 2.1 */
		GSBOOL found_media = FALSE;
		int i;
		int n = 12;
		char buf[MAXSTR];
		buf[0] = '\0';
		dsc->id = CDSC_PAPERSIZE;
		dsc_copy_string(buf, sizeof(buf) - 1, dsc->line + n, dsc->line_length - n,
				NULL);
		for(i = 0; i < (int)dsc->media_count; i++) {
			if(dsc->media[i] && dsc->media[i]->name &&
			   (dsc_stricmp(buf, dsc->media[i]->name) == 0)) {
				dsc->page_media = dsc->media[i];
				found_media = TRUE;
				break;
			}
		}
		if(!found_media) {
			/* It didn't match %%DocumentPaperSizes: */
			/* Try our known media */
			const CDSCMEDIA *m = dsc_known_media;
			while(m->name) {
				if(dsc_stricmp(buf, m->name) == 0) {
					dsc->page_media = m;
					break;
				}
				m++;
			}
			if(m->name == NULL)
				dsc_unknown(dsc);
		}
	} else {
		/* All other DSC comments are unknown, but not an error */
		dsc->id = CDSC_UNKNOWNDSC;
		dsc_unknown(dsc);
	}

	dsc->endsetup = DSC_END(dsc);
	return CDSC_OK;
}

dsc_private int
dsc_scan_page(CDSC *dsc)
{
	/* Page section ends at */
	/*  %%Page */
	/*  %%Trailer */
	/*  %%EOF */
	char *line = dsc->line;
	dsc->id = CDSC_OK;

	if(dsc->scan_section == scan_pre_pages) {
		if(IS_DSC(line, "%%Page:")) {
			dsc->scan_section = scan_pages;
			/* fall through */
		} else {
			/* %%Page: didn't follow %%EndSetup
	     * Keep reading until reach %%Page or %%Trailer
	     * and add it to previous section.
	     */
			DSC_OFFSET *last;
			if(dsc->endsetup != 0)
				last = &dsc->endsetup;
			else if(dsc->endprolog != 0)
				last = &dsc->endprolog;
			else if(dsc->enddefaults != 0)
				last = &dsc->enddefaults;
			else if(dsc->endpreview != 0)
				last = &dsc->endpreview;
			else if(dsc->endcomments != 0)
				last = &dsc->endcomments;
			else
				last = &dsc->begincomments;
			*last = DSC_START(dsc);
			if(IS_DSC(line, "%%Trailer") || IS_DSC(line, "%%EOF")) {
				dsc->scan_section = scan_pre_trailer;
				return CDSC_PROPAGATE;
			}
			*last = DSC_END(dsc);
			return CDSC_OK;
		}
	}

	if(NOT_DSC_LINE(line)) {
		/* ignore */
	} else if(IS_DSC(line, "%%Page:")) {
		int code;
		dsc->id = CDSC_PAGE;
		if(dsc->page_count) {
			dsc->page[dsc->page_count - 1].end = DSC_START(dsc);
			if(dsc_check_match(dsc))
				return CDSC_NOTDSC;
		}

		if((code = dsc_parse_page(dsc)) != CDSC_OK)
			return code;
		if(dsc->page_count == 0)
			dsc->scan_section = scan_pre_pages;
	} else if(IS_DSC(line, "%%BeginPreview")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginDefaults")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginProlog")) {
		/* ignore because we have already processed this section */
	} else if(IS_DSC(line, "%%BeginSetup")) {
		/* ignore because we have already processed this section */
	} else if(dsc_is_section(line)) {
		if(IS_DSC(line, "%%Trailer")) {
			if(dsc->page_count)
				dsc->page[dsc->page_count - 1].end = DSC_START(dsc);
			if(dsc->file_length) {
				if((!dsc->doseps_end &&
				    ((DSC_END(dsc) + 32768) < dsc->file_length)) ||
				   ((dsc->doseps_end) &&
				    ((DSC_END(dsc) + 32768) < dsc->doseps_end))) {
					int rc = dsc_error(dsc, CDSC_MESSAGE_EARLY_TRAILER,
							   dsc->line, dsc->line_length);
					switch(rc) {
					case CDSC_RESPONSE_OK:
						/* ignore early trailer */
						break;
					case CDSC_RESPONSE_CANCEL:
						/* this is the trailer */
						dsc->scan_section = scan_pre_trailer;
						if(dsc_check_match(dsc))
							return CDSC_NOTDSC;
						return CDSC_PROPAGATE;
					case CDSC_RESPONSE_IGNORE_ALL:
						return CDSC_NOTDSC;
					}
				} else {
					dsc->scan_section = scan_pre_trailer;
					if(dsc_check_match(dsc))
						return CDSC_NOTDSC;
					return CDSC_PROPAGATE;
				}
			} else {
				dsc->scan_section = scan_pre_trailer;
				if(dsc_check_match(dsc))
					return CDSC_NOTDSC;
				return CDSC_PROPAGATE;
			}
		} else if(IS_DSC(line, "%%EOF")) {
			if(dsc->page_count)
				dsc->page[dsc->page_count - 1].end = DSC_START(dsc);
			if(dsc->file_length) {
				if((!dsc->doseps_end &&
				    ((DSC_END(dsc) + 100) < dsc->file_length)) ||
				   ((dsc->doseps_end) &&
				    ((DSC_END(dsc) + 100) < dsc->doseps_end))) {
					int rc = dsc_error(dsc, CDSC_MESSAGE_EARLY_EOF,
							   dsc->line, dsc->line_length);
					switch(rc) {
					case CDSC_RESPONSE_OK:
						/* %%EOF is wrong, ignore it */
						break;
					case CDSC_RESPONSE_CANCEL:
						/* %%EOF is correct */
						dsc->scan_section = scan_eof;
						dsc->eof = TRUE;
						if(dsc_check_match(dsc))
							return CDSC_NOTDSC;
						return CDSC_PROPAGATE;
					case CDSC_RESPONSE_IGNORE_ALL:
						return CDSC_NOTDSC;
					}
				}
			} else {
				/* ignore it */
				if(dsc_check_match(dsc))
					return CDSC_NOTDSC;
				return CDSC_OK;
			}
		} else {
			/* Section comment, probably from a badly */
			/* encapsulated EPS file. */
			int rc = dsc_error(dsc, CDSC_MESSAGE_BAD_SECTION,
					   dsc->line, dsc->line_length);
			if(rc == CDSC_RESPONSE_IGNORE_ALL)
				return CDSC_NOTDSC;
		}
	} else if(IS_DSC(line, "%%PageTrailer")) {
		dsc->id = CDSC_PAGETRAILER;
		/* ignore */
	} else if(IS_DSC(line, "%%BeginPageSetup")) {
		dsc->id = CDSC_BEGINPAGESETUP;
		/* ignore */
	} else if(IS_DSC(line, "%%EndPageSetup")) {
		dsc->id = CDSC_ENDPAGESETUP;
		/* ignore */
	} else if(IS_DSC(line, "%%PageMedia:")) {
		dsc->id = CDSC_PAGEMEDIA;
		if(dsc->page_count)
			dsc_parse_media(dsc, &(dsc->page[dsc->page_count - 1].media));
	} else if(IS_DSC(line, "%%PaperColor:")) {
		dsc->id = CDSC_PAPERCOLOR;
		/* ignore */
	} else if(IS_DSC(line, "%%PaperForm:")) {
		dsc->id = CDSC_PAPERFORM;
		/* ignore */
	} else if(IS_DSC(line, "%%PaperWeight:")) {
		dsc->id = CDSC_PAPERWEIGHT;
		/* ignore */
	} else if(IS_DSC(line, "%%PaperSize:")) {
		/* DSC 2.1 */
		GSBOOL found_media = FALSE;
		int i;
		int n = 12;
		char buf[MAXSTR];
		buf[0] = '\0';
		dsc_copy_string(buf, sizeof(buf) - 1, dsc->line + n,
				dsc->line_length - n, NULL);
		for(i = 0; i < (int)dsc->media_count; i++) {
			if(dsc->media[i] && dsc->media[i]->name &&
			   (dsc_stricmp(buf, dsc->media[i]->name) == 0)) {
				if(dsc->page_count)
					dsc->page[dsc->page_count - 1].media = dsc->media[i];
				found_media = TRUE;
				break;
			}
		}
		if(!found_media) {
			/* It didn't match %%DocumentPaperSizes: */
			/* Try our known media */
			const CDSCMEDIA *m = dsc_known_media;
			while(m->name) {
				if(dsc_stricmp(buf, m->name) == 0) {
					if(dsc->page_count)
						dsc->page[dsc->page_count - 1].media = m;
					break;
				}
				m++;
			}
			if(m->name == NULL)
				dsc_unknown(dsc);
		}
	} else if(IS_DSC(line, "%%PageOrientation:")) {
		if(dsc->page_count) {
			dsc->id = CDSC_PAGEORIENTATION;
			if(dsc_parse_orientation(dsc,
						 &(dsc->page[dsc->page_count - 1].orientation), 18))
				return CDSC_NOTDSC;
		}
	} else if(IS_DSC(line, "%%PageBoundingBox:")) {
		if(dsc->page_count) {
			dsc->id = CDSC_PAGEBOUNDINGBOX;
			if(dsc_parse_bounding_box(dsc,
						  &dsc->page[dsc->page_count - 1].bbox, 18))
				return CDSC_NOTDSC;
		}
	} else if(IS_DSC(line, "%%ViewingOrientation:")) {
		if(dsc->page_count) {
			dsc->id = CDSC_VIEWINGORIENTATION;
			if(dsc_parse_viewing_orientation(dsc,
							 &dsc->page[dsc->page_count - 1].viewing_orientation))
				return CDSC_ERROR;
		}
	} else if(IS_DSC(line, "%%PageCropBox:")) {
		if(dsc->page_count) {
			dsc->id = CDSC_PAGECROPBOX;
			if(dsc_parse_float_bounding_box(dsc,
							&(dsc->page[dsc->page_count - 1].crop_box), 14))
				return CDSC_ERROR;
		}
	} else if(IS_DSC(line, "%%BeginFont:")) {
		dsc->id = CDSC_BEGINFONT;
		/* ignore Begin/EndFont, apart form making sure */
		/* that they are matched. */
		dsc->begin_font_count++;
	} else if(IS_DSC(line, "%%EndFont")) {
		dsc->id = CDSC_BEGINFONT;
		dsc->begin_font_count--;
	} else if(IS_DSC(line, "%%BeginFeature:")) {
		dsc->id = CDSC_BEGINFEATURE;
		/* ignore Begin/EndFeature, apart form making sure */
		/* that they are matched. */
		dsc->begin_feature_count++;
	} else if(IS_DSC(line, "%%EndFeature")) {
		dsc->id = CDSC_ENDFEATURE;
		dsc->begin_feature_count--;
	} else if(IS_DSC(line, "%%BeginResource:")) {
		dsc->id = CDSC_BEGINRESOURCE;
		/* ignore Begin/EndResource, apart form making sure */
		/* that they are matched. */
		dsc->begin_resource_count++;
	} else if(IS_DSC(line, "%%EndResource")) {
		dsc->id = CDSC_ENDRESOURCE;
		dsc->begin_resource_count--;
	} else if(IS_DSC(line, "%%BeginProcSet:")) {
		dsc->id = CDSC_BEGINPROCSET;
		/* ignore Begin/EndProcSet, apart form making sure */
		/* that they are matched. */
		dsc->begin_procset_count++;
	} else if(IS_DSC(line, "%%EndProcSet")) {
		dsc->id = CDSC_ENDPROCSET;
		dsc->begin_procset_count--;
	} else if(IS_DSC(line, "%%IncludeFont:")) {
		dsc->id = CDSC_INCLUDEFONT;
		/* ignore */
	} else {
		/* All other DSC comments are unknown, but not an error */
		dsc->id = CDSC_UNKNOWNDSC;
		dsc_unknown(dsc);
	}

	if(dsc->page_count)
		dsc->page[dsc->page_count - 1].end = DSC_END(dsc);
	return CDSC_OK;
}

/* Valid Trailer comments are
 * %%Trailer
 * %%EOF
 * or the following deferred with (atend)
 * %%BoundingBox:
 * %%DocumentCustomColors:
 * %%DocumentFiles:
 * %%DocumentFonts:
 * %%DocumentNeededFiles:
 * %%DocumentNeededFonts:
 * %%DocumentNeededProcSets:
 * %%DocumentNeededResources:
 * %%DocumentProcSets:
 * %%DocumentProcessColors:
 * %%DocumentSuppliedFiles:
 * %%DocumentSuppliedFonts:
 * %%DocumentSuppliedProcSets: 
 * %%DocumentSuppliedResources: 
 * %%Orientation: 
 * %%Pages: 
 * %%PageOrder: 
 *
 * Our supported subset is
 * %%Trailer
 * %%EOF
 * %%BoundingBox:
 * %%CropBox:
 * %%HiResBoundingBox:
 * %%DocumentCustomColors:
 * %%DocumentProcessColors:
 * %%Orientation: 
 * %%Pages: 
 * %%PageOrder: 
 * In addition to these, we support
 * %%DocumentMedia:
 * 
 * A %%PageTrailer can have the following:
 * %%PageBoundingBox: 
 * %%PageCustomColors: 
 * %%PageFiles: 
 * %%PageFonts: 
 * %%PageOrientation: 
 * %%PageProcessColors: 
 * %%PageResources: 
 */

dsc_private int
dsc_scan_trailer(CDSC *dsc)
{
	/* Trailer section start at */
	/*  %%Trailer */
	/* and ends at */
	/*  %%EOF */
	char *line = dsc->line;
	GSBOOL continued = FALSE;
	dsc->id = CDSC_OK;

	if(dsc->scan_section == scan_pre_trailer) {
		if(IS_DSC(line, "%%Trailer")) {
			dsc->id = CDSC_TRAILER;
			dsc->begintrailer = DSC_START(dsc);
			dsc->endtrailer = DSC_END(dsc);
			dsc->scan_section = scan_trailer;
			return CDSC_OK;
		} else if(IS_DSC(line, "%%EOF")) {
			dsc->id = CDSC_EOF;
			dsc->begintrailer = DSC_START(dsc);
			dsc->endtrailer = DSC_END(dsc);
			dsc->scan_section = scan_trailer;
			/* Continue, in case we found %%EOF in an embedded document */
			return CDSC_OK;
		} else {
			/* %%Page: didn't follow %%EndSetup
	     * Keep reading until reach %%Page or %%Trailer
	     * and add it to setup section
	     */
			/* append to previous section */
			if(dsc->beginsetup)
				dsc->endsetup = DSC_END(dsc);
			else if(dsc->beginprolog)
				dsc->endprolog = DSC_END(dsc);
			else {
				/* horribly confused */
			}
			return CDSC_OK;
		}
	}

	/* Handle continuation lines.
     * See comment above about our restrictive processing of 
     * continuation lines
     */
	if(IS_DSC(line, "%%+")) {
		line = dsc->last_line;
		continued = TRUE;
	} else
		dsc_save_line(dsc);

	if(NOT_DSC_LINE(line)) {
		/* ignore */
	} else if(IS_DSC(dsc->line, "%%EOF")) {
		/* Keep scanning, in case we have a false trailer */
		dsc->id = CDSC_EOF;
	} else if(IS_DSC(dsc->line, "%%Trailer")) {
		/* Cope with no pages with code after setup and before trailer. */
		/* Last trailer is the correct one. */
		dsc->id = CDSC_TRAILER;
		dsc->begintrailer = DSC_START(dsc);
	} else if(IS_DSC(line, "%%Pages:")) {
		dsc->id = CDSC_PAGES;
		if(dsc_parse_pages(dsc) != 0)
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%BoundingBox:")) {
		dsc->id = CDSC_BOUNDINGBOX;
		if(dsc_parse_bounding_box(dsc, &(dsc->bbox), continued ? 3 : 14))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%HiResBoundingBox:")) {
		dsc->id = CDSC_HIRESBOUNDINGBOX;
		if(dsc_parse_float_bounding_box(dsc, &(dsc->hires_bbox),
						continued ? 3 : 19))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%CropBox:")) {
		dsc->id = CDSC_CROPBOX;
		if(dsc_parse_float_bounding_box(dsc, &(dsc->crop_box),
						continued ? 3 : 10))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%Orientation:")) {
		dsc->id = CDSC_ORIENTATION;
		if(dsc_parse_orientation(dsc, &(dsc->page_orientation), continued ? 3 : 14))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%PageOrder:")) {
		dsc->id = CDSC_PAGEORDER;
		if(dsc_parse_order(dsc))
			return CDSC_ERROR;
	} else if(IS_DSC(line, "%%DocumentMedia:")) {
		dsc->id = CDSC_DOCUMENTMEDIA;
		if(dsc_parse_document_media(dsc))
			return CDSC_ERROR;
	} else if(IS_DSC(dsc->line, "%%Page:")) {
		/* This should not occur in the trailer, but we might see 
	 * this if a document has been incorrectly embedded.
	 */
		int rc = dsc_error(dsc, CDSC_MESSAGE_PAGE_IN_TRAILER,
				   dsc->line, dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
			/* Assume that we are really in the previous */
			/* page, not the trailer */
			dsc->scan_section = scan_pre_pages;
			if(dsc->page_count)
				dsc->page[dsc->page_count - 1].end = DSC_START(dsc);
			return CDSC_PROPAGATE; /* try again */
		case CDSC_RESPONSE_CANCEL:
			/* ignore pages in trailer */
			break;
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	} else if(IS_DSC(line, "%%DocumentNeededFonts:")) {
		dsc->id = CDSC_DOCUMENTNEEDEDFONTS;
		/* ignore */
	} else if(IS_DSC(line, "%%DocumentSuppliedFonts:")) {
		dsc->id = CDSC_DOCUMENTSUPPLIEDFONTS;
		/* ignore */
	} else if(IS_DSC(line, "%%DocumentProcessColors:")) {
		dsc->id = CDSC_DOCUMENTPROCESSCOLORS;
		if(dsc_parse_process_colours(dsc) != CDSC_OK)
			dsc->id = CDSC_UNKNOWNDSC;
	} else if(IS_DSC(line, "%%DocumentCustomColors:")) {
		dsc->id = CDSC_DOCUMENTCUSTOMCOLORS;
		if(dsc_parse_custom_colours(dsc) != CDSC_OK)
			dsc->id = CDSC_UNKNOWNDSC;
	} else {
		/* All other DSC comments are unknown, but not an error */
		dsc->id = CDSC_UNKNOWNDSC;
		dsc_unknown(dsc);
	}

	dsc->endtrailer = DSC_END(dsc);
	return CDSC_OK;
}

dsc_private char *
dsc_alloc_string(CDSC *dsc, const char *str, int len)
{
	char *p;
	if(dsc->string_head == NULL) {
		dsc->string_head = (CDSCSTRING *)dsc_memalloc(dsc, sizeof(CDSCSTRING));
		if(dsc->string_head == NULL)
			return NULL; /* no memory */
		dsc->string = dsc->string_head;
		dsc->string->next = NULL;
		dsc->string->data = (char *)dsc_memalloc(dsc, CDSC_STRING_CHUNK);
		if(dsc->string->data == NULL) {
			dsc_reset(dsc);
			return NULL; /* no memory */
		}
		dsc->string->index = 0;
		dsc->string->length = CDSC_STRING_CHUNK;
	}
	if(dsc->string->index + len + 1 > dsc->string->length) {
		/* allocate another string block */
		CDSCSTRING *newstring = (CDSCSTRING *)dsc_memalloc(dsc, sizeof(CDSCSTRING));
		if(newstring == NULL) {
			dsc_debug_print(dsc, "Out of memory\n");
			return NULL;
		}
		newstring->next = NULL;
		newstring->length = 0;
		newstring->index = 0;
		newstring->data = (char *)dsc_memalloc(dsc, CDSC_STRING_CHUNK);
		if(newstring->data == NULL) {
			dsc_memfree(dsc, newstring);
			dsc_debug_print(dsc, "Out of memory\n");
			return NULL; /* no memory */
		}
		newstring->length = CDSC_STRING_CHUNK;
		dsc->string->next = newstring;
		dsc->string = newstring;
	}
	if(dsc->string->index + len + 1 > dsc->string->length)
		return NULL; /* failed */
	p = dsc->string->data + dsc->string->index;
	memcpy(p, str, len);
	*(p + len) = '\0';
	dsc->string->index += len + 1;
	return p;
}

/* store line, ignoring leading spaces */
dsc_private char *
dsc_add_line(CDSC *dsc, const char *line, unsigned int len)
{
	char *newline;
	unsigned int i;
	while(len && (IS_WHITE(*line))) {
		len--;
		line++;
	}
	newline = dsc_alloc_string(dsc, line, len);
	if(newline == NULL)
		return NULL;

	for(i = 0; i < len; i++) {
		if(newline[i] == '\r') {
			newline[i] = '\0';
			break;
		}
		if(newline[i] == '\n') {
			newline[i] = '\0';
			break;
		}
	}
	return newline;
}

/* Copy string on line to new allocated string str */
/* String is always null terminated */
/* String is no longer than len */
/* Return pointer to string  */
/* Store number of used characters from line */
/* Don't copy enclosing () */
dsc_private char *
dsc_copy_string(char *str, unsigned int slen, char *line,
		unsigned int len, unsigned int *offset)
{
	int quoted = FALSE;
	int instring = 0;
	unsigned int newlength = 0;
	unsigned int i = 0;
	unsigned char ch;
	if(len > slen)
		len = slen - 1;
	while((i < len) && IS_WHITE(line[i]))
		i++; /* skip leading spaces */
	if((i < len) && (line[i] == '(')) {
		quoted = TRUE;
		instring++;
		i++; /* don't copy outside () */
	}
	while(i < len) {
		str[newlength] = ch = line[i];
		i++;
		if(quoted) {
			if(ch == '(')
				instring++;
			if(ch == ')')
				instring--;
			if(instring == 0)
				break;
		} else if(ch == ' ')
			break;

		if(ch == '\r')
			break;
		if(ch == '\n')
			break;
		else if((ch == '\\') && (i + 1 < len)) {
			ch = line[i];
			if((ch >= '0') && (ch <= '9')) {
				/* octal coded character */
				int j = 3;
				ch = 0;
				while(j && (i < len) && line[i] >= '0' && line[i] <= '7') {
					ch = (unsigned char)((ch << 3) + (line[i] - '0'));
					i++;
					j--;
				}
				str[newlength] = ch;
			} else if(ch == '(') {
				str[newlength] = ch;
				i++;
			} else if(ch == ')') {
				str[newlength] = ch;
				i++;
			} else if(ch == 'b') {
				str[newlength] = '\b';
				i++;
			} else if(ch == 'f') {
				str[newlength] = '\b';
				i++;
			} else if(ch == 'n') {
				str[newlength] = '\n';
				i++;
			} else if(ch == 'r') {
				str[newlength] = '\r';
				i++;
			} else if(ch == 't') {
				str[newlength] = '\t';
				i++;
			} else if(ch == '\\') {
				str[newlength] = '\\';
				i++;
			}
		}
		newlength++;
	}
	str[newlength] = '\0';
	if(offset != (unsigned int *)NULL)
		*offset = i;
	return str;
}

dsc_private int
dsc_get_int(const char *line, unsigned int len, unsigned int *offset)
{
	char newline[MAXSTR];
	int newlength = 0;
	unsigned int i = 0;
	unsigned char ch;

	len = min(len, sizeof(newline) - 1);
	while((i < len) && IS_WHITE(line[i]))
		i++; /* skip leading spaces */
	while(i < len) {
		newline[newlength] = ch = line[i];
		if(!(isdigit(ch) || (ch == '-') || (ch == '+')))
			break; /* not part of an integer number */
		i++;
		newlength++;
	}
	while((i < len) && IS_WHITE(line[i]))
		i++; /* skip trailing spaces */
	newline[newlength] = '\0';
	if(offset != (unsigned int *)NULL)
		*offset = i;
	return atoi(newline);
}

dsc_private float
dsc_get_real(const char *line, unsigned int len, unsigned int *offset)
{
	char newline[MAXSTR];
	int newlength = 0;
	unsigned int i = 0;
	unsigned char ch;

	len = min(len, sizeof(newline) - 1);
	while((i < len) && IS_WHITE(line[i]))
		i++; /* skip leading spaces */
	while(i < len) {
		newline[newlength] = ch = line[i];
		if(!(isdigit(ch) || (ch == '.') || (ch == '-') || (ch == '+') || (ch == 'e') || (ch == 'E')))
			break; /* not part of a real number */
		i++;
		newlength++;
	}
	while((i < len) && IS_WHITE(line[i]))
		i++; /* skip trailing spaces */

	newline[newlength] = '\0';

	if(offset != (unsigned int *)NULL)
		*offset = i;
	return (float)atof(newline);
}

int
dsc_stricmp(const char *s, const char *t)
{
	while(toupper(*s) == toupper(*t)) {
		if(*s == '\0')
			return 0;
		s++;
		t++;
	}
	return (toupper(*s) - toupper(*t));
}

dsc_private int
dsc_parse_page(CDSC *dsc)
{
	char *p;
	unsigned int i;
	char page_label[MAXSTR];
	char *pl;
	int page_ordinal;
	int page_number;

	p = dsc->line + 7;
	pl = dsc_copy_string(page_label, sizeof(page_label), p, dsc->line_length - 7, &i);
	if(pl == NULL)
		return CDSC_ERROR;
	p += i;
	if(dsc->line_length - 7 - i == 0) {
		/* Ordinal missing, or parentheses not matched in label */
		/* Try to find ordinal at end of line */
		while(i > 0) {
			if(!IS_WHITE_OR_EOL(p[-1]))
				break;
			p--;
			i--;
		}
		while(i > 0) {
			if(!isdigit((int)p[-1]))
				break;
			p--;
			i--;
		}
	}
	page_ordinal = dsc_get_int(p, dsc->line_length - 7 - i, NULL);

	if((page_ordinal == 0) || (strlen(page_label) == 0) ||
	   (dsc->page_count &&
	    (page_ordinal != dsc->page[dsc->page_count - 1].ordinal + 1))) {
		int rc = dsc_error(dsc, CDSC_MESSAGE_PAGE_ORDINAL, dsc->line,
				   dsc->line_length);
		switch(rc) {
		case CDSC_RESPONSE_OK:
			/* ignore this page */
			return CDSC_OK;
		case CDSC_RESPONSE_CANCEL:
			/* accept the page */
			break;
		case CDSC_RESPONSE_IGNORE_ALL:
			return CDSC_NOTDSC;
		}
	}

	page_number = dsc->page_count;
	dsc_add_page(dsc, page_ordinal, page_label);
	dsc->page[page_number].begin = DSC_START(dsc);
	dsc->page[page_number].end = DSC_START(dsc);

	if(dsc->page[page_number].label == NULL)
		return CDSC_ERROR; /* no memory */

	return CDSC_OK;
}

/* DSC error reporting */

void
dsc_debug_print(CDSC *dsc, const char *str)
{
	if(dsc->debug_print_fn)
		dsc->debug_print_fn(dsc->caller_data, str);
}

/* Display a message about a problem with the DSC comments.
 * 
 * explanation = an index to to a multiline explanation in dsc_message[]
 * line = pointer to the offending DSC line (if any)
 * return code = 
 *   CDSC_RESPONSE_OK 	       DSC was wrong, make a guess about what 
 *                             was really meant.
 *   CDSC_RESPONSE_CANCEL      Assume DSC was correct, ignore if it 
 *                             is misplaced.
 *   CDSC_RESPONSE_IGNORE_ALL  Ignore all DSC.
 */
/* Silent operation.  Don't display errors. */
dsc_private int
dsc_error(CDSC *dsc, unsigned int explanation,
	  char *line, unsigned int line_len)
{
	/* if error function provided, use it */
	if(dsc->dsc_error_fn)
		return dsc->dsc_error_fn(dsc->caller_data, dsc,
					 explanation, line, line_len);

	/* treat DSC as being correct */
	return CDSC_RESPONSE_CANCEL;
}

/* Fixup if DCS 2.0 was used */
dsc_private int
dsc_dcs2_fixup(CDSC *dsc)
{
	char composite[] = "Composite";
	/* If DCS 2.0 single file format found, expose the separations
     * as multiple pages.  Treat the initial EPS file as a single
     * page without comments, prolog or trailer.
     */
	if(dsc->dcs2) {
		int code = CDSC_OK;
		int page_number;
		DSC_OFFSET *pbegin;
		DSC_OFFSET *pend;
		DSC_OFFSET end;
		CDCS2 *pdcs = dsc->dcs2;
		/* Now treat the initial EPS file as a single page without
	 * headers or trailer, so page extraction will fetch the
	 * the correct separation. */
		if(dsc->page_count == 0)
			code = dsc_add_page(dsc, 1, composite);
		else if(dsc->page_count == 1)
			dsc->page[0].label =
			    dsc_alloc_string(dsc, composite, (int)strlen(composite) + 1);
		if(code != CDSC_OK)
			return code;
		page_number = dsc->page_count - 1;
		pbegin = &dsc->page[page_number].begin;
		pend = &dsc->page[page_number].end;
		if(*pbegin == *pend) {
			/* no page, so force it to conform to the following sections */
			*pbegin = 999999999;
			*pend = 0;
		}

		if(dsc->begincomments != dsc->endcomments) {
			*pbegin = min(dsc->begincomments, *pbegin);
			dsc->begincomments = 0;
			*pend = max(dsc->endcomments, *pend);
			dsc->endcomments = 0;
		}

		if(dsc->beginpreview != dsc->endpreview) {
			*pbegin = min(dsc->beginpreview, *pbegin);
			dsc->beginpreview = 0;
			*pend = max(dsc->endpreview, *pend);
			dsc->endpreview = 0;
		}

		if(dsc->begindefaults != dsc->enddefaults) {
			*pbegin = min(dsc->begindefaults, *pbegin);
			dsc->begindefaults = 0;
			*pend = max(dsc->enddefaults, *pend);
			dsc->enddefaults = 0;
		}

		if(dsc->beginprolog != dsc->endprolog) {
			*pbegin = min(dsc->beginprolog, *pbegin);
			dsc->beginprolog = 0;
			*pend = max(dsc->endprolog, *pend);
			dsc->endprolog = 0;
		}

		if(dsc->beginsetup != dsc->endsetup) {
			*pbegin = min(dsc->beginsetup, *pbegin);
			dsc->beginsetup = 0;
			*pend = max(dsc->endsetup, *pend);
			dsc->endsetup = 0;
		}

		if(dsc->begintrailer != dsc->endtrailer) {
			*pbegin = min(dsc->begintrailer, *pbegin);
			dsc->begintrailer = 0;
			*pend = max(dsc->endtrailer, *pend);
			dsc->endtrailer = 0;
		}

		if(*pbegin == 999999999)
			*pbegin = *pend;
		end = 0; /* end of composite is start of first separation */

		while(pdcs) {
			page_number = dsc->page_count;
			if((pdcs->begin) && (pdcs->colourname != NULL)) {
				/* Single file DCS 2.0 */
				code = dsc_add_page(dsc, page_number + 1, pdcs->colourname);
				if(code)
					return code;
				dsc->page[page_number].begin = pdcs->begin;
				dsc->page[page_number].end = pdcs->end;
				if(end != 0)
					end = min(end, pdcs->begin);
				else
					end = pdcs->begin; /* first separation  */
			} else {
				/* Multiple file DCS 2.0 */
				if((pdcs->location != NULL) &&
				   (pdcs->filetype != NULL) &&
				   (pdcs->colourname != NULL) &&
				   (dsc_stricmp(pdcs->location, "Local") == 0) &&
				   ((dsc_stricmp(pdcs->filetype, "EPS") == 0) ||
				    (dsc_stricmp(pdcs->filetype, "EPSF") == 0))) {
					code = dsc_add_page(dsc, page_number + 1, pdcs->colourname);
					if(code)
						return code;
					dsc->page[page_number].begin = 0;
					dsc->page[page_number].end = 0;
				}
			}
			pdcs = pdcs->next;
		}
		/* end of composite is start of first separation */
		if(end != 0)
			*pend = end;
		/* According to the DCS2 specification, the size of the composite 
	 * section can be determined by the smallest #offset.
	 * Some incorrect DCS2 files don't put the separations inside
	 * the DOS EPS PostScript section, and have a TIFF separation
	 * between the composite and the first separation.  This
	 * contravenes the DCS2 specification.  If we see one of these 
 	 * files, bring the end of the composite back to the end of 
	 * the DOS EPS PostScript section.
	 */
		if(dsc->doseps_end && (*pend > dsc->doseps_end))
			*pend = dsc->doseps_end;
	}
	return 0;
}

dsc_private int
dsc_parse_platefile(CDSC *dsc)
{
	unsigned int i, n;
	CDCS2 dcs2;
	CDCS2 *pdcs2;
	char colourname[MAXSTR];
	char filetype[MAXSTR];
	char location[MAXSTR];
	char *filename = NULL;
	int filename_length = 0;
	GSBOOL blank_line;
	GSBOOL single = FALSE;
	if(IS_DSC(dsc->line, "%%PlateFile:"))
		n = 12;
	else if(IS_DSC(dsc->line, "%%+"))
		n = 3;
	else
		return CDSC_ERROR; /* error */

	memset(&dcs2, 0, sizeof(dcs2));
	memset(&colourname, 0, sizeof(colourname));
	memset(&filetype, 0, sizeof(filetype));
	memset(&location, 0, sizeof(location));
	memset(&filename, 0, sizeof(filename));

	/* check for blank remainder of line */
	blank_line = TRUE;
	for(i = n; i < dsc->line_length; i++) {
		if(!IS_WHITE_OR_EOL(dsc->line[i])) {
			blank_line = FALSE;
			break;
		}
	}

	if(!blank_line) {
		dsc_copy_string(colourname, sizeof(colourname),
				dsc->line + n, dsc->line_length - n, &i);
		n += i;
		if(i)
			dsc_copy_string(filetype, sizeof(filetype),
					dsc->line + n, dsc->line_length - n, &i);
		n += i;
		while(IS_WHITE_OR_EOL(dsc->line[n]))
			n++;
		if(dsc->line[n] == '#') {
			/* single file DCS 2.0 */
			single = TRUE;
			n++;
			if(i)
				dcs2.begin = dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i)
				dcs2.end = dcs2.begin +
					   dsc_get_int(dsc->line + n, dsc->line_length - n, &i);
		} else {
			/* multiple file DCS 2.0 */
			if(i)
				dsc_copy_string(location, sizeof(location),
						dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i) {
				filename = dsc->line + n;
				filename_length = dsc->line_length - n;
			}
		}
		if(i == 0)
			dsc_unknown(dsc); /* we didn't get all fields */
		else {
			/* Allocate strings */
			if(strlen(colourname))
				dcs2.colourname = dsc_alloc_string(dsc,
								   colourname, (int)strlen(colourname));
			if(strlen(filetype))
				dcs2.filetype = dsc_alloc_string(dsc,
								 filetype, (int)strlen(filetype));
			if(strlen(location))
				dcs2.location = dsc_alloc_string(dsc,
								 location, (int)strlen(location));
			if(filename)
				dcs2.filename = dsc_add_line(dsc, filename, filename_length);

			/* Prevent parser from reading separations */
			if(single)
				dsc->file_length = min(dsc->file_length, dcs2.begin);
			/* Allocate it */
			pdcs2 = (CDCS2 *)dsc_memalloc(dsc, sizeof(CDCS2));
			if(pdcs2 == NULL)
				return CDSC_ERROR; /* out of memory */
			memcpy(pdcs2, &dcs2, sizeof(CDCS2));
			/* Then add to list of separations */
			if(dsc->dcs2 == NULL)
				dsc->dcs2 = pdcs2;
			else {
				CDCS2 *this_dcs2 = dsc->dcs2;
				while(this_dcs2->next)
					this_dcs2 = this_dcs2->next;
				this_dcs2->next = pdcs2;
			}
		}
	}
	return CDSC_OK;
}

/* Parse a DCS 1.0 plate comment, storing like a multi file DSC 2.0 */
dsc_private int
dsc_parse_dcs1plate(CDSC *dsc)
{
	unsigned int i, n = 0;
	CDCS2 dcs2;
	CDCS2 *pdcs2;
	const char *colourname;
	char filename[MAXSTR];
	GSBOOL blank_line;
	GSBOOL continued = FALSE;
	char *line = dsc->line;

	memset(&dcs2, 0, sizeof(dcs2));
	memset(&filename, 0, sizeof(filename));

	if(IS_DSC(line, "%%+")) {
		n = 3;
		line = dsc->last_line;
		continued = TRUE;
	}

	if(IS_DSC(line, "%%CyanPlate:")) {
		colourname = "Cyan";
		if(!continued)
			n = 12;
	} else if(IS_DSC(line, "%%MagentaPlate:")) {
		colourname = "Magenta";
		if(!continued)
			n = 15;
	} else if(IS_DSC(line, "%%YellowPlate:")) {
		colourname = "Yellow";
		if(!continued)
			n = 14;
	} else if(IS_DSC(line, "%%BlackPlate:")) {
		colourname = "Black";
		if(!continued)
			n = 13;
	} else
		return CDSC_ERROR; /* error */

	/* check for blank remainder of line */
	blank_line = TRUE;
	for(i = n; i < dsc->line_length; i++) {
		if(!IS_WHITE_OR_EOL(dsc->line[i])) {
			blank_line = FALSE;
			break;
		}
	}

	if(!blank_line) {
		dsc_copy_string(filename, sizeof(filename),
				dsc->line + n, dsc->line_length - n, &i);
		if(i == 0)
			dsc_unknown(dsc); /* we didn't get all fields */
		else {
			/* Allocate strings */
			dcs2.colourname = dsc_alloc_string(dsc,
							   colourname, (int)strlen(colourname));
			dcs2.filetype = dsc_alloc_string(dsc, "EPS", 3);
			dcs2.location = dsc_alloc_string(dsc, "Local", 5);
			if(strlen(filename))
				dcs2.filename = dsc_alloc_string(dsc,
								 filename, (int)strlen(filename));
			/* Allocate it */
			pdcs2 = (CDCS2 *)dsc_memalloc(dsc, sizeof(CDCS2));
			if(pdcs2 == NULL)
				return CDSC_ERROR; /* out of memory */
			memcpy(pdcs2, &dcs2, sizeof(CDCS2));
			/* Then add to list of separations */
			if(dsc->dcs2 == NULL)
				dsc->dcs2 = pdcs2;
			else {
				CDCS2 *this_dcs2 = dsc->dcs2;
				while(this_dcs2->next)
					this_dcs2 = this_dcs2->next;
				this_dcs2->next = pdcs2;
			}
		}
	}
	return CDSC_OK;
}

/* Find the filename which corresponds to this separation.
 * Used with multiple file DCS 2.0.
 * Returns NULL if there is no filename, or not DCS 2.0,
 * or single file DCS 2.0.
 * Caller will need to obtain the filesize from the file.
 */
const char *
dsc_find_platefile(CDSC *dsc, int page)
{
	CDCS2 *pdcs = dsc->dcs2;
	int i = 1;
	while(pdcs) {
		if(pdcs->begin != pdcs->end)
			return NULL; /* Single file DCS 2.0 */
		if(pdcs->location && pdcs->filetype && pdcs->colourname && (dsc_stricmp(pdcs->location, "Local") == 0) && ((dsc_stricmp(pdcs->filetype, "EPS") == 0) ||
															   (dsc_stricmp(pdcs->filetype, "EPSF") == 0))) {
			if(i == page)
				return pdcs->filename;
			i++;
		}
		pdcs = pdcs->next;
	}
	return NULL;
}

dsc_private CDSCCOLOUR *
dsc_find_colour(CDSC *dsc, const char *colourname)
{
	CDSCCOLOUR *colour = dsc->colours;
	while(colour) {
		if(colour->name && (dsc_stricmp(colour->name, colourname) == 0))
			return colour;
		colour = colour->next;
	}
	return 0;
}

dsc_private int
dsc_parse_process_colours(CDSC *dsc)
{
	unsigned int i, n;
	CDSCCOLOUR *pcolour;
	char colourname[MAXSTR];
	GSBOOL blank_line;
	if(IS_DSC(dsc->line, "%%DocumentProcessColors:"))
		n = 24;
	else if(IS_DSC(dsc->line, "%%+"))
		n = 3;
	else
		return CDSC_ERROR; /* error */

	memset(&colourname, 0, sizeof(colourname));

	/* check for blank remainder of line */
	blank_line = TRUE;
	for(i = n; i < dsc->line_length; i++) {
		if(!IS_WHITE_OR_EOL(dsc->line[i])) {
			blank_line = FALSE;
			break;
		}
	}
	while(IS_WHITE(dsc->line[n]))
		n++;
	if(COMPARE(dsc->line + n, "(atend)")) {
		if(dsc->scan_section == scan_comments)
			blank_line = TRUE;
		else {
			dsc_unknown(dsc);
			return CDSC_NOTDSC;
		}
	}

	if(!blank_line) {
		do {
			dsc_copy_string(colourname, sizeof(colourname),
					dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i && strlen(colourname)) {
				if((pcolour = dsc_find_colour(dsc, colourname)) == NULL) {
					pcolour = (CDSCCOLOUR *)
					    dsc_memalloc(dsc, sizeof(CDSCCOLOUR));
					if(pcolour == NULL)
						return CDSC_ERROR; /* out of memory */
					memset(pcolour, 0, sizeof(CDSCCOLOUR));
					pcolour->custom = CDSC_CUSTOM_COLOUR_UNKNOWN;
					pcolour->name = dsc_alloc_string(dsc,
									 colourname, (int)strlen(colourname));
					if(dsc->colours == NULL)
						dsc->colours = pcolour;
					else {
						CDSCCOLOUR *this_colour = dsc->colours;
						while(this_colour->next)
							this_colour = this_colour->next;
						this_colour->next = pcolour;
					}
				}
				pcolour->type = CDSC_COLOUR_PROCESS;
				if(dsc_stricmp(colourname, "Cyan") == 0) {
					pcolour->custom = CDSC_CUSTOM_COLOUR_CMYK;
					pcolour->cyan = 1.0;
					pcolour->magenta = pcolour->yellow = pcolour->black = 0.0;
				} else if(dsc_stricmp(colourname, "Magenta") == 0) {
					pcolour->custom = CDSC_CUSTOM_COLOUR_CMYK;
					pcolour->magenta = 1.0;
					pcolour->cyan = pcolour->yellow = pcolour->black = 0.0;
				} else if(dsc_stricmp(colourname, "Yellow") == 0) {
					pcolour->custom = CDSC_CUSTOM_COLOUR_CMYK;
					pcolour->yellow = 1.0;
					pcolour->cyan = pcolour->magenta = pcolour->black = 0.0;
				} else if(dsc_stricmp(colourname, "Black") == 0) {
					pcolour->custom = CDSC_CUSTOM_COLOUR_CMYK;
					pcolour->black = 1.0;
					pcolour->cyan = pcolour->magenta = pcolour->yellow = 0.0;
				} else if(dsc_stricmp(colourname, "Red") == 0) {
					pcolour->custom = CDSC_CUSTOM_COLOUR_RGB;
					pcolour->red = 1.0;
					pcolour->green = pcolour->blue = 0.0;
				} else if(dsc_stricmp(colourname, "Green") == 0) {
					pcolour->custom = CDSC_CUSTOM_COLOUR_RGB;
					pcolour->green = 1.0;
					pcolour->red = pcolour->blue = 0.0;
				} else if(dsc_stricmp(colourname, "Blue") == 0) {
					pcolour->custom = CDSC_CUSTOM_COLOUR_RGB;
					pcolour->blue = 1.0;
					pcolour->red = pcolour->green = 0.0;
				}
			}
		} while(i != 0);
	}
	return CDSC_OK;
}

dsc_private int
dsc_parse_custom_colours(CDSC *dsc)
{
	unsigned int i, n;
	CDSCCOLOUR *pcolour;
	char colourname[MAXSTR];
	GSBOOL blank_line;
	if(IS_DSC(dsc->line, "%%DocumentCustomColors:"))
		n = 23;
	else if(IS_DSC(dsc->line, "%%+"))
		n = 3;
	else
		return CDSC_ERROR; /* error */

	memset(&colourname, 0, sizeof(colourname));

	/* check for blank remainder of line */
	blank_line = TRUE;
	for(i = n; i < dsc->line_length; i++) {
		if(!IS_WHITE_OR_EOL(dsc->line[i])) {
			blank_line = FALSE;
			break;
		}
	}
	while(IS_WHITE(dsc->line[n]))
		n++;
	if(COMPARE(dsc->line + n, "(atend)")) {
		if(dsc->scan_section == scan_comments)
			blank_line = TRUE;
		else {
			dsc_unknown(dsc);
			return CDSC_NOTDSC;
		}
	}

	if(!blank_line) {
		do {
			dsc_copy_string(colourname, sizeof(colourname),
					dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i && strlen(colourname)) {
				if((pcolour = dsc_find_colour(dsc, colourname)) == NULL) {
					pcolour = (CDSCCOLOUR *)
					    dsc_memalloc(dsc, sizeof(CDSCCOLOUR));
					if(pcolour == NULL)
						return CDSC_ERROR; /* out of memory */
					memset(pcolour, 0, sizeof(CDSCCOLOUR));
					pcolour->name = dsc_alloc_string(dsc,
									 colourname, (int)strlen(colourname));
					pcolour->custom = CDSC_CUSTOM_COLOUR_UNKNOWN;
					if(dsc->colours == NULL)
						dsc->colours = pcolour;
					else {
						CDSCCOLOUR *this_colour = dsc->colours;
						while(this_colour->next)
							this_colour = this_colour->next;
						this_colour->next = pcolour;
					}
				}
				pcolour->type = CDSC_COLOUR_CUSTOM;
			}
		} while(i != 0);
	}
	return CDSC_OK;
}

dsc_private int
dsc_parse_cmyk_custom_colour(CDSC *dsc)
{
	unsigned int i, n;
	CDSCCOLOUR *pcolour;
	char colourname[MAXSTR];
	float cyan, magenta, yellow, black;
	GSBOOL blank_line;
	if(IS_DSC(dsc->line, "%%CMYKCustomColor:"))
		n = 18;
	else if(IS_DSC(dsc->line, "%%+"))
		n = 3;
	else
		return CDSC_ERROR; /* error */

	memset(&colourname, 0, sizeof(colourname));

	/* check for blank remainder of line */

	do {
		blank_line = TRUE;
		for(i = n; i < dsc->line_length; i++) {
			if(!IS_WHITE_OR_EOL(dsc->line[i])) {
				blank_line = FALSE;
				break;
			}
		}
		if(blank_line)
			break;
		else {
			cyan = magenta = yellow = black = 0.0;
			cyan = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i)
				magenta = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i)
				yellow = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i)
				black = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i)
				dsc_copy_string(colourname, sizeof(colourname),
						dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i && strlen(colourname)) {
				if((pcolour = dsc_find_colour(dsc, colourname)) == NULL) {
					pcolour = (CDSCCOLOUR *)
					    dsc_memalloc(dsc, sizeof(CDSCCOLOUR));
					if(pcolour == NULL)
						return CDSC_ERROR; /* out of memory */
					memset(pcolour, 0, sizeof(CDSCCOLOUR));
					pcolour->name = dsc_alloc_string(dsc,
									 colourname, (int)strlen(colourname));
					pcolour->type = CDSC_COLOUR_UNKNOWN;
					if(dsc->colours == NULL)
						dsc->colours = pcolour;
					else {
						CDSCCOLOUR *this_colour = dsc->colours;
						while(this_colour->next)
							this_colour = this_colour->next;
						this_colour->next = pcolour;
					}
				}
				pcolour->custom = CDSC_CUSTOM_COLOUR_CMYK;
				pcolour->cyan = cyan;
				pcolour->magenta = magenta;
				pcolour->yellow = yellow;
				pcolour->black = black;
			}
		}
	} while(i != 0);
	return CDSC_OK;
}

dsc_private int
dsc_parse_rgb_custom_colour(CDSC *dsc)
{
	unsigned int i, n;
	CDSCCOLOUR *pcolour;
	char colourname[MAXSTR];
	float red, green, blue;
	GSBOOL blank_line;
	if(IS_DSC(dsc->line, "%%RGBCustomColor:"))
		n = 17;
	else if(IS_DSC(dsc->line, "%%+"))
		n = 3;
	else
		return CDSC_ERROR; /* error */

	memset(&colourname, 0, sizeof(colourname));

	/* check for blank remainder of line */

	do {
		blank_line = TRUE;
		for(i = n; i < dsc->line_length; i++) {
			if(!IS_WHITE_OR_EOL(dsc->line[i])) {
				blank_line = FALSE;
				break;
			}
		}
		if(blank_line)
			break;
		else {
			red = green = blue = 0.0;
			red = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i)
				green = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i)
				blue = dsc_get_real(dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i)
				dsc_copy_string(colourname, sizeof(colourname),
						dsc->line + n, dsc->line_length - n, &i);
			n += i;
			if(i && strlen(colourname)) {
				if((pcolour = dsc_find_colour(dsc, colourname)) == NULL) {
					pcolour = (CDSCCOLOUR *)
					    dsc_memalloc(dsc, sizeof(CDSCCOLOUR));
					if(pcolour == NULL)
						return CDSC_ERROR; /* out of memory */
					memset(pcolour, 0, sizeof(CDSCCOLOUR));
					pcolour->name = dsc_alloc_string(dsc,
									 colourname, (int)strlen(colourname));
					pcolour->type = CDSC_COLOUR_UNKNOWN;
					if(dsc->colours == NULL)
						dsc->colours = pcolour;
					else {
						CDSCCOLOUR *this_colour = dsc->colours;
						while(this_colour->next)
							this_colour = this_colour->next;
						this_colour->next = pcolour;
					}
				}
				pcolour->custom = CDSC_CUSTOM_COLOUR_RGB;
				pcolour->red = red;
				pcolour->green = green;
				pcolour->blue = blue;
			}
		}
	} while(i != 0);
	return CDSC_OK;
}
