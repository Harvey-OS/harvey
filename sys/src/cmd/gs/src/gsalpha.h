/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsalpha.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* API for alpha value in graphics state */

#ifndef gsalpha_INCLUDED
#  define gsalpha_INCLUDED

/*
 * This tiny little file is separate so that it can be included by
 * gsstate.c for initializing the alpha value, even in configurations
 * that don't have full alpha support.
 */

/* Set/read alpha value. */
int gs_setalpha(P2(gs_state *, floatp));
float gs_currentalpha(P1(const gs_state *));

#endif /* gsalpha_INCLUDED */
