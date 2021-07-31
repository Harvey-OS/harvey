/* Copyright (C) 1989, 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gsline.c */
/* Line parameter operators for Ghostscript library */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxfixed.h"			/* ditto */
#include "gxmatrix.h"			/* for gzstate */
#include "gzstate.h"
#include "gzline.h"

/* ------ Device-independent parameters ------ */

/* setlinewidth */
int
gs_setlinewidth(gs_state *pgs, floatp width)
{	pgs->line_params->width = width / 2;
	return 0;
}

/* currentlinewidth */
float
gs_currentlinewidth(const gs_state *pgs)
{	return (float)(pgs->line_params->width * 2);
}

/* setlinecap */
int
gs_setlinecap(gs_state *pgs, gs_line_cap cap)
{	pgs->line_params->cap = cap;
	return 0;
}

/* currentlinecap */
gs_line_cap
gs_currentlinecap(const gs_state *pgs)
{	return pgs->line_params->cap;
}

/* setlinejoin */
int
gs_setlinejoin(gs_state *pgs, gs_line_join join)
{	pgs->line_params->join = join;
	return 0;
}

/* currentlinejoin */
gs_line_join
gs_currentlinejoin(const gs_state *pgs)
{	return pgs->line_params->join;
}

/* setmiterlimit */
int
gs_setmiterlimit(gs_state *pgs, floatp limit)
{	if ( limit < 1.0 ) return_error(gs_error_rangecheck);
	pgs->line_params->miter_limit = limit;
	/* The supplied miter limit is an upper bound on */
	/* 1/sin(phi/2).  We convert this to a lower bound on */
	/* tan(phi).  Note that if phi > pi/2, this is negative. */
	/* We use the half-angle and angle-sum formulas here */
	/* to avoid the trig functions.... */
	   {	double limit_sq = limit * limit;
	/* We need a special check for phi/2 close to pi/4. */
	/* Some C compilers can't handle the following as a */
	/* conditional expression.... */
		if ( limit_sq < 2.0001 && limit_sq > 1.9999 )
			pgs->line_params->miter_check = 1.0e6;
		else
			pgs->line_params->miter_check =
				sqrt(limit_sq - 1) * 2 / (limit_sq - 2);
	   }
	return 0;
}

/* currentmiterlimit */
float
gs_currentmiterlimit(const gs_state *pgs)
{	return pgs->line_params->miter_limit;
}

/* setdash */
int
gs_setdash(gs_state *pgs, const float *pattern, uint length, floatp offset)
{	uint n = length;
	const float *dfrom = pattern;
	bool ink = true;
	int index = 0;
	float pattern_length = 0.0;
	float dist_left;
	gx_dash_params *dash = &pgs->line_params->dash;
	float *ppat;
	/* Check the dash pattern */
	while ( n-- )
	{	float elt = *dfrom++;
		if ( elt < 0 )
			return_error(gs_error_rangecheck);
		pattern_length += elt;
	}
	if ( length == 0 )		/* empty pattern */
	{	dist_left = 0.0;
		ppat = 0;
	}
	else
	{	if ( pattern_length == 0 )
			return_error(gs_error_rangecheck);
		/* Compute the initial index, ink_on, and distance left */
		/* in the pattern, according to the offset. */
#define f_mod(a, b) ((a) - floor((a) / (b)) * (b))
		if ( length & 1 )
		{	/* Odd and even repetitions of the pattern */
			/* have opposite ink values! */
			float length2 = pattern_length * 2;
			dist_left = f_mod(offset, length2);
			if ( dist_left >= pattern_length )
				dist_left -= pattern_length,
				ink = !ink;
		}
		else
			dist_left = f_mod(offset, pattern_length);
		while ( (dist_left -= pattern[index]) >= 0 )
			ink = !ink, index++;
		ppat = (float *)gs_alloc_bytes(pgs->memory,
					       length * sizeof(float),
					       "dash pattern");
		if ( ppat == 0 ) return_error(gs_error_VMerror);
		memcpy(ppat, pattern, length * sizeof(float));
	}
	dash->pattern = ppat;
	dash->pattern_size = length;
	dash->offset = offset;
	dash->init_ink_on = ink;
	dash->init_index = index;
	dash->init_dist_left = -dist_left;
	return 0;
}

/* currentdash */
uint
gs_currentdash_length(const gs_state *pgs)
{	return pgs->line_params->dash.pattern_size;
}
const float *
gs_currentdash_pattern(const gs_state *pgs)
{	return pgs->line_params->dash.pattern;
}
float
gs_currentdash_offset(const gs_state *pgs)
{	return pgs->line_params->dash.offset;
}

/* Internal accessor for line parameters */
const gx_line_params *
gs_currentlineparams(const gs_state *pgs)
{	return pgs->line_params;
}

/* ------ Device-dependent parameters ------ */

/* setflat */
int
gs_setflat(gs_state *pgs, floatp flat)
{	if ( flat <= 0.2 ) flat = 0.2;
	else if ( flat > 100 ) flat = 100;
	pgs->flatness = flat;
	return 0;
}

/* currentflat */
float
gs_currentflat(const gs_state *pgs)
{	return pgs->flatness;
}

/* setstrokeadjust */
int
gs_setstrokeadjust(gs_state *pgs, bool stroke_adjust)
{	pgs->stroke_adjust = stroke_adjust;
	return 0;
}

/* currentstrokeadjust */
bool
gs_currentstrokeadjust(const gs_state *pgs)
{	return pgs->stroke_adjust;
}
