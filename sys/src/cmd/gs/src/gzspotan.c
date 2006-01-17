/* Copyright (C) 2003 Aladdin Enterprises.  All rights reserved.
  
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

/*$Id: gzspotan.c,v 1.14 2005/05/12 02:01:32 alexcher Exp $ */
/* A spot analyzer device implementation. */
/*
    This implements a spot topology analyzis and 
    stem recognition for True Type grid fitting.
*/
#include "gx.h"
#include "gserrors.h"
#include "gsdevice.h"
#include "gzspotan.h"
#include "gxfixed.h"
#include "gxdevice.h"
#include "gxfdrop.h" /* Only for VD_* constants. */
#include "gzpath.h"
#include "memory_.h"
#include "math_.h"
#include "vdtrace.h"
#include <assert.h>

#define VD_TRAP_N_COLOR RGB(128, 128, 0)
#define VD_TRAP_U_COLOR RGB(0, 0, 255)
#define VD_CONT_COLOR RGB(0, 255, 0)
#define VD_STEM_COLOR RGB(255, 255, 255)
#define VD_HINT_COLOR RGB(255, 0, 0)

public_st_device_spot_analyzer();
private_st_san_trap();
private_st_san_trap_contact();

private dev_proc_open_device(san_open);
private dev_proc_close_device(san_close);
private dev_proc_get_clipping_box(san_get_clipping_box);


/* --------------------- List management ------------------------- */
/* fixme : use something like C++ patterns to generate same functions for various types. */


private inline void
free_trap_list(gs_memory_t *mem, gx_san_trap **list)
{
    gx_san_trap *t = *list, *t1;

    for (t = *list; t != NULL; t = t1) {
	t1 = t->link;
	gs_free_object(mem, t, "free_trap_list");
    }
    *list = 0;
}

private inline void
free_cont_list(gs_memory_t *mem, gx_san_trap_contact **list)
{
    gx_san_trap_contact *t = *list, *t1;

    for (t = *list; t != NULL; t = t1) {
	t1 = t->link;
	gs_free_object(mem, t, "free_trap_list");
    }
    *list = 0;
}

private inline gx_san_trap *
trap_reserve(gx_device_spot_analyzer *padev)
{
    gx_san_trap *t = padev->trap_free;

    if (t != NULL) {
	padev->trap_free = t->link;
    } else {
	if (padev->trap_buffer_count > 10000)
	    return NULL;
	t = gs_alloc_struct(padev->memory, gx_san_trap, 
		&st_san_trap, "trap_reserve");
	if (t == NULL)
	    return NULL;
	t->link = NULL;
	if (padev->trap_buffer_last == NULL)
	    padev->trap_buffer = t;
	else
	    padev->trap_buffer_last->link = t;
	padev->trap_buffer_last = t;
	padev->trap_buffer_count++;
    }
    return t;
}

private inline gx_san_trap_contact *
cont_reserve(gx_device_spot_analyzer *padev)
{
    gx_san_trap_contact *t = padev->cont_free;

    if (t != NULL) {
	padev->cont_free = t->link;
    } else {
	if (padev->cont_buffer_count > 10000)
	    return NULL;
	t = gs_alloc_struct(padev->memory, gx_san_trap_contact, 
		&st_san_trap_contact, "cont_reserve");
	if (t == NULL)
	    return NULL;
	t->link = NULL;
	if (padev->cont_buffer_last == NULL)
	    padev->cont_buffer = t;
	else
	    padev->cont_buffer_last->link = t;
	padev->cont_buffer_last = t;
	padev->cont_buffer_count++;
    }
    return t;
}

private inline void
trap_unreserve(gx_device_spot_analyzer *padev, gx_san_trap *t)
{
    /* Assuming the last reserved one. */
    assert(t->link == padev->trap_free);
    padev->trap_free = t;
}

private inline void
cont_unreserve(gx_device_spot_analyzer *padev, gx_san_trap_contact *t)
{
    /* Assuming the last reserved one. */
    assert(t->link == padev->cont_free);
    padev->cont_free = t;
}

private inline gx_san_trap *
band_list_last(const gx_san_trap *list)
{
    /* Assuming a non-empty cyclic list, and the anchor points to the first element.  */
    return list->prev;
}

private inline gx_san_trap_contact *
cont_list_last(const gx_san_trap_contact *list)
{
    /* Assuming a non-empty cyclic list, and the anchor points to the first element.  */
    return list->prev;
}

private inline void
band_list_remove(gx_san_trap **list, gx_san_trap *t)
{
    /* Assuming a cyclic list, and the element is in it. */
    if (t->next == t) {
	*list = NULL;
    } else {
	if (*list == t)
	    *list = t->next;
	t->next->prev = t->prev;
	t->prev->next = t->next;
    }
    t->next = t->prev = NULL; /* Safety. */
}

private inline void
band_list_insert_last(gx_san_trap **list, gx_san_trap *t)
{
    /* Assuming a cyclic list. */
    if (*list == 0) {
	*list = t->next = t->prev = t;
    } else {
	gx_san_trap *last = band_list_last(*list);
	gx_san_trap *first = *list;

	t->next = first;
	t->prev = last;
	last->next = first->prev = t;
    }
}

private inline void
cont_list_insert_last(gx_san_trap_contact **list, gx_san_trap_contact *t)
{
    /* Assuming a cyclic list. */
    if (*list == 0) {
	*list = t->next = t->prev = t;
    } else {
	gx_san_trap_contact *last = cont_list_last(*list);
	gx_san_trap_contact *first = *list;

	t->next = first;
	t->prev = last;
	last->next = first->prev = t;
    }
}

private inline bool
trap_is_last(const gx_san_trap *list, const gx_san_trap *t)
{
    /* Assuming a non-empty cyclic list, and the anchor points to the first element.  */
    return t->next == list; 
}

/* ---------------------The device ---------------------------- */

/* The device descriptor */
/* Many of these procedures won't be called; they are set to NULL. */
private const gx_device_spot_analyzer gx_spot_analyzer_device =
{std_device_std_body(gx_device_spot_analyzer, 0, "spot analyzer",
		     0, 0, 1, 1),
 {san_open,
  NULL,
  NULL,
  NULL,
  san_close,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  gx_default_fill_path,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  san_get_clipping_box,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  gx_default_finish_copydevice,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL
 }
};

private int
san_open(register gx_device * dev)
{
    gx_device_spot_analyzer * const padev = (gx_device_spot_analyzer *)dev;

    padev->trap_buffer = padev->trap_buffer_last = NULL;
    padev->cont_buffer = padev->cont_buffer_last = NULL;
    padev->trap_buffer_count = 0;
    padev->cont_buffer_count = 0;
    padev->xmin = 0;
    padev->xmax = -1;
    return 0;
}

private int
san_close(gx_device * dev)
{
    gx_device_spot_analyzer * const padev = (gx_device_spot_analyzer *)dev;

    free_trap_list(padev->memory, &padev->trap_buffer);
    free_cont_list(padev->memory, &padev->cont_buffer);
    padev->trap_buffer_last = NULL;
    padev->cont_buffer_last = NULL;
    return 0;
}

void
san_get_clipping_box(gx_device * dev, gs_fixed_rect * pbox)
{
    pbox->p.x = min_int;
    pbox->p.y = min_int;
    pbox->q.x = max_int;
    pbox->q.y = max_int;
}

/* --------------------- Utilities ------------------------- */

private inline void
check_band_list(const gx_san_trap *list)
{
#ifdef DEBUG
    if (list != NULL) {
	const gx_san_trap *t = list;

	while (t->next != list) {
	    assert(t->xrtop <= t->next->xltop);
	    t = t->next;
	}
    }
#endif
}

private void
try_unite_last_trap(gx_device_spot_analyzer *padev, fixed xlbot)
{
    if (padev->bot_band != NULL && padev->top_band != NULL) {
	gx_san_trap *last = band_list_last(padev->top_band);
	gx_san_trap *t = padev->bot_current;
	/* If the last trapezoid is a prolongation of its bottom contact, 
	   unite it and release the last trapezoid and the last contact. */
	if (t != NULL && t->upper != NULL && last->xrbot < xlbot && 
		(last->prev == last || last->prev->xrbot < last->xlbot)) {
	    if ((t->next == NULL || t->xrtop < t->next->xltop) &&
	        (t->upper->next == t->upper &&
		    t->l == last->l && t->r == last->r)) {
		if (padev->bot_current == t)
		    padev->bot_current = (t == band_list_last(padev->bot_band) ? NULL : t->next);
		assert(t->upper->upper == last); 
		band_list_remove(&padev->top_band, last);
		band_list_remove(&padev->bot_band, t);
		band_list_insert_last(&padev->top_band, t);
		t->ytop = last->ytop;
		t->xltop = last->xltop;
		t->xrtop = last->xrtop;
		t->rightmost &= last->rightmost;
		t->leftmost &= last->leftmost;
		vd_quad(t->xlbot, t->ybot, t->xrbot, t->ybot, 
			t->xrtop, t->ytop, t->xltop, t->ytop, 1, VD_TRAP_U_COLOR);
		trap_unreserve(padev, last);
		cont_unreserve(padev, t->upper);
		t->upper = NULL;
	    }
	}
    }
}

private inline double 
trap_area(gx_san_trap *t)
{
    return (double)(t->xrbot - t->xlbot + t->xrtop - t->xltop) * (t->ytop - t->ybot) / 2;
}

private inline double 
trap_axis_length(gx_san_trap *t)
{
    double xbot = (t->xlbot + t->xrbot) / 2.0;
    double xtop = (t->xltop + t->xrtop) / 2.0;

    return hypot(xtop - xbot, (double)t->ytop - t->ybot); /* See Bug 687238 */
}

private inline bool
is_stem_boundaries(gx_san_trap *t, int side_mask)
{
    double dx, norm, cosine;
    const double cosine_threshold = 0.9; /* Arbitrary */
    double dy = t->ytop - t->ybot;

    if (side_mask & 1) {
	dx = t->xltop - t->xlbot;
	norm = hypot(dx, dy);
	cosine = dx / norm;
	if (any_abs(cosine) > cosine_threshold)
	    return false;
    }
    if (side_mask & 2) {
	dx = t->xrtop - t->xrbot;
	norm = hypot(dx, dy);
	cosine = dx / norm;
	if (any_abs(cosine) > cosine_threshold)
	    return false;
    }
    return true;
}

/* --------------------- Accessories ------------------------- */

/* Obtain a spot analyzer device. */
int
gx_san__obtain(gs_memory_t *mem, gx_device_spot_analyzer **ppadev)
{
    gx_device_spot_analyzer *padev;
    int code;

    if (*ppadev != 0) {
	(*ppadev)->lock++;
	return 0;
    }
    padev = gs_alloc_struct(mem, gx_device_spot_analyzer, 
		&st_device_spot_analyzer, "gx_san__obtain");
    if (padev == 0)
	return_error(gs_error_VMerror);
    gx_device_init((gx_device *)padev, (const gx_device *)&gx_spot_analyzer_device,
		   mem, false);
    code = gs_opendevice((gx_device *)padev);
    if (code < 0) {
	gs_free_object(mem, padev, "gx_san__obtain");
	return code;
    }
    padev->lock = 1;
    *ppadev = padev;
    return 0;
}

void
gx_san__release(gx_device_spot_analyzer **ppadev)
{
    gx_device_spot_analyzer *padev = *ppadev;

    if (padev == NULL) {
	eprintf("Extra call to gx_san__release.");
	return;
    }
    if(--padev->lock < 0) {
	eprintf("Wrong lock to gx_san__release.");
	return;
    }
    if (padev->lock == 0) {
	*ppadev = NULL;
	rc_decrement(padev, "gx_san__release");
    }
}

/* Start accumulating a path. */
void 
gx_san_begin(gx_device_spot_analyzer *padev)
{
    padev->bot_band = NULL;
    padev->top_band = NULL;
    padev->bot_current = NULL;
    padev->trap_free = padev->trap_buffer;
    padev->cont_free = padev->cont_buffer;
}

/* Store a tarpezoid. */
/* Assumes an Y-band scanning order with increasing X inside a band. */
int
gx_san_trap_store(gx_device_spot_analyzer *padev, 
    fixed ybot, fixed ytop, fixed xlbot, fixed xrbot, fixed xltop, fixed xrtop,
    const segment *l, const segment *r, int dir_l, int dir_r)
{
    gx_san_trap *last;

    if (padev->top_band != NULL && padev->top_band->ytop != ytop) {
	try_unite_last_trap(padev, max_int);
	/* Step to a new band. */
	padev->bot_band = padev->bot_current = padev->top_band;
	padev->top_band = NULL;
    }
    if (padev->bot_band != NULL && padev->bot_band->ytop != ybot) {
	/* The Y-projection of the spot is not contiguous. */
	padev->top_band = NULL;
    }
    if (padev->top_band != NULL)
	try_unite_last_trap(padev, xlbot);
    check_band_list(padev->bot_band);
    check_band_list(padev->top_band);
    /* Make new trapezoid. */
    last = trap_reserve(padev);
    if (last == NULL)
	return_error(gs_error_VMerror);
    last->ybot = ybot;
    last->ytop = ytop;
    last->xlbot = xlbot;
    last->xrbot = xrbot;
    last->xltop = xltop;
    last->xrtop = xrtop;
    last->l = l;
    last->r = r;
    last->dir_l = dir_l;
    last->dir_r = dir_r;
    last->upper = 0;
    last->fork = 0;
    last->visited = false;
    last->leftmost = last->rightmost = true;
    vd_quad(last->xlbot, last->ybot, last->xrbot, last->ybot, 
	    last->xrtop, last->ytop, last->xltop, last->ytop, 1, VD_TRAP_N_COLOR);
    if (padev->top_band != NULL) {
	padev->top_band->rightmost = false;
	last->leftmost = false;
    }
    band_list_insert_last(&padev->top_band, last);
    check_band_list(padev->top_band);
    while (padev->bot_current != NULL && padev->bot_current->xrtop < xlbot)
	padev->bot_current = (trap_is_last(padev->bot_band, padev->bot_current)
				    ? NULL : padev->bot_current->next);
    if (padev->bot_current != 0) {
	gx_san_trap *t = padev->bot_current;
	gx_san_trap *bot_last = band_list_last(padev->bot_band);

	while(t->xltop <= xrbot) {
	    gx_san_trap_contact *cont = cont_reserve(padev);

	    if (cont == NULL)
		return_error(gs_error_VMerror);
	    cont->lower = t;
	    cont->upper = last;
	    vd_bar((t->xltop + t->xrtop + t->xlbot + t->xrbot) / 4, (t->ytop + t->ybot) / 2,
		   (last->xltop + last->xrtop + last->xlbot + last->xrbot) / 4, 
		   (last->ytop + last->ybot) / 2, 0, VD_CONT_COLOR);
	    cont_list_insert_last(&t->upper, cont);
	    last->fork++;
	    if (t == bot_last)
		break;
	    t = t->next;
	}
    }
    if (padev->xmin > padev->xmax) {
	padev->xmin = min(xlbot, xltop);
	padev->xmax = max(xrbot, xrtop);
    } else {
	padev->xmin = min(padev->xmin, min(xlbot, xltop));
	padev->xmax = max(padev->xmax, max(xrbot, xrtop));
    }
    return 0;
}


/* Finish accumulating a path. */
void 
gx_san_end(const gx_device_spot_analyzer *padev)
{
}

private int
hint_by_trap(gx_device_spot_analyzer *padev, int side_mask,
    void *client_data, gx_san_trap *t0, gx_san_trap *t1, double ave_width,
    int (*handler)(void *client_data, gx_san_sect *ss))
{   gx_san_trap *t;
    double w, wd, best_width_diff = ave_width * 10;
    gx_san_trap *best_trap = NULL;
    bool at_top = false;
    gx_san_sect sect;
    int code;

    for (t = t0; ; t = t->upper->upper) {
	w = t->xrbot - t->xlbot;
	wd = any_abs(w - ave_width);
	if (w > 0 && wd < best_width_diff) {
	    best_width_diff = wd;
	    best_trap = t;
	}
	if (t == t1)
	    break;
    }
    w = t->xrtop - t->xltop;
    wd = any_abs(w - ave_width);
    if (w > 0 && wd < best_width_diff) {
	best_width_diff = wd;
	best_trap = t;
	at_top = true;
    }
    if (best_trap != NULL) {
	/* Make a stem section hint at_top of best_trap : */
	sect.yl = at_top ? best_trap->ytop : best_trap->ybot; 
	sect.yr = sect.yl;
	sect.xl = at_top ? best_trap->xltop : best_trap->xlbot; 
	sect.xr = at_top ? best_trap->xrtop : best_trap->xrbot;
	sect.l = best_trap->l; 
	sect.r = best_trap->r;
	vd_bar(sect.xl, sect.yl, sect.xr, sect.yr, 0, VD_HINT_COLOR);
	code = handler(client_data, &sect);
	if (code < 0)
	    return code;
    }
    return 0;
}

private inline void
choose_by_vector(fixed x0, fixed y0, fixed x1, fixed y1, const segment *s, 
	double *slope, double *len, const segment **store_segm, fixed *store_x, fixed *store_y)
{
    if (y0 != y1) {
	double t = (double)any_abs(x1 - x0) / any_abs(y1 - y0);
	double l = any_abs(y1 - y0); /* Don't want 'hypot'. */

	if (*slope > t || (*slope == t && l > *len)) {
	    *slope = t;
	    *len = l;
	    *store_segm = s;
	    *store_x = x1;
	    *store_y = y1;
	}
    }
}

private inline void
choose_by_tangent(const segment *p, const segment *s, 
	double *slope, double *len, const segment **store_segm, fixed *store_x, fixed *store_y,
	fixed ybot, fixed ytop)
{
    if (s->type == s_curve) {
	const curve_segment *c = (const curve_segment *)s;
	vd_curve(p->pt.x, p->pt.y, c->p1.x, c->p1.y, c->p2.x, c->p2.y, 
		 s->pt.x, s->pt.y, 0, VD_HINT_COLOR);
	if (ybot <= p->pt.y && p->pt.y <= ytop)
	    choose_by_vector(c->p1.x, c->p1.y, p->pt.x, p->pt.y, s, slope, len, store_segm, store_x, store_y);
	if (ybot <= s->pt.y && s->pt.y <= ytop)
	    choose_by_vector(c->p2.x, c->p2.y, s->pt.x, s->pt.y, s, slope, len, store_segm, store_x, store_y);
    } else {
	vd_bar(p->pt.x, p->pt.y, s->pt.x, s->pt.y, 0, VD_HINT_COLOR);
	choose_by_vector(s->pt.x, s->pt.y, p->pt.x, p->pt.y, s, slope, len, store_segm, store_x, store_y);
    }
}

private gx_san_trap * 
upper_neighbour(gx_san_trap *t0, int left_right)
{
    gx_san_trap_contact *cont = t0->upper, *c0 = cont, *c;
    fixed x = (!left_right ? cont->upper->xlbot : cont->upper->xrbot);

    for (c = c0->next; c != c0; c = c->next) {
	fixed xx = (!left_right ? c->upper->xlbot : c->upper->xrbot);

	if ((xx - x) * (left_right * 2 - 1) > 0) {
    	    cont = c;
	    x = xx;
	}
    }
    return cont->upper;
}

private int
hint_by_tangent(gx_device_spot_analyzer *padev, int side_mask,
    void *client_data, gx_san_trap *t0, gx_san_trap *t1, double ave_width,
    int (*handler)(void *client_data, gx_san_sect *ss))
{   gx_san_trap *t;
    gx_san_sect sect;
    double slope0 = 0.2, slope1 = slope0, len0 = 0, len1 = 0;
    const segment *s, *p;
    int left_right = (side_mask & 1 ? 0 : 1);
    int code;
    
    sect.l = sect.r = NULL;
    sect.xl = t0->xltop; /* only for vdtrace. */
    sect.xr = t0->xrtop; /* only for vdtrace. */
    sect.yl = sect.yr = t0->ytop; /* only for vdtrace. */
    sect.side_mask = side_mask;
    for (t = t0; ; t = upper_neighbour(t, left_right)) {
	if (side_mask & 1) {
	    s = t->l;
	    if (t->dir_l < 0)
		s = (s->type == s_line_close ? ((const line_close_segment *)s)->sub->next : s->next);
	    p = (s->type == s_start ? ((const subpath *)s)->last->prev : s->prev);
	    choose_by_tangent(p, s, &slope0, &len0, &sect.l, &sect.xl, &sect.yl, t->ybot, t->ytop);
	}
	if (side_mask & 2) {
	    s = t->r;
	    if (t->dir_r < 0)
		s = (s->type == s_line_close ? ((const line_close_segment *)s)->sub->next : s->next);
	    p = (s->type == s_start ? ((const subpath *)s)->last->prev : s->prev);
	    choose_by_tangent(p, s, &slope1, &len1, &sect.r, &sect.xr, &sect.yr, t->ybot, t->ytop);
	}
	if (t == t1)
	    break;
    }
    if ((sect.l != NULL  || !(side_mask & 1)) && 
	(sect.r != NULL  || !(side_mask & 2))) {
	const int w = 3;

	if (!(side_mask & 1)) {
	    if (sect.xr < (padev->xmin * w + padev->xmax) / (w + 1))
		return 0;
	    sect.xl = padev->xmin - 1000; /* ignore side */
	}
	if (!(side_mask & 2)) {
	    if (sect.xl > (padev->xmax * w + padev->xmin) / (w + 1))
		return 0;
	    sect.xr = padev->xmax + 1000; /* ignore side */
	}
	vd_bar(sect.xl, sect.yl, sect.xr, sect.yr, 0, VD_HINT_COLOR);
	code = handler(client_data, &sect);
	if (code < 0)
	    return code;
    }
    return 0;
}

/* Generate stems. */
private int 
gx_san_generate_stems_aux(gx_device_spot_analyzer *padev, 
		bool overall_hints, void *client_data,
		int (*handler)(void *client_data, gx_san_sect *ss))
{
    gx_san_trap *t0;
    const bool by_trap = false;
    int k;

    /* Overall hints : */
    /* An overall hint designates an outer side of a glyph,
       being nearly parallel to a coordinate axis. 
       It aligns a stem end rather than stem sides.
       See t1_hinter__overall_hstem.
     */
    for (k = 0; overall_hints && k < 2; k++) { /* left, right. */
	for (t0 = padev->trap_buffer; t0 != padev->trap_free; t0 = t0->link) {
	    if (!t0->visited && (!k ? t0->leftmost : t0->rightmost)) {
		if (is_stem_boundaries(t0, 1 << k)) {
		    gx_san_trap *t1 = t0, *tt = t0, *t = t0;
		    int code;

		    while (t->upper != NULL) {
			t = upper_neighbour(tt, k);
			if (!k ? !t->leftmost : !t->rightmost) {
			    break;
			}
			if (!is_stem_boundaries(t, 1 << k)) {
			    t->visited = true;
			    break;
			}
			if ((!k ? tt->xltop : tt->xrtop) != (!k ? t->xlbot : t->xrbot))
			    break; /* Not a contigouos boundary. */
    			t->visited = true;
			tt = t;
		    }
		    if (!k ? !t->leftmost : !t->rightmost)
			continue;
		    t1 = t;
		    /* leftmost/rightmost boundary from t0 to t1. */
		    code = hint_by_tangent(padev, 1 << k, client_data, t0, t1, 0, handler);
		    if (code < 0)
			return code;
		}
	    }
	}
	for (t0 = padev->trap_buffer; t0 != padev->trap_free; t0 = t0->link)
	    t0->visited = false;
    }
    /* Stem hints : */
    for (t0 = padev->trap_buffer; t0 != padev->trap_free; t0 = t0->link) {
	if (!t0->visited) {
	    if (is_stem_boundaries(t0, 3)) {
		gx_san_trap_contact *cont = t0->upper;
		gx_san_trap *t1 = t0, *t;
		double area = 0, length = 0, ave_width;
		
		while(cont != NULL && cont->next == cont /* <= 1 descendent. */) {
		    gx_san_trap *t = cont->upper;

		    if (!is_stem_boundaries(t, 3)) {
			t->visited = true;
			break;
		    }
		    if (t->fork > 1)
			break; /* > 1 accendents.  */
		    if (t1->xltop != t->xlbot || t1->xrtop != t->xrbot)
			break; /* Not a contigouos boundary. */
		    t1 = t;
		    cont = t1->upper;
		    t1->visited = true;
		}
		/* We've got a stem suspection from t0 to t1. */
		vd_quad(t0->xlbot, t0->ybot, t0->xrbot, t0->ybot, 
			t1->xrtop, t1->ytop, t1->xltop, t1->ytop, 1, VD_STEM_COLOR);
		for (t = t0; ; t = t->upper->upper) {
		    length += trap_axis_length(t);
		    area += trap_area(t);
		    if (t == t1)
			break;
		}
		ave_width = area / length;
		if (length > ave_width / ( 2.0 /* arbitrary */)) {
		    /* We've got a stem from t0 to t1. */
		    int code = (by_trap ? hint_by_trap : hint_by_tangent)(padev, 
			3, client_data, t0, t1, ave_width, handler);

		    if (code < 0)
			return code;
		}
	    }
	}
	t0->visited = true;
    }
    return 0;
}

int 
gx_san_generate_stems(gx_device_spot_analyzer *padev, 
		bool overall_hints, void *client_data,
		int (*handler)(void *client_data, gx_san_sect *ss))
{
    int code;

    vd_get_dc('f');
    vd_set_shift(0, 0);
    vd_set_scale(VD_SCALE);
    vd_set_origin(0, 0);
    code = gx_san_generate_stems_aux(padev, overall_hints, client_data, handler);
    vd_release_dc;
    return code;
}
