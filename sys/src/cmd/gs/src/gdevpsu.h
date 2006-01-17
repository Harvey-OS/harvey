/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsu.h,v 1.9 2003/03/20 20:27:46 alexcher Exp $ */
/* Interface to PostScript-writing utilities */

#ifndef gdevpsu_INCLUDED
#  define gdevpsu_INCLUDED

/* Define parameters and state for PostScript-writing drivers. */
typedef struct gx_device_pswrite_common_s {
    float LanguageLevel;
    bool ProduceEPS;
    int ProcSet_version;
    long bbox_position;		/* set when writing file header */
} gx_device_pswrite_common_t;
#define PSWRITE_COMMON_PROCSET_VERSION 1000 /* for definitions in gdevpsu.c */
#define PSWRITE_COMMON_VALUES(ll, eps, psv)\
  {ll, eps, PSWRITE_COMMON_PROCSET_VERSION + (psv)}

/* ---------------- Low level ---------------- */

/* Write a 0-terminated array of strings as lines. */
int psw_print_lines(FILE *f, const char *const lines[]);

/* ---------------- File level ---------------- */

/*
 * Write the file header, up through the BeginProlog.  This must write to a
 * file, not a stream, because it may be called during finalization.
 */
int psw_begin_file_header(FILE *f, const gx_device *dev,
			   const gs_rect *pbbox,
			   gx_device_pswrite_common_t *pdpc, bool ascii);

/* End the file header.*/
int psw_end_file_header(FILE *f);

/* End the file. */
int psw_end_file(FILE *f, const gx_device *dev,
		  const gx_device_pswrite_common_t *pdpc,
		  const gs_rect *pbbox, int page_count);

/* ---------------- Page level ---------------- */

/*
 * Write the page header.
 */
int psw_write_page_header(stream *s, const gx_device *dev,
			   const gx_device_pswrite_common_t *pdpc,
			   bool do_scale, long page_ord,  int dictsize);
/*
 * Write the page trailer.  We do this directly to the file, rather than to
 * the stream, because we may have to do it during finalization.
 */
int psw_write_page_trailer(FILE *f, int num_copies, int flush);

#endif /* gdevpsu_INCLUDED */

