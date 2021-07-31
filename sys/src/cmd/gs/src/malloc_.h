/* Copyright (C) 1989, 1992, 1996, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: malloc_.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Generic substitute for Unix malloc.h */

#ifndef malloc__INCLUDED
#  define malloc__INCLUDED

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"

#ifdef __TURBOC__
#  include <alloc.h>
#else
#  if defined(BSD4_2) || defined(apollo) || defined(vax) || defined(sequent) || defined(UTEK)
#    if defined(_POSIX_SOURCE) || (defined(__STDC__) && (!defined(sun) || defined(__svr4__)))	/* >>> */
#      include <stdlib.h>
#    else			/* Ancient breakage */
extern char *malloc();
extern void free();

#    endif
#  else
#    if defined(_HPUX_SOURCE) || defined(__CONVEX__) || defined(__convex__) || defined(__OSF__) || defined(__386BSD__) || defined(_POSIX_SOURCE) || defined(__STDC__) || defined(VMS)
#      include <stdlib.h>
#    else
#      include <malloc.h>
#    endif			/* !_HPUX_SOURCE, ... */
#  endif			/* !BSD4_2, ... */
#endif /* !__TURBOC__ */

/* (At least some versions of) Linux don't have a working realloc.... */
#ifdef linux
#  define malloc__need_realloc
void *gs_realloc(P3(void *, size_t, size_t));

#else
#  define gs_realloc(ptr, old_size, new_size) realloc(ptr, new_size)
#endif

#endif /* malloc__INCLUDED */
