/* Copyright (C) 1989, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsstate.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Public graphics state API */

#ifndef gsstate_INCLUDED
#  define gsstate_INCLUDED

/* Opaque type for a graphics state */
#ifndef gs_state_DEFINED
#  define gs_state_DEFINED
typedef struct gs_state_s gs_state;
#endif

/* Initial allocation and freeing */
gs_state *gs_state_alloc(P1(gs_memory_t *));	/* 0 if fails */
int gs_state_free(P1(gs_state *));

/* Initialization, saving, restoring, and copying */
int gs_gsave(P1(gs_state *)), gs_grestore(P1(gs_state *)), gs_grestoreall(P1(gs_state *));
int gs_grestore_only(P1(gs_state *));
int gs_gsave_for_save(P2(gs_state *, gs_state **)), gs_grestoreall_for_restore(P2(gs_state *, gs_state *));
gs_state *gs_gstate(P1(gs_state *));
gs_state *gs_state_copy(P2(gs_state *, gs_memory_t *));
int gs_copygstate(P2(gs_state * /*to */ , const gs_state * /*from */ )),
      gs_currentgstate(P2(gs_state * /*to */ , const gs_state * /*from */ )),
      gs_setgstate(P2(gs_state * /*to */ , const gs_state * /*from */ ));
int gs_initgraphics(P1(gs_state *));

/* Device control */
#include "gsdevice.h"

/* Line parameters and quality */
#include "gsline.h"

/* Color and gray */
#include "gscolor.h"

/* Halftone screen */
#include "gsht.h"
#include "gscsel.h"
int gs_setscreenphase(P4(gs_state *, int, int, gs_color_select_t));
int gs_currentscreenphase(P3(const gs_state *, gs_int_point *,
			     gs_color_select_t));

#define gs_sethalftonephase(pgs, px, py)\
  gs_setscreenphase(pgs, px, py, gs_color_select_all)
#define gs_currenthalftonephase(pgs, ppt)\
  gs_currentscreenphase(pgs, ppt, 0)
int gx_imager_setscreenphase(P4(gs_imager_state *, int, int,
				gs_color_select_t));

/* Miscellaneous */
int gs_setfilladjust(P3(gs_state *, floatp, floatp));
int gs_currentfilladjust(P2(const gs_state *, gs_point *));
void gs_setlimitclamp(P2(gs_state *, bool));
bool gs_currentlimitclamp(P1(const gs_state *));
#include "gscpm.h"
gs_in_cache_device_t gs_incachedevice(P1(const gs_state *));

#endif /* gsstate_INCLUDED */
