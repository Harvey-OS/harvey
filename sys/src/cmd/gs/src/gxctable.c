/* Copyright (C) 1995, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxctable.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Color table lookup and interpolation */
#include "gx.h"
#include "gxfixed.h"
#include "gxfrac.h"
#include "gxctable.h"

/* See gxctable.h for the API and structure definitions. */

/*
 * Define an implementation that simply picks the nearest value without
 * any interpolation.
 */
void
gx_color_interpolate_nearest(const fixed * pi,
			     const gx_color_lookup_table * pclt, frac * pv)
{
    const int *pdim = pclt->dims;
    int m = pclt->m;
    const gs_const_string *table = pclt->table;

    if (pclt->n > 3) {
	table += fixed2int_var_rounded(pi[0]) * pdim[1];
	++pi, ++pdim;
    } {
	int ic = fixed2int_var_rounded(pi[2]);
	int ib = fixed2int_var_rounded(pi[1]);
	int ia = fixed2int_var_rounded(pi[0]);
	const byte *p = pclt->table[ia].data + (ib * pdim[2] + ic) * m;
	int j;

	for (j = 0; j < m; ++j, ++p)
	    pv[j] = byte2frac(*p);
    }
}

/*
 * Define an implementation that uses trilinear interpolation.
 */
private void
interpolate_accum(const fixed * pi, const gx_color_lookup_table * pclt,
		  frac * pv, fixed factor)
{
    const int *pdim = pclt->dims;
    int m = pclt->m;

    if (pclt->n > 3) {
	/* Do two 3-D interpolations, interpolating between them. */
	gx_color_lookup_table clt3;
	int ix = fixed2int_var(pi[0]);
	fixed fx = fixed_fraction(pi[0]);

	clt3.n = 3;
	/*clt3.dims[0] = pdim[1]; *//* not used */
	clt3.dims[1] = pdim[2];
	clt3.dims[2] = pdim[3];
	clt3.m = m;
	clt3.table = pclt->table + ix * pdim[1];
	interpolate_accum(pi + 1, &clt3, pv, fixed_1);
	if (ix == pdim[0] - 1)
	    return;
	clt3.table += pdim[1];
	interpolate_accum(pi + 1, &clt3, pv, fx);
    } else {
	int ic = fixed2int_var(pi[2]);
	fixed fc = fixed_fraction(pi[2]);
	uint dc1 = (ic == pdim[2] - 1 ? 0 : m);
	int ib = fixed2int_var(pi[1]);
	fixed fb = fixed_fraction(pi[1]);
	uint db1 = (ib == pdim[1] - 1 ? 0 : pdim[2] * m);
	uint dbc = (ib * pdim[2] + ic) * m;
	uint dbc1 = db1 + dc1;
	int ia = fixed2int_var(pi[0]);
	fixed fa = fixed_fraction(pi[0]);
	const byte *pa0 = pclt->table[ia].data + dbc;
	const byte *pa1 =
	    (ia == pdim[0] - 1 ? pa0 : pclt->table[ia + 1].data + dbc);
	int j;

	/* The values to be interpolated are */
	/* pa{0,1}[{0,db1,dc1,dbc1}]. */
	for (j = 0; j < m; ++j, ++pa0, ++pa1) {
	    frac v000 = byte2frac(pa0[0]);
	    frac v001 = byte2frac(pa0[dc1]);
	    frac v010 = byte2frac(pa0[db1]);
	    frac v011 = byte2frac(pa0[dbc1]);
	    frac v100 = byte2frac(pa1[0]);
	    frac v101 = byte2frac(pa1[dc1]);
	    frac v110 = byte2frac(pa1[db1]);
	    frac v111 = byte2frac(pa1[dbc1]);
	    frac rv;

	    frac v00 = v000 +
		(frac) arith_rshift((long)fc * (v001 - v000),
				    _fixed_shift);
	    frac v01 = v010 +
		(frac) arith_rshift((long)fc * (v011 - v010),
				    _fixed_shift);
	    frac v10 = v100 +
		(frac) arith_rshift((long)fc * (v101 - v100),
				    _fixed_shift);
	    frac v11 = v110 +
		(frac) arith_rshift((long)fc * (v111 - v110),
				    _fixed_shift);

	    frac v0 = v00 +
		(frac) arith_rshift((long)fb * (v01 - v00),
				    _fixed_shift);
	    frac v1 = v10 +
		(frac) arith_rshift((long)fb * (v11 - v10),
				    _fixed_shift);

	    rv = v0 +
		(frac) arith_rshift((long)fa * (v1 - v0),
				    _fixed_shift);
	    if (factor == fixed_1)
		pv[j] = rv;
	    else
		pv[j] += (frac) arith_rshift((long)factor * (rv - pv[j]),
					     _fixed_shift);
	}
    }
}
void
gx_color_interpolate_linear(const fixed * pi,
			    const gx_color_lookup_table * pclt, frac * pv)
{
    interpolate_accum(pi, pclt, pv, fixed_1);
}
