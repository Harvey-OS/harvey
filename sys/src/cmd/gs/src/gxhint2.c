/* Copyright (C) 1990, 1995, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxhint2.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Character level hints for Type 1 fonts. */
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "gxtype1.h"

/* Define the tolerance for testing whether a point is in a zone, */
/* in device pixels.  (Maybe this should be variable?) */
#define STEM_TOLERANCE float2fixed(0.05)

/* Forward references */

private stem_hint *type1_stem(P4(const gs_type1_state *, stem_hint_table *, fixed, fixed));
private fixed find_snap(P3(fixed, const stem_snap_table *, const pixel_scale *));
private alignment_zone *find_zone(P3(gs_type1_state *, fixed, fixed));

/* Reset the stem hints. */
void
reset_stem_hints(gs_type1_state * pcis)
{
    pcis->hstem_hints.count = pcis->hstem_hints.replaced_count = 0;
    pcis->vstem_hints.count = pcis->vstem_hints.replaced_count = 0;
    update_stem_hints(pcis);
}

/* Prepare to replace the stem hints. */
private void
save_replaced_hints(stem_hint_table * psht)
{
    int rep_count = min(psht->replaced_count + psht->count, max_stems);

    memmove(&psht->data[max_stems - rep_count], &psht->data[0],
	    psht->count * sizeof(psht->data[0]));
    psht->replaced_count = rep_count;
    psht->count = psht->current = 0;
}
void
type1_replace_stem_hints(gs_type1_state * pcis)
{
    if_debug2('y', "[y]saving hints: %d hstem, %d vstem\n",
	      pcis->hstem_hints.count, pcis->vstem_hints.count);
    save_replaced_hints(&pcis->hstem_hints);
    save_replaced_hints(&pcis->vstem_hints);
    if_debug2('y', "[y]total saved hints: %d hstem, %d vstem\n",
	      pcis->hstem_hints.replaced_count,
	      pcis->vstem_hints.replaced_count);
}

/* Update the internal stem hint pointers after moving or copying the state. */
void
update_stem_hints(gs_type1_state * pcis)
{
    pcis->hstem_hints.current = 0;
    pcis->vstem_hints.current = 0;
}

/* ------ Add hints ------ */

#undef c_fixed
#define c_fixed(d, c) m_fixed(d, c, pcis->fc, max_coeff_bits)

#define if_debug_print_add_stem(chr, msg, psht, psh, c, dc, v, dv)\
	if_debug10(chr, "%s %d/%d: %g,%g -> %g(%g)%g ; d = %g,%g\n",\
		   msg, (int)((psh) - &(psht)->data[0]), (psht)->count,\
		   fixed2float(c), fixed2float(dc),\
		   fixed2float(v), fixed2float(dv), fixed2float((v) + (dv)),\
		   fixed2float((psh)->dv0), fixed2float((psh)->dv1))

/* Compute and store the adjusted stem coordinates. */
private void
store_stem_deltas(const stem_hint_table * psht, stem_hint * psh,
		  const pixel_scale * psp, fixed v, fixed dv, fixed adj_dv)
{
    /*
     * We want to align the stem so its edges fall on pixel boundaries
     * (possibly "big pixel" boundaries if we are oversampling),
     * but if hint replacement has occurred, we must shift edges in a
     * consistent way.  This is a real nuisance, but I don't see how
     * to avoid it; if we don't do it, we get bizarre anomalies like
     * disappearing stems.
     */
    const stem_hint *psh0 = 0;
    const stem_hint *psh1 = 0;
    int i;

    /*
     * If we ever had a hint with the same edge(s), align this one
     * the same way.
     */
    for (i = max_stems - psht->replaced_count; i < max_stems; ++i) {
	const stem_hint *ph = &psht->data[i];

	if (ph == psh)
	    continue;
	if (ph->v0 == psh->v0)
	    psh0 = ph;
	if (ph->v1 == psh->v1)
	    psh1 = ph;
    }
    for (i = 0; i < psht->count; ++i) {
	const stem_hint *ph = &psht->data[i];

	if (ph == psh)
	    continue;
	if (ph->v0 == psh->v0)
	    psh0 = ph;
	if (ph->v1 == psh->v1)
	    psh1 = ph;
    }
    if (psh0 != 0) {
	psh->dv0 = psh0->dv0;
	if (psh1 != 0) {	/* Both edges are determined. */
	    psh->dv1 = psh1->dv1;
	} else {		/* Only the lower edge is determined. */
	    psh->dv1 = psh->dv0 + adj_dv - dv;
	}
    } else if (psh1 != 0) {	/* Only the upper edge is determined. */
	psh->dv1 = psh1->dv1;
	psh->dv0 = psh->dv1 + adj_dv - dv;
    } else {			/* Neither edge is determined. */
	fixed diff2_dv = arith_rshift_1(adj_dv - dv);
	fixed edge = v - diff2_dv;
	fixed diff_v = scaled_rounded(edge, psp) - edge;

	psh->dv0 = diff_v - diff2_dv;
	psh->dv1 = diff_v + diff2_dv;
    }
}

/*
 * The Type 1 font format uses negative stem widths to indicate edge hints.
 * We need to convert these into zero-width stem hints.
 */
private void
detect_edge_hint(fixed *xy, fixed *dxy)
{
    if (*dxy == -21) {
	/* Bottom edge hint. */
	*xy -= 21, *dxy = 0;
    } else if (*dxy == -20) {
	/* Top edge hint. */
	*dxy = 0;
    }
}

/* Add a horizontal stem hint. */
void
type1_do_hstem(gs_type1_state * pcis, fixed y, fixed dy,
	       const gs_matrix_fixed * pmat)
{
    stem_hint *psh;
    alignment_zone *pz;
    const pixel_scale *psp;
    fixed v, dv, adj_dv;
    fixed vtop, vbot;

    if (!pcis->fh.use_y_hints || !pmat->txy_fixed_valid)
	return;
    detect_edge_hint(&y, &dy);
    y += pcis->lsb.y + pcis->adxy.y;
    if (pcis->fh.axes_swapped) {
	psp = &pcis->scale.x;
	v = pcis->vs_offset.x + c_fixed(y, yx) + pmat->tx_fixed;
	dv = c_fixed(dy, yx);
    } else {
	psp = &pcis->scale.y;
	v = pcis->vs_offset.y + c_fixed(y, yy) + pmat->ty_fixed;
	dv = c_fixed(dy, yy);
    }
    if (dy < 0)
	vbot = v + dv, vtop = v;
    else
	vbot = v, vtop = v + dv;
    if (dv < 0)
	v += dv, dv = -dv;
    psh = type1_stem(pcis, &pcis->hstem_hints, v, dv);
    if (psh == 0)
	return;
    adj_dv = find_snap(dv, &pcis->fh.snap_h, psp);
    pz = find_zone(pcis, vbot, vtop);
    if (pz != 0) {		/* Use the alignment zone to align the outer stem edge. */
	int inverted =
	(pcis->fh.axes_swapped ? pcis->fh.x_inverted : pcis->fh.y_inverted);
	int adjust_v1 =
	(inverted ? !pz->is_top_zone : pz->is_top_zone);
	fixed flat_v = pz->flat;
	fixed overshoot =
	(pz->is_top_zone ? vtop - flat_v : flat_v - vbot);
	fixed pos_over =
	(inverted ? -overshoot : overshoot);
	fixed ddv = adj_dv - dv;
	fixed shift = scaled_rounded(flat_v, psp) - flat_v;

	if (pos_over > 0) {
	    if (pos_over < pcis->fh.blue_shift || pcis->fh.suppress_overshoot) {	/* Character is small, suppress overshoot. */
		if_debug0('y', "[y]suppress overshoot\n");
		if (pz->is_top_zone)
		    shift -= overshoot;
		else
		    shift += overshoot;
	    } else if (pos_over < psp->unit) {	/* Enforce overshoot. */
		if_debug0('y', "[y]enforce overshoot\n");
		if (overshoot < 0)
		    overshoot = -psp->unit - overshoot;
		else
		    overshoot = psp->unit - overshoot;
		if (pz->is_top_zone)
		    shift += overshoot;
		else
		    shift -= overshoot;
	    }
	}
	if (adjust_v1)
	    psh->dv1 = shift, psh->dv0 = shift - ddv;
	else
	    psh->dv0 = shift, psh->dv1 = shift + ddv;
	if_debug2('y', "[y]flat_v = %g, overshoot = %g for:\n",
		  fixed2float(flat_v), fixed2float(overshoot));
    } else {			/* Align the stem so its edges fall on pixel boundaries. */
	store_stem_deltas(&pcis->hstem_hints, psh, psp, v, dv, adj_dv);
    }
    if_debug_print_add_stem('y', "[y]hstem", &pcis->hstem_hints, psh,
			    y, dy, v, dv);
}

/* Add a vertical stem hint. */
void
type1_do_vstem(gs_type1_state * pcis, fixed x, fixed dx,
	       const gs_matrix_fixed * pmat)
{
    stem_hint *psh;
    const pixel_scale *psp;
    fixed v, dv, adj_dv;

    if (!pcis->fh.use_x_hints)
	return;
    detect_edge_hint(&x, &dx);
    x += pcis->lsb.x + pcis->adxy.x;
    if (pcis->fh.axes_swapped) {
	psp = &pcis->scale.y;
	v = pcis->vs_offset.y + c_fixed(x, xy) + pmat->ty_fixed;
	dv = c_fixed(dx, xy);
    } else {
	psp = &pcis->scale.x;
	v = pcis->vs_offset.x + c_fixed(x, xx) + pmat->tx_fixed;
	dv = c_fixed(dx, xx);
    }
    if (dv < 0)
	v += dv, dv = -dv;
    psh = type1_stem(pcis, &pcis->vstem_hints, v, dv);
    if (psh == 0)
	return;
    adj_dv = find_snap(dv, &pcis->fh.snap_v, psp);
    if (pcis->pfont->data.ForceBold && adj_dv < psp->unit)
	adj_dv = psp->unit;
    /* Align the stem so its edges fall on pixel boundaries. */
    store_stem_deltas(&pcis->vstem_hints, psh, psp, v, dv, adj_dv);
    if_debug_print_add_stem('y', "[y]vstem", &pcis->vstem_hints, psh,
			    x, dx, v, dv);
}

/* Adjust the character center for a vstem3. */
/****** NEEDS UPDATING FOR SCALE ******/
void
type1_do_center_vstem(gs_type1_state * pcis, fixed x0, fixed dx,
		      const gs_matrix_fixed * pmat)
{
    fixed x1 = x0 + dx;
    gs_fixed_point pt0, pt1, width;
    fixed center, int_width;
    fixed *psxy;

    if (gs_point_transform2fixed(pmat, fixed2float(x0), 0.0, &pt0) < 0 ||
	gs_point_transform2fixed(pmat, fixed2float(x1), 0.0, &pt1) < 0
	) {			/* Punt. */
	return;
    }
    width.x = pt0.x - pt1.x;
    if (width.x < 0)
	width.x = -width.x;
    width.y = pt0.y - pt1.y;
    if (width.y < 0)
	width.y = -width.y;
    if (width.y < float2fixed(0.05)) {	/* Vertical on device */
	center = arith_rshift_1(pt0.x + pt1.x);
	int_width = fixed_rounded(width.x);
	psxy = &pcis->vs_offset.x;
    } else {			/* Horizontal on device */
	center = arith_rshift_1(pt0.y + pt1.y);
	int_width = fixed_rounded(width.y);
	psxy = &pcis->vs_offset.y;
    }
    if (int_width == fixed_0 || (int_width & fixed_1) == 0) {	/* Odd width, center stem over pixel. */
	*psxy = fixed_floor(center) + fixed_half - center;
    } else {			/* Even width, center stem between pixels. */
	*psxy = fixed_rounded(center) - center;
    }
    /* We can't fix up the current point here, */
    /* but we can fix up everything else. */
/****** TO BE COMPLETED ******/
}

/* Add a stem hint, keeping the table sorted. */
/* We know that d >= 0. */
/* Return the stem hint pointer, or 0 if the table is full. */
private stem_hint *
type1_stem(const gs_type1_state * pcis, stem_hint_table * psht,
	   fixed v0, fixed d)
{
    stem_hint *bot = &psht->data[0];
    stem_hint *top = bot + psht->count;

    if (psht->count >= max_stems)
	return 0;
    while (top > bot && v0 < top[-1].v0) {
	*top = top[-1];
	top--;
    }
    /* Add a little fuzz for insideness testing. */
    top->v0 = v0 - STEM_TOLERANCE;
    top->v1 = v0 + d + STEM_TOLERANCE;
    top->index = pcis->hstem_hints.count + pcis->vstem_hints.count;
    top->active = true;
    psht->count++;
    return top;
}

/* Compute the adjusted width of a stem. */
/* The value returned is always a multiple of scale.unit. */
private fixed
find_snap(fixed dv, const stem_snap_table * psst, const pixel_scale * pps)
{				/* We aren't sure why a maximum difference of pps->half */
    /* works better than pps->unit, but it does. */
#define max_snap_distance (pps->half)
    fixed best = max_snap_distance;
    fixed adj_dv;
    int i;

    for (i = 0; i < psst->count; i++) {
	fixed diff = psst->data[i] - dv;

	if (any_abs(diff) < any_abs(best)) {
	    if_debug3('Y', "[Y]possibly snap %g to [%d]%g\n",
		      fixed2float(dv), i,
		      fixed2float(psst->data[i]));
	    best = diff;
	}
    }
    adj_dv = scaled_rounded((any_abs(best) < max_snap_distance ?
			     dv + best : dv),
			    pps);
    if (adj_dv == 0)
	adj_dv = pps->unit;
#ifdef DEBUG
    if (adj_dv == dv)
	if_debug1('Y', "[Y]no snap %g\n", fixed2float(dv));
    else
	if_debug2('Y', "[Y]snap %g to %g\n",
		  fixed2float(dv), fixed2float(adj_dv));
#endif
    return adj_dv;
#undef max_snap_distance
}

/* Find the applicable alignment zone for a stem, if any. */
/* vbot and vtop are the bottom and top of the stem, */
/* but without interchanging if the y axis is inverted. */
private alignment_zone *
find_zone(gs_type1_state * pcis, fixed vbot, fixed vtop)
{
    alignment_zone *pz;

    for (pz = &pcis->fh.a_zones[pcis->fh.a_zone_count];
	 --pz >= &pcis->fh.a_zones[0];
	) {
	fixed v = (pz->is_top_zone ? vtop : vbot);

	if (v >= pz->v0 && v <= pz->v1) {
	    if_debug2('Y', "[Y]stem crosses %s-zone %d\n",
		      (pz->is_top_zone ? "top" : "bottom"),
		      (int)(pz - &pcis->fh.a_zones[0]));
	    return pz;
	}
    }
    return 0;
}
