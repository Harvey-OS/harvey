/* Copyright (C) 1995, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: icfontab.c,v 1.4 2002/02/21 22:24:53 giles Exp $ */
/* Table of compiled fonts */
#include "ccfont.h"

/*
 * This is compiled separately and linked with the fonts themselves,
 * in a shared library when applicable.
 */

#undef font_
#define font_(fname, fproc, zfproc) extern ccfont_proc(fproc);
#ifndef GCONFIGF_H
# include "gconfigf.h"
#else
# include GCONFIGF_H
#endif

private const ccfont_fproc fprocs[] = {
#undef font_
#define font_(fname, fproc, zfproc) fproc,  /* fname, zfproc are not needed */
#ifndef GCONFIGF_H
# include "gconfigf.h"
#else
# include GCONFIGF_H
#endif
    0
};

int
ccfont_fprocs(int *pnum_fprocs, const ccfont_fproc ** pfprocs)
{
    *pnum_fprocs = countof(fprocs) - 1;
    *pfprocs = &fprocs[0];
    return ccfont_version;	/* for compatibility checking */
}
