/* Copyright (C) 1996, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gstype2.c,v 1.36 2004/10/28 08:39:21 igor Exp $ */
/* Adobe Type 2 charstring interpreter */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxarith.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxcoord.h"
#include "gxistate.h"
#include "gxpath.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "gxtype1.h"
#include "gxhintn.h"

/* NOTE: The following are not yet implemented:
 *	Registry items other than 0
 *	Counter masks (but they are parsed correctly)
 *	'random' operator
 */

/* ------ Internal routines ------ */

/*
 * Set the character width.  This is provided as an optional extra operand
 * on the stack for the first operator.  After setting the width, we remove
 * the extra operand, and back up the interpreter pointer so we will
 * re-execute the operator when control re-enters the interpreter.
 */
#define check_first_operator(explicit_width)\
  BEGIN\
    if ( pcis->init_done < 0 )\
      { ipsp->ip = cip, ipsp->dstate = state;\
	return type2_sbw(pcis, csp, cstack, ipsp, explicit_width);\
      }\
  END
private int
type2_sbw(gs_type1_state * pcis, cs_ptr csp, cs_ptr cstack, ip_state_t * ipsp,
	  bool explicit_width)
{
    t1_hinter *h = &pcis->h;
    fixed sbx = fixed_0, sby = fixed_0, wx, wy = fixed_0;
    int code;

    if (explicit_width) {
	wx = cstack[0] + pcis->pfont->data.nominalWidthX;
	memmove(cstack, cstack + 1, (csp - cstack) * sizeof(*cstack));
	--csp;
    } else
	wx = pcis->pfont->data.defaultWidthX;
    if (pcis->seac_accent < 0) {
	if (pcis->sb_set) {
	    sbx = pcis->lsb.x;
	    sby = pcis->lsb.y;
	}
	if (pcis->width_set) {
	    wx = pcis->width.x;
	    wy = pcis->width.y;
	}
    }
    code = t1_hinter__sbw(h, sbx, sby, wx, wy);
    if (code < 0)
	return code;
    gs_type1_sbw(pcis, fixed_0, fixed_0, wx, fixed_0);
    /* Back up the interpretation pointer. */
    ipsp->ip--;
    decrypt_skip_previous(*ipsp->ip, ipsp->dstate);
    /* Save the interpreter state. */
    pcis->os_count = csp + 1 - cstack;
    pcis->ips_count = ipsp - &pcis->ipstack[0] + 1;
    memcpy(pcis->ostack, cstack, pcis->os_count * sizeof(cstack[0]));
    if (pcis->init_done < 0) {	/* Finish init when we return. */
	pcis->init_done = 0;
    }
    return type1_result_sbw;
}
private int
type2_vstem(gs_type1_state * pcis, cs_ptr csp, cs_ptr cstack)
{
    fixed x = 0;
    cs_ptr ap;
    t1_hinter *h = &pcis->h;
    int code;

    for (ap = cstack; ap + 1 <= csp; x += ap[1], ap += 2) {
        code = t1_hinter__vstem(h, x += ap[0], ap[1]);
	if (code < 0)
	    return code;
    }
    pcis->num_hints += (csp + 1 - cstack) >> 1;
    return 0;
}

/* ------ Main interpreter ------ */

/*
 * Continue interpreting a Type 2 charstring.  If str != 0, it is taken as
 * the byte string to interpret.  Return 0 on successful completion, <0 on
 * error, or >0 when client intervention is required (or allowed).  The int*
 * argument is only for compatibility with the Type 1 charstring interpreter.
 */
int
gs_type2_interpret(gs_type1_state * pcis, const gs_glyph_data_t *pgd,
		   int *ignore_pindex)
{
    gs_font_type1 *pfont = pcis->pfont;
    gs_type1_data *pdata = &pfont->data;
    t1_hinter *h = &pcis->h;
    bool encrypted = pdata->lenIV >= 0;
    fixed cstack[ostack_size];
    cs_ptr csp;
#define clear CLEAR_CSTACK(cstack, csp)
    ip_state_t *ipsp = &pcis->ipstack[pcis->ips_count - 1];
    register const byte *cip;
    register crypt_state state;
    register int c;
    cs_ptr ap;
    bool vertical;
    int code = 0;

/****** FAKE THE REGISTRY ******/
    struct {
	float *values;
	uint size;
    } Registry[1];

    Registry[0].values = pcis->pfont->data.WeightVector.values;

    switch (pcis->init_done) {
	case -1:
	    t1_hinter__init(h, pcis->path);
	    break;
	case 0:
	    gs_type1_finish_init(pcis);	/* sets origin */
            code = t1_hinter__set_mapping(h, &pcis->pis->ctm,
			    &pfont->FontMatrix, &pfont->base->FontMatrix,
			    pcis->scale.x.log2_unit, pcis->scale.x.log2_unit,
			    pcis->scale.x.log2_unit - pcis->log2_subpixels.x,
			    pcis->scale.y.log2_unit - pcis->log2_subpixels.y,
			    pcis->origin.x, pcis->origin.y, 
			    gs_currentaligntopixels(pfont->dir));
	    if (code < 0)
	    	return code;
	    code = t1_hinter__set_font_data(h, 2, pdata, pcis->no_grid_fitting);
	    if (code < 0)
	    	return code;
	    break;
	default /*case 1 */ :
	    break;
    }
    INIT_CSTACK(cstack, csp, pcis);

    if (pgd == 0)
	goto cont;
    ipsp->cs_data = *pgd;
    cip = pgd->bits.data;
  call:state = crypt_charstring_seed;
    if (encrypted) {
	int skip = pdata->lenIV;

	/* Skip initial random bytes */
	for (; skip > 0; ++cip, --skip)
	    decrypt_skip_next(*cip, state);
    }
    goto top;
  cont:cip = ipsp->ip;
    state = ipsp->dstate;
  top:for (;;) {
	uint c0 = *cip++;

	charstring_next(c0, state, c, encrypted);
	if (c >= c_num1) {
	    /* This is a number, decode it and push it on the stack. */

	    if (c < c_pos2_0) {	/* 1-byte number */
		decode_push_num1(csp, cstack, c);
	    } else if (c < cx_num4) {	/* 2-byte number */
		decode_push_num2(csp, cstack, c, cip, state, encrypted);
	    } else if (c == cx_num4) {	/* 4-byte number */
		long lw;

		decode_num4(lw, cip, state, encrypted);
		/* 32-bit numbers are 16:16. */
		CS_CHECK_PUSH(csp, cstack);
		*++csp = arith_rshift(lw, 16 - _fixed_shift);
	    } else		/* not possible */
		return_error(gs_error_invalidfont);
	  pushed:if_debug3('1', "[1]%d: (%d) %f\n",
		      (int)(csp - cstack), c, fixed2float(*csp));
	    continue;
	}
#ifdef DEBUG
	if (gs_debug['1']) {
	    static const char *const c2names[] =
	    {char2_command_names};

	    if (c2names[c] == 0)
		dlprintf2("[1]0x%lx: %02x??\n", (ulong) (cip - 1), c);
	    else
		dlprintf3("[1]0x%lx: %02x %s\n", (ulong) (cip - 1), c,
			  c2names[c]);
	}
#endif
	switch ((char_command) c) {
#define cnext clear; goto top

		/* Commands with identical functions in Type 1 and Type 2, */
		/* except for 'escape'. */

	    case c_undef0:
	    case c_undef2:
	    case c_undef17:
		return_error(gs_error_invalidfont);
	    case c_callsubr:
		c = fixed2int_var(*csp) + pdata->subroutineNumberBias;
		code = pdata->procs.subr_data
		    (pfont, c, false, &ipsp[1].cs_data);
	      subr:if (code < 0) {
	            /* Calling a Subr with an out-of-range index is clearly a error:
	             * the Adobe documentation says the results of doing this are
	             * undefined. However, we have seen a PDF file produced by Adobe
	             * PDF Library 4.16 that included a Type 2 font that called an
	             * out-of-range Subr, and Acrobat Reader did not signal an error.
	             * Therefore, we ignore such calls.
	             */
                    cip++;
                    goto top;
                }
		--csp;
		ipsp->ip = cip, ipsp->dstate = state;
		++ipsp;
		cip = ipsp->cs_data.bits.data;
		goto call;
	    case c_return:
		gs_glyph_data_free(&ipsp->cs_data, "gs_type2_interpret");
		--ipsp;
		goto cont;
	    case c_undoc15:
		/* See gstype1.h for information on this opcode. */
		cnext;

		/* Commands with similar but not identical functions */
		/* in Type 1 and Type 2 charstrings. */

	    case cx_hstem:
		goto hstem;
	    case cx_vstem:
		goto vstem;
	    case cx_vmoveto:
		check_first_operator(csp > cstack);
                code = t1_hinter__rmoveto(h, 0, *csp);
	      move:
	      cc:
		if (code < 0)
		    return code;
		goto pp;
	    case cx_rlineto:
		for (ap = cstack; ap + 1 <= csp; ap += 2) {
		    code = t1_hinter__rlineto(h, ap[0], ap[1]);
		    if (code < 0)
			return code;
		}
	      pp:
		cnext;
	    case cx_hlineto:
		vertical = false;
		goto hvl;
	    case cx_vlineto:
		vertical = true;
	      hvl:for (ap = cstack; ap <= csp; vertical = !vertical, ++ap) {
		    if (vertical) {
			code = t1_hinter__rlineto(h, 0, ap[0]);
		    } else {
			code = t1_hinter__rlineto(h, ap[0], 0);
		    }
		    if (code < 0)
			return code;
		}
		goto pp;
	    case cx_rrcurveto:
		for (ap = cstack; ap + 5 <= csp; ap += 6) {
		    code = t1_hinter__rcurveto(h, ap[0], ap[1], ap[2],
					    ap[3], ap[4], ap[5]);
		    if (code < 0)
			return code;
		}
		goto pp;
	    case cx_endchar:
  		/*
		 * It is a feature of Type 2 CharStrings that if endchar is
		 * invoked with 4 or 5 operands, it is equivalent to the
		 * Type 1 seac operator. In this case, the asb operand of
		 * seac is missing: we assume it is the same as the
		 * l.s.b. of the accented character.  This feature was
		 * undocumented until the 16 March 2000 version of the Type
		 * 2 Charstring Format specification, but, thankfully, is
		 * described in that revision.
		 */
		if (csp >= cstack + 3) {
		    check_first_operator(csp > cstack + 3);
		    code = gs_type1_seac(pcis, cstack, 0, ipsp);
		    if (code < 0)
			return code;
		    clear;
		    cip = ipsp->cs_data.bits.data;
		    goto call;
		}
		/*
		 * This might be the only operator in the charstring.
		 * In this case, there might be a width on the stack.
		 */
		check_first_operator(csp >= cstack);
		code = t1_hinter__endchar(h, (pcis->seac_accent >= 0));
		if (code < 0)
		    return code;
		if (pcis->seac_accent < 0) {
		    code = t1_hinter__endglyph(h);
		    if (code < 0)
			return code;
		    code = gx_setcurrentpoint_from_path(pcis->pis, pcis->path);
		    if (code < 0)
			return code;
		} else
		    t1_hinter__setcurrentpoint(h, pcis->save_adxy.x, pcis->save_adxy.y);
		code = gs_type1_endchar(pcis);
		if (code == 1) {
		    /*
		     * Reset the total hint count so that hintmask will
		     * parse its following data correctly.
		     * (gs_type1_endchar already reset the actual hint
		     * tables.)
		     */
		    pcis->num_hints = 0;
		    /* do accent of seac */
		    ipsp = &pcis->ipstack[pcis->ips_count - 1];
		    cip = ipsp->cs_data.bits.data;
		    goto call;
		}
		return code;
	    case cx_rmoveto:
		/* See vmoveto above re closing the subpath. */
		check_first_operator(!((csp - cstack) & 1));
		if (csp > cstack + 1) {
		  /* Some Type 2 charstrings omit the vstemhm operator before rmoveto, 
		     even though this is only allowed before hintmask and cntrmask.
		     Thanks to Felix Pahl.
		   */
		  type2_vstem(pcis, csp - 2, cstack);
		  cstack [0] = csp [-1];
		  cstack [1] = csp [ 0];
		  csp = cstack + 1;
		}
                code = t1_hinter__rmoveto(h, csp[-1], *csp);
		goto move;
	    case cx_hmoveto:
		/* See vmoveto above re closing the subpath. */
		check_first_operator(csp > cstack);
                code = t1_hinter__rmoveto(h, *csp, 0);
		goto move;
	    case cx_vhcurveto:
		vertical = true;
		goto hvc;
	    case cx_hvcurveto:
		vertical = false;
	      hvc:for (ap = cstack; ap + 3 <= csp; vertical = !vertical, ap += 4) {
		    gs_fixed_point pt[2] = {{0, 0}, {0, 0}};
		    if (vertical) {
			pt[0].y = ap[0];
			pt[1].x = ap[3];
			if (ap + 4 == csp)
			    pt[1].y = ap[4];
		    } else {
			pt[0].x = ap[0];
			if (ap + 4 == csp)
			    pt[1].x = ap[4];
			pt[1].y = ap[3];
		    }
		    code = t1_hinter__rcurveto(h, pt[0].x, pt[0].y, ap[1], ap[2], pt[1].x, pt[1].y);
		    if (code < 0)
			return code;
		}
		goto pp;

			/***********************
			 * New Type 2 commands *
			 ***********************/

	    case c2_blend:
		{
		    int n = fixed2int_var(*csp);
		    int num_values = csp - cstack;
		    gs_font_type1 *pfont = pcis->pfont;
		    int k = pfont->data.WeightVector.count;
		    int i, j;
		    cs_ptr base, deltas;

		    base = csp - 1 - num_values;
		    deltas = base + n - 1;
		    for (j = 0; j < n; j++, base++, deltas += k - 1)
			for (i = 1; i < k; i++)
			    *base += (fixed)(deltas[i] * 
				pfont->data.WeightVector.values[i]);
		}
		cnext;
	    case c2_hstemhm:
	      hstem:check_first_operator(!((csp - cstack) & 1));
		{
		    fixed x = 0;

		    for (ap = cstack; ap + 1 <= csp; x += ap[1], ap += 2) {
			    code = t1_hinter__hstem(h, x += ap[0], ap[1]);
			    if (code < 0)
				return code;
		    }
		}
		pcis->num_hints += (csp + 1 - cstack) >> 1;
		cnext;
	    case c2_hintmask:
		/*
		 * A hintmask at the beginning of the CharString is
		 * equivalent to vstemhm + hintmask.  For simplicity, we use
		 * this interpretation everywhere.
		 */
	    case c2_cntrmask:
		check_first_operator(!((csp - cstack) & 1));
		type2_vstem(pcis, csp, cstack);
		/*
		 * We should clear the stack here only if this is the
		 * initial mask operator that includes the implicit
		 * vstemhm, but currently this is too much trouble to
		 * detect.
		 */
		clear;
		{
		    byte mask[max_total_stem_hints / 8];
		    int i;

		    for (i = 0; i < pcis->num_hints; ++cip, i += 8) {
			charstring_next(*cip, state, mask[i >> 3], encrypted);
			if_debug1('1', " 0x%02x", mask[i >> 3]);
		    }
		    if_debug0('1', "\n");
		    ipsp->ip = cip;
		    ipsp->dstate = state;
		    if (c == c2_cntrmask) {
			/****** NYI ******/
		    } else {	/* hintmask or equivalent */
			if_debug0('1', "[1]hstem hints:\n");
			if_debug0('1', "[1]vstem hints:\n");
			code = t1_hinter__hint_mask(h, mask);
			if (code < 0)
			    return code;
		    }
		}
		break;
	    case c2_vstemhm:
	      vstem:check_first_operator(!((csp - cstack) & 1));
		type2_vstem(pcis, csp, cstack);
		cnext;
	    case c2_rcurveline:
		for (ap = cstack; ap + 5 <= csp; ap += 6) {
		    code = t1_hinter__rcurveto(h, ap[0], ap[1], ap[2], ap[3],
					    ap[4], ap[5]);
		    if (code < 0)
			return code;
		}
		code = t1_hinter__rlineto(h, ap[0], ap[1]);
		goto cc;
	    case c2_rlinecurve:
		for (ap = cstack; ap + 7 <= csp; ap += 2) {
		    code = t1_hinter__rlineto(h, ap[0], ap[1]);
		    if (code < 0)
			return code;
		}
		code = t1_hinter__rcurveto(h, ap[0], ap[1], ap[2], ap[3],
					ap[4], ap[5]);
		goto cc;
	    case c2_vvcurveto:
		ap = cstack;
		{
		    int n = csp + 1 - cstack;
		    fixed dxa = (n & 1 ? *ap++ : 0);

		    for (; ap + 3 <= csp; ap += 4) {
			code = t1_hinter__rcurveto(h, dxa, ap[0], ap[1], ap[2],
						fixed_0, ap[3]);
			if (code < 0)
			    return code;
			dxa = 0;
		    }
		}
		goto pp;
	    case c2_hhcurveto:
		ap = cstack;
		{
		    int n = csp + 1 - cstack;
		    fixed dya = (n & 1 ? *ap++ : 0);

		    for (; ap + 3 <= csp; ap += 4) {
			code = t1_hinter__rcurveto(h, ap[0], dya, ap[1], ap[2],
						ap[3], fixed_0);
			if (code < 0)
			    return code;
			dya = 0;
		    }
		}
		goto pp;
	    case c2_shortint:
		{
		    int c1, c2;

		    charstring_next(*cip, state, c1, encrypted);
		    ++cip;
		    charstring_next(*cip, state, c2, encrypted);
		    ++cip;
		    CS_CHECK_PUSH(csp, cstack);
		    *++csp = int2fixed((((c1 ^ 0x80) - 0x80) << 8) + c2);
		}
		goto pushed;
	    case c2_callgsubr:
		c = fixed2int_var(*csp) + pdata->gsubrNumberBias;
		code = pdata->procs.subr_data
		    (pfont, c, true, &ipsp[1].cs_data);
		goto subr;
	    case cx_escape:
		charstring_next(*cip, state, c, encrypted);
		++cip;
#ifdef DEBUG
		if (gs_debug['1'] && c < char2_extended_command_count) {
		    static const char *const ce2names[] =
		    {char2_extended_command_names};

		    if (ce2names[c] == 0)
			dlprintf2("[1]0x%lx: %02x??\n", (ulong) (cip - 1), c);
		    else
			dlprintf3("[1]0x%lx: %02x %s\n", (ulong) (cip - 1), c,
				  ce2names[c]);
		}
#endif
		switch ((char2_extended_command) c) {
		    case ce2_and:
			csp[-1] = ((csp[-1] != 0) & (*csp != 0) ? fixed_1 : 0);
			--csp;
			break;
		    case ce2_or:
			csp[-1] = (csp[-1] | *csp ? fixed_1 : 0);
			--csp;
			break;
		    case ce2_not:
			*csp = (*csp ? 0 : fixed_1);
			break;
		    case ce2_store:
			{
			    int i, n = fixed2int_var(*csp);
			    float *to = Registry[fixed2int_var(csp[-3])].values +
			    fixed2int_var(csp[-2]);
			    const fixed *from =
			    pcis->transient_array + fixed2int_var(csp[-1]);

			    for (i = 0; i < n; ++i)
				to[i] = fixed2float(from[i]);
			}
			csp -= 4;
			break;
		    case ce2_abs:
			if (*csp < 0)
			    *csp = -*csp;
			break;
		    case ce2_add:
			csp[-1] += *csp;
			--csp;
			break;
		    case ce2_sub:
			csp[-1] -= *csp;
			--csp;
			break;
		    case ce2_div:
			csp[-1] = float2fixed((double)csp[-1] / *csp);
			--csp;
			break;
		    case ce2_load:
			/* The specification says there is no j (starting index */
			/* in registry array) argument.... */
			{
			    int i, n = fixed2int_var(*csp);
			    const float *from = Registry[fixed2int_var(csp[-2])].values;
			    fixed *to =
			    pcis->transient_array + fixed2int_var(csp[-1]);

			    for (i = 0; i < n; ++i)
				to[i] = float2fixed(from[i]);
			}
			csp -= 3;
			break;
		    case ce2_neg:
			*csp = -*csp;
			break;
		    case ce2_eq:
			csp[-1] = (csp[-1] == *csp ? fixed_1 : 0);
			--csp;
			break;
		    case ce2_drop:
			--csp;
			break;
		    case ce2_put:
			pcis->transient_array[fixed2int_var(*csp)] = csp[-1];
			csp -= 2;
			break;
		    case ce2_get:
			*csp = pcis->transient_array[fixed2int_var(*csp)];
			break;
		    case ce2_ifelse:
			if (csp[-1] > *csp)
			    csp[-3] = csp[-2];
			csp -= 3;
			break;
		    case ce2_random:
			CS_CHECK_PUSH(csp, cstack);
			++csp;
			/****** NYI ******/
			break;
		    case ce2_mul:
			{
			    double prod = fixed2float(csp[-1]) * *csp;

			    csp[-1] = 
				(prod > max_fixed ? max_fixed :
				 prod < min_fixed ? min_fixed : (fixed)prod);
			}
			--csp;
			break;
		    case ce2_sqrt:
			if (*csp >= 0)
			    *csp = float2fixed(sqrt(fixed2float(*csp)));
			break;
		    case ce2_dup:
			CS_CHECK_PUSH(csp, cstack);
			csp[1] = *csp;
			++csp;
			break;
		    case ce2_exch:
			{
			    fixed top = *csp;

			    *csp = csp[-1], csp[-1] = top;
			}
			break;
		    case ce2_index:
			*csp =
			    (*csp < 0 ? csp[-1] : csp[-1 - fixed2int_var(csp[-1])]);
			break;
		    case ce2_roll:
			{
			    int distance = fixed2int_var(*csp);
			    int count = fixed2int_var(csp[-1]);
			    cs_ptr bot;

			    csp -= 2;
			    if (count < 0 || count > csp + 1 - cstack)
				return_error(gs_error_invalidfont);
			    if (count == 0)
				break;
			    if (distance < 0)
				distance = count - (-distance % count);
			    bot = csp + 1 - count;
			    while (--distance >= 0) {
				fixed top = *csp;

				memmove(bot + 1, bot,
					(count - 1) * sizeof(fixed));
				*bot = top;
			    }
			}
			break;
		    case ce2_hflex:
			csp[6] = fixed_half;	/* fd/100 */
			csp[4] = *csp, csp[5] = 0;	/* dx6, dy6 */
			csp[2] = csp[-1], csp[3] = -csp[-4];	/* dx5, dy5 */
			*csp = csp[-2], csp[1] = 0;	/* dx4, dy4 */
			csp[-2] = csp[-3], csp[-1] = 0;		/* dx3, dy3 */
			csp[-3] = csp[-4], csp[-4] = csp[-5];	/* dx2, dy2 */
			csp[-5] = 0;	/* dy1 */
			csp += 6;
			goto flex;
		    case ce2_flex:
			*csp /= 100;	/* fd/100 */
flex:			{
			    fixed x_join = csp[-12] + csp[-10] + csp[-8];
			    fixed y_join = csp[-11] + csp[-9] + csp[-7];
			    fixed x_end = x_join + csp[-6] + csp[-4] + csp[-2];
			    fixed y_end = y_join + csp[-5] + csp[-3] + csp[-1];
			    gs_point join, end;
			    double flex_depth;

			    if ((code =
				 gs_distance_transform(fixed2float(x_join),
						       fixed2float(y_join),
						       &ctm_only(pcis->pis),
						       &join)) < 0 ||
				(code =
				 gs_distance_transform(fixed2float(x_end),
						       fixed2float(y_end),
						       &ctm_only(pcis->pis),
						       &end)) < 0
				)
				return code;
			    /*
			     * Use the X or Y distance depending on whether
			     * the curve is more horizontal or more
			     * vertical.
			     */
			    if (any_abs(end.y) > any_abs(end.x))
				flex_depth = join.x;
			    else
				flex_depth = join.y;
			    if (fabs(flex_depth) < fixed2float(*csp)) {
				/* Do flex as line. */
				code = t1_hinter__rlineto(h, x_end, y_end);
			    } else {
				/*
				 * Do flex as curve.  We can't jump to rrc,
				 * because the flex operators don't clear
				 * the stack (!).
				 */
				code = t1_hinter__rcurveto(h, 
					csp[-12], csp[-11], csp[-10],
					csp[-9], csp[-8], csp[-7]);
				if (code < 0)
				    return code;
				code = t1_hinter__rcurveto(h, 
					csp[-6], csp[-5], csp[-4],
					csp[-3], csp[-2], csp[-1]);
			    }
			    if (code < 0)
				return code;
			    csp -= 13;
			}
			cnext;
		    case ce2_hflex1:
			csp[4] = fixed_half;	/* fd/100 */
			csp[2] = *csp;          /* dx6 */
			csp[3] = -(csp[-7] + csp[-5] + csp[-1]);	/* dy6 */
			*csp = csp[-2], csp[1] = csp[-1];	/* dx5, dy5 */
			csp[-2] = csp[-3], csp[-1] = 0;		/* dx4, dy4 */
			csp[-3] = 0;	/* dy3 */
			csp += 4;
			goto flex;
		    case ce2_flex1:
			{
			    fixed dx = csp[-10] + csp[-8] + csp[-6] + csp[-4] + csp[-2];
			    fixed dy = csp[-9] + csp[-7] + csp[-5] + csp[-3] + csp[-1];

			    if (any_abs(dx) > any_abs(dy))
				csp[1] = -dy;	/* d6 is dx6 */
			    else
				csp[1] = *csp, *csp = -dx;	/* d6 is dy6 */
			}
			csp[2] = fixed_half;	/* fd/100 */
			csp += 2;
			goto flex;
		}
		break;

		/* Fill up the dispatch up to 32. */

	      case_c2_undefs:
	    default:		/* pacify compiler */
		return_error(gs_error_invalidfont);
	}
    }
}
