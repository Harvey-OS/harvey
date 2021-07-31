/* Copyright (C) 1995, 1996, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: opextern.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
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
int zadd(P1(i_ctx_t *));
int zdef(P1(i_ctx_t *));
int zdup(P1(i_ctx_t *));
int zexch(P1(i_ctx_t *));
int zif(P1(i_ctx_t *));
int zifelse(P1(i_ctx_t *));
int zindex(P1(i_ctx_t *));
int zpop(P1(i_ctx_t *));
int zroll(P1(i_ctx_t *));
int zsub(P1(i_ctx_t *));
/* Internal entry points for the interpreter. */
int zop_add(P1(ref *));
int zop_def(P1(i_ctx_t *));
int zop_sub(P1(ref *));

/* Operators exported for server loop implementations. */
int zflush(P1(i_ctx_t *));
int zflushpage(P1(i_ctx_t *));
int zsave(P1(i_ctx_t *));
int zrestore(P1(i_ctx_t *));

/* Operators exported for save/restore. */
int zgsave(P1(i_ctx_t *));
int zgrestore(P1(i_ctx_t *));

/* Operators exported for Level 2 pagedevice facilities. */
int zcopy_gstate(P1(i_ctx_t *));
int zcurrentgstate(P1(i_ctx_t *));
int zgrestoreall(P1(i_ctx_t *));
int zgstate(P1(i_ctx_t *));
int zreadonly(P1(i_ctx_t *));
int zsetdevice(P1(i_ctx_t *));
int zsetgstate(P1(i_ctx_t *));

/* Operators exported for Level 2 "wrappers". */
int zcopy(P1(i_ctx_t *));
int zimage(P1(i_ctx_t *));
int zimagemask(P1(i_ctx_t *));
int zwhere(P1(i_ctx_t *));

/* Operators exported for specific-VM operators. */
int zarray(P1(i_ctx_t *));
int zdict(P1(i_ctx_t *));
int zpackedarray(P1(i_ctx_t *));
int zstring(P1(i_ctx_t *));

/* Operators exported for user path decoding. */
/* Note that only operators defined in all configurations are declared here. */
int zclosepath(P1(i_ctx_t *));
int zcurveto(P1(i_ctx_t *));
int zlineto(P1(i_ctx_t *));
int zmoveto(P1(i_ctx_t *));
int zrcurveto(P1(i_ctx_t *));
int zrlineto(P1(i_ctx_t *));
int zrmoveto(P1(i_ctx_t *));

/* Operators exported for the FunctionType 4 interpreter. */
/* zarith.c: */
int zabs(P1(i_ctx_t *));
int zceiling(P1(i_ctx_t *));
int zdiv(P1(i_ctx_t *));
int zfloor(P1(i_ctx_t *));
int zidiv(P1(i_ctx_t *));
int zmod(P1(i_ctx_t *));
int zmul(P1(i_ctx_t *));
int zneg(P1(i_ctx_t *));
int zround(P1(i_ctx_t *));
int ztruncate(P1(i_ctx_t *));
/* zmath.c: */
int zatan(P1(i_ctx_t *));
int zcos(P1(i_ctx_t *));
int zexp(P1(i_ctx_t *));
int zln(P1(i_ctx_t *));
int zlog(P1(i_ctx_t *));
int zsin(P1(i_ctx_t *));
int zsqrt(P1(i_ctx_t *));
/* zrelbit.c: */
int zand(P1(i_ctx_t *));
int zbitshift(P1(i_ctx_t *));
int zeq(P1(i_ctx_t *));
int zge(P1(i_ctx_t *));
int zgt(P1(i_ctx_t *));
int zle(P1(i_ctx_t *));
int zlt(P1(i_ctx_t *));
int zne(P1(i_ctx_t *));
int znot(P1(i_ctx_t *));
int zor(P1(i_ctx_t *));
int zxor(P1(i_ctx_t *));
/* ztype.c: */
int zcvi(P1(i_ctx_t *));
int zcvr(P1(i_ctx_t *));

/* Operators exported for CIE cache loading. */
int zcvx(P1(i_ctx_t *));
int zexec(P1(i_ctx_t *));		/* also for .runexec */
int zfor(P1(i_ctx_t *));

/* Odds and ends */
int zbegin(P1(i_ctx_t *));
int zcleartomark(P1(i_ctx_t *));
int zclosefile(P1(i_ctx_t *));	/* for runexec_cleanup */
int zcopy_dict(P1(i_ctx_t *));	/* for zcopy */
int zend(P1(i_ctx_t *));
int zfor_fraction(P1(i_ctx_t *)); /* for color function sampling */
int zsetfont(P1(i_ctx_t *));	/* for cshow_continue */

/* Operators exported for special customer needs. */
int zcurrentdevice(P1(i_ctx_t *));
int ztoken(P1(i_ctx_t *));
int ztokenexec(P1(i_ctx_t *));
int zwrite(P1(i_ctx_t *));

#endif /* opextern_INCLUDED */
