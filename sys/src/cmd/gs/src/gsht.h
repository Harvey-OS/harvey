/* Copyright (C) 1993, 1994, 1997 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsht.h,v 1.5 2002/06/16 08:45:42 lpd Exp $ */
/* Public interface to halftone functionality */

#ifndef gsht_INCLUDED
#  define gsht_INCLUDED

/* Client definition of (Type 1) halftones */
typedef struct gs_screen_halftone_s {
    float frequency;
    float angle;
    float (*spot_function) (floatp, floatp);
    /* setscreen or sethalftone sets these: */
    /* (a Level 2 feature, but we include them in Level 1) */
    float actual_frequency;
    float actual_angle;
} gs_screen_halftone;

#define st_screen_halftone_max_ptrs 0

/* Client definition of color (Type 2) halftones */
typedef struct gs_colorscreen_halftone_s {
    union _css {
	gs_screen_halftone indexed[4];
	struct _csc {
	    gs_screen_halftone red, green, blue, gray;
	} colored;
    } screens;
} gs_colorscreen_halftone;

#define st_colorscreen_halftone_max_ptrs 0

/* Procedural interface */
int gs_setscreen(gs_state *, gs_screen_halftone *);
int gs_currentscreen(const gs_state *, gs_screen_halftone *);
int gs_currentscreenlevels(const gs_state *);

/*
 * Enumeration-style definition of a single screen.  The client must:
 *      - probably, call gs_screen_enum_alloc;
 *      - call gs_screen_init;
 *      - in a loop,
 *              - call gs_screen_currentpoint; if it returns 1, exit;
 *              - call gs_screen_next;
 *      - if desired, call gs_screen_install to install the screen.
 */
typedef struct gs_screen_enum_s gs_screen_enum;
gs_screen_enum *gs_screen_enum_alloc(gs_memory_t *, client_name_t);
int gs_screen_init(gs_screen_enum *, gs_state *, gs_screen_halftone *);
int gs_screen_currentpoint(gs_screen_enum *, gs_point *);
int gs_screen_next(gs_screen_enum *, floatp);
int gs_screen_install(gs_screen_enum *);

#endif /* gsht_INCLUDED */
