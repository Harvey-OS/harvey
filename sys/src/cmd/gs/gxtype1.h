/* Copyright (C) 1990, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gxtype1.h */
/* Private Adobe Type 1 font definitions for Ghostscript library */
#include "gscrypt1.h"
#include "gstype1.h"

/* This file defines the structures for the state of a Type 1 interpreter. */

/*
 * Because of oversampling, one pixel in the Type 1 interpreter may
 * correspond to several device pixels.  This is also true of the hint data,
 * since the CTM reflects the transformation to the oversampled space.
 * To help keep the font level hints separated from the character level hints,
 * we store the scaling factor separately with each set of hints.
 */
typedef struct pixel_scale_s {
	fixed unit;		/* # of pixels per device pixel */
	fixed half;		/* unit / 2 */
	int log2_unit;		/* log2(unit / fixed_1) */
} pixel_scale;
typedef struct point_scale_s {
	pixel_scale x, y;
} point_scale;
#define set_pixel_scale(pps, log2)\
  (pps)->unit = ((pps)->half = fixed_half << ((pps)->log2_unit = log2)) << 1
#define scaled_rounded(v, pps)\
  (((v) + (pps)->half) & -(pps)->unit)

/* ------ Font level hints ------ */

/* Define the standard stem width tables. */
/* Each table is sorted, since the StemSnap arrays are sorted. */
#define max_snaps (1 + max_StemSnap)
typedef struct {
	int count;
	fixed data[max_snaps];
} stem_snap_table;

/* Define the alignment zone structure. */
/* These are in device coordinates also. */
#define max_a_zones (max_BlueValues + max_OtherBlues)
typedef struct {
	int is_top_zone;
	fixed v0, v1;			/* range for testing */
	fixed flat;			/* flat position */
} alignment_zone;

/* Define the structure for hints that depend only on the font and CTM, */
/* not on the individual character.  Eventually these should be cached */
/* with the font/matrix pair. */
typedef struct font_hints_s {
	bool axes_swapped;		/* true if x & y axes interchanged */
					/* (only set if using hints) */
	bool x_inverted, y_inverted;	/* true if axis is inverted */
	bool use_x_hints;		/* true if we should use hints */
					/* for char space x coords (vstem) */
	bool use_y_hints;		/* true if we should use hints */
					/* for char space y coords (hstem) */
	point_scale scale;		/* oversampling scale */
	stem_snap_table snap_h;		/* StdHW, StemSnapH */
	stem_snap_table snap_v;		/* StdVW, StemSnapV */
	fixed blue_fuzz, blue_shift;	/* alignment zone parameters */
					/* in device pixels */
	bool suppress_overshoot;	/* (computed from BlueScale) */
	int a_zone_count;		/* # of alignment zones */
	alignment_zone a_zones[max_a_zones];	/* the alignment zones */
} font_hints;

/* ------ Character level hints ------ */

/* Define the stem hint tables. */
/* Each stem hint table is kept sorted. */
/* Stem hints are in device coordinates. */
#define max_stems 6			/* arbitrary */
typedef struct {
	fixed v0, v1;			/* coordinates (widened a little) */
	fixed dv0, dv1;			/* adjustment values */
} stem_hint;
typedef struct {
	int count;
	int current;			/* cache cursor for search */
	stem_hint data[max_stems];
} stem_hint_table;

/* ------ Interpreter state ------ */

/* Define the control state of the interpreter. */
/* This is what must be saved and restored */
/* when calling a CharString subroutine. */
typedef struct {
	const byte *ip;
	crypt_state dstate;
	gs_const_string char_string;	/* original CharString or Subr, */
					/* for GC */
} ip_state;

#ifndef segment_DEFINED
#  define segment_DEFINED
typedef struct segment_s segment;
#endif

/* This is the full state of the Type 1 interpreter. */
#define ostack_size 24			/* per documentation */
#define ipstack_size 10			/* per documentation */
struct gs_type1_state_s {
		/* The following are set at initialization */
	gs_show_enum *penum;		/* show enumerator */
	gs_state *pgs;			/* graphics state */
	gs_type1_data *pdata;		/* font-specific data */
	int charpath_flag;		/* 0 for show, 1 for false */
					/* charpath, 2 for true charpath */
	int paint_type;			/* 0/3 for fill, 1/2 for stroke */
	fixed_coeff fc;			/* cached fixed coefficients */
	float flatness;			/* flatness for character curves */
	point_scale scale;		/* oversampling scale */
	font_hints fh;			/* font-level hints */
	gs_fixed_point origin;		/* character origin */
		/* The following are updated dynamically */
	fixed ostack[ostack_size];	/* the Type 1 operand stack */
	int os_count;			/* # of occupied stack entries */
	ip_state ipstack[ipstack_size+1];	/* control stack */
	int ips_count;			/* # of occupied entries */
	bool sb_set;			/* true if lsb is preset */
	bool width_set;			/* true if width is set (for */
					/* seac components) */
	gs_fixed_point lsb;		/* left side bearing */
	gs_fixed_point width;		/* character width (char coords) */
	int seac_base;			/* base character code for seac, */
					/* or -1 */
	gs_fixed_point adxy;		/* seac accent displacement, */
					/* only needed to adjust Flex endpoint */
	gs_fixed_point position;	/* save unadjusted position */
					/* when returning temporarily */
					/* to caller */
	int flex_path_was_open;		/* record whether path was open */
					/* at start of Flex section */
#define flex_max 8
	gs_fixed_point flex_points[flex_max];	/* points for Flex */
	int flex_count;
	int ignore_pops;		/* # of pops to ignore (after */
					/* a known othersubr call) */
		/* The following are set dynamically. */
#define dotsection_in 0
#define dotsection_out (-1)
	int dotsection_flag;		/* 0 if inside dotsection, */
					/* -1 if outside */
	bool vstem3_set;		/* true if vstem3 seen */
	gs_fixed_point vs_offset;	/* device space offset for centering */
					/* middle stem of vstem3 */
	int hints_initial;		/* hints for beginning of first */
					/* segment of subpath */
	segment *hint_next;		/* next segment needing hinting, */
					/* 0 means entire current subpath */
	int hints_pending;		/* hints not yet applied at hint_next */
	stem_hint_table hstem_hints;	/* horizontal stem hints */
	stem_hint_table vstem_hints;	/* vertical stem hints */
};
extern_st(st_gs_type1_state);
#define public_st_gs_type1_state() /* in gstype1.c */\
  gs_public_st_composite(st_gs_type1_state, gs_type1_state, "gs_type1_state",\
    gs_type1_state_enum_ptrs, gs_type1_state_reloc_ptrs)


/* ------ Interface between main Type 1 interpreter and hint routines ------ */

/* Font level hints */
void	reset_font_hints(P2(font_hints *, const gs_log2_scale_point *));
void	compute_font_hints(P4(font_hints *, const gs_matrix_fixed *,
			      const gs_log2_scale_point *,
			      const gs_type1_data *));
/* Character level hints */
void	reset_stem_hints(P1(gs_type1_state *)),
	update_stem_hints(P1(gs_type1_state *)),
	apply_path_hints(P2(gs_type1_state *, bool)),
	apply_hstem_hints(P3(gs_type1_state *, fixed, gs_fixed_point *)),
	apply_vstem_hints(P3(gs_type1_state *, fixed, gs_fixed_point *)),
	type1_hstem(P3(gs_type1_state *, fixed, fixed)),
	type1_vstem(P3(gs_type1_state *, fixed, fixed)),
	center_vstem(P3(gs_type1_state *, fixed, fixed));
