/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gdevpsu.h,v 1.2 2000/09/19 19:00:22 lpd Exp $ */
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
void psw_print_lines(P2(FILE *f, const char *const lines[]));

/* ---------------- File level ---------------- */

/*
 * Write the file header, up through the BeginProlog.  This must write to a
 * file, not a stream, because it may be called during finalization.
 */
void psw_begin_file_header(P5(FILE *f, const gx_device *dev,
			      const gs_rect *pbbox,
			      gx_device_pswrite_common_t *pdpc, bool ascii));

/* End the file header.*/
void psw_end_file_header(P1(FILE *f));

/* End the file. */
void psw_end_file(P4(FILE *f, const gx_device *dev,
		     const gx_device_pswrite_common_t *pdpc,
		     const gs_rect *pbbox));

/* ---------------- Page level ---------------- */

/*
 * Write the page header.
 */
void psw_write_page_header(P4(stream *s, const gx_device *dev,
			      const gx_device_pswrite_common_t *pdpc,
			      bool do_scale));
/*
 * Write the page trailer.  We do this directly to the file, rather than to
 * the stream, because we may have to do it during finalization.
 */
void psw_write_page_trailer(P3(FILE *f, int num_copies, int flush));

#endif /* gdevpsu_INCLUDED */

