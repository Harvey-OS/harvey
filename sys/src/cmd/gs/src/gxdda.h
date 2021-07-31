/* Copyright (C) 1994, 1995, 1996, 1997, 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxdda.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Definitions for DDAs */
/* Requires gxfixed.h */

#ifndef gxdda_INCLUDED
#  define gxdda_INCLUDED

/*
 * We use the familiar Bresenham DDA algorithm for several purposes:
 *      - tracking the edges when filling trapezoids;
 *      - tracking the current pixel corner coordinates when rasterizing
 *      skewed or rotated images;
 *      - converting curves to sequences of lines (this is a 3rd-order
 *      DDA, the others are 1st-order);
 *      - perhaps someday for drawing single-pixel lines.
 * In the case of trapezoids, lines, and curves, we need to use
 * the DDA to find the integer X values at integer+0.5 values of Y;
 * in the case of images, we use DDAs to compute the (fixed)
 * X and Y values at (integer) source pixel corners.
 *
 * The purpose of the DDA is to compute the exact values Q(i) = floor(i*D/N)
 * for increasing integers i, 0 <= i <= N.  D is considered to be an
 * integer, although it may actually be a fixed.  For the algorithm,
 * we maintain i*D/N as Q + (N-R)/N where Q and R are integers, 0 < R <= N,
 * with the following auxiliary values:
 *      dQ = floor(D/N)
 *      dR = D mod N (0 <= dR < N)
 *      NdR = N - dR
 * Then at each iteration we do:
 *      Q += dQ;
 *      if ( R > dR ) R -= dR; else ++Q, R += NdR;
 * These formulas work regardless of the sign of D, and never let R go
 * out of range.
 */
/* In the following structure definitions, ntype must be an unsigned type. */
#define dda_state_struct(sname, dtype, ntype)\
  struct sname { dtype Q; ntype R; }
#define dda_step_struct(sname, dtype, ntype)\
  struct sname { dtype dQ; ntype dR, NdR; }
/* DDA with fixed Q and (unsigned) integer N */
typedef 
dda_state_struct(_a, fixed, uint) gx_dda_state_fixed;
     typedef dda_step_struct(_e, fixed, uint) gx_dda_step_fixed;
     typedef struct gx_dda_fixed_s {
	 gx_dda_state_fixed state;
	 gx_dda_step_fixed step;
     } gx_dda_fixed;
/*
 * Define a pair of DDAs for iterating along an arbitrary line.
 */
     typedef struct gx_dda_fixed_point_s {
	 gx_dda_fixed x, y;
     } gx_dda_fixed_point;
/*
 * Initialize a DDA.  The sign test is needed only because C doesn't
 * provide reliable definitions of / and % for integers (!!!).
 */
#define dda_init_state(dstate, init, N)\
  (dstate).Q = (init), (dstate).R = (N)
#define dda_init_step(dstep, D, N)\
  if ( (N) == 0 )\
    (dstep).dQ = 0, (dstep).dR = 0;\
  else if ( (D) < 0 )\
   { (dstep).dQ = -(-(D) / (N));\
     if ( ((dstep).dR = -(D) % (N)) != 0 )\
       --(dstep).dQ, (dstep).dR = (N) - (dstep).dR;\
   }\
  else\
   { (dstep).dQ = (D) / (N); (dstep).dR = (D) % (N); }\
  (dstep).NdR = (N) - (dstep).dR
#define dda_init(dda, init, D, N)\
  dda_init_state((dda).state, init, N);\
  dda_init_step((dda).step, D, N)
/*
 * Compute the sum of two DDA steps with the same D and N.
 * Note that since dR + NdR = N, this quantity must be the same in both
 * fromstep and tostep.
 */
#define dda_step_add(tostep, fromstep)\
  (tostep).dQ +=\
    ((tostep).dR < (fromstep).NdR ?\
     ((tostep).dR += (fromstep).dR, (tostep).NdR -= (fromstep).dR,\
      (fromstep).dQ) :\
     ((tostep).dR -= (fromstep).NdR, (tostep).NdR += (fromstep).NdR,\
      (fromstep).dQ + 1))
/*
 * Return the current value in a DDA.
 */
#define dda_state_current(dstate) (dstate).Q
#define dda_current(dda) dda_state_current((dda).state)
#define dda_current_fixed2int(dda)\
  fixed2int_var(dda_state_current((dda).state))
/*
 * Increment a DDA to the next point.
 * Returns the updated current value.
 */
#define dda_state_next(dstate, dstep)\
  (dstate).Q +=\
    ((dstate).R > (dstep).dR ?\
     ((dstate).R -= (dstep).dR, (dstep).dQ) :\
     ((dstate).R += (dstep).NdR, (dstep).dQ + 1))
#define dda_next(dda) dda_state_next((dda).state, (dda).step)
/*
 * Back up a DDA to the previous point.
 * Returns the updated current value.
 */
#define dda_state_previous(dstate, dstep)\
  (dstate).Q -=\
    ((dstate).R <= (dstep).NdR ?\
     ((dstate).R += (dstep).dR, (dstep).dQ) :\
     ((dstate).R -= (dstep).NdR, (dstep).dQ + 1))
#define dda_previous(dda) dda_state_previous((dda).state, (dda).step)
/*
 * Advance a DDA by an arbitrary number of steps.
 * This implementation is very inefficient; we'll improve it if needed.
 */
#define dda_state_advance(dstate, dstep, nsteps)\
  BEGIN\
    uint n_ = (nsteps);\
    (dstate).Q += (dstep).dQ * (nsteps);\
    while ( n_-- )\
      if ( (dstate).R > (dstep).dR ) (dstate).R -= (dstep).dR;\
      else (dstate).R += (dstep).NdR, (dstate).Q++;\
  END
#define dda_advance(dda, nsteps)\
  dda_state_advance((dda).state, (dda).step, nsteps)
/*
 * Translate the position of a DDA by a given amount.
 */
#define dda_state_translate(dstate, delta)\
  ((dstate).Q += (delta))
#define dda_translate(dda, delta)\
  dda_state_translate((dda).state, delta)

#endif /* gxdda_INCLUDED */
