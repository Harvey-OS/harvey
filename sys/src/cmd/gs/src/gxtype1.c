/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gxtype1.c,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Adobe Type 1 font interpreter support */
#include "math_.h"
#include "gx.h"
#include "gserrors.h"
#include "gsccode.h"
#include "gsline.h"
#include "gsstruct.h"
#include "gxarith.h"
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
    if (index < pcis->ips_count + 4) {
	ENUM_RETURN_CONST_STRING_PTR(gs_type1_state,
				     ipstack[index - 4].char_string);
    }
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
	ip_state *ipsp = &pcis->ipstack[i];
	int diff = ipsp->ip - ipsp->char_string.data;

	RELOC_CONST_STRING_VAR(ipsp->char_string);
	ipsp->ip = ipsp->char_string.data + diff;
    }
} RELOC_PTRS_END

/* ------ Interpreter services ------ */

#define s (*ps)

/* We export this for the Type 2 charstring interpreter. */
void
accum_xy_proc(register is_ptr ps, fixed dx, fixed dy)
{
    ptx += c_fixed(dx, xx),
	pty += c_fixed(dy, yy);
    if (sfc.skewed)
	ptx += c_fixed(dy, yx),
	    pty += c_fixed(dx, xy);
}

/* Initialize a Type 1 interpreter. */
/* The caller must supply a string to the first call of gs_type1_interpret. */
int
gs_type1_interp_init(register gs_type1_state * pcis, gs_imager_state * pis,
    gx_path * ppath, const gs_log2_scale_point * pscale, bool charpath_flag,
		     int paint_type, gs_font_type1 * pfont)
{
    static const gs_log2_scale_point no_scale = {0, 0};
    const gs_log2_scale_point *plog2_scale =
	(FORCE_HINTS_TO_BIG_PIXELS ? pscale : &no_scale);

    pcis->pfont = pfont;
    pcis->pis = pis;
    pcis->path = ppath;
    pcis->callback_data = pfont; /* default callback data */
    /*
     * charpath_flag controls coordinate rounding, hinting, and
     * flatness enhancement.  If we allow it to be set to true,
     * charpath may produce results quite different from show.
     */
    pcis->charpath_flag = false /*charpath_flag */ ;
    pcis->paint_type = paint_type;
    pcis->os_count = 0;
    pcis->ips_count = 1;
    pcis->ipstack[0].ip = 0;
    pcis->ipstack[0].char_string.data = 0;
    pcis->ipstack[0].char_string.size = 0;
    pcis->ignore_pops = 0;
    pcis->init_done = -1;
    pcis->sb_set = false;
    pcis->width_set = false;
    pcis->have_hintmask = false;
    pcis->num_hints = 0;
    pcis->seac_accent = -1;

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
gs_type1_finish_init(gs_type1_state * pcis, gs_op1_state * ps)
{
    gs_imager_state *pis = pcis->pis;

    /* Set up the fixed version of the transformation. */
    gx_matrix_to_fixed_coeff(&ctm_only(pis), &pcis->fc, max_coeff_bits);
    sfc = pcis->fc;

    /* Set the current point of the path to the origin, */
    /* in anticipation of the initial [h]sbw. */
    {
	gx_path *ppath = pcis->path;

	ptx = pcis->origin.x = ppath->position.x;
	pty = pcis->origin.y = ppath->position.y;
    }

    /* Initialize hint-related scalars. */
    pcis->asb_diff = pcis->adxy.x = pcis->adxy.y = 0;
    pcis->flex_count = flex_max;	/* not in Flex */
    pcis->dotsection_flag = dotsection_out;
    pcis->vstem3_set = false;
    pcis->vs_offset.x = pcis->vs_offset.y = 0;
    pcis->hints_initial = 0;	/* probably not needed */
    pcis->hint_next = 0;
    pcis->hints_pending = 0;

    /* Assimilate the hints proper. */
    {
	gs_log2_scale_point log2_scale;

	log2_scale.x = pcis->scale.x.log2_unit;
	log2_scale.y = pcis->scale.y.log2_unit;
	if (pcis->charpath_flag)
	    reset_font_hints(&pcis->fh, &log2_scale);
	else
	    compute_font_hints(&pcis->fh, &pis->ctm, &log2_scale,
			       &pcis->pfont->data);
    }
    reset_stem_hints(pcis);

    /*
     * Set the flatness to a value that is likely to produce reasonably
     * good-looking curves, regardless of its current value in the
     * graphics state.  If the character is very small, set the flatness
     * to zero, which will produce very accurate curves.
     */
    {
	float cxx = fabs(pis->ctm.xx), cyy = fabs(pis->ctm.yy);

	if (is_fzero(cxx) || (cyy < cxx && !is_fzero(cyy)))
	    cxx = cyy;
	if (!is_xxyy(&pis->ctm)) {
	    float cxy = fabs(pis->ctm.xy), cyx = fabs(pis->ctm.yx);

	    if (is_fzero(cxx) || (cxy < cxx && !is_fzero(cxy)))
		cxx = cxy;
	    if (is_fzero(cxx) || (cyx < cxx && !is_fzero(cyx)))
		cxx = cyx;
	}
	/* Don't let the flatness be worse than the default. */
	if (cxx > pis->flatness)
	    cxx = pis->flatness;
	/* If the character is tiny, force accurate curves. */
	if (cxx < 0.2)
	    cxx = 0;
	pcis->flatness = cxx;
    }

    /* Move to the side bearing point. */
    accum_xy(pcis->lsb.x, pcis->lsb.y);
    pcis->position.x = ptx;
    pcis->position.y = pty;

    pcis->init_done = 1;
}

/* ------ Operator procedures ------ */

/* We put these before the interpreter to save having to write */
/* prototypes for all of them. */

int
gs_op1_closepath(register is_ptr ps)
{				/* Note that this does NOT reset the current point! */
    gx_path *ppath = sppath;
    subpath *psub;
    segment *pseg;
    fixed dx, dy;
    int code;

    /* Check for and suppress a microscopic closing line. */
    if ((psub = ppath->current_subpath) != 0 &&
	(pseg = psub->last) != 0 &&
	(dx = pseg->pt.x - psub->pt.x,
	 any_abs(dx) < float2fixed(0.1)) &&
	(dy = pseg->pt.y - psub->pt.y,
	 any_abs(dy) < float2fixed(0.1))
	)
	switch (pseg->type) {
	    case s_line:
		code = gx_path_pop_close_subpath(sppath);
		break;
	    case s_curve:
		/*
		 * Unfortunately, there is no "s_curve_close".  (Maybe there
		 * should be?)  Just adjust the final point of the curve so it
		 * is identical to the closing point.
		 */
		pseg->pt = psub->pt;
#define pcseg ((curve_segment *)pseg)
		pcseg->p2.x -= dx;
		pcseg->p2.y -= dy;
#undef pcseg
		/* falls through */
	    default:
		/* What else could it be?? */
		code = gx_path_close_subpath(sppath);
    } else
	code = gx_path_close_subpath(sppath);
    if (code < 0)
	return code;
    return gx_path_add_point(ppath, ptx, pty);	/* put the point where it was */
}

int
gs_op1_rrcurveto(register is_ptr ps, fixed dx1, fixed dy1,
		 fixed dx2, fixed dy2, fixed dx3, fixed dy3)
{
    gs_fixed_point pt1, pt2;
    fixed ax0 = sppath->position.x - ptx;
    fixed ay0 = sppath->position.y - pty;

    accum_xy(dx1, dy1);
    pt1.x = ptx + ax0, pt1.y = pty + ay0;
    accum_xy(dx2, dy2);
    pt2.x = ptx, pt2.y = pty;
    accum_xy(dx3, dy3);
    return gx_path_add_curve(sppath, pt1.x, pt1.y, pt2.x, pt2.y, ptx, pty);
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

/*
 * Handle a seac.  Do the base character now; when it finishes (detected
 * in endchar), do the accent.  Note that we pass only 4 operands on the
 * stack, and pass asb separately.
 */
int
gs_type1_seac(gs_type1_state * pcis, const fixed * cstack, fixed asb,
	      ip_state * ipsp)
{
    gs_font_type1 *pfont = pcis->pfont;
    gs_const_string bcstr;
    int code;

    /* Save away all the operands. */
    pcis->seac_accent = fixed2int_var(cstack[3]);
    pcis->save_asb = asb;
    pcis->save_lsb = pcis->lsb;
    pcis->save_adxy.x = cstack[0];
    pcis->save_adxy.y = cstack[1];
    pcis->os_count = 0;		/* clear */
    /* Ask the caller to provide the base character's CharString. */
    code = (*pfont->data.procs->seac_data)
	(pfont, fixed2int_var(cstack[2]), NULL, &bcstr);
    if (code != 0)
	return code;
    /* Continue with the supplied string. */
    ipsp->char_string = bcstr;
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
    gx_path *ppath = pcis->path;

    if (pcis->seac_accent >= 0) {	/* We just finished the base character of a seac. */
	/* Do the accent. */
	gs_font_type1 *pfont = pcis->pfont;
	gs_op1_state s;
	gs_const_string astr;
	int achar = pcis->seac_accent;
	int code;

	pcis->seac_accent = -1;
	/* Reset the coordinate system origin */
	sfc = pcis->fc;
	ptx = pcis->origin.x, pty = pcis->origin.y;
	pcis->asb_diff = pcis->save_asb - pcis->save_lsb.x;
	pcis->adxy = pcis->save_adxy;
	/*
	 * We're going to add in the lsb of the accented character
	 * (*not* the lsb of the accent) when we encounter the
	 * [h]sbw of the accent, so ignore the lsb for now.
	 */
	accum_xy(pcis->adxy.x, pcis->adxy.y);
	ppath->position.x = pcis->position.x = ptx;
	ppath->position.y = pcis->position.y = pty;
	pcis->os_count = 0;	/* clear */
	/* Clear the ipstack, in case the base character */
	/* ended inside a subroutine. */
	pcis->ips_count = 1;
	/* Remove any base character hints. */
	reset_stem_hints(pcis);
	/* Ask the caller to provide the accent's CharString. */
	code = (*pfont->data.procs->seac_data)(pfont, achar, NULL, &astr);
	if (code < 0)
	    return code;
	/* Continue with the supplied string. */
	pcis->ips_count = 1;
	pcis->ipstack[0].char_string = astr;
	return 1;
    }
    if (pcis->hint_next != 0 || path_is_drawing(ppath))
	apply_path_hints(pcis, true);
    /* Set the current point to the character origin */
    /* plus the width. */
    {
	gs_fixed_point pt;

	gs_point_transform2fixed(&pis->ctm,
				 fixed2float(pcis->width.x),
				 fixed2float(pcis->width.y),
				 &pt);
	gx_path_add_point(ppath, pt.x, pt.y);
    }
    if (pcis->scale.x.log2_unit + pcis->scale.y.log2_unit == 0) {	/*
									 * Tweak up the fill adjustment.  This is a hack for when
									 * we can't oversample.  The values here are based entirely
									 * on experience, not theory, and are designed primarily
									 * for displays and low-resolution fax.
									 */
	gs_fixed_rect bbox;
	int dx, dy, dmax;

	gx_path_bbox(ppath, &bbox);
	dx = fixed2int_ceiling(bbox.q.x - bbox.p.x);
	dy = fixed2int_ceiling(bbox.q.y - bbox.p.y);
	dmax = max(dx, dy);
	if (pcis->fh.snap_h.count || pcis->fh.snap_v.count ||
	    pcis->fh.a_zone_count
	    ) {			/* We have hints.  Only tweak up a little at */
	    /* very small sizes, to help nearly-vertical */
	    /* or nearly-horizontal diagonals. */
	    pis->fill_adjust.x = pis->fill_adjust.y =
		(dmax < 15 ? float2fixed(0.15) :
		 dmax < 25 ? float2fixed(0.1) :
		 fixed_0);
	} else {		/* No hints.  Tweak a little more to compensate */
	    /* for lack of snapping to pixel grid. */
	    pis->fill_adjust.x = pis->fill_adjust.y =
		(dmax < 10 ? float2fixed(0.2) :
		 dmax < 25 ? float2fixed(0.1) :
		 float2fixed(0.05));
	}
    } else {			/* Don't do any adjusting. */
	pis->fill_adjust.x = pis->fill_adjust.y = fixed_0;
    }
    /* Set the flatness for curve rendering. */
    if (!pcis->charpath_flag)
	gs_imager_setflat(pis, pcis->flatness);
    return 0;
}

/* ------ Font procedures ------ */

int
gs_type1_glyph_info(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
		    int members, gs_glyph_info_t *info)
{
    gs_font_type1 *const pfont = (gs_font_type1 *)font;
    gs_type1_data *const pdata = &pfont->data;
    int wmode = pfont->WMode;
    int piece_members = members & (GLYPH_INFO_NUM_PIECES | GLYPH_INFO_PIECES);
    int width_members = members & (GLYPH_INFO_WIDTH0 << wmode);
    int default_members = members - (piece_members + width_members);
    int code = 0;
    gs_const_string str;

    if (default_members) {
	code = gs_default_glyph_info(font, glyph, pmat, default_members, info);

	if (code < 0)
	    return code;
    } else
	info->members = 0;

    if (default_members != members) {
	if ((code = pdata->procs->glyph_data(pfont, glyph, &str)) < 0)
	    return code;		/* non-existent glyph */
    }

    if (piece_members) {
	gs_glyph *pieces =
	    (members & GLYPH_INFO_PIECES ? info->pieces : (gs_glyph *)0);
	/*
	 * Decode the CharString looking for seac.  We have to process
	 * callsubr, callothersubr, and return operators, but if we see
	 * any other operators other than [h]sbw, pop, or hint operators,
	 * we can return immediately.  This includes all Type 2 operators,
	 * since Type 2 CharStrings don't use seac.
	 *
	 * It's really unfortunate that we have to duplicate so much parsing
	 * code, but factoring out the parser from the interpreter would
	 * involve more restructuring than we're prepared to do right now.
	 */
	bool encrypted = pdata->lenIV >= 0;
	fixed cstack[ostack_size];
	fixed *csp = cstack - 1;
	ip_state ipstack[ipstack_size + 1];
	ip_state *ipsp = &ipstack[0];
	const byte *cip;
	crypt_state state;
	int c;
    
	info->num_pieces = 0;	/* default */
	cip = str.data;
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
		    decode_push_num1(csp, c);
		} else if (c < cx_num4) {	/* 2-byte number */
		    decode_push_num2(csp, c, cip, state, encrypted);
		} else if (c == cx_num4) {	/* 4-byte number */
		    long lw;

		    decode_num4(lw, cip, state, encrypted);
		    *++csp = int2fixed(lw);
		} else		/* not possible */
		    return_error(gs_error_invalidfont);
		continue;
	    }
#define cnext csp = cstack - 1; goto top
	    switch ((char_command) c) {
	    default:
		goto out;
	    case c_callsubr:
		c = fixed2int_var(*csp);
		code = pdata->procs->subr_data
		    (pfont, c, false, &ipsp[1].char_string);
		if (code < 0)
		    return_error(code);
		--csp;
		ipsp->ip = cip, ipsp->dstate = state;
		++ipsp;
		cip = ipsp->char_string.data;
		goto call;
	    case c_return:
		--ipsp;
		cip = ipsp->ip, state = ipsp->dstate;
		goto top;
	    case cx_hstem:
	    case cx_vstem:
	    case c1_hsbw:
		cnext;
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
		    /* This is the payoff for all this code! */
		    if (pieces) {
			gs_char bchar = fixed2int(csp[-1]);
			gs_char achar = fixed2int(csp[0]);
			int bcode =
			    pdata->procs->seac_data(pfont, bchar,
						    &pieces[0], NULL);
			int acode =
			    pdata->procs->seac_data(pfont, achar,
						    &pieces[1], NULL);

			code = (bcode < 0 ? bcode : acode);
		    }
		    info->num_pieces = 2;
		    goto out;
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
out:	info->members |= piece_members;
    }

    if (width_members) {
	/*
	 * Interpret the CharString until we get to the [h]sbw.
	 */
	gs_imager_state gis;
	gs_type1_state cis;
	static const gs_log2_scale_point no_scale = {0, 0};
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
				    &no_scale, true, 0, pfont);
	if (code < 0)
	    return code;
	cis.charpath_flag = true;	/* suppress hinting */
	code = pdata->interpret(&cis, &str, &value);
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
	    break;
	}
	info->members |= width_members;
    }

    return code;
}
