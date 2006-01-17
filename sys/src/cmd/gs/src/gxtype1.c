/* Copyright (C) 1997, 1998, 1999, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gxtype1.c,v 1.41 2004/11/19 04:39:11 ray Exp $ */
/* Adobe Type 1 font interpreter support */
#include "math_.h"
#include "memory_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsccode.h"
#include "gsline.h"
#include "gsstruct.h"
#include "gxarith.h"
#include "gxchrout.h"
#include "gxfixed.h"
#include "gxistate.h"
#include "gxmatrix.h"
#include "gxcoord.h"
#include "gxfont.h"
#include "gxfont1.h"
#include "gxtype1.h"
#include "gzpath.h"

/*
 * The routines in this file are used for both Type 1 and Type 2
 * charstring interpreters.
 */

/*
 * Define whether or not to force hints to "big pixel" boundaries
 * when rasterizing at higher resolution.  With the current algorithms,
 * a value of 1 is better for devices without alpha capability,
 * but 0 is better if alpha is available.
 */
#define FORCE_HINTS_TO_BIG_PIXELS 1

/* Structure descriptor */
public_st_gs_font_type1();

/* Define the structure type for a Type 1 interpreter state. */
public_st_gs_type1_state();
/* GC procedures */
private 
ENUM_PTRS_WITH(gs_type1_state_enum_ptrs, gs_type1_state *pcis)
{
    index -= 4;
    if (index < pcis->ips_count * ST_GLYPH_DATA_NUM_PTRS)
	return ENUM_USING(st_glyph_data,
			&pcis->ipstack[index / ST_GLYPH_DATA_NUM_PTRS].cs_data,
			  sizeof(pcis->ipstack[0].cs_data),
			  index % ST_GLYPH_DATA_NUM_PTRS);
    return 0;
}
ENUM_PTR3(0, gs_type1_state, pfont, pis, path);
ENUM_PTR(3, gs_type1_state, callback_data);
ENUM_PTRS_END
private RELOC_PTRS_WITH(gs_type1_state_reloc_ptrs, gs_type1_state *pcis)
{
    int i;

    RELOC_PTR(gs_type1_state, pfont);
    RELOC_PTR(gs_type1_state, pis);
    RELOC_PTR(gs_type1_state, path);
    RELOC_PTR(gs_type1_state, callback_data);
    for (i = 0; i < pcis->ips_count; i++) {
	ip_state_t *ipsp = &pcis->ipstack[i];
	int diff = ipsp->ip - ipsp->cs_data.bits.data;

	RELOC_USING(st_glyph_data, &ipsp->cs_data, sizeof(ipsp->cs_data));
	ipsp->ip = ipsp->cs_data.bits.data + diff;
    }
} RELOC_PTRS_END

/* Define a string to interact with unique_name in lib/pdf_font.ps .
   The string is used to resolve glyph name conflict while
   converting PDF Widths into Metrics.
 */
const char gx_extendeg_glyph_name_separator[] = "~GS~";


/* ------ Interpreter services ------ */

#define s (*ps)

/* Initialize a Type 1 interpreter. */
/* The caller must supply a string to the first call of gs_type1_interpret. */
int
gs_type1_interp_init(register gs_type1_state * pcis, gs_imager_state * pis,
    gx_path * ppath, const gs_log2_scale_point * pscale, 
    const gs_log2_scale_point * psubpixels, bool no_grid_fitting,
		     int paint_type, gs_font_type1 * pfont)
{
    static const gs_log2_scale_point no_scale = {0, 0};
    const gs_log2_scale_point *plog2_scale =
	(FORCE_HINTS_TO_BIG_PIXELS && pscale != NULL ? pscale : &no_scale);
    const gs_log2_scale_point *plog2_subpixels =
	(FORCE_HINTS_TO_BIG_PIXELS ? (psubpixels != NULL ? psubpixels : plog2_scale) : &no_scale);

    if_debug0('1', "[1]gs_type1_interp_init\n");
    pcis->pfont = pfont;
    pcis->pis = pis;
    pcis->path = ppath;
    pcis->callback_data = pfont; /* default callback data */
    pcis->no_grid_fitting = no_grid_fitting;
    pcis->paint_type = paint_type;
    pcis->os_count = 0;
    pcis->ips_count = 1;
    pcis->ipstack[0].ip = 0;
    gs_glyph_data_from_null(&pcis->ipstack[0].cs_data);
    pcis->ignore_pops = 0;
    pcis->init_done = -1;
    pcis->sb_set = false;
    pcis->width_set = false;
    pcis->num_hints = 0;
    pcis->seac_accent = -1;
    pcis->log2_subpixels = *plog2_subpixels;

    /* Set the sampling scale. */
    set_pixel_scale(&pcis->scale.x, plog2_scale->x);
    set_pixel_scale(&pcis->scale.y, plog2_scale->y);

    return 0;
}

/* Set the push/pop callback data. */
void
gs_type1_set_callback_data(gs_type1_state *pcis, void *callback_data)
{
    pcis->callback_data = callback_data;
}


/* Preset the left side bearing and/or width. */
void
gs_type1_set_lsb(gs_type1_state * pcis, const gs_point * psbpt)
{
    pcis->lsb.x = float2fixed(psbpt->x);
    pcis->lsb.y = float2fixed(psbpt->y);
    pcis->sb_set = true;
}
void
gs_type1_set_width(gs_type1_state * pcis, const gs_point * pwpt)
{
    pcis->width.x = float2fixed(pwpt->x);
    pcis->width.y = float2fixed(pwpt->y);
    pcis->width_set = true;
}

/* Finish initializing the interpreter if we are actually rasterizing */
/* the character, as opposed to just computing the side bearing and width. */
void
gs_type1_finish_init(gs_type1_state * pcis)
{
    gs_imager_state *pis = pcis->pis;
    const int max_coeff_bits = 11;	/* max coefficient in char space */

    /* Set up the fixed version of the transformation. */
    gx_matrix_to_fixed_coeff(&ctm_only(pis), &pcis->fc, max_coeff_bits);

    /* Set the current point of the path to the origin, */
    pcis->origin.x = pcis->path->position.x;
    pcis->origin.y = pcis->path->position.y;

    /* Initialize hint-related scalars. */
    pcis->asb_diff = pcis->adxy.x = pcis->adxy.y = 0;
    pcis->flex_count = flex_max;	/* not in Flex */
    pcis->vs_offset.x = pcis->vs_offset.y = 0;


    /* Compute the flatness needed for accurate rendering. */
    pcis->flatness = gs_char_flatness(pis, 0.001);

    pcis->init_done = 1;
}


#undef s

/* Record the side bearing and character width. */
int
gs_type1_sbw(gs_type1_state * pcis, fixed lsbx, fixed lsby, fixed wx, fixed wy)
{
    if (!pcis->sb_set)
	pcis->lsb.x = lsbx, pcis->lsb.y = lsby,
	    pcis->sb_set = true;	/* needed for accented chars */
    if (!pcis->width_set)
	pcis->width.x = wx, pcis->width.y = wy,
	    pcis->width_set = true;
    if_debug4('1', "[1]sb=(%g,%g) w=(%g,%g)\n",
	      fixed2float(pcis->lsb.x), fixed2float(pcis->lsb.y),
	      fixed2float(pcis->width.x), fixed2float(pcis->width.y));
    return 0;
}

/* Blend values for a Multiple Master font instance. */
/* The stack holds values ... K*N othersubr#. */
int
gs_type1_blend(gs_type1_state *pcis, fixed *csp, int num_results)
{
    gs_type1_data *pdata = &pcis->pfont->data;
    int num_values = fixed2int_var(csp[-1]);
    int k1 = num_values / num_results - 1;
    int i, j;
    fixed *base;
    fixed *deltas;

    if (num_values < num_results ||
	num_values % num_results != 0
	)
	return_error(gs_error_invalidfont);
    base = csp - 1 - num_values;
    deltas = base + num_results - 1;
    for (j = 0; j < num_results;
	 j++, base++, deltas += k1
	 )
	for (i = 1; i <= k1; i++)
	    *base += (fixed)(deltas[i] *
		pdata->WeightVector.values[i]);
    pcis->ignore_pops = num_results;
    return num_values - num_results + 2;
}

/*
 * Handle a seac.  Do the base character now; when it finishes (detected
 * in endchar), do the accent.  Note that we pass only 4 operands on the
 * stack, and pass asb separately.
 */
int
gs_type1_seac(gs_type1_state * pcis, const fixed * cstack, fixed asb,
	      ip_state_t * ipsp)
{
    gs_font_type1 *pfont = pcis->pfont;
    gs_glyph_data_t bgdata;
    gs_const_string gstr;
    int code;

    /* Save away all the operands. */
    pcis->seac_accent = fixed2int_var(cstack[3]);
    pcis->save_asb = asb;
    pcis->save_lsb = pcis->lsb;
    pcis->save_adxy.x = cstack[0];
    pcis->save_adxy.y = cstack[1];
    pcis->os_count = 0;		/* clear */
    /* Ask the caller to provide the base character's CharString. */
    code = pfont->data.procs.seac_data
	(pfont, fixed2int_var(cstack[2]), NULL, &gstr, &bgdata);
    if (code < 0)
	return code;
    /* Continue with the supplied string. */
    ipsp->cs_data = bgdata;
    return 0;
}

/*
 * Handle the end of a character.  Return 0 if this is really the end of a
 * character, or 1 if we still have to process the accent of a seac.
 * In the latter case, the interpreter control stack has been set up to
 * point to the start of the accent's CharString; the caller must
 * also set ptx/y to pcis->position.x/y.
 */
int
gs_type1_endchar(gs_type1_state * pcis)
{
    gs_imager_state *pis = pcis->pis;

    if (pcis->seac_accent >= 0) {	/* We just finished the base character of a seac. */
	/* Do the accent. */
	gs_font_type1 *pfont = pcis->pfont;
	gs_glyph_data_t agdata;
	int achar = pcis->seac_accent;
	gs_const_string gstr;
	int code;

	agdata.memory = pfont->memory;
	pcis->seac_accent = -1;
	/* Reset the coordinate system origin */
	pcis->asb_diff = pcis->save_asb - pcis->save_lsb.x;
	pcis->adxy = pcis->save_adxy;
	pcis->os_count = 0;	/* clear */
	/* Clear the ipstack, in case the base character */
	/* ended inside a subroutine. */
	pcis->ips_count = 1;
	/* Ask the caller to provide the accent's CharString. */
	code = pfont->data.procs.seac_data(pfont, achar, NULL, &gstr, &agdata);
	if (code == gs_error_undefined) {
	    /* 
	     * The font is missing the accent's CharString (due to
	     * bad subsetting).  Just end drawing here without error. 
	     * This is like Acrobat Reader behaves.
	     */
	    char buf0[gs_font_name_max + 1], buf1[30];
	    int l0 = min(pcis->pfont->font_name.size, sizeof(buf0) - 1);
	    int l1 = min(gstr.size, sizeof(buf1) - 1);

	    memcpy(buf0, pcis->pfont->font_name.chars, l0);
	    buf0[l0] = 0;
	    memcpy(buf1, gstr.data, l1);
	    buf1[l1] = 0;
	    eprintf2("The font '%s' misses the glyph '%s' . Continue skipping the glyph.", buf0, buf1);
	    return 0;
	}
	if (code < 0)
	    return code;
	/* Continue with the supplied string. */
	pcis->ips_count = 1;
	pcis->ipstack[0].cs_data = agdata;
	return 1;
    }
    if (pcis->pfont->PaintType == 0)
	pis->fill_adjust.x = pis->fill_adjust.y = -1;
    /* Set the flatness for curve rendering. */
    if (!pcis->no_grid_fitting)
	gs_imager_setflat(pis, pcis->flatness);
    return 0;
}

/* Get the metrics (l.s.b. and width) from the Type 1 interpreter. */
void
type1_cis_get_metrics(const gs_type1_state * pcis, double psbw[4])
{
    psbw[0] = fixed2float(pcis->lsb.x);
    psbw[1] = fixed2float(pcis->lsb.y);
    psbw[2] = fixed2float(pcis->width.x);
    psbw[3] = fixed2float(pcis->width.y);
}

/* ------ Font procedures ------ */


/*
 * If a Type 1 character is defined with 'seac', store the character codes
 * in chars[0] and chars[1] and return 1; otherwise, return 0 or <0.
 * This is exported only for the benefit of font copying.
 */
int
gs_type1_piece_codes(/*const*/ gs_font_type1 *pfont,
		     const gs_glyph_data_t *pgd, gs_char *chars)
{
    gs_type1_data *const pdata = &pfont->data;
    /*
     * Decode the CharString looking for seac.  We have to process
     * callsubr, callothersubr, and return operators, but if we see
     * any other operators other than [h]sbw, pop, hint operators,
     * or endchar, we can return immediately.  We have to include
     * endchar because it is an (undocumented) equivalent for seac
     * in Type 2 CharStrings: see the cx_endchar case in
     * gs_type2_interpret in gstype2.c.
     *
     * It's really unfortunate that we have to duplicate so much parsing
     * code, but factoring out the parser from the interpreter would
     * involve more restructuring than we're prepared to do right now.
     */
    bool encrypted = pdata->lenIV >= 0;
    fixed cstack[ostack_size];
    fixed *csp;
    ip_state_t ipstack[ipstack_size + 1];
    ip_state_t *ipsp = &ipstack[0];
    const byte *cip;
    crypt_state state;
    int c;
    int code;
    
    CLEAR_CSTACK(cstack, csp);
    cip = pgd->bits.data;
 call:
    state = crypt_charstring_seed;
    if (encrypted) {
	int skip = pdata->lenIV;

	/* Skip initial random bytes */
	for (; skip > 0; ++cip, --skip)
	    decrypt_skip_next(*cip, state);
    }
 top:
    for (;;) {
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
		CS_CHECK_PUSH(csp, cstack);
		*++csp = int2fixed(lw);
	    } else		/* not possible */
		return_error(gs_error_invalidfont);
	    continue;
	}
#define cnext CLEAR_CSTACK(cstack, csp); goto top
	switch ((char_command) c) {
	default:
	    goto out;
	case c_callsubr:
	    c = fixed2int_var(*csp) + pdata->subroutineNumberBias;
	    code = pdata->procs.subr_data
		(pfont, c, false, &ipsp[1].cs_data);
	    if (code < 0)
		return_error(code);
	    --csp;
	    ipsp->ip = cip, ipsp->dstate = state;
	    ++ipsp;
	    cip = ipsp->cs_data.bits.data;
	    goto call;
	case c_return:
	    gs_glyph_data_free(&ipsp->cs_data, "gs_type1_piece_codes");
	    --ipsp;
	    cip = ipsp->ip, state = ipsp->dstate;
	    goto top;
	case cx_hstem:
	case cx_vstem:
	case c1_hsbw:
	    cnext;
	case cx_endchar:
	    if (csp < cstack + 3)
		goto out;	/* not seac */
	do_seac:
	    /* This is the payoff for all this code! */
	    chars[0] = fixed2int(csp[-1]);
	    chars[1] = fixed2int(csp[0]);
	    return 1;
	case cx_escape:
	    charstring_next(*cip, state, c, encrypted);
	    ++cip;
	    switch ((char1_extended_command) c) {
	    default:
		goto out;
	    case ce1_vstem3:
	    case ce1_hstem3:
	    case ce1_sbw:
		cnext;
	    case ce1_pop:
		/*
		 * pop must do nothing, since it is used after
		 * subr# 1 3 callothersubr.
		 */
		goto top;
	    case ce1_seac:
		goto do_seac;
	    case ce1_callothersubr:
		switch (fixed2int_var(*csp)) {
		default:
		    goto out;
		case 3:
		    csp -= 2;
		    goto top;
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		    cnext;
		}
	    }
	}
#undef cnext
    }
 out:
    return 0;
}

/*
 * Get PIECES and/or NUM_PIECES of a Type 1 glyph.  Sets info->num_pieces
 * and/or stores into info->pieces.  Updates info->members.  This is a
 * single-use procedure broken out only for readability.
 */
private int
gs_type1_glyph_pieces(gs_font_type1 *pfont, const gs_glyph_data_t *pgd,
		      int members, gs_glyph_info_t *info)
{
    gs_char chars[2];
    gs_glyph glyphs[2];
    int code = gs_type1_piece_codes(pfont, pgd, chars);
    gs_type1_data *const pdata = &pfont->data;
    gs_glyph *pieces =
	(members & GLYPH_INFO_PIECES ? info->pieces : glyphs);
    gs_const_string gstr;
    int acode, bcode;

    info->num_pieces = 0;	/* default */
    if (code <= 0)		/* no seac, possibly error */
	return code;
    bcode = pdata->procs.seac_data(pfont, chars[0], &pieces[0], &gstr, NULL);
    acode = pdata->procs.seac_data(pfont, chars[1], &pieces[1], &gstr, NULL);
    code = (bcode < 0 ? bcode : acode);
    info->num_pieces = 2;
    return code;
}

int
gs_type1_glyph_info(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
		    int members, gs_glyph_info_t *info)
{
    gs_font_type1 *const pfont = (gs_font_type1 *)font;
    gs_type1_data *const pdata = &pfont->data;
    int wmode = ((members & GLYPH_INFO_WIDTH1) != 0);
    int piece_members = members & (GLYPH_INFO_NUM_PIECES | GLYPH_INFO_PIECES);
    int width_members = (members & ((GLYPH_INFO_WIDTH0 << wmode) | (GLYPH_INFO_VVECTOR0 << wmode)));
    int default_members = members & ~(piece_members | GLYPH_INFO_WIDTHS |
				     GLYPH_INFO_VVECTOR0 | GLYPH_INFO_VVECTOR1 |
				     GLYPH_INFO_OUTLINE_WIDTHS);
    int code = 0;
    gs_glyph_data_t gdata;

    if (default_members) {
	code = gs_default_glyph_info(font, glyph, pmat, default_members, info);

	if (code < 0)
	    return code;
    } else
	info->members = 0;

    if (default_members == members)
	return code;		/* all done */

    gdata.memory = pfont->memory;
    if ((code = pdata->procs.glyph_data(pfont, glyph, &gdata)) < 0)
	return code;		/* non-existent glyph */

    if (piece_members) {
	code = gs_type1_glyph_pieces(pfont, &gdata, members, info);
	if (code < 0)
	    return code;
	info->members |= piece_members;
    }

    if (width_members) {
	/*
	 * Interpret the CharString until we get to the [h]sbw.
	 */
	gs_imager_state gis;
	gs_type1_state cis;
	int value;

	/* Initialize just enough of the imager state. */
	if (pmat)
	    gs_matrix_fixed_from_matrix(&gis.ctm, pmat);
	else {
	    gs_matrix imat;

	    gs_make_identity(&imat);
	    gs_matrix_fixed_from_matrix(&gis.ctm, &imat);
	}
	gis.flatness = 0;
	code = gs_type1_interp_init(&cis, &gis, NULL /* no path needed */,
				    NULL, NULL, true, 0, pfont);
	if (code < 0)
	    return code;
	cis.no_grid_fitting = true;
	code = pdata->interpret(&cis, &gdata, &value);
	switch (code) {
	case 0:		/* done with no [h]sbw, error */
	    code = gs_note_error(gs_error_invalidfont);
	default:		/* code < 0, error */
	    return code;
	case type1_result_callothersubr:	/* unknown OtherSubr */
	    return_error(gs_error_rangecheck); /* can't handle it */
	case type1_result_sbw:
	    info->width[wmode].x = fixed2float(cis.width.x);
	    info->width[wmode].y = fixed2float(cis.width.y);
	    info->v.x = fixed2float(cis.lsb.x);
	    info->v.y = fixed2float(cis.lsb.y);
	    break;
	}
	info->members |= width_members | (GLYPH_INFO_VVECTOR0 << wmode);
    }

    gs_glyph_data_free(&gdata, "gs_type1_glyph_info");
    return code;
}

/* Get font parent (a Type 9 font). */
const gs_font_base *
gs_font_parent(const gs_font_base *pbfont)
{
    if (pbfont->FontType == ft_encrypted || pbfont->FontType == ft_encrypted2) {
	const gs_font_type1 *pfont1 = (const gs_font_type1 *)pbfont;

	if (pfont1->data.parent != NULL)
	    return pfont1->data.parent;
    }
    return pbfont;
}
