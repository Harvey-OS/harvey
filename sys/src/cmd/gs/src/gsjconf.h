/* Copyright (C) 1994, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsjconf.h,v 1.4 2002/02/21 22:24:52 giles Exp $ */
/* jconfig.h file for Independent JPEG Group code */

#ifndef gsjconf_INCLUDED
#  define gsjconf_INCLUDED

/*
 * We should have the following here:

#include "stdpre.h"

 * But because of the directory structure used to build the IJG library, we
 * actually concatenate stdpre.h on the front of this file instead to
 * construct the jconfig.h file used for the compilation.
 */

#include "arch.h"

/* See IJG's jconfig.doc for the contents of this file. */

#ifdef __PROTOTYPES__
#  define HAVE_PROTOTYPES
#endif

#define HAVE_UNSIGNED_CHAR
#define HAVE_UNSIGNED_SHORT
#undef CHAR_IS_UNSIGNED

#ifdef __STDC__			/* is this right? */
#  define HAVE_STDDEF_H
#  define HAVE_STDLIB_H
#endif

#undef NEED_BSD_STRINGS		/* WRONG */
#undef NEED_SYS_TYPES_H		/* WRONG */
#undef NEED_FAR_POINTERS
#undef NEED_SHORT_EXTERNAL_NAMES

#undef INCOMPLETE_TYPES_BROKEN

/* The following is documented in jmemsys.h, not jconfig.doc. */
#if ARCH_SIZEOF_INT <= 2
#  undef MAX_ALLOC_CHUNK
#  define MAX_ALLOC_CHUNK 0xfff0
#endif

#ifdef JPEG_INTERNALS

#if ARCH_ARITH_RSHIFT == 0
#  define RIGHT_SHIFT_IS_UNSIGNED
#else
#  undef RIGHT_SHIFT_IS_UNSIGNED
#endif

#endif /* JPEG_INTERNALS */

#endif /* gsjconf_INCLUDED */
