/* Copyright (C) 1989 Aladdin Enterprises.  All rights reserved.
  
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

/* gzline.h */
/* Private line parameters for Ghostscript */
#include "gsline.h"

/* Line parameter structures */
/* gx_dash_params are never instantiated by themselves. */
typedef struct gx_dash_params_s {
	float *pattern;
	uint pattern_size;
	float offset;
	/* The rest of the parameters are computed from the above */
	bool init_ink_on;		/* true if ink is initially on */
	int init_index;			/* initial index in pattern */
	float init_dist_left;
} gx_dash_params;
typedef struct gx_line_params_s {
	float width;			/* one-half line width */
	gs_line_cap cap;
	gs_line_join join;
	float miter_limit;
	float miter_check;		/* computed from miter limit, */
					/* see gs_setmiterlimit and */
					/* gs_stroke */
	gx_dash_params dash;
} gx_line_params;
#define private_st_line_params() /* in gsstate.c */\
  gs_private_st_ptrs1(st_line_params, gx_line_params, "line_params",\
    line_params_enum_ptrs, line_params_reloc_ptrs, dash.pattern)
#define st_line_params_max_ptrs 1

/* Internal accessor for line parameters in graphics state */
const gx_line_params *gs_currentlineparams(P1(const gs_state *));
