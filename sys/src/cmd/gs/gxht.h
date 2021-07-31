/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxht.h */
/* Rest of (client) halftone definitions */

#ifndef gxht_INCLUDED
#  define gxht_INCLUDED

#include "gsht1.h"
#include "gxtmap.h"

/* Halftone types */
typedef enum {
	ht_type_none,			/* is this needed? */
	ht_type_screen,			/* set by setscreen */
	ht_type_colorscreen,		/* set by setcolorscreen */
	ht_type_spot,			/* Type 1 halftone dictionary */
	ht_type_threshold,		/* Type 3 halftone dictionary */
	ht_type_multiple		/* Type 5 halftone dictionary */
} gs_halftone_type;

/* Type 1 halftone.  This is just a Level 1 halftone with */
/* a few extra members. */
typedef struct gs_spot_halftone_s {
	gs_screen_halftone screen;
	bool accurate_screens;
	gs_mapping_proc transfer;
} gs_spot_halftone;
#define st_spot_halftone_max_ptrs st_screen_halftone_max_ptrs

/* Type 3 halftone. */
typedef struct gs_threshold_halftone_s {
	int width;
	int height;
	const byte *thresholds;
	gs_mapping_proc transfer;
} gs_threshold_halftone;
#define st_threshold_halftone_max_ptrs 1

/* Define the separation "names" for a Type 5 halftone. */
typedef enum {
	gs_ht_separation_Default,	/* must be first */
	gs_ht_separation_Gray,
	gs_ht_separation_Red,
	gs_ht_separation_Green,
	gs_ht_separation_Blue,
	gs_ht_separation_Cyan,
	gs_ht_separation_Magenta,
	gs_ht_separation_Yellow,
	gs_ht_separation_Black
} gs_ht_separation_name;
#define gs_ht_separation_name_strings\
  "Default", "Gray", "Red", "Green", "Blue",\
  "Cyan", "Magenta", "Yellow", "Black"

/* Define the elements of a Type 5 halftone. */
typedef struct gs_halftone_component_s {
	gs_ht_separation_name cname;
	gs_halftone_type type;
	union {
		gs_spot_halftone spot;		/* Type 1 */
		gs_threshold_halftone threshold;	/* Type 3 */
	} params;
} gs_halftone_component;
#define private_st_halftone_component()	/* in zht2.c */\
  gs_private_st_composite(st_halftone_component, gs_halftone_component,\
    "gs_halftone_component", halftone_component_enum_ptrs,\
    halftone_component_reloc_ptrs)
#define st_halftone_component_max_ptrs\
  max(st_spot_halftone_max_ptrs, st_threshold_halftone_max_ptrs)

/* Define the Type 5 halftone itself. */
typedef struct gs_multiple_halftone_s {
	gs_halftone_component *components;
	uint num_comp;
} gs_multiple_halftone;
#define st_multiple_halftone_max_ptrs 1

/* The halftone stored in the graphics state is the union of */
/* setscreen, setcolorscreen, Type 1, Type 3, and Type 5. */
struct gs_halftone_s {
	gs_halftone_type type;
	union {
		gs_screen_halftone screen;		/* setscreen */
		gs_colorscreen_halftone colorscreen;	/* setcolorscreen */
		gs_spot_halftone spot;			/* Type 1 */
		gs_threshold_halftone threshold;	/* Type 3 */
		gs_multiple_halftone multiple;		/* Type 5 */
	} params;
};
extern_st(st_halftone);
#define public_st_halftone()	/* in gsht.c */\
  gs_public_st_composite(st_halftone, gs_halftone, "gs_halftone",\
    halftone_enum_ptrs, halftone_reloc_ptrs)
#define st_halftone_max_ptrs\
  max(max(st_screen_halftone_max_ptrs, st_colorscreen_halftone_max_ptrs),\
      max(max(st_spot_halftone_max_ptrs, st_threshold_halftone_max_ptrs),\
	  st_multiple_halftone_max_ptrs))

/* Procedural interface for AccurateScreens */

/* Set/get the default AccurateScreens value (for set[color]screen). */
/* Note that this value is stored in a static variable. */
void	gs_setaccuratescreens(P1(bool));
bool	gs_currentaccuratescreens(P0());

/* Initiate screen sampling with optional AccurateScreens. */
int	gs_screen_init_accurate(P4(gs_screen_enum *, gs_state *,
				   gs_screen_halftone *, bool));

#endif					/* gxht_INCLUDED */
