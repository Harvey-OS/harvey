/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/* Copyright (C) 1996-2005 artofcode LLC. All rights reserved.
  
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

/* $Id: gscdef.c,v 1.58 2005/10/20 19:46:55 ray Exp $ */
/* Configuration scalars */

#include "std.h"
#include "gscdefs.h"  /* interface */
#include "gconfigd.h" /* for #defines */

/* ---------------- Miscellaneous system parameters ---------------- */

/* All of these can be set in the makefile. */
/* Normally they are all const; see gscdefs.h for more information. */

#ifndef GS_BUILDTIME
#define GS_BUILDTIME \
	0 /* should be set in the makefile */
#endif
CONFIG_CONST int32_t gs_buildtime = GS_BUILDTIME;

#ifndef GS_COPYRIGHT
#define GS_COPYRIGHT \
	"Copyright (C) 2005 artofcode LLC, Benicia, CA.  All rights reserved."
#endif
const char *CONFIG_CONST gs_copyright = GS_COPYRIGHT;

#ifndef GS_PRODUCTFAMILY
#define GS_PRODUCTFAMILY \
	"AFPL Ghostscript"
#endif
const char *CONFIG_CONST gs_productfamily = GS_PRODUCTFAMILY;

#ifndef GS_PRODUCT
#define GS_PRODUCT \
	GS_PRODUCTFAMILY
#endif
const char *CONFIG_CONST gs_product = GS_PRODUCT;

const char *
gs_program_name(void)
{
	return gs_product;
}

/* GS_REVISION must be defined in the makefile. */
CONFIG_CONST int32_t gs_revision = GS_REVISION;

int32_t
gs_revision_number(void)
{
	return gs_revision;
}

/* GS_REVISIONDATE must be defined in the makefile. */
CONFIG_CONST int32_t gs_revisiondate = GS_REVISIONDATE;

#ifndef GS_SERIALNUMBER
#define GS_SERIALNUMBER \
	42 /* a famous number */
#endif
CONFIG_CONST int32_t gs_serialnumber = GS_SERIALNUMBER;

/* ---------------- Installation directories and files ---------------- */

/* Here is where the library search path, the name of the */
/* initialization file, and the doc directory are defined. */
/* Define the documentation directory (only used in help messages). */
const char *const gs_doc_directory = GS_DOCDIR;

/* Define the default library search path. */
const char *const gs_lib_default_path = GS_LIB_DEFAULT;

/* Define the interpreter initialization file. */
const char *const gs_init_file = GS_INIT;
