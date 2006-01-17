/* Copyright (C) 1994, 1997, 1998 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gshsb.c,v 1.5 2002/06/16 05:48:55 lpd Exp $ */
/* HSB color operators for Ghostscript library */
#include "gx.h"
#include "gscolor.h"
#include "gshsb.h"		/* interface definition */
#include "gxfrac.h"

/* Forward references */
private void color_hsb_to_rgb(floatp h, floatp s, floatp b, float rgb[3]);
private void color_rgb_to_hsb(floatp r, floatp g, floatp b, float hsb[3]);

/* Force a parameter into the range [0.0..1.0]. */
#define force_unit(p) (p < 0.0 ? 0.0 : p > 1.0 ? 1.0 : p)

/* sethsbcolor */
int
gs_sethsbcolor(gs_state * pgs, floatp h, floatp s, floatp b)
{
    float rgb[3];

    color_hsb_to_rgb(force_unit(h), force_unit(s), force_unit(b), rgb);
    return gs_setrgbcolor(pgs, rgb[0], rgb[1], rgb[2]);
}

/* currenthsbcolor */
int
gs_currenthsbcolor(const gs_state * pgs, float pr3[3])
{
    float rgb[3];

    gs_currentrgbcolor(pgs, rgb);
    color_rgb_to_hsb(rgb[0], rgb[1], rgb[2], pr3);
    return 0;
}

/* ------ Internal routines ------ */

/* Note: the color model conversion algorithms are taken from */
/* Rogers, Procedural Elements for Computer Graphics, pp. 401-403. */

/* Convert RGB to HSB. */
private void
color_rgb_to_hsb(floatp r, floatp g, floatp b, float hsb[3])
{
    frac red = float2frac(r), green = float2frac(g), blue = float2frac(b);

#define rhue hsb[0]
#define rsat hsb[1]
#define rbri hsb[2]
    if (red == green && green == blue) {
	rhue = 0;		/* arbitrary */
	rsat = 0;
	rbri = r;		/* pick any one */
    } else {			/* Convert rgb to hsb */
	frac V, Temp, diff;
	long H;

	V = (red > green ? red : green);
	if (blue > V)
	    V = blue;
	Temp = (red > green ? green : red);
	if (blue < Temp)
	    Temp = blue;
	diff = V - Temp;
	if (V == red)
	    H = (green - blue) * frac_1_long / diff;
	else if (V == green)
	    H = (blue - red) * frac_1_long / diff + 2 * frac_1_long;
	else			/* V == blue */
	    H = (red - green) * frac_1_long / diff + 4 * frac_1_long;
	if (H < 0)
	    H += 6 * frac_1_long;
	rhue = H / (frac_1 * 6.0);
	rsat = diff / (float)V;
	rbri = frac2float(V);
    }
#undef rhue
#undef rsat
#undef rbri
}

/* Convert HSB to RGB. */
private void
color_hsb_to_rgb(floatp hue, floatp saturation, floatp brightness, float rgb[3])
{
    if (saturation == 0) {
	rgb[0] = rgb[1] = rgb[2] = brightness;
    } else {			/* Convert hsb to rgb. */
	/* We rely on the fact that the product of two */
	/* fracs fits into an unsigned long. */
	floatp h6 = hue * 6;
	ulong V = float2frac(brightness);	/* force arithmetic to long */
	frac S = float2frac(saturation);
	int I = (int)h6;
	ulong F = float2frac(h6 - I);	/* ditto */

	/* M = V*(1-S), N = V*(1-S*F), K = V*(1-S*(1-F)) = M-N+V */
	frac M = V * (frac_1_long - S) / frac_1_long;
	frac N = V * (frac_1_long - S * F / frac_1_long) / frac_1_long;
	frac K = M - N + V;
	frac R, G, B;

	switch (I) {
	    default:
		R = V;
		G = K;
		B = M;
		break;
	    case 1:
		R = N;
		G = V;
		B = M;
		break;
	    case 2:
		R = M;
		G = V;
		B = K;
		break;
	    case 3:
		R = M;
		G = N;
		B = V;
		break;
	    case 4:
		R = K;
		G = M;
		B = V;
		break;
	    case 5:
		R = V;
		G = M;
		B = N;
		break;
	}
	rgb[0] = frac2float(R);
	rgb[1] = frac2float(G);
	rgb[2] = frac2float(B);
#ifdef DEBUG
	if (gs_debug_c('c')) {
	    dlprintf7("[c]hsb(%g,%g,%g)->VSFI(%ld,%d,%ld,%d)->\n",
		      hue, saturation, brightness, V, S, F, I);
	    dlprintf6("   RGB(%d,%d,%d)->rgb(%g,%g,%g)\n",
		      R, G, B, rgb[0], rgb[1], rgb[2]);
	}
#endif
    }
}
