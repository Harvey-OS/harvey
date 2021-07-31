/* Copyright (C) 1993, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: ctype_.h,v 1.1 2000/03/09 08:40:40 lpd Exp $ */
/* Wrapper for ctype.h */

#ifndef ctype__INCLUDED
#  define ctype__INCLUDED

/* We must include std.h before any file that includes sys/types.h. */
#include "std.h"

/* ... and that's the only reason for having this file at all. */
#include <ctype.h>

#endif /* ctype__INCLUDED */
