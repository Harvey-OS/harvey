/* Copyright (C) 1995, 1996, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: opextern.h,v 1.6 2002/06/16 04:47:10 lpd Exp $ */
/* Externally accessible operator declarations */

#ifndef opextern_INCLUDED
#  define opextern_INCLUDED

/*
 * Normally, the procedures that implement PostScript operators (named zX
 * where X is the name of the operator, e.g., zadd) are private to the
 * file in which they are defined.  There are, however, a surprising
 * number of these procedures that are used from other files.
 * This file, opextern.h, declares all z* procedures that are
 *      - referenced from outside their defining file, and
 *      - present in *all* configurations of the interpreter.
 * For z* procedures referenced from outside their file but not present
 * in all configurations (e.g., Level 2 operators), the file making the
 * reference must include a local extern.  Not pretty, but c'est la vie.
 */

/* Operators exported for the special operator encoding in interp.c. */
int zadd(i_ctx_t *);
int zdef(i_ctx_t *);
int zdup(i_ctx_t *);
int zexch(i_ctx_t *);
int zif(i_ctx_t *);
int zifelse(i_ctx_t *);
int zindex(i_ctx_t *);
int zpop(i_ctx_t *);
int zroll(i_ctx_t *);
int zsub(i_ctx_t *);
/* Internal entry points for the interpreter. */
int zop_add(ref *);
int zop_def(i_ctx_t *);
int zop_sub(ref *);

/* Operators exported for server loop implementations. */
int zflush(i_ctx_t *);
int zflushpage(i_ctx_t *);
int zsave(i_ctx_t *);
int zrestore(i_ctx_t *);

/* Operators exported for save/restore. */
int zgsave(i_ctx_t *);
int zgrestore(i_ctx_t *);

/* Operators exported for Level 2 pagedevice facilities. */
int zcopy_gstate(i_ctx_t *);
int zcurrentgstate(i_ctx_t *);
int zgrestoreall(i_ctx_t *);
int zgstate(i_ctx_t *);
int zreadonly(i_ctx_t *);
int zsetdevice(i_ctx_t *);
int zsetgstate(i_ctx_t *);

/* Operators exported for Level 2 "wrappers". */
int zcopy(i_ctx_t *);
int zimage(i_ctx_t *);
int zimagemask(i_ctx_t *);
int zwhere(i_ctx_t *);

/* Operators exported for specific-VM operators. */
int zarray(i_ctx_t *);
int zdict(i_ctx_t *);
int zpackedarray(i_ctx_t *);
int zstring(i_ctx_t *);

/* Operators exported for user path decoding. */
/* Note that only operators defined in all configurations are declared here. */
int zclosepath(i_ctx_t *);
int zcurveto(i_ctx_t *);
int zlineto(i_ctx_t *);
int zmoveto(i_ctx_t *);
int zrcurveto(i_ctx_t *);
int zrlineto(i_ctx_t *);
int zrmoveto(i_ctx_t *);

/* Operators exported for the FunctionType 4 interpreter. */
/* zarith.c: */
int zabs(i_ctx_t *);
int zceiling(i_ctx_t *);
int zdiv(i_ctx_t *);
int zfloor(i_ctx_t *);
int zidiv(i_ctx_t *);
int zmod(i_ctx_t *);
int zmul(i_ctx_t *);
int zneg(i_ctx_t *);
int zround(i_ctx_t *);
int ztruncate(i_ctx_t *);
/* zmath.c: */
int zatan(i_ctx_t *);
int zcos(i_ctx_t *);
int zexp(i_ctx_t *);
int zln(i_ctx_t *);
int zlog(i_ctx_t *);
int zsin(i_ctx_t *);
int zsqrt(i_ctx_t *);
/* zrelbit.c: */
int zand(i_ctx_t *);
int zbitshift(i_ctx_t *);
int zeq(i_ctx_t *);
int zge(i_ctx_t *);
int zgt(i_ctx_t *);
int zle(i_ctx_t *);
int zlt(i_ctx_t *);
int zne(i_ctx_t *);
int znot(i_ctx_t *);
int zor(i_ctx_t *);
int zxor(i_ctx_t *);
/* ztype.c: */
int zcvi(i_ctx_t *);
int zcvr(i_ctx_t *);

/* Operators exported for CIE cache loading. */
int zcvx(i_ctx_t *);
int zexec(i_ctx_t *);		/* also for .runexec */
int zfor(i_ctx_t *);

/* Odds and ends */
int zbegin(i_ctx_t *);
int zcleartomark(i_ctx_t *);
int zclosefile(i_ctx_t *);	/* for runexec_cleanup */
int zcopy_dict(i_ctx_t *);	/* for zcopy */
int zend(i_ctx_t *);
int zfor_samples(i_ctx_t *); /* for function sampling */
int zsetfont(i_ctx_t *);	/* for cshow_continue */

/* Operators exported for special customer needs. */
int zcurrentdevice(i_ctx_t *);
int ztoken(i_ctx_t *);
int ztokenexec(i_ctx_t *);
int zwrite(i_ctx_t *);

#endif /* opextern_INCLUDED */
