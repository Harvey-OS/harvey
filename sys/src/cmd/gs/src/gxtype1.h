/* Copyright (C) 1990, 2000, 2001 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxtype1.h,v 1.19 2004/09/22 13:52:33 igor Exp $ */
/* Private Adobe Type 1 / Type 2 charstring interpreter definitions */

#ifndef gxtype1_INCLUDED
#  define gxtype1_INCLUDED

#include "gscrypt1.h"
#include "gsgdata.h"
#include "gstype1.h"
#include "gxhintn.h"

/* This file defines the structures for the state of a Type 1 / */
/* Type 2 charstring interpreter. */

/*
 * Because of oversampling, one pixel in the Type 1 interpreter may
 * correspond to several device pixels.  This is also true of the hint data,
 * since the CTM reflects the transformation to the oversampled space.
 * To help keep the font level hints separated from the character level hints,
 * we store the scaling factor separately with each set of hints.
 */
typedef struct pixel_scale_s {
    fixed unit;			/* # of pixels per device pixel */
    fixed half;			/* unit / 2 */
    int log2_unit;		/* log2(unit / fixed_1) */
} pixel_scale;
typedef struct point_scale_s {
    pixel_scale x, y;
} point_scale;

#define set_pixel_scale(pps, log2)\
  (pps)->unit = ((pps)->half = fixed_half << ((pps)->log2_unit = log2)) << 1
#define scaled_rounded(v, pps)\
  (((v) + (pps)->half) & -(pps)->unit)


/*
 * The Type 2 charstring documentation says that the total number of hints
 * is limited to 96.
 */

#define max_total_stem_hints 96

/* ------ Interpreter state ------ */

/* Define the control state of the interpreter. */
/* This is what must be saved and restored */
/* when calling a CharString subroutine. */
typedef struct {
    const byte *ip;
    crypt_state dstate;
    gs_glyph_data_t cs_data;	/* original CharString or Subr, */
				/* for GC */
} ip_state_t;

/* Get the next byte from a CharString.  It may or may not be encrypted. */
#define charstring_this(ch, state, encrypted)\
  (encrypted ? decrypt_this(ch, state) : ch)
#define charstring_next(ch, state, chvar, encrypted)\
  (encrypted ? (chvar = decrypt_this(ch, state),\
		decrypt_skip_next(ch, state)) :\
   (chvar = ch))
#define charstring_skip_next(ch, state, encrypted)\
  (encrypted ? decrypt_skip_next(ch, state) : 0)

#ifndef gx_path_DEFINED
#  define gx_path_DEFINED
typedef struct gx_path_s gx_path;
#endif

#ifndef segment_DEFINED
#  define segment_DEFINED
typedef struct segment_s segment;
#endif

/* This is the full state of the Type 1 interpreter. */
#define ostack_size 48		/* per Type 2 documentation */
#define ipstack_size 10		/* per documentation */
struct gs_type1_state_s {
    t1_hinter h;
    /* The following are set at initialization */
    gs_font_type1 *pfont;	/* font-specific data */
    gs_imager_state *pis;	/* imager state */
    gx_path *path;		/* path for appending */
    bool no_grid_fitting;
    int paint_type;		/* 0/3 for fill, 1/2 for stroke */
    void *callback_data;
    fixed_coeff fc;		/* cached fixed coefficients */
    float flatness;		/* flatness for character curves */
    point_scale scale;		/* oversampling scale */
    gs_log2_scale_point log2_subpixels;	/* log2 of the number of subpixels */
    gs_fixed_point origin;	/* character origin */
    /* The following are updated dynamically */
    fixed ostack[ostack_size];	/* the Type 1 operand stack */
    int os_count;		/* # of occupied stack entries */
    ip_state_t ipstack[ipstack_size + 1];	/* control stack */
    int ips_count;		/* # of occupied entries */
    int init_done;		/* -1 if not done & not needed, */
				/* 0 if not done & needed, 1 if done */
    bool sb_set;		/* true if lsb is preset */
    bool width_set;		/* true if width is set (for seac parts) */
    /* (Type 2 charstrings only) */
    int num_hints;		/* number of hints (Type 2 only) */
    gs_fixed_point lsb;		/* left side bearing (char coords) */
    gs_fixed_point width;	/* character width (char coords) */
    int seac_accent;		/* accent character code for seac, or -1 */
    fixed save_asb;		/* save seac asb */
    gs_fixed_point save_lsb;	/* save seac accented lsb */
    gs_fixed_point save_adxy;	/* save seac adx/ady */
    fixed asb_diff;		/* save_asb - save_lsb.x, */
				/* needed to adjust Flex endpoint */
    gs_fixed_point adxy;	/* seac accent displacement, */
				/* needed to adjust currentpoint */
    int flex_path_state_flags;	/* record whether path was open */
				/* at start of Flex section */
#define flex_max 8
    int flex_count;
    int ignore_pops;		/* # of pops to ignore (after */
				/* a known othersubr call) */
    /* The following are set dynamically. */
    gs_fixed_point vs_offset;	/* device space offset for centering */
				/* middle stem of vstem3 */
				/* of subpath */
    fixed transient_array[32];	/* Type 2 transient array, */
    /* will be variable-size someday */
};

extern_st(st_gs_type1_state);
#define public_st_gs_type1_state() /* in gxtype1.c */\
  gs_public_st_composite(st_gs_type1_state, gs_type1_state, "gs_type1_state",\
    gs_type1_state_enum_ptrs, gs_type1_state_reloc_ptrs)

/* ------ Shared Type 1 / Type 2 interpreter fragments ------ */

/* Define a pointer to the charstring interpreter stack. */
typedef fixed *cs_ptr;

/* Clear the operand stack. */
/* The cast avoids compiler warning about a "negative subscript." */
#define CLEAR_CSTACK(cstack, csp)\
  (csp = (cs_ptr)(cstack) - 1)

/* Copy the operand stack out of the saved state. */
#define INIT_CSTACK(cstack, csp, pcis)\
  BEGIN\
    if ( pcis->os_count == 0 )\
      CLEAR_CSTACK(cstack, csp);\
    else {\
      memcpy(cstack, pcis->ostack, pcis->os_count * sizeof(fixed));\
      csp = &cstack[pcis->os_count - 1];\
    }\
  END

#define CS_CHECK_PUSH(csp, cstack)\
  BEGIN\
    if (csp >= &cstack[countof(cstack)-1])\
      return_error(gs_error_invalidfont);\
  END

/* Decode a 1-byte number. */
#define decode_num1(var, c)\
  (var = c_value_num1(c))
#define decode_push_num1(csp, cstack, c)\
  BEGIN\
    CS_CHECK_PUSH(csp, cstack);\
    *++csp = int2fixed(c_value_num1(c));\
  END

/* Decode a 2-byte number. */
#define decode_num2(var, c, cip, state, encrypted)\
  BEGIN\
    uint c2 = *cip++;\
    int cn = charstring_this(c2, state, encrypted);\
\
    var = (c < c_neg2_0 ? c_value_pos2(c, 0) + cn :\
	   c_value_neg2(c, 0) - cn);\
    charstring_skip_next(c2, state, encrypted);\
  END
#define decode_push_num2(csp, cstack, c, cip, state, encrypted)\
  BEGIN\
    uint c2 = *cip++;\
    int cn;\
\
    CS_CHECK_PUSH(csp, cstack);\
    cn = charstring_this(c2, state, encrypted);\
    if ( c < c_neg2_0 )\
      { if_debug2('1', "[1] (%d)+%d\n", c_value_pos2(c, 0), cn);\
        *++csp = int2fixed(c_value_pos2(c, 0) + (int)cn);\
      }\
    else\
      { if_debug2('1', "[1] (%d)-%d\n", c_value_neg2(c, 0), cn);\
        *++csp = int2fixed(c_value_neg2(c, 0) - (int)cn);\
      }\
    charstring_skip_next(c2, state, encrypted);\
  END

/* Decode a 4-byte number, but don't push it, because Type 1 and Type 2 */
/* charstrings scale it differently. */
#if arch_sizeof_long > 4
#  define sign_extend_num4(lw)\
     lw = (lw ^ 0x80000000L) - 0x80000000L
#else
#  define sign_extend_num4(lw) DO_NOTHING
#endif
#define decode_num4(lw, cip, state, encrypted)\
  BEGIN\
    int i;\
    uint c4;\
\
    lw = 0;\
    for ( i = 4; --i >= 0; )\
      { charstring_next(*cip, state, c4, encrypted);\
        lw = (lw << 8) + c4;\
	cip++;\
      }\
    sign_extend_num4(lw);\
  END

/* ------ Shared Type 1 / Type 2 charstring utilities ------ */

void gs_type1_finish_init(gs_type1_state * pcis);

int gs_type1_sbw(gs_type1_state * pcis, fixed sbx, fixed sby,
		 fixed wx, fixed wy);

/* blend returns the number of values to pop. */
int gs_type1_blend(gs_type1_state *pcis, fixed *csp, int num_results);

int gs_type1_seac(gs_type1_state * pcis, const fixed * cstack,
		  fixed asb_diff, ip_state_t * ipsp);

int gs_type1_endchar(gs_type1_state * pcis);

/* Get the metrics (l.s.b. and width) from the Type 1 interpreter. */
void type1_cis_get_metrics(const gs_type1_state * pcis, double psbw[4]);

#endif /* gxtype1_INCLUDED */
