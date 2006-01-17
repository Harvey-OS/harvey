/* Copyright (C) 2002 artofcode LLC.  All rights reserved.
  
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
/*$Id: gswts.c,v 1.5 2003/08/26 15:38:50 igor Exp $ */
/* Screen generation for Well Tempered Screening. */
#include "stdpre.h"
#include <stdlib.h> /* for malloc */
#include "gx.h"
#include "gxstate.h"
#include "gsht.h"
#include "math_.h"
#include "gserrors.h"
#include "gxfrac.h"
#include "gxwts.h"
#include "gswts.h"

#define VERBOSE

#ifdef UNIT_TEST
/**
 * This file can be compiled independently for unit testing purposes.
 * Try this invocation:
 *
 * gcc -I../obj -DUNIT_TEST gswts.c gxwts.c -o test_gswts -lm
 * ./test_gswts | sed '/P5/,$!d' | xv -
 **/
#undef printf
#undef stdout
#undef dlprintf1
#define dlprintf1 printf
#undef dlprintf2
#define dlprintf2 printf
#undef dlprintf3
#define dlprintf3 printf
#undef dlprintf4
#define dlprintf4 printf
#undef dlprintf5
#define dlprintf5 printf
#undef dlprintf6
#define dlprintf6 printf
#undef dlprintf7
#define dlprintf7 printf
#endif

/* A datatype used for representing the product of two 32 bit integers.
   If a 64 bit integer type is available, it may be a better choice. */
typedef double wts_bigint;

typedef struct gx_wts_cell_params_j_s gx_wts_cell_params_j_t;
typedef struct gx_wts_cell_params_h_s gx_wts_cell_params_h_t;

struct gx_wts_cell_params_j_s {
    gx_wts_cell_params_t base;
    int shift;

    double ufast_a;
    double vfast_a;
    double uslow_a;
    double vslow_a;

    /* Probabilities of "jumps". A and B jumps can happen when moving
       one pixel to the right. C and D can happen when moving one pixel
       down. */
    double pa;
    double pb;
    double pc;
    double pd;

    int xa;
    int ya;
    int xb;
    int yb;
    int xc;
    int yc;
    int xd;
    int yd;
};

struct gx_wts_cell_params_h_s {
    gx_wts_cell_params_t base;

    /* This is the exact value that x1 and (width-x1) approximates. */
    double px;
    /* Ditto y1 and (height-y1). */
    double py;

    int x1;
    int y1;
};

#define WTS_EXTRA_SORT_BITS 9
#define WTS_SORTED_MAX (((frac_1) << (WTS_EXTRA_SORT_BITS)) - 1)

typedef struct {
    int u;
    int v;
    int k;
    int l;
} wts_vec_t;

private void
wts_vec_set(wts_vec_t *wv, int u, int v, int k, int l)
{
    wv->u = u;
    wv->v = v;
    wv->k = k;
    wv->l = l;
}

private wts_bigint
wts_vec_smag(const wts_vec_t *a)
{
    wts_bigint u = a->u;
    wts_bigint v = a->v;
    return u * u + v * v;
}

private void
wts_vec_add(wts_vec_t *a, const wts_vec_t *b, const wts_vec_t *c)
{
    a->u = b->u + c->u;
    a->v = b->v + c->v;
    a->k = b->k + c->k;
    a->l = b->l + c->l;
}

private void
wts_vec_sub(wts_vec_t *a, const wts_vec_t *b, const wts_vec_t *c)
{
    a->u = b->u - c->u;
    a->v = b->v - c->v;
    a->k = b->k - c->k;
    a->l = b->l - c->l;
}

/**
 * wts_vec_gcd3: Compute 3-way "gcd" of three vectors.
 * @a, @b, @c: Vectors.
 *
 * Compute pair of vectors satisfying three constraints:
 * They are integer combinations of the source vectors.
 * The source vectors are integer combinations of the results.
 * The magnitude of the vectors is minimized.
 *
 * The algorithm for this computation is quite reminiscent of the
 * classic Euclid GCD algorithm for integers.
 *
 * On return, @a and @b contain the result. @c is destroyed.
 **/
private void
wts_vec_gcd3(wts_vec_t *a, wts_vec_t *b, wts_vec_t *c)
{
    wts_vec_t d, e;
    double ra, rb, rc, rd, re;

    wts_vec_set(&d, 0, 0, 0, 0);
    wts_vec_set(&e, 0, 0, 0, 0);
    for (;;) {
	ra = wts_vec_smag(a);
	rb = wts_vec_smag(b);
	rc = wts_vec_smag(c);
	wts_vec_sub(&d, a, b);
	wts_vec_add(&e, a, b);
	rd = wts_vec_smag(&d);
	re = wts_vec_smag(&e);
	if (re && re < rd) {
	    d = e;
	    rd = re;
	}
	if (rd && rd < ra && ra >= rb) *a = d;
	else if (rd < rb && rb > ra) *b = d;
	else {
	    wts_vec_sub(&d, a, c);
	    wts_vec_add(&e, a, c);
	    rd = wts_vec_smag(&d);
	    re = wts_vec_smag(&e);
	    if (re < rd) {
		d = e;
		rd = re;
	    }
	    if (rd && rd < ra && ra >= rc) *a = d;
	    else if (rd < rc && rc > ra) *c = d;
	    else {
		wts_vec_sub(&d, b, c);
		wts_vec_add(&e, b, c);
		rd = wts_vec_smag(&d);
		re = wts_vec_smag(&e);
		if (re < rd) {
		    d = e;
		    rd = re;
		}
		if (rd && rd < rb && rb >= rc) *b = d;
		else if (rd < rc && rc > rb) *c = d;
		else
		    break;
	    }
	}
    }
}

private wts_bigint
wts_vec_cross(const wts_vec_t *a, const wts_vec_t *b)
{
    wts_bigint au = a->u;
    wts_bigint av = a->v;
    wts_bigint bu = b->u;
    wts_bigint bv = b->v;
    return au * bv - av * bu;
}

private void
wts_vec_neg(wts_vec_t *a)
{
    a->u = -a->u;
    a->v = -a->v;
    a->k = -a->k;
    a->l = -a->l;
}

/* compute k mod m */
private void
wts_vec_modk(wts_vec_t *a, int m)
{
    while (a->k < 0) a->k += m;
    while (a->k >= m) a->k -= m;
}

/* Compute modulo in rational cell. */
private void
wts_vec_modkls(wts_vec_t *a, int m, int i, int s)
{
    while (a->l < 0) {
	a->l += i;
	a->k -= s;
    }
    while (a->l >= i) {
	a->l -= i;
	a->k += s;
    }
    while (a->k < 0) a->k += m;
    while (a->k >= m) a->k -= m;
}

private void
wts_set_mat(gx_wts_cell_params_t *wcp, double sratiox, double sratioy,
	    double sangledeg)
{
    double sangle = sangledeg * M_PI / 180;

    wcp->ufast = cos(sangle) / sratiox;
    wcp->vfast = -sin(sangle) / sratiox;
    wcp->uslow = sin(sangle) / sratioy;
    wcp->vslow = cos(sangle) / sratioy;
}


/**
 * Calculate Screen H cell sizes.
 **/
private void
wts_cell_calc_h(double inc, int *px1, int *pxwidth, double *pp1, double memw)
{
    double minrep = pow(2, memw) * 50 / pow(2, 7.5);
    int m1 = 0, m2 = 0;
    double e1, e2;

    int uacc;

    e1 = 1e5;
    e2 = 1e5;
    for (uacc = (int)ceil(minrep * inc); uacc <= floor(2 * minrep * inc); uacc++) {
	int mt;
	double et;

	mt = (int)floor(uacc / inc + 1e-5);
	et = uacc / inc - mt + mt * 0.001;
	if (et < e1) {
	    e1 = et;
	    m1 = mt;
	}
	mt = (int)ceil(uacc / inc - 1e-5);
	et = mt - uacc / inc + mt * 0.001;
	if (et < e2) {
	    e2 = et;
	    m2 = mt;
	}
    }
    if (m1 == m2) {
	*px1 = m1;
	*pxwidth = m1;
	*pp1 = 1.0;
    } else {
	*px1 = m1;
	*pxwidth = m1 + m2;
	e1 = fabs(m1 * inc - floor(0.5 + m1 * inc));
	e2 = fabs(m2 * inc - floor(0.5 + m2 * inc));
	*pp1 = e2 / (e1 + e2);
    }
}

/* Implementation for Screen H. This is optimized for 0 and 45 degree
   rotations. */
private gx_wts_cell_params_t *
wts_pick_cell_size_h(double sratiox, double sratioy, double sangledeg,
		     double memw)
{
    gx_wts_cell_params_h_t *wcph;
    double xinc, yinc;

    wcph = malloc(sizeof(gx_wts_cell_params_h_t));
    if (wcph == NULL)
	return NULL;

    wcph->base.t = WTS_SCREEN_H;
    wts_set_mat(&wcph->base, sratiox, sratioy, sangledeg);

    xinc = fabs(wcph->base.ufast);
    if (xinc == 0)
	xinc = fabs(wcph->base.vfast);

    wts_cell_calc_h(xinc, &wcph->x1, &wcph->base.width, &wcph->px, memw);

    yinc = fabs(wcph->base.uslow);
    if (yinc == 0)
	yinc = fabs(wcph->base.vslow);
    wts_cell_calc_h(yinc, &wcph->y1, &wcph->base.height, &wcph->py, memw);

    return &wcph->base;
}

private double
wts_qart(double r, double rbase, double p, double pbase)
{
   if (p < 1e-5) {
      return ((r + rbase) * p);
   } else {
      return ((r + rbase) * (p + pbase) - rbase * pbase);
   }
}

#ifdef VERBOSE
private void
wts_print_j_jump(const gx_wts_cell_params_j_t *wcpj, const char *name,
		 double pa, int xa, int ya)
{
    dlprintf6("jump %s: (%d, %d) %f, actual (%f, %f)\n",
	      name, xa, ya, pa,
	      wcpj->ufast_a * xa + wcpj->uslow_a * ya,
	      wcpj->vfast_a * xa + wcpj->vslow_a * ya);
}

private void
wts_j_add_jump(const gx_wts_cell_params_j_t *wcpj, double *pu, double *pv, 
	       double pa, int xa, int ya)
{
    double jump_u = wcpj->ufast_a * xa + wcpj->uslow_a * ya;
    double jump_v = wcpj->vfast_a * xa + wcpj->vslow_a * ya;
    jump_u -= floor(jump_u + 0.5);
    jump_v -= floor(jump_v + 0.5);
    *pu += pa * jump_u;
    *pv += pa * jump_v;
}

private void
wts_print_j(const gx_wts_cell_params_j_t *wcpj)
{
    double uf, vf;
    double us, vs;

    dlprintf3("cell = %d x %d, shift = %d\n",
	      wcpj->base.width, wcpj->base.height, wcpj->shift);
    wts_print_j_jump(wcpj, "a", wcpj->pa, wcpj->xa, wcpj->ya);
    wts_print_j_jump(wcpj, "b", wcpj->pb, wcpj->xb, wcpj->yb);
    wts_print_j_jump(wcpj, "c", wcpj->pc, wcpj->xc, wcpj->yc);
    wts_print_j_jump(wcpj, "d", wcpj->pd, wcpj->xd, wcpj->yd);
    uf = wcpj->ufast_a;
    vf = wcpj->vfast_a;
    us = wcpj->uslow_a;
    vs = wcpj->vslow_a;
    wts_j_add_jump(wcpj, &uf, &vf, wcpj->pa, wcpj->xa, wcpj->ya);
    wts_j_add_jump(wcpj, &uf, &vf, wcpj->pb, wcpj->xb, wcpj->yb);
    wts_j_add_jump(wcpj, &us, &vs, wcpj->pc, wcpj->xc, wcpj->yc);
    wts_j_add_jump(wcpj, &us, &vs, wcpj->pd, wcpj->xd, wcpj->yd);
    dlprintf6("d: %f, %f; a: %f, %f; err: %g, %g\n",
	    wcpj->base.ufast, wcpj->base.vfast,
	    wcpj->ufast_a, wcpj->vfast_a,
	    wcpj->base.ufast - uf, wcpj->base.vfast - vf);
    dlprintf6("d: %f, %f; a: %f, %f; err: %g, %g\n",
	    wcpj->base.uslow, wcpj->base.vslow,
	    wcpj->uslow_a, wcpj->vslow_a,
	    wcpj->base.uslow - us, wcpj->base.vslow - vs);
}
#endif

/**
 * wts_set_scr_jxi_try: Try a width for setting Screen J parameters.
 * @wcpj: Screen J parameters.
 * @m: Width to try.
 * @qb: Best quality score seen so far.
 * @jmem: Weight given to memory usage.
 *
 * Evaluate the quality for width @i. If the quality is better than
 * @qb, then set the rest of the parameters in @wcpj.
 *
 * This routine corresponds to SetScrJXITrySP in the WTS source.
 *
 * Return value: Quality score for parameters chosen.
 **/
private double
wts_set_scr_jxi_try(gx_wts_cell_params_j_t *wcpj, int m, double qb,
		    double jmem)
{
    const double uf = wcpj->base.ufast;
    const double vf = wcpj->base.vfast;
    const double us = wcpj->base.uslow;
    const double vs = wcpj->base.vslow;
    wts_vec_t a, b, c;
    double ufj, vfj;
    const double rbase = 0.1;
    const double pbase = 0.001;
    double ug, vg;
    double pp, pa, pb;
    double rf;
    double xa, ya, ra, xb, yb, rb;
    double q, qx, qw, qxl, qbi;
    double pya, pyb;
    int i;
    bool jumpok;

    wts_vec_set(&a, (int)floor(uf * m + 0.5), (int)floor(vf * m + 0.5), 1, 0);
    if (a.u == 0 && a.v == 0)
	return qb + 1;
	
    ufj = a.u / (double)m;
    vfj = a.v / (double)m;
    /* (ufj, vfj) = movement in UV space from (0, 1) in XY space */

    wts_vec_set(&b, m, 0, 0, 0);
    wts_vec_set(&c, 0, m, 0, 0);
    wts_vec_gcd3(&a, &b, &c);
    ug = (uf - ufj) * m;
    vg = (vf - vfj) * m;
    pp = 1.0 / wts_vec_cross(&b, &a);
    pa = (b.u * vg - ug * b.v) * pp;
    pb = (ug * a.v - a.u * vg) * pp;
    if (pa < 0) {
	wts_vec_neg(&a);
	pa = -pa;
    }
    if (pb < 0) {
	wts_vec_neg(&b);
	pb = -pb;
    }
    wts_vec_modk(&a, m);
    wts_vec_modk(&b, m);

    /* Prefer nontrivial jumps. */
    jumpok = (wcpj->xa == 0 && wcpj->ya == 0) ||
      (wcpj->xb == 0 && wcpj->yb == 0) ||
      (a.k != 0 && b.k != 0);

    rf = (uf * uf + vf * vf) * m;
    xa = (a.u * uf + a.v * vf) / rf;
    ya = (a.v * uf - a.u * vf) / rf;
    xb = (b.u * uf + b.v * vf) / rf;
    yb = (b.v * uf - b.u * vf) / rf;
    ra = sqrt(xa * xa + 0.0625 * ya * ya);
    rb = sqrt(xb * xb + 0.0625 * yb * yb);
    qx = 0.5 * (wts_qart(ra, rbase, pa, pbase) +
		wts_qart(rb, rbase, pb, pbase));
    if (qx < 1.0 / 4000.0)
	qx *= 0.25;
    else
	qx -= 0.75 / 4000.0;
    if (m < 7500)
	qw = 0;
    else
	qw = 0.00025; /* cache penalty */
    qxl = qx + qw;
    if (qxl > qb)
	return qxl;

    /* width is ok, now try heights */

    pp = m / (double)wts_vec_cross(&b, &a);
    pya = (b.u * vs - us * b.v) * pp;
    pyb = (us * a.v - a.u * vs) * pp;

    qbi = qb;
    for (i = 1; qxl + m * i * jmem < qbi; i++) {
	int s = m * i;
	int ca, cb;
	double usj, vsj;
	double usg, vsg;
	wts_vec_t a1, b1, a2, b2;
	double pc, pd;
	int ck;
	double qy, qm;

	ca = (int)floor(i * pya + 0.5);
	cb = (int)floor(i * pyb + 0.5);
	wts_vec_set(&c, ca * a.u + cb * b.u, ca * a.v + cb * b.v, 0, 1);
	usj = c.u / (double)s;
	vsj = c.v / (double)s;
	usg = (us - usj);
	vsg = (vs - vsj);

	a1 = a;
	b1 = b;
	a1.u *= i;
	a1.v *= i;
	b1.u *= i;
	b1.v *= i;
	wts_vec_gcd3(&a1, &b1, &c);
	a2 = a1;
	b2 = b1;
	pp = s / (double)wts_vec_cross(&b1, &a1);
	pc = (b1.u * vsg - usg * b1.v) * pp;
	pd = (usg * a1.v - a1.u * vsg) * pp;
	if (pc < 0) {
	    wts_vec_neg(&a1);
	    pc = -pc;
	}
	if (pd < 0) {
	    wts_vec_neg(&b1);
	    pd = -pd;
	}
	ck = ca * a.k + cb * b.k;
	while (ck < 0) ck += m;
	while (ck >= m) ck -= m;
	wts_vec_modkls(&a1, m, i, ck);
	wts_vec_modkls(&b1, m, i, ck);
	rf = (us * us - vs * vs) * s;
	xa = (a1.u * us + a1.v * vs) / rf;
	ya = (a1.v * us - a1.u * vs) / rf;
	xb = (b1.u * us + b1.v * vs) / rf;
	yb = (b1.v * us - b1.u * vs) / rf;
	ra = sqrt(xa * xa + 0.0625 * ya * ya);
	rb = sqrt(xb * xb + 0.0625 * yb * yb);
	qy = 0.5 * (wts_qart(ra, rbase, pc, pbase) +
		    wts_qart(rb, rbase, pd, pbase));
	if (qy < 1.0 / 100.0)
	    qy *= 0.025;
	else
	    qy -= 0.75 / 100.0;
	qm = s * jmem;

	/* first try a and b jumps within the scanline */
	q = qm + qw + qx + qy;
	if (q < qbi && jumpok) {
#ifdef VERBOSE
	    dlprintf7("m = %d, n = %d, q = %d, qx = %d, qy = %d, qm = %d, qw = %d\n",
		      m, i, (int)(q * 1e6), (int)(qx * 1e6), (int)(qy * 1e6), (int)(qm * 1e6), (int)(qw * 1e6));
#endif
	    qbi = q;
	    wcpj->base.width = m;
	    wcpj->base.height = i;
	    wcpj->shift = ck;
	    wcpj->ufast_a = ufj;
	    wcpj->vfast_a = vfj;
	    wcpj->uslow_a = usj;
	    wcpj->vslow_a = vsj;
	    wcpj->xa = a.k;
	    wcpj->ya = 0;
	    wcpj->xb = b.k;
	    wcpj->yb = 0;
	    wcpj->xc = a1.k;
	    wcpj->yc = a1.l;
	    wcpj->xd = b1.k;
	    wcpj->yd = b1.l;
	    wcpj->pa = pa;
	    wcpj->pb = pb;
	    wcpj->pc = pc;
	    wcpj->pd = pd;
#ifdef VERBOSE
	    wts_print_j(wcpj);
#endif
	}

	/* then try unconstrained a and b jumps */
	if (i > 1) {
	    double pa2, pb2, pp2;
	    double qx2, qw2, q2;

	    pp2 = pp;
	    pa2 = (b2.u * vg - ug * b2.v) * pp2;
	    pb2 = (ug * a2.v - a2.u * vg) * pp2;
	    rf = (uf * uf + vf * vf) * s;
	    xa = (a2.u * uf + a2.v * vf) / rf;
	    ya = (a2.v * uf - a2.u * vf) / rf;
	    xb = (b2.u * uf + b2.v * vf) / rf;
	    yb = (b2.v * uf - b2.u * vf) / rf;
	    ra = sqrt(xa * xa + 0.0625 * ya * ya);
	    rb = sqrt(xb * xb + 0.0625 * yb * yb);
	    if (pa2 < 0) {
		pa2 = -pa2;
		wts_vec_neg(&a2);
	    }
	    if (pb2 < 0) {
		pb2 = -pb2;
		wts_vec_neg(&b2);
	    }
	    wts_vec_modkls(&a2, m, i, ck);
	    wts_vec_modkls(&b2, m, i, ck);
	    qx2 = 0.5 * (wts_qart(ra, rbase, pa2, pbase) +
			 wts_qart(rb, rbase, pb2, pbase));
	    if (qx2 < 1.0 / 4000.0)
		qx2 *= 0.25;
	    else
		qx2 -= 0.75 / 4000.0;
	    if (s < 7500)
		qw2 = 0;
	    else
		qw2 = 0.00025; /* cache penalty */
	    q2 = qm + qw2 + qx2 + qy;
	    if (q2 < qbi) {
#ifdef VERBOSE
		dlprintf7("m = %d, n = %d, q = %d, qx2 = %d, qy = %d, qm = %d, qw2 = %d\n",
			  m, i, (int)(q * 1e6), (int)(qx * 1e6), (int)(qy * 1e6), (int)(qm * 1e6), (int)(qw2 * 1e6));
#endif
		if (qxl > qw2 + qx2)
		    qxl = qw2 + qx2;
		qbi = q2;
		wcpj->base.width = m;
		wcpj->base.height = i;
		wcpj->shift = ck;
		wcpj->ufast_a = ufj;
		wcpj->vfast_a = vfj;
		wcpj->uslow_a = usj;
		wcpj->vslow_a = vsj;
		wcpj->xa = a2.k;
		wcpj->ya = a2.l;
		wcpj->xb = b2.k;
		wcpj->yb = a2.l;
		wcpj->xc = a1.k;
		wcpj->yc = a1.l;
		wcpj->xd = b1.k;
		wcpj->yd = b1.l;
		wcpj->pa = pa2;
		wcpj->pb = pb2;
		wcpj->pc = pc;
		wcpj->pd = pd;
#ifdef VERBOSE
		wts_print_j(wcpj);
#endif
	    }
	} /* if (i > 1) */
	if (qx > 10 * qy)
	    break;
    }
    return qbi;
}

private int
wts_double_to_int_cap(double d)
{
    if (d > 0x7fffffff)
	return 0x7fffffff;
    else return (int)d;
}

/**
 * wts_set_scr_jxi: Choose Screen J parameters.
 * @wcpj: Screen J parameters.
 * @jmem: Weight given to memory usage.
 *
 * Chooses a cell size based on the [uv]{fast,slow} parameters,
 * setting the rest of the parameters in @wcpj. Essentially, this
 * algorithm iterates through all plausible widths for the cell.
 *
 * This routine corresponds to SetScrJXISP in the WTS source.
 *
 * Return value: Quality score for parameters chosen.
 **/
private double
wts_set_scr_jxi(gx_wts_cell_params_j_t *wcpj, double jmem)
{
    int i, imax;
    double q, qb;

    wcpj->xa = 0;
    wcpj->ya = 0;
    wcpj->xb = 0;
    wcpj->yb = 0;
    wcpj->xc = 0;
    wcpj->yc = 0;
    wcpj->xd = 0;
    wcpj->yd = 0;

    qb = 1.0;
    imax = wts_double_to_int_cap(qb / jmem);
    for (i = 1; i <= imax; i++) {
	if (i > 1) {
	    q = wts_set_scr_jxi_try(wcpj, i, qb, jmem);
	    if (q < qb) {
		qb = q;
		imax = wts_double_to_int_cap(q / jmem);
		if (imax >= 7500)
		    imax = wts_double_to_int_cap((q - 0.00025) / jmem);
		if (imax < 7500) {
		    imax = 7500;
		}
	    }
	}
    }
    return qb;
}

/* Implementation for Screen J. This is optimized for general angles. */
private gx_wts_cell_params_t *
wts_pick_cell_size_j(double sratiox, double sratioy, double sangledeg,
		     double memw)
{
    gx_wts_cell_params_j_t *wcpj;
    double code;

    wcpj = malloc(sizeof(gx_wts_cell_params_j_t));
    if (wcpj == NULL)
	return NULL;

    wcpj->base.t = WTS_SCREEN_J;
    wts_set_mat(&wcpj->base, sratiox, sratioy, sangledeg);

    code = wts_set_scr_jxi(wcpj, pow(0.1, memw));
    if (code < 0) {
	free(wcpj);
	return NULL;
    }

    return &wcpj->base;
}

/**
 * wts_pick_cell_size: Choose cell size for WTS screen.
 * @ph: The halftone parameters.
 * @pmat: Transformation from 1/72" Y-up coords to device coords.
 *
 * Return value: The WTS cell parameters, or NULL on error.
 **/
gx_wts_cell_params_t *
wts_pick_cell_size(gs_screen_halftone *ph, const gs_matrix *pmat)
{
    gx_wts_cell_params_t *result;

    /* todo: deal with landscape and mirrored coordinate systems */
    double sangledeg = ph->angle;
    double sratiox = 72.0 * fabs(pmat->xx) / ph->frequency;
    double sratioy = 72.0 * fabs(pmat->yy) / ph->frequency;
    double octangle;
    double memw = 8.0;

    octangle = sangledeg;
    while (octangle >= 45.0) octangle -= 45.0;
    while (octangle < -45.0) octangle += 45.0;
    if (fabs(octangle) < 1e-4)
	result = wts_pick_cell_size_h(sratiox, sratioy, sangledeg, memw);
    else
	result = wts_pick_cell_size_j(sratiox, sratioy, sangledeg, memw);

    if (result != NULL) {
	ph->actual_frequency = ph->frequency;
	ph->actual_angle = ph->angle;
    }
    return result;
}

struct gs_wts_screen_enum_s {
    wts_screen_type t;
    bits32 *cell;
    int width;
    int height;
    int size;

    int idx;
};

typedef struct {
    gs_wts_screen_enum_t base;
    const gx_wts_cell_params_j_t *wcpj;
} gs_wts_screen_enum_j_t;

typedef struct {
    gs_wts_screen_enum_t base;
    const gx_wts_cell_params_h_t *wcph;

    /* an argument can be made that these should be in the params */
    double ufast1, vfast1;
    double ufast2, vfast2;
    double uslow1, vslow1;
    double uslow2, vslow2;
} gs_wts_screen_enum_h_t;

private gs_wts_screen_enum_t *
gs_wts_screen_enum_j_new(gx_wts_cell_params_t *wcp)
{
    gs_wts_screen_enum_j_t *wsej;

    wsej = malloc(sizeof(gs_wts_screen_enum_j_t));
    wsej->base.t = WTS_SCREEN_J;
    wsej->wcpj = (const gx_wts_cell_params_j_t *)wcp;
    wsej->base.width = wcp->width;
    wsej->base.height = wcp->height;
    wsej->base.size = wcp->width * wcp->height;
    wsej->base.cell = malloc(wsej->base.size * sizeof(wsej->base.cell[0]));
    wsej->base.idx = 0;

    return (gs_wts_screen_enum_t *)wsej;
}

private int
gs_wts_screen_enum_j_currentpoint(gs_wts_screen_enum_t *self,
				  gs_point *ppt)
{
    gs_wts_screen_enum_j_t *z = (gs_wts_screen_enum_j_t *)self;
    const gx_wts_cell_params_j_t *wcpj = z->wcpj;
    
    int x, y;
    double u, v;

    if (z->base.idx == z->base.size) {
	return 1;
    }
    x = z->base.idx % wcpj->base.width;
    y = z->base.idx / wcpj->base.width;
    u = wcpj->ufast_a * x + wcpj->uslow_a * y;
    v = wcpj->vfast_a * x + wcpj->vslow_a * y;
    u -= floor(u);
    v -= floor(v);
    ppt->x = 2 * u - 1;
    ppt->y = 2 * v - 1;
    return 0;
}

private gs_wts_screen_enum_t *
gs_wts_screen_enum_h_new(gx_wts_cell_params_t *wcp)
{
    gs_wts_screen_enum_h_t *wseh;
    const gx_wts_cell_params_h_t *wcph = (const gx_wts_cell_params_h_t *)wcp;
    int x1 = wcph->x1;
    int x2 = wcp->width - x1;
    int y1 = wcph->y1;
    int y2 = wcp->height - y1;

    wseh = malloc(sizeof(gs_wts_screen_enum_h_t));
    wseh->base.t = WTS_SCREEN_H;
    wseh->wcph = wcph;
    wseh->base.width = wcp->width;
    wseh->base.height = wcp->height;
    wseh->base.size = wcp->width * wcp->height;
    wseh->base.cell = malloc(wseh->base.size * sizeof(wseh->base.cell[0]));
    wseh->base.idx = 0;

    wseh->ufast1 = floor(0.5 + wcp->ufast * x1) / x1;
    wseh->vfast1 = floor(0.5 + wcp->vfast * x1) / x1;
    if (x2 > 0) {
	wseh->ufast2 = floor(0.5 + wcp->ufast * x2) / x2;
	wseh->vfast2 = floor(0.5 + wcp->vfast * x2) / x2;
    }
    wseh->uslow1 = floor(0.5 + wcp->uslow * y1) / y1;
    wseh->vslow1 = floor(0.5 + wcp->vslow * y1) / y1;
    if (y2 > 0) {
	wseh->uslow2 = floor(0.5 + wcp->uslow * y2) / y2;
	wseh->vslow2 = floor(0.5 + wcp->vslow * y2) / y2;
    }

    return &wseh->base;
}

private int
gs_wts_screen_enum_h_currentpoint(gs_wts_screen_enum_t *self,
				  gs_point *ppt)
{
    gs_wts_screen_enum_h_t *z = (gs_wts_screen_enum_h_t *)self;
    const gx_wts_cell_params_h_t *wcph = z->wcph;
    
    int x, y;
    double u, v;

    if (self->idx == self->size) {
	return 1;
    }
    x = self->idx % wcph->base.width;
    y = self->idx / wcph->base.width;
    if (x < wcph->x1) {
	u = z->ufast1 * x;
	v = z->vfast1 * x;
    } else {
	u = z->ufast2 * (x - wcph->x1);
	v = z->vfast2 * (x - wcph->x1);
    }
    if (y < wcph->y1) {
	u += z->uslow1 * y;
	v += z->vslow1 * y;
    } else {
	u += z->uslow2 * (y - wcph->y1);
	v += z->vslow2 * (y - wcph->y1);
    }
    u -= floor(u);
    v -= floor(v);
    ppt->x = 2 * u - 1;
    ppt->y = 2 * v - 1;
    return 0;
}

gs_wts_screen_enum_t *
gs_wts_screen_enum_new(gx_wts_cell_params_t *wcp)
{
    if (wcp->t == WTS_SCREEN_J)
	return gs_wts_screen_enum_j_new(wcp);
    else if (wcp->t == WTS_SCREEN_H)
	return gs_wts_screen_enum_h_new(wcp);
    else
	return NULL;
}

int
gs_wts_screen_enum_currentpoint(gs_wts_screen_enum_t *wse, gs_point *ppt)
{
    if (wse->t == WTS_SCREEN_J)
	return gs_wts_screen_enum_j_currentpoint(wse, ppt);
    if (wse->t == WTS_SCREEN_H)
	return gs_wts_screen_enum_h_currentpoint(wse, ppt);
    else
	return -1;
}

int
gs_wts_screen_enum_next(gs_wts_screen_enum_t *wse, floatp value)
{
    bits32 sample;

    if (value < -1.0 || value > 1.0)
	return_error(gs_error_rangecheck);
    sample = (bits32) ((value + 1) * 0x7fffffff);
    wse->cell[wse->idx] = sample;
    wse->idx++;
    return 0;
}

/* Run the enum with a square dot. This is useful for testing. */
#ifdef UNIT_TEST
private void
wts_run_enum_squaredot(gs_wts_screen_enum_t *wse)
{
    int code;
    gs_point pt;
    floatp spot;

    for (;;) {
	code = gs_wts_screen_enum_currentpoint(wse, &pt);
	if (code)
	    break;
	spot = 0.5 * (cos(pt.x * M_PI) + cos(pt.y * M_PI));
	gs_wts_screen_enum_next(wse, spot);
    }
}
#endif /* UNIT_TEST */

private int
wts_sample_cmp(const void *av, const void *bv) {
    const bits32 *const *a = (const bits32 *const *)av;
    const bits32 *const *b = (const bits32 *const *)bv;

    if (**a > **b) return 1;
    if (**a < **b) return -1;
    return 0;
}

/* This implementation simply sorts the threshold values (evening the
   distribution), without applying any moire reduction. */
int
wts_sort_cell(gs_wts_screen_enum_t *wse)
{
    int size = wse->width * wse->height;
    bits32 *cell = wse->cell;
    bits32 **pcell;
    int i;

    pcell = malloc(size * sizeof(bits32 *));
    if (pcell == NULL)
	return -1;
    for (i = 0; i < size; i++)
	pcell[i] = &cell[i];
    qsort(pcell, size, sizeof(bits32 *), wts_sample_cmp);
    for (i = 0; i < size; i++)
	*pcell[i] = (bits32)floor(WTS_SORTED_MAX * (i + 0.5) / size);
    free(pcell);
    return 0;
}

/**
 * wts_blue_bump: Generate bump function for BlueDot.
 *
 * Return value: newly allocated bump.
 **/
private bits32 *
wts_blue_bump(gs_wts_screen_enum_t *wse)
{
    const gx_wts_cell_params_t *wcp;
    int width = wse->width;
    int height = wse->height;
    int shift;
    int size = width * height;
    bits32 *bump;
    int i;
    double uf, vf;
    double am, eg;
    int z;
    int x0, y0;
    int x, y;

    if (wse->t == WTS_SCREEN_J) {
	gs_wts_screen_enum_j_t *wsej = (gs_wts_screen_enum_j_t *)wse;
	shift = wsej->wcpj->shift;
	wcp = &wsej->wcpj->base;
    } else if (wse->t == WTS_SCREEN_H) {
	gs_wts_screen_enum_h_t *wseh = (gs_wts_screen_enum_h_t *)wse;
	shift = 0;
	wcp = &wseh->wcph->base;
    } else
	return NULL;

    bump = (bits32 *)malloc(size * sizeof(bits32));
    if (bump == NULL)
	return NULL;

    for (i = 0; i < size; i++)
	bump[i] = 0;
    /* todo: more intelligence for anisotropic scaling */
    uf = wcp->ufast;
    vf = wcp->vfast;

    am = uf * uf + vf * vf;
    eg = (1 << 24) * 2.0 * sqrt (am);
    z = (int)(5 / sqrt (am));

    x0 = -(z / width) * shift - z;
    y0 = -(z % width);
    while (y0 < 0) {
	x0 -= shift;
	y0 += height;
    }
    while (x0 < 0) x0 += width;
    for (y = -z; y <= z; y++) {
	int x1 = x0;
	for (x = -z; x <= z; x++) {
	    bump[y0 * width + x1] += (bits32)(eg * exp (-am * (x * x + y * y)));
	    x1++;
	    if (x1 == width)
		x1 = 0;
	}
	y0++;
	if (y0 == height) {
	    x0 += shift;
	    if (x0 >= width) x0 -= width;
	    y0 = 0;
	}
    }
    return bump;
}

/**
 * wts_sort_blue: Sort cell using BlueDot.
 **/
int
wts_sort_blue(gs_wts_screen_enum_t *wse)
{
    bits32 *cell = wse->cell;
    int width = wse->width;
    int height = wse->height;
    int shift;
    int size = width * height;
    bits32 *ref;
    bits32 **pcell;
    bits32 *bump;
    int i;

    if (wse->t == WTS_SCREEN_J) {
	gs_wts_screen_enum_j_t *wsej = (gs_wts_screen_enum_j_t *)wse;
	shift = wsej->wcpj->shift;
    } else
	shift = 0;

    ref = (bits32 *)malloc(size * sizeof(bits32));
    pcell = (bits32 **)malloc(size * sizeof(bits32 *));
    bump = wts_blue_bump(wse);
    if (ref == NULL || pcell == NULL || bump == NULL) {
	free(ref);
	free(pcell);
	free(bump);
	return -1;
    }
    for (i = 0; i < size; i++)
	pcell[i] = &cell[i];
    qsort(pcell, size, sizeof(bits32 *), wts_sample_cmp);
    /* set ref to sorted cell; pcell will now point to ref */
    for (i = 0; i < size; i++) {
	pcell[i] = (pcell[i] - cell) + ref;
	*pcell[i] = (bits32)floor((1 << 24) * (i + 0.5) / size);
	cell[i] = 0;
    }

    for (i = 0; i < size; i++) {
	bits32 gmin = *pcell[i];
	int j;
	int j_end = i + 5000;
	int jmin = i;
	int ix;
	int x0, y0;
	int x, y;
	int ref_ix, bump_ix;

	/* find minimum cell value, but prune search */
	if (j_end > size) j_end = size;
	for (j = i + 1; j < j_end; j++) {
	    if (*pcell[j] < gmin) {
		gmin = *pcell[j];
		jmin = j;
	    }
	}
	ix = pcell[jmin] - ref;
	pcell[jmin] = pcell[i];
	cell[ix] = (bits32)floor(WTS_SORTED_MAX * (i + 0.5) / size);

	x0 = ix % width;
	y0 = ix / width;

	/* Add in bump, centered at (x0, y0) */
	ref_ix = y0 * width;
	bump_ix = 0;
	for (y = 0; y < height; y++) {
	    for (x = x0; x < width; x++)
		ref[ref_ix + x] += bump[bump_ix++];
	    for (x = 0; x < x0; x++)
		ref[ref_ix + x] += bump[bump_ix++];
	    ref_ix += width;
	    y0++;
	    if (y0 == height) {
		x0 += shift;
		if (x0 >= width) x0 -= width;
		y0 = 0;
		ref_ix = 0;
	    }
	}

	/* Remove DC component to avoid integer overflow. */
	if ((i & 255) == 255 && i + 1 < size) {
	    bits32 gmin = *pcell[i + 1];
	    int j;

	    for (j = i + 2; j < size; j++) {
		if (*pcell[j] < gmin) {
		    gmin = *pcell[j];
		}
	    }
#ifdef VERBOSE
	    if_debug1('h', "[h]gmin = %d\n", gmin);
#endif
	    for (j = i + 1; j < size; j++)
		*pcell[j] -= gmin;
	    
	}
    }

    free(ref);
    free(pcell);
    free(bump);
    return 0;
}

private wts_screen_t *
wts_screen_from_enum_j(const gs_wts_screen_enum_t *wse)
{
    const gs_wts_screen_enum_j_t *wsej = (const gs_wts_screen_enum_j_t *)wse;
    wts_screen_j_t *wsj;
    wts_screen_sample_t *samples;
    int size;
    int i;

    wsj = malloc(sizeof(wts_screen_j_t));
    wsj->base.type = WTS_SCREEN_J;
    wsj->base.cell_width = wsej->base.width;
    wsj->base.cell_height = wsej->base.height;
    size = wsj->base.cell_width * wsj->base.cell_height;
    wsj->base.cell_shift = wsej->wcpj->shift;
    wsj->pa = (int)floor(wsej->wcpj->pa * (1 << 16) + 0.5);
    wsj->pb = (int)floor(wsej->wcpj->pb * (1 << 16) + 0.5);
    wsj->pc = (int)floor(wsej->wcpj->pc * (1 << 16) + 0.5);
    wsj->pd = (int)floor(wsej->wcpj->pd * (1 << 16) + 0.5);
    wsj->XA = wsej->wcpj->xa;
    wsj->YA = wsej->wcpj->ya;
    wsj->XB = wsej->wcpj->xb;
    wsj->YB = wsej->wcpj->yb;
    wsj->XC = wsej->wcpj->xc;
    wsj->YC = wsej->wcpj->yc;
    wsj->XD = wsej->wcpj->xd;
    wsj->YD = wsej->wcpj->yd;

    samples = malloc(sizeof(wts_screen_sample_t) * size);
    wsj->base.samples = samples;
    for (i = 0; i < size; i++) {
	samples[i] = wsej->base.cell[i] >> WTS_EXTRA_SORT_BITS;
    }

    return &wsj->base;
}

private wts_screen_t *
wts_screen_from_enum_h(const gs_wts_screen_enum_t *wse)
{
    const gs_wts_screen_enum_h_t *wseh = (const gs_wts_screen_enum_h_t *)wse;
    wts_screen_h_t *wsh;
    wts_screen_sample_t *samples;
    int size;
    int i;

    /* factor some of this out into a common init routine? */
    wsh = malloc(sizeof(wts_screen_h_t));
    wsh->base.type = WTS_SCREEN_H;
    wsh->base.cell_width = wseh->base.width;
    wsh->base.cell_height = wseh->base.height;
    size = wsh->base.cell_width * wsh->base.cell_height;
    wsh->base.cell_shift = 0;
    wsh->px = wseh->wcph->px;
    wsh->py = wseh->wcph->py;
    wsh->x1 = wseh->wcph->x1;
    wsh->y1 = wseh->wcph->y1;

    samples = malloc(sizeof(wts_screen_sample_t) * size);
    wsh->base.samples = samples;
    for (i = 0; i < size; i++) {
	samples[i] = wseh->base.cell[i] >> WTS_EXTRA_SORT_BITS;
    }

    return &wsh->base;
}

wts_screen_t *
wts_screen_from_enum(const gs_wts_screen_enum_t *wse)
{
    if (wse->t == WTS_SCREEN_J)
	return wts_screen_from_enum_j(wse);
    else if (wse->t == WTS_SCREEN_H)
	return wts_screen_from_enum_h(wse);
    else
	return NULL;
}

void
gs_wts_free_enum(gs_wts_screen_enum_t *wse)
{
    free(wse);
}

void
gs_wts_free_screen(wts_screen_t * wts)
{
    free(wts);
}

#ifdef UNIT_TEST
private int
dump_thresh(const wts_screen_t *ws, int width, int height)
{
    int x, y;
    wts_screen_sample_t *s0;
    int dummy;

    wts_get_samples(ws, 0, 0, &s0, &dummy);


    printf("P5\n%d %d\n255\n", width, height);
    for (y = 0; y < height; y++) {
	for (x = 0; x < width;) {
	    wts_screen_sample_t *samples;
	    int n_samples, i;

	    wts_get_samples(ws, x, y, &samples, &n_samples);
#if 1
	    for (i = 0; x + i < width && i < n_samples; i++)
		fputc(samples[i] >> 7, stdout);
#else
	    printf("(%d, %d): %d samples at %d\n",
		   x, y, n_samples, samples - s0);
#endif
	    x += n_samples;
	}
    }
    return 0;
}

int
main (int argc, char **argv)
{
    gs_screen_halftone h;
    gs_matrix mat;
    double xres = 1200;
    double yres = 1200;
    gx_wts_cell_params_t *wcp;
    gs_wts_screen_enum_t *wse;
    wts_screen_t *ws;

    mat.xx = xres / 72.0;
    mat.xy = 0;
    mat.yx = 0;
    mat.yy = yres / 72.0;

    h.frequency = 121;
    h.angle = 45;

    wcp = wts_pick_cell_size(&h, &mat);
    dlprintf2("cell size = %d x %d\n", wcp->width, wcp->height);
    wse = gs_wts_screen_enum_new(wcp);
    wts_run_enum_squaredot(wse);
#if 1
    wts_sort_blue(wse);
#else
    wts_sort_cell(wse);
#endif
    ws = wts_screen_from_enum(wse);

    dump_thresh(ws, 512, 512);
    return 0;
}
#endif
