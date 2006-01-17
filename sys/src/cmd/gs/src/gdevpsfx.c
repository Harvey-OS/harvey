/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gdevpsfx.c,v 1.25 2005/07/30 02:39:45 alexcher Exp $ */
/* Convert Type 1 Charstrings to Type 2 */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gxfixed.h"
#include "gxmatrix.h"		/* for gsfont.h */
#include "gxfont.h"
#include "gxfont1.h"
#include "gxtype1.h"
#include "stream.h"
#include "gdevpsf.h"

/* ------ Type 1 Charstring parsing ------ */

/*
 * The parsing code handles numbers on its own; it reports callsubr and
 * return operators to the caller, but also executes them.
 *
 * Only the following elements of the Type 1 state are used:
 *	ostack, os_count, ipstack, ips_count
 */

#define CE_OFFSET 32		/* offset for extended opcodes */

typedef struct {
    fixed v0, v1;		/* coordinates */
    ushort index;		/* sequential index of hint */
} cv_stem_hint;
typedef struct {
    int count;
    int current;		/* cache cursor for search */
    /*
     * For dotsection and Type 1 Charstring hint replacement,
     * we store active hints at the bottom of the table, and
     * replaced hints at the top.
     */
    int replaced_count;		/* # of replaced hints at top */
    cv_stem_hint data[max_total_stem_hints];
} cv_stem_hint_table;

/* Skip over the initial bytes in a Charstring, if any. */
private void
skip_iv(gs_type1_state *pcis)
{
    int skip = pcis->pfont->data.lenIV;
    ip_state_t *ipsp = &pcis->ipstack[pcis->ips_count - 1];
    const byte *cip = ipsp->cs_data.bits.data;
    crypt_state state = crypt_charstring_seed;

    for (; skip > 0; ++cip, --skip)
	decrypt_skip_next(*cip, state);
    ipsp->ip = cip;
    ipsp->dstate = state;
}

/*
 * Set up for parsing a Type 1 Charstring.
 *
 * Only uses the following elements of *pfont:
 *	data.lenIV
 */
private void
type1_next_init(gs_type1_state *pcis, const gs_glyph_data_t *pgd,
		gs_font_type1 *pfont)
{
    gs_type1_interp_init(pcis, NULL, NULL, NULL, NULL, false, 0, pfont);
    pcis->flex_count = flex_max;
    pcis->ipstack[0].cs_data = *pgd;
    skip_iv(pcis);
}

/* Clear the Type 1 operand stack. */
inline private void
type1_clear(gs_type1_state *pcis)
{
    pcis->os_count = 0;
}

/* Execute a callsubr. */
private int
type1_callsubr(gs_type1_state *pcis, int index)
{
    gs_font_type1 *pfont = pcis->pfont;
    ip_state_t *ipsp1 = &pcis->ipstack[pcis->ips_count];
    int code = pfont->data.procs.subr_data(pfont, index, false,
					   &ipsp1->cs_data);

    if (code < 0)
	return_error(code);
    pcis->ips_count++;
    skip_iv(pcis);
    return code;
}

/* Add 1 or 3 stem hints. */
private int
type1_stem1(gs_type1_state *pcis, cv_stem_hint_table *psht, const fixed *pv,
	    fixed lsb, byte *active_hints)
{
    fixed v0 = pv[0] + lsb, v1 = v0 + pv[1];
    cv_stem_hint *bot = &psht->data[0];
    cv_stem_hint *orig_top = bot + psht->count;
    cv_stem_hint *top = orig_top;

    if (psht->count >= max_total_stem_hints)
	return_error(gs_error_limitcheck);
    while (top > bot &&
	   (v0 < top[-1].v0 || (v0 == top[-1].v0 && v1 < top[-1].v1))
	   ) {
	*top = top[-1];
	top--;
    }
    if (top > bot && v0 == top[-1].v0 && v1 == top[-1].v1) {
	/* Duplicate hint, don't add it. */
	memmove(top, top + 1, (char *)orig_top - (char *)top);
	if (active_hints) {
	    uint index = top[-1].index;

	    active_hints[index >> 3] |= 0x80 >> (index & 7);
	}
	return 0;
    }
    top->v0 = v0;
    top->v1 = v1;
    psht->count++;
    return 0;
}
private void
type1_stem3(gs_type1_state *pcis, cv_stem_hint_table *psht, const fixed *pv3,
	    fixed lsb, byte *active_hints)
{
    type1_stem1(pcis, psht, pv3, lsb, active_hints);
    type1_stem1(pcis, psht, pv3 + 2, lsb, active_hints);
    type1_stem1(pcis, psht, pv3 + 4, lsb, active_hints);
}

/*
 * Get the next operator from a Type 1 Charstring.  This procedure handles
 * numbers, div, blend, pop, and callsubr/return.
 */
private int
type1_next(gs_type1_state *pcis)
{
    ip_state_t *ipsp = &pcis->ipstack[pcis->ips_count - 1];
    const byte *cip, *cipe;
    crypt_state state;
#define CLEAR (csp = pcis->ostack - 1)
    fixed *csp = &pcis->ostack[pcis->os_count - 1];
    const bool encrypted = pcis->pfont->data.lenIV >= 0;
    int c, code, num_results, c0;

 load:
    cip = ipsp->ip;
    cipe = ipsp->cs_data.bits.data + ipsp->cs_data.bits.size;
    state = ipsp->dstate;
    for (;;) {
        if (cip >= cipe)
	    return_error(gs_error_invalidfont);
	c0 = *cip++;
	charstring_next(c0, state, c, encrypted);
	if (c >= c_num1) {
	    /* This is a number, decode it and push it on the stack. */
	    if (c < c_pos2_0) {	/* 1-byte number */
		decode_push_num1(csp, pcis->ostack, c);
	    } else if (c < cx_num4) {	/* 2-byte number */
		decode_push_num2(csp, pcis->ostack, c, cip, state, encrypted);
	    } else if (c == cx_num4) {	/* 4-byte number */
		long lw;

		decode_num4(lw, cip, state, encrypted);
		CS_CHECK_PUSH(csp, pcis->ostack);
		*++csp = int2fixed(lw);
	    } else		/* not possible */
		return_error(gs_error_invalidfont);
	    continue;
	}
#ifdef DEBUG
	if (gs_debug_c('1')) {
	    const fixed *p;

	    for (p = pcis->ostack; p <= csp; ++p)
		dprintf1(" %g", fixed2float(*p));
	    if (c == cx_escape) {
		crypt_state cstate = state;
		int cn;

		charstring_next(*cip, cstate, cn, encrypted);
		dprintf1(" [*%d]\n", cn);
	    } else
		dprintf1(" [%d]\n", c);
	}
#endif
	switch ((char_command) c) {
	default:
	    break;
	case c_undef0:
	case c_undef2:
	case c_undef17:
	    return_error(gs_error_invalidfont);
	case c_callsubr:
	    code = type1_callsubr(pcis, fixed2int_var(*csp) +
				  pcis->pfont->data.subroutineNumberBias);
	    if (code < 0)
		return_error(code);
	    ipsp->ip = cip, ipsp->dstate = state;
	    --csp;
	    ++ipsp;
	    goto load;
	case c_return:
	    gs_glyph_data_free(&ipsp->cs_data, "type1_next");
	    pcis->ips_count--;
	    --ipsp;
	    goto load;
	case c_undoc15:
	    /* See gstype1.h for information on this opcode. */
	    CLEAR;
	    continue;
	case cx_escape:
	    charstring_next(*cip, state, c, encrypted);
	    ++cip;
	    switch ((char1_extended_command) c) {
	    default:
		c += CE_OFFSET;
		break;
	    case ce1_div:
		csp[-1] = float2fixed((double)csp[-1] / (double)*csp);
		--csp;
		continue;
	    case ce1_undoc15:	/* see gstype1.h */
		CLEAR;
		continue;
	    case ce1_callothersubr:
		switch (fixed2int_var(*csp)) {
		case 0:
		    pcis->ignore_pops = 2;
		    break;	/* pass to caller */
		case 3:
		    pcis->ignore_pops = 1;
		    break;	/* pass to caller */
		case 14:
		    num_results = 1; goto blend;
		case 15:
		    num_results = 2; goto blend;
		case 16:
		    num_results = 3; goto blend;
		case 17:
		    num_results = 4; goto blend;
		case 18:
		    num_results = 6;
		blend:
		    code = gs_type1_blend(pcis, csp, num_results);
		    if (code < 0)
			return code;
		    csp -= code;
		    continue;
		default:
		    break;	/* pass to caller */
		}
		break;
	    case ce1_pop:
		if (pcis->ignore_pops != 0) {
		    pcis->ignore_pops--;
		    continue;
		}
		return_error(gs_error_rangecheck);
	    }
	    break;
	}
	break;
    }
    ipsp->ip = cip, ipsp->dstate = state;
    pcis->ips_count = ipsp + 1 - &pcis->ipstack[0];
    pcis->os_count = csp + 1 - &pcis->ostack[0];
    return c;
}

/* ------ Output ------ */

/* Put 2 or 4 bytes on a stream (big-endian). */
private void
sputc2(stream *s, int i)
{
    sputc(s, (byte)(i >> 8));
    sputc(s, (byte)i);
}
private void
sputc4(stream *s, int i)
{
    sputc2(s, i >> 16);
    sputc2(s, i);
}

/* Put a Type 2 operator on a stream. */
private void
type2_put_op(stream *s, int op)
{
    if (op >= CE_OFFSET) {
	spputc(s, cx_escape);
	spputc(s, (byte)(op - CE_OFFSET));
    } else
	sputc(s, (byte)op);
}

/* Put a Type 2 number on a stream. */
private void
type2_put_int(stream *s, int i)
{
    if (i >= -107 && i <= 107)
	sputc(s, (byte)(i + 139));
    else if (i <= 1131 && i >= 0)
	sputc2(s, (c_pos2_0 << 8) + i - 108);
    else if (i >= -1131 && i < 0)
	sputc2(s, (c_neg2_0 << 8) - i - 108);
    else if (i >= -32768 && i <= 32767) {
	spputc(s, c2_shortint);
	sputc2(s, i);
    } else {
	/*
	 * We can't represent this number directly: compute it.
	 * (This can be done much more efficiently in particular cases;
	 * we'll do this if it ever seems worthwhile.)
	 */
	type2_put_int(s, i >> 10);
	type2_put_int(s, 1024);
	type2_put_op(s, CE_OFFSET + ce2_mul);
	type2_put_int(s, i & 1023);
	type2_put_op(s, CE_OFFSET + ce2_add);
    }
}

/* Put a fixed value on a stream. */
private void
type2_put_fixed(stream *s, fixed v)
{
    if (fixed_is_int(v))
	type2_put_int(s, fixed2int_var(v));
    else if (v >= int2fixed(-32768) && v < int2fixed(32768)) {
	/* We can represent this as a 16:16 number. */
	spputc(s, cx_num4);
	sputc4(s, v << (16 - _fixed_shift));
    } else {
	type2_put_int(s, fixed2int_var(v));
	type2_put_fixed(s, fixed_fraction(v));
	type2_put_op(s, CE_OFFSET + ce2_add);
    }
}

/* Put a stem hint table on a stream. */
private void
type2_put_stems(stream *s, int os_count, const cv_stem_hint_table *psht, int op)
{
    fixed prev = 0;
    int pushed = os_count;
    int i;

    for (i = 0; i < psht->count; ++i, pushed += 2) {
	fixed v0 = psht->data[i].v0;
	fixed v1 = psht->data[i].v1;

	if (pushed > ostack_size - 2) {
	    type2_put_op(s, op);
	    pushed = 0;
	}
	type2_put_fixed(s, v0 - prev);
	type2_put_fixed(s, v1 - v0);
	prev = v1;
    }
    type2_put_op(s, op);
}

/* Put out a hintmask command. */
private void
type2_put_hintmask(stream *s, const byte *mask, uint size)
{
    uint ignore;

    type2_put_op(s, c2_hintmask);
    sputs(s, mask, size, &ignore);
}

/* ------ Main program ------ */


/*
 * Convert a Type 1 Charstring to (unencrypted) Type 2.
 * For simplicity, we expand all Subrs in-line.
 * We still need to optimize the output using these patterns:
 *	(vhcurveto hvcurveto)* (vhcurveto hrcurveto | vrcurveto) =>
 *	  vhcurveto
 *	(hvcurveto vhcurveto)* (hvcurveto vrcurveto | hrcurveto) =>
 *	  hvcurveto
 */
#define MAX_STACK ostack_size
int
psf_convert_type1_to_type2(stream *s, const gs_glyph_data_t *pgd,
			   gs_font_type1 *pfont)
{
    gs_type1_state cis;
    cv_stem_hint_table hstem_hints;	/* horizontal stem hints */
    cv_stem_hint_table vstem_hints;	/* vertical stem hints */
    bool first = true;
    bool replace_hints = false;
    bool hints_changed = false;
    enum {
	dotsection_in = 0,
	dotsection_out = -1
    } dotsection_flag = dotsection_out;
    byte active_hints[(max_total_stem_hints + 7) / 8];
    byte dot_save_hints[(max_total_stem_hints + 7) / 8];
    uint hintmask_size;
#define HINTS_CHANGED()\
  BEGIN\
    hints_changed = replace_hints;\
    if (hints_changed)\
	CHECK_OP();		/* see below */\
  END
#define CHECK_HINTS_CHANGED()\
  BEGIN\
    if (hints_changed) {\
	type2_put_hintmask(s, active_hints, hintmask_size);\
	hints_changed = false;\
    }\
  END
    /* 
     * In order to combine Type 1 operators, we usually delay writing
     * out operators (but not their operands).  We must keep track of
     * the stack depth so we don't exceed it when combining operators.
     */
    int depth;			/* of operands on stack */
    int prev_op;		/* operator to write, -1 if none */
#define CLEAR_OP()\
  (depth = 0, prev_op = -1)
#define CHECK_OP()\
  BEGIN\
    if (prev_op >= 0) {\
	type2_put_op(s, prev_op);\
	CLEAR_OP();\
    }\
  END
    fixed mx0 = 0, my0 = 0; /* See ce1_setcurrentpoint. */

    /*
     * Do a first pass to collect hints.  Note that we must also process
     * [h]sbw, because the hint coordinates are relative to the lsb.
     */
    hstem_hints.count = hstem_hints.replaced_count = hstem_hints.current = 0;
    vstem_hints.count = vstem_hints.replaced_count = vstem_hints.current = 0;
    type1_next_init(&cis, pgd, pfont);
    for (;;) {
	int c = type1_next(&cis);
	fixed *csp = &cis.ostack[cis.os_count - 1];

	switch (c) {
	default:
	    if (c < 0)
		return c;
	    type1_clear(&cis);
	    continue;
	case c1_hsbw:
	    gs_type1_sbw(&cis, cis.ostack[0], fixed_0, cis.ostack[1], fixed_0);
	    goto clear;
	case cx_hstem:
	    type1_stem1(&cis, &hstem_hints, csp - 1, cis.lsb.y, NULL);
	    goto clear;
	case cx_vstem:
	    type1_stem1(&cis, &vstem_hints, csp - 1, cis.lsb.x, NULL);
	    goto clear;
	case CE_OFFSET + ce1_sbw:
	    gs_type1_sbw(&cis, cis.ostack[0], cis.ostack[1],
			 cis.ostack[2], cis.ostack[3]);
	    goto clear;
	case CE_OFFSET + ce1_vstem3:
	    type1_stem3(&cis, &vstem_hints, csp - 5, cis.lsb.x, NULL);
	    goto clear;
	case CE_OFFSET + ce1_hstem3:
	    type1_stem3(&cis, &hstem_hints, csp - 5, cis.lsb.y, NULL);
	clear:
	    type1_clear(&cis);
	    continue;
	case ce1_callothersubr:
	    if (*csp == int2fixed(3))
		replace_hints = true;
	    cis.os_count -= 2;
	    continue;
	case CE_OFFSET + ce1_dotsection:
	    replace_hints = true;
	    continue;
	case CE_OFFSET + ce1_seac:
	case cx_endchar:
	    break;
	}
	break;
    }
    /*
     * Number the hints for hintmask.  We must do this even if we never
     * replace hints, because type1_stem# uses the index to set bits in
     * active_hints.
     */
    {
	int i;

	for (i = 0; i < hstem_hints.count; ++i)
	    hstem_hints.data[i].index = i;
	for (i = 0; i < vstem_hints.count; ++i)
	    vstem_hints.data[i].index = i + hstem_hints.count;
    }
    if (replace_hints) {
	hintmask_size =
	    (hstem_hints.count + vstem_hints.count + 7) / 8;
	memset(active_hints, 0, hintmask_size);
    } else 
	hintmask_size = 0;

    /* Do a second pass to write the result. */
    type1_next_init(&cis, pgd, pfont);
    CLEAR_OP();
    for (;;) {
	int c = type1_next(&cis);
	fixed *csp = &cis.ostack[cis.os_count - 1];
#define POP(n)\
  (csp -= (n), cis.os_count -= (n))
	int i;
	fixed mx, my;

	switch (c) {
	default:
	    if (c < 0)
		return c;
	    if (c >= CE_OFFSET)
		return_error(gs_error_rangecheck);
	    /* The Type 1 use of all other operators is the same in Type 2. */
	copy:
	    CHECK_OP();
	    CHECK_HINTS_CHANGED();
	put:
	    for (i = 0; i < cis.os_count; ++i)
		type2_put_fixed(s, cis.ostack[i]);
	    depth += cis.os_count;
	    prev_op = c;
	    type1_clear(&cis);
	    continue;
	case cx_hstem:
	    type1_stem1(&cis, &hstem_hints, csp - 1, cis.lsb.y, active_hints);
	hint:
	    HINTS_CHANGED();
	    type1_clear(&cis);
	    continue;
	case cx_vstem:
	    type1_stem1(&cis, &vstem_hints, csp - 1, cis.lsb.x, active_hints);
	    goto hint;
	case CE_OFFSET + ce1_vstem3:
	    type1_stem3(&cis, &vstem_hints, csp - 5, cis.lsb.x, active_hints);
	    goto hint;
	case CE_OFFSET + ce1_hstem3:
	    type1_stem3(&cis, &hstem_hints, csp - 5, cis.lsb.y, active_hints);
	    goto hint;
	case CE_OFFSET + ce1_dotsection:
	    if (dotsection_flag == dotsection_out) {
		memcpy(dot_save_hints, active_hints, hintmask_size);
		memset(active_hints, 0, hintmask_size);
		dotsection_flag = dotsection_in;
	    } else {
		memcpy(active_hints, dot_save_hints, hintmask_size);
		dotsection_flag = dotsection_out;
	    }
	    HINTS_CHANGED();
	    continue;
	case c1_closepath:
	    continue;
	case CE_OFFSET + ce1_setcurrentpoint:
	    if (first) {
		/*  A workaround for fonts which use ce1_setcurrentpoint 
		    in an illegal way for shifting a path. 
		    See t1_hinter__setcurrentpoint for more information. */
		mx0 = csp[-1], my0 = *csp;
	    }
	    continue;
	case cx_vmoveto:
	    mx = 0, my = *csp;
	    POP(1); goto move;
	case cx_hmoveto:
	    mx = *csp, my = 0;
	    POP(1); goto move;
	case cx_rmoveto:
	    mx = csp[-1], my = *csp;
	    POP(2);
	move:
	    CHECK_OP();
	    if (first) {
		if (cis.os_count)
		    type2_put_fixed(s, *csp); /* width */
		mx += cis.lsb.x + mx0, my += cis.lsb.y + my0;
		first = false;
	    }
	    if (cis.flex_count != flex_max) {
		/* We're accumulating points for a flex. */
		if (type1_next(&cis) != ce1_callothersubr)
		    return_error(gs_error_rangecheck);
		csp = &cis.ostack[cis.os_count - 1];
		if (*csp != int2fixed(2) || csp[-1] != fixed_0)
		    return_error(gs_error_rangecheck);
		cis.flex_count++;
		csp[-1] = mx, *csp = my;
		continue;
	    }
	    CHECK_HINTS_CHANGED();
	    if (mx == 0) {
		type2_put_fixed(s, my);
		depth = 1, prev_op = cx_vmoveto;
	    } else if (my == 0) {
		type2_put_fixed(s, mx);
		depth = 1, prev_op = cx_hmoveto;
	    } else {
		type2_put_fixed(s, mx);
		type2_put_fixed(s, my);
		depth = 2, prev_op = cx_rmoveto;
	    }
	    type1_clear(&cis);
	    continue;
	case c1_hsbw:
	    gs_type1_sbw(&cis, cis.ostack[0], fixed_0, cis.ostack[1], fixed_0);
	    /*
	     * Leave the l.s.b. on the operand stack for the initial hint,
	     * moveto, or endchar command.
	     */
	    cis.ostack[0] = cis.ostack[1];
	sbw:
	    if (cis.ostack[0] == pfont->data.defaultWidthX)
		cis.os_count = 0;
	    else {
		cis.ostack[0] -= pfont->data.nominalWidthX;
		cis.os_count = 1;
	    }
	    if (hstem_hints.count) {
		if (cis.os_count)
		    type2_put_fixed(s, cis.ostack[0]);
		type2_put_stems(s, cis.os_count, &hstem_hints,
				(replace_hints ? c2_hstemhm : cx_hstem));
		cis.os_count = 0;
	    }
	    if (vstem_hints.count) {
		if (cis.os_count)
		    type2_put_fixed(s, cis.ostack[0]);
		type2_put_stems(s, cis.os_count, &vstem_hints,
				(replace_hints ? c2_vstemhm : cx_vstem));
		cis.os_count = 0;
	    }
	    continue;
	case CE_OFFSET + ce1_seac:
	    /*
	     * It is an undocumented feature of the Type 2 CharString
	     * format that endchar + 4 or 5 operands is equivalent to
	     * seac with an implicit asb operand + endchar with 0 or 1
	     * operands.  Remove the asb argument from the stack, but
	     * adjust the adx argument to compensate for the fact that
	     * Type 2 CharStrings don't have any concept of l.s.b.
	     */
	    csp[-3] += cis.lsb.x - csp[-4];
	    memmove(csp - 4, csp - 3, sizeof(*csp) * 4);
	    POP(1);
	    /* (falls through) */
	case cx_endchar:
	    CHECK_OP();
	    for (i = 0; i < cis.os_count; ++i)
		type2_put_fixed(s, cis.ostack[i]);
	    type2_put_op(s, cx_endchar);
	    return 0;
	case CE_OFFSET + ce1_sbw:
	    gs_type1_sbw(&cis, cis.ostack[0], cis.ostack[1],
			 cis.ostack[2], cis.ostack[3]);
	    cis.ostack[0] = cis.ostack[2];
	    goto sbw;
	case ce1_callothersubr:
	    CHECK_OP();
	    switch (fixed2int_var(*csp)) {
	    default:
		return_error(gs_error_rangecheck);
	    case 0:
		/*
		 * The operand stack contains: delta to reference point,
		 * 6 deltas for the two curves, fd, final point, 3, 0.
		 */
		csp[-18] += csp[-16], csp[-17] += csp[-15];
		memmove(csp - 16, csp - 14, sizeof(*csp) * 11);
		cis.os_count -= 6, csp -= 6;
		/*
		 * We could optimize by using [h]flex[1],
		 * but it isn't worth the trouble.
		 */
		c = CE_OFFSET + ce2_flex;
		cis.flex_count = flex_max;	/* not inside flex */
		cis.ignore_pops = 2;
		goto copy;
	    case 1:
		cis.flex_count = 0;
		cis.os_count -= 2;
		continue;
	    /*case 2:*/		/* detected in *moveto */
	    case 3:
		memset(active_hints, 0, hintmask_size);
		HINTS_CHANGED();
		cis.ignore_pops = 1;
		cis.os_count -= 2;
		continue;
	    case 12:
	    case 13:
		/* Counter control is not implemented. */
		cis.os_count -= 2 + fixed2int(csp[-1]);
		continue;
	    }
	    /*
	     * The remaining cases are strictly for optimization.
	     */
	case cx_rlineto:
	    if (depth > MAX_STACK - 2)
		goto copy;
	    switch (prev_op) {
	    case cx_rlineto:	/* rlineto+ => rlineto */
		goto put;
	    case cx_rrcurveto:	/* rrcurveto+ rlineto => rcurveline */
		c = c2_rcurveline;
		goto put;
	    default:
		goto copy;
	    }
	case cx_hlineto:  /* hlineto (vlineto hlineto)* [vlineto] => hlineto */
	    if (depth > MAX_STACK - 1 ||
		prev_op != (depth & 1 ? cx_vlineto : cx_hlineto))
		goto copy;
	    c = prev_op;
	    goto put;
	case cx_vlineto:  /* vlineto (hlineto vlineto)* [hlineto] => vlineto */
	    if (depth > MAX_STACK - 1 ||
		prev_op != (depth & 1 ? cx_hlineto : cx_vlineto))
		goto copy;
	    c = prev_op;
	    goto put;
	case cx_hvcurveto: /* hvcurveto (vhcurveto hvcurveto)* => hvcurveto */
				/* (vhcurveto hvcurveto)+ => vhcurveto  */
	    /*
	     * We have to check (depth & 1) because the last curve might
	     * have 5 parameters rather than 4 (see rrcurveto below).
	     */
	    if ((depth & 1) || depth > MAX_STACK - 4 ||
		prev_op != (depth & 4 ? cx_vhcurveto : cx_hvcurveto))
		goto copy;
	    c = prev_op;
	    goto put;
	case cx_vhcurveto: /* vhcurveto (hvcurveto vhcurveto)* => vhcurveto */
				/* (hvcurveto vhcurveto)+ => hvcurveto  */
	    /* See above re the (depth & 1) check. */
	    if ((depth & 1) || depth > MAX_STACK - 4 ||
		prev_op != (depth & 4 ? cx_hvcurveto : cx_vhcurveto))
		goto copy;
	    c = prev_op;
	    goto put;
	case cx_rrcurveto:
	    if (depth == 0) {
		if (csp[-1] == 0) {
		    /* A|0 B C D 0 F rrcurveto => [A] B C D F vvcurveto */
		    c = c2_vvcurveto;
		    csp[-1] = csp[0];
		    if (csp[-5] == 0) {
			memmove(csp - 5, csp - 4, sizeof(*csp) * 4);
			POP(2);
		    } else
			POP(1);
		} else if (*csp == 0) {
		    /* A B|0 C D E 0 rrcurveto => [B] A C D E hhcurveto */
		    c = c2_hhcurveto;
		    if (csp[-4] == 0) {
			memmove(csp - 4, csp - 3, sizeof(*csp) * 3);
			POP(2);
		    } else {
			*csp = csp[-5], csp[-5] = csp[-4], csp[-4] = *csp;
			POP(1);
		    }
		}
		/*
		 * We could also optimize:
		 *   0 B C D E F|0 rrcurveto => B C D E [F] vhcurveto
		 *   A 0 C D E|0 F rrcurveto => A C D F [E] hvcurveto
		 * but this gets in the way of subsequent optimization
		 * of multiple rrcurvetos, so we don't do it.
		 */
		goto copy;
	    }
	    if (depth > MAX_STACK - 6)
		goto copy;
	    switch (prev_op) {
	    case c2_hhcurveto:	/* hrcurveto (x1 0 x2 y2 x3 0 rrcurveto)* => */
				/* hhcurveto */
		if (csp[-4] == 0 && *csp == 0) {
		    memmove(csp - 4, csp - 3, sizeof(*csp) * 3);
		    c = prev_op;
		    POP(2);
		    goto put;
		}
		goto copy;
	    case c2_vvcurveto:	/* rvcurveto (0 y1 x2 y2 0 y3 rrcurveto)* => */
				/* vvcurveto */
		if (csp[-5] == 0 && csp[-1] == 0) {
		    memmove(csp - 5, csp - 4, sizeof(*csp) * 3);
		    csp[-2] = *csp;
		    c = prev_op;
		    POP(2);
		    goto put;
		}
		goto copy;
	    case cx_hvcurveto:
		if (depth & 1)
		    goto copy;
		if (!(depth & 4))
		    goto hrc;
	    vrc:  /* (vhcurveto hvcurveto)+ vrcurveto => vhcurveto */
		/* hvcurveto (vhcurveto hvcurveto)* vrcurveto => hvcurveto */
		if (csp[-5] != 0)
		    goto copy;
		memmove(csp - 5, csp - 4, sizeof(*csp) * 5);
		c = prev_op;
		POP(1);
		goto put;
	    case cx_vhcurveto:
		if (depth & 1)
		    goto copy;
		if (!(depth & 4))
		    goto vrc;
	    hrc:  /* (hvcurveto vhcurveto)+ hrcurveto => hvcurveto */
		/* vhcurveto (hvcurveto vhcurveto)* hrcurveto => vhcurveto */
		if (csp[-4] != 0)
		    goto copy;
		/* A 0 C D E F => A C D F E */
		memmove(csp - 4, csp - 3, sizeof(*csp) * 2);
		csp[-2] = *csp;
		c = prev_op;
		POP(1);
		goto put;
	    case cx_rlineto:	/* rlineto+ rrcurveto => rlinecurve */
		c = c2_rlinecurve;
		goto put;
	    case cx_rrcurveto:	/* rrcurveto+ => rrcurveto */
		goto put;
	    default:
		goto copy;
	    }
	}
    }
}
