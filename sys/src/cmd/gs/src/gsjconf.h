/* Copyright (C) 1994, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsjconf.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
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
