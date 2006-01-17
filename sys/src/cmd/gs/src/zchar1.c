/* Copyright (C) 1993, 2000, 2002 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zchar1.c,v 1.44 2005/04/14 19:10:14 raph Exp $ */
/* Type 1 character display operator */
#include "memory_.h"
#include "ghost.h"
#include "oper.h"
#include "gsstruct.h"
#include "gxfixed.h"
#include "gxmatrix.h"
#include "gxdevice.h"		/* for gxfont.h */
#include "gxfont.h"
#include "gxfont1.h"
#include "gxtype1.h"
#include "gzstate.h"		/* for path for gs_type1_init */
				/* (should only be gsstate.h) */
#include "gscencs.h"
#include "gspaint.h"		/* for gs_fill, gs_stroke */
#include "gspath.h"
#include "gsrect.h"
#include "estack.h"
#include "ialloc.h"
#include "ichar.h"
#include "ichar1.h"
#include "icharout.h"
#include "idict.h"
#include "ifont.h"
#include "igstate.h"
#include "iname.h"
#include "iutil.h"
#include "store.h"

/*
 * Properly designed fonts, which have no self-intersecting outlines
 * and in which outer and inner outlines are drawn in opposite
 * directions, aren't affected by choice of filling rule; but some
 * badly designed fonts in the Genoa test suite seem to require
 * using the even-odd rule to match Adobe interpreters.
 */
#define GS_CHAR_FILL gs_eofill	/* gs_fill or gs_eofill */

/* ============== PATCH BEG=================== */
/*
 * On April 4, 2002, we received bug report #539359
 * which we interpret as some Genoa test are now obsolete,
 * so we need to drop the bad font tolerance feature
 * explained above. This temporary patch changes
 * the even-odd rule back to non-zero rule.
 * This patch to be kept until we accumulate
 * enough information from regression testing and
 * from user responses.
 */

#undef  GS_CHAR_FILL
#define GS_CHAR_FILL gs_fill	/* gs_fill or gs_eofill */
 
/* ============== PATCH END=================== */

/* ---------------- Utilities ---------------- */

/* Test whether a font is a CharString font. */
private bool
font_uses_charstrings(const gs_font *pfont)
{
    return (pfont->FontType == ft_encrypted ||
	    pfont->FontType == ft_encrypted2 ||
	    pfont->FontType == ft_disk_based);
}

/* Initialize a Type 1 interpreter. */
private int
type1_exec_init(gs_type1_state *pcis, gs_text_enum_t *penum,
		gs_state *pgs, gs_font_type1 *pfont1)
{
    /*
     * We have to disregard penum->pis and penum->path, and render to
     * the current gstate and path.  This is a design bug that we will
     * have to address someday!
     */

    int alpha_bits = 1; 
    gs_log2_scale_point log2_subpixels;
    
    if (color_is_pure(pgs->dev_color)) /* Keep consistency with alpha_buffer_bits() */
	alpha_bits = (*dev_proc(pgs->device, get_alpha_bits)) (pgs->device, go_text);
    if (alpha_bits <= 1) {
	/* We render to cache device or the target device has no alpha bits. */
	log2_subpixels = penum->log2_scale;
    } else {
	/* We'll render to target device through alpha buffer. */
	/* Keep consistency with alpha_buffer_init() */
	log2_subpixels.x = log2_subpixels.y = ilog2(alpha_bits); 
    }
    return gs_type1_interp_init(pcis, (gs_imager_state *)pgs, pgs->path,
				&penum->log2_scale, &log2_subpixels,
				(penum->text.operation & TEXT_DO_ANY_CHARPATH) != 0 ||
				penum->device_disabled_grid_fitting,
				pfont1->PaintType, pfont1);
}

/* ---------------- .type1execchar ---------------- */

/*
 * This is the workhorse for %Type1/2BuildChar, %Type1/2BuildGlyph,
 * CCRun, and CID fonts.  Eventually this will appear in the C API;
 * even now, its normal control path doesn't use any continuations.
 */

/*
 * Define the state record for this operator, which must save the metrics
 * separately as well as the Type 1 interpreter state.
 */
typedef struct gs_type1exec_state_s {
    gs_type1_state cis;		/* must be first */
    i_ctx_t *i_ctx_p;		/* so push/pop can access o-stack */
    double sbw[4];
    int /*metrics_present */ present;
    gs_rect char_bbox;
    bool use_FontBBox_as_Metrics2;
    /*
     * The following elements are only used locally to make the stack clean
     * for OtherSubrs: they don't need to be declared for the garbage
     * collector.
     */
    ref save_args[6];
    int num_args;
    bool AlignToPixels;
} gs_type1exec_state;

gs_private_st_suffix_add1(st_gs_type1exec_state, gs_type1exec_state,
			  "gs_type1exec_state", gs_type1exec_state_enum_ptrs,
			  gs_type1exec_state_reloc_ptrs, st_gs_type1_state,
			  i_ctx_p);

/* Forward references */
private int bbox_continue(i_ctx_t *);
private int nobbox_continue(i_ctx_t *);
private int type1_push_OtherSubr(i_ctx_t *, const gs_type1exec_state *,
				 int (*)(i_ctx_t *), const ref *);
private int type1_call_OtherSubr(i_ctx_t *, const gs_type1exec_state *,
				 int (*)(i_ctx_t *), const ref *);
private int type1_callout_dispatch(i_ctx_t *, int (*)(i_ctx_t *), int);
private int type1_continue_dispatch(i_ctx_t *, gs_type1exec_state *,
				    const ref *, ref *, int);
private int op_type1_cleanup(i_ctx_t *);
private void op_type1_free(i_ctx_t *);
private int bbox_getsbw_continue(i_ctx_t *);
private int type1exec_bbox(i_ctx_t *, gs_type1exec_state *, gs_font *, op_proc_t *exec_cont);
private int bbox_finish_fill(i_ctx_t *);
private int bbox_finish_stroke(i_ctx_t *);
private int bbox_fill(i_ctx_t *);
private int bbox_stroke(i_ctx_t *);
private int nobbox_finish(i_ctx_t *, gs_type1exec_state *);
private int nobbox_draw(i_ctx_t *, int (*)(gs_state *));
private int nobbox_fill(i_ctx_t *);
private int nobbox_stroke(i_ctx_t *);

/* <font> <code|name> <name> <charstring> .type1execchar - */
private int
ztype1execchar(i_ctx_t *i_ctx_p)
{
    return charstring_execchar(i_ctx_p, (1 << (int)ft_encrypted) |
			       (1 << (int)ft_disk_based));
}
int
charstring_execchar(i_ctx_t *i_ctx_p, int font_type_mask)
{
    os_ptr op = osp;
    gs_font *pfont;
    int code = font_param(op - 3, &pfont);
    gs_font_base *const pbfont = (gs_font_base *) pfont;
    gs_font_type1 *const pfont1 = (gs_font_type1 *) pfont;
    const gs_type1_data *pdata;
    gs_text_enum_t *penum = op_show_find(i_ctx_p);
    gs_type1exec_state cxs;
    gs_type1_state *const pcis = &cxs.cis;

    if (code < 0)
	return code;
    if (penum == 0 ||
	pfont->FontType >= sizeof(font_type_mask) * 8 ||
	!(font_type_mask & (1 << (int)pfont->FontType)))
	return_error(e_undefined);
    pdata = &pfont1->data;
    /*
     * Any reasonable implementation would execute something like
     *    1 setmiterlimit 0 setlinejoin 0 setlinecap
     * here, but the Adobe implementations don't.
     *
     * If this is a stroked font, set the stroke width.
     */
    if (pfont->PaintType)
	gs_setlinewidth(igs, pfont->StrokeWidth);
    check_estack(3);		/* for continuations */
    /*
     * Execute the definition of the character.
     */
    if (r_is_proc(op))
	return zchar_exec_char_proc(i_ctx_p);
    /*
     * The definition must be a Type 1 CharString.
     * Note that we do not require read access: this is deliberate.
     */
    check_type(*op, t_string);
    if (r_size(op) <= max(pdata->lenIV, 0))
	return_error(e_invalidfont);
    /*
     * In order to make character oversampling work, we must
     * set up the cache before calling .type1addpath.
     * To do this, we must get the bounding box from the FontBBox,
     * and the width from the CharString or the Metrics.
     * If the FontBBox isn't valid, we can't do any of this.
     */

    if ((penum->FontBBox_as_Metrics2.x == 0 &&
	 penum->FontBBox_as_Metrics2.y == 0) ||
	gs_rootfont(igs)->WMode == 0 ) {
	code = zchar_get_metrics(pbfont, op - 1, cxs.sbw);
	if (code < 0)
	    return code;
	cxs.present = code;
	cxs.use_FontBBox_as_Metrics2 = false;
    }  else {  /* pass here if FontType==9,11 && WMode==1*/
	cxs.sbw[0] = penum->FontBBox_as_Metrics2.x / 2;
	cxs.sbw[1] = penum->FontBBox_as_Metrics2.y;
	cxs.sbw[2] = 0;
	cxs.sbw[3] = -penum->FontBBox_as_Metrics2.x; /* Sic! */
	cxs.use_FontBBox_as_Metrics2 = true;
    }
    /* Establish a current point. */
    code = gs_moveto(igs, 0.0, 0.0);
    if (code < 0)
	return code;
    code = type1_exec_init(pcis, penum, igs, pfont1);
    if (code < 0)
	return code;
    gs_type1_set_callback_data(pcis, &cxs);
    if (pfont1->FontBBox.q.x > pfont1->FontBBox.p.x &&
	pfont1->FontBBox.q.y > pfont1->FontBBox.p.y
	) {
	/* The FontBBox appears to be valid. */
	op_proc_t exec_cont = 0;

	cxs.char_bbox = pfont1->FontBBox;
	code = type1exec_bbox(i_ctx_p, &cxs, pfont, &exec_cont);
	if (code >= 0 && exec_cont != 0)
	    code = (*exec_cont)(i_ctx_p);
	return code;
    } else {
	/*
	 * The FontBBox is not valid.  In this case,
	 * we create the path first, then do the setcachedevice.
	 * If we are oversampling (in this case, only for anti-
	 * aliasing, not just to improve quality), we have to
	 * create the path twice, since we can't know the
	 * oversampling factor until after setcachedevice.
	 */
	const ref *opstr = op;
	ref other_subr;

	if (cxs.present == metricsSideBearingAndWidth) {
	    gs_point sbpt;

	    sbpt.x = cxs.sbw[0], sbpt.y = cxs.sbw[1];
	    gs_type1_set_lsb(pcis, &sbpt);
	}
	/* Continue interpreting. */
      icont:
	code = type1_continue_dispatch(i_ctx_p, &cxs, opstr, &other_subr, 4);
	op = osp;		/* OtherSubrs might change it */
	switch (code) {
	    case 0:		/* all done */
		return nobbox_finish(i_ctx_p, &cxs);
	    default:		/* code < 0, error */
		return code;
	    case type1_result_callothersubr:	/* unknown OtherSubr */
		return type1_call_OtherSubr(i_ctx_p, &cxs, nobbox_continue,
					    &other_subr);
	    case type1_result_sbw:	/* [h]sbw, just continue */
		if (cxs.present != metricsSideBearingAndWidth)
		    type1_cis_get_metrics(pcis, cxs.sbw);
		opstr = 0;
		goto icont;
	}
    }
}

/* -------- bbox case -------- */

/* Do all the work for the case where we have a bounding box. */
/* Returns exec_cont - a function, which must be called by caller after this function. */
private int
type1exec_bbox(i_ctx_t *i_ctx_p, gs_type1exec_state * pcxs,
	       gs_font * pfont, op_proc_t *exec_cont)
{
    os_ptr op = osp;
    gs_type1_state *const pcis = &pcxs->cis;
    gs_font_base *const pbfont = (gs_font_base *) pfont;
    op_proc_t cont = (pbfont->PaintType == 0 ? bbox_finish_fill : bbox_finish_stroke);


    /*
     * We appear to have a valid bounding box.  If we don't have Metrics for
     * this character, start interpreting the CharString; do the
     * setcachedevice as soon as we know the (side bearing and) width.
     */
    if (pcxs->present == metricsNone) {
	/* Get the width from the CharString, */
	/* then set the cache device. */
	ref cnref;
	ref other_subr;
	int code;

	/* Since an OtherSubr callout might change osp, */
	/* save the character name now. */
	ref_assign(&cnref, op - 1);
	code = type1_continue_dispatch(i_ctx_p, pcxs, op, &other_subr, 4);
	op = osp;		/* OtherSubrs might change it */
	switch (code) {
	    default:		/* code < 0 or done, error */
		return ((code < 0 ? code :
			 gs_note_error(e_invalidfont)));
	    case type1_result_callothersubr:	/* unknown OtherSubr */
		return type1_call_OtherSubr(i_ctx_p, pcxs,
					    bbox_getsbw_continue,
					    &other_subr);
	    case type1_result_sbw:	/* [h]sbw, done */
		break;
	}
	type1_cis_get_metrics(pcis, pcxs->sbw);
	return zchar_set_cache(i_ctx_p, pbfont, &cnref,
			       NULL, pcxs->sbw + 2,
			       &pcxs->char_bbox,
			       cont, exec_cont, NULL);
    } else {
	/* We have the width and bounding box: */
	/* set up the cache device now. */
 	return zchar_set_cache(i_ctx_p, pbfont, op - 1,
			       (pcxs->present == metricsSideBearingAndWidth
			        && !pcxs->use_FontBBox_as_Metrics2 ?
			        pcxs->sbw : NULL),
			       pcxs->sbw + 2,
			       &pcxs->char_bbox,
			       cont, exec_cont, 
			       (pcxs->use_FontBBox_as_Metrics2 ? pcxs->sbw : NULL));
    }
}

/* Continue from an OtherSubr callout while getting metrics. */
private int
bbox_getsbw_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref other_subr;
    gs_type1exec_state *pcxs = r_ptr(esp, gs_type1exec_state);
    gs_type1_state *const pcis = &pcxs->cis;
    int code;

    code = type1_continue_dispatch(i_ctx_p, pcxs, NULL, &other_subr, 4);
    op = osp;			/* in case z1_push/pop_proc was called */
    switch (code) {
	default:		/* code < 0 or done, error */
	    op_type1_free(i_ctx_p);
	    return ((code < 0 ? code : gs_note_error(e_invalidfont)));
	case type1_result_callothersubr:	/* unknown OtherSubr */
	    return type1_push_OtherSubr(i_ctx_p, pcxs, bbox_getsbw_continue,
					&other_subr);
	case type1_result_sbw: {	/* [h]sbw, done */
	    double sbw[4];
	    const gs_font_base *const pbfont =
		(const gs_font_base *)pcis->pfont;
	    gs_rect bbox;
	    op_proc_t cont = (pbfont->PaintType == 0 ? bbox_finish_fill : bbox_finish_stroke), exec_cont = 0;

	    /* Get the metrics before freeing the state. */
	    type1_cis_get_metrics(pcis, sbw);
	    bbox = pcxs->char_bbox;
	    op_type1_free(i_ctx_p);
	    code = zchar_set_cache(i_ctx_p, pbfont, op - 1, sbw, sbw + 2, &bbox,
				   cont, &exec_cont, NULL);
	    if (code >= 0 && exec_cont != 0)
		code = (*exec_cont)(i_ctx_p);
	    return code;
	}
    }
}

/* <font> <code|name> <name> <charstring> <sbx> <sby> %bbox_{fill|stroke} - */
/* <font> <code|name> <name> <charstring> %bbox_{fill|stroke} - */
private int bbox_finish(i_ctx_t *i_ctx_p, op_proc_t cont, op_proc_t *exec_cont);
private int
bbox_finish_fill(i_ctx_t *i_ctx_p)
{
    op_proc_t exec_cont = 0;
    int code;

    code = bbox_finish(i_ctx_p, bbox_fill, &exec_cont);
    if (code >= 0 && exec_cont != 0)
	code = exec_cont(i_ctx_p);
    return code;
}
private int
bbox_finish_stroke(i_ctx_t *i_ctx_p)
{
    op_proc_t exec_cont = 0;
    int code;

    code = bbox_finish(i_ctx_p, bbox_stroke, &exec_cont);
    if (code >= 0 && exec_cont != 0)
	code = exec_cont(i_ctx_p);
    return code;
}

private int
bbox_finish(i_ctx_t *i_ctx_p, op_proc_t cont, op_proc_t *exec_cont)
{   /* Returns exec_cont - a function, which must be called by caller after this function. */
    os_ptr op = osp;
    gs_font *pfont;
    int code;
    gs_text_enum_t *penum = op_show_find(i_ctx_p);
    gs_type1exec_state cxs;	/* stack allocate to avoid sandbars */
    gs_type1_state *const pcis = &cxs.cis;
    double sbxy[2];
    gs_point sbpt;
    gs_point *psbpt = 0;
    os_ptr opc = op;
    const ref *opstr;
    ref other_subr;

    if (!r_has_type(opc, t_string)) {
	check_op(3);
	code = num_params(op, 2, sbxy);
	if (code < 0)
	    return code;
	sbpt.x = sbxy[0];
	sbpt.y = sbxy[1];
	psbpt = &sbpt;
	opc -= 2;
	check_type(*opc, t_string);
    }
    code = font_param(opc - 3, &pfont);
    if (code < 0)
	return code;
    if (penum == 0 || !font_uses_charstrings(pfont))
	return_error(e_undefined);
    {
	gs_font_type1 *const pfont1 = (gs_font_type1 *) pfont;
	int lenIV = pfont1->data.lenIV;

	if (lenIV > 0 && r_size(opc) <= lenIV)
	    return_error(e_invalidfont);
	check_estack(5);	/* in case we need to do a callout */
	code = type1_exec_init(pcis, penum, igs, pfont1);
	if (code < 0)
	    return code;
	if (psbpt)
	    gs_type1_set_lsb(pcis, psbpt);
    }
    opstr = opc;
  icont:
    code = type1_continue_dispatch(i_ctx_p, &cxs, opstr, &other_subr,
				   (psbpt ? 6 : 4));
    op = osp;		/* OtherSubrs might have altered it */
    switch (code) {
	case 0:		/* all done */
	    /* Call the continuation now. */
	    if (psbpt)
		pop(2);
	    *exec_cont = cont;
	    return 0;
	case type1_result_callothersubr:	/* unknown OtherSubr */
	    push_op_estack(cont);	/* call later */
	    return type1_call_OtherSubr(i_ctx_p, &cxs, bbox_continue,
					&other_subr);
	case type1_result_sbw:	/* [h]sbw, just continue */
	    opstr = 0;
	    goto icont;
	default:		/* code < 0, error */
	    return code;
    }
}

private int
bbox_continue(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int npop = (r_has_type(op, t_string) ? 4 : 6);
    int code = type1_callout_dispatch(i_ctx_p, bbox_continue, npop);

    if (code == 0) {
	op = osp;		/* OtherSubrs might have altered it */
	npop -= 4;		/* nobbox_fill/stroke handles the rest */
	pop(npop);
	op -= npop;
	op_type1_free(i_ctx_p);
    }
    return code;
}

/*
 * Check the path against FontBBox before drawing.  The original operands
 * of type1execchar are still on the o-stack.
 * Returns exec_cont - a function, which must be called by caller after this function.
 */
private int
bbox_draw(i_ctx_t *i_ctx_p, int (*draw)(gs_state *), op_proc_t *exec_cont)
{
    os_ptr op = osp;
    gs_rect bbox;
    gs_font *pfont;
    gs_text_enum_t *penum;
    gs_font_base * pbfont;
    gs_font_type1 * pfont1;
    gs_type1exec_state cxs;
    int code;

    if (igs->in_cachedevice < 2)	/* not caching */
	return nobbox_draw(i_ctx_p, draw);
    if ((code = font_param(op - 3, &pfont)) < 0)
	return code;
    penum = op_show_find(i_ctx_p);
    if (penum == 0 || !font_uses_charstrings(pfont))
	return_error(e_undefined);
    if ((code = gs_pathbbox(igs, &bbox)) < 0) {
	/*
	 * If the matrix is singular, all user coordinates map onto a
	 * straight line.  Don't bother rendering the character at all.
	 */
	if (code == e_undefinedresult) {
	    pop(4);
	    gs_newpath(igs);
	    return 0;
	}
	return code;
    }
    if (draw == gs_stroke) {
	/* Expand the bounding box by the line width. */
	float width = gs_currentlinewidth(igs) * 1.41422;

	bbox.p.x -= width, bbox.p.y -= width;
	bbox.q.x += width, bbox.q.y += width;
    }
    pbfont = (gs_font_base *)pfont;
    if (rect_within(bbox, pbfont->FontBBox))	/* within bounds */
	return nobbox_draw(i_ctx_p, draw);
    /* Enlarge the FontBBox to save work in the future. */
    rect_merge(pbfont->FontBBox, bbox);
    /* Dismantle everything we've done, and start over. */
    gs_text_retry(penum);
    pfont1 = (gs_font_type1 *) pfont;
    if ((penum->FontBBox_as_Metrics2.x == 0 &&
	 penum->FontBBox_as_Metrics2.y == 0) ||
	gs_rootfont(igs)->WMode == 0 ) {
	code = zchar_get_metrics(pbfont, op - 1, cxs.sbw);
	if (code < 0)
	    return code;
	cxs.present = code;
	cxs.use_FontBBox_as_Metrics2 = false;
    }  else {
	cxs.sbw[0] = penum->FontBBox_as_Metrics2.x / 2;
	cxs.sbw[1] = penum->FontBBox_as_Metrics2.y;
	cxs.sbw[2] = 0;
	cxs.sbw[3] = -penum->FontBBox_as_Metrics2.x; /* Sic! */
	cxs.use_FontBBox_as_Metrics2 = true;
	cxs.present = metricsSideBearingAndWidth;
    }
    code = type1_exec_init(&cxs.cis, penum, igs, pfont1);
    if (code < 0)
	return code;
    cxs.char_bbox = pfont1->FontBBox;
    code = type1exec_bbox(i_ctx_p, &cxs, pfont, exec_cont);
    return code;
}
private int
bbox_fill(i_ctx_t *i_ctx_p)
{
    op_proc_t exec_cont = 0;
    int code;

    /* See above re GS_CHAR_FILL. */
    code = bbox_draw(i_ctx_p, GS_CHAR_FILL, &exec_cont);
    if (code >= 0 && exec_cont != 0)
	code = (*exec_cont)(i_ctx_p);
    return code;
}
private int
bbox_stroke(i_ctx_t *i_ctx_p)
{
    op_proc_t exec_cont = 0;
    int code;

    code = bbox_draw(i_ctx_p, gs_stroke, &exec_cont);
    if (code >= 0 && exec_cont != 0)
	code = (*exec_cont)(i_ctx_p);
    return code;
}

/* -------- Common code -------- */

/* Handle the results of interpreting the CharString. */
/* pcref points to a t_string ref. */
private int
type1_continue_dispatch(i_ctx_t *i_ctx_p, gs_type1exec_state *pcxs,
			const ref * pcref, ref *pos, int num_args)
{
    int value;
    int code;
    gs_glyph_data_t cs_data;
    gs_glyph_data_t *pcsd;

    cs_data.memory = imemory;
    if (pcref == 0) {
	pcsd = 0;
    } else {
	gs_glyph_data_from_string(&cs_data, pcref->value.const_bytes,
				  r_size(pcref), NULL);
	pcsd = &cs_data;
    }
    /*
     * Since OtherSubrs may push or pop values on the PostScript operand
     * stack, remove the arguments of .type1execchar before calling the
     * Type 1 interpreter, and put them back afterwards unless we're
     * about to execute an OtherSubr procedure.  Also, we must set up
     * the callback data for pushing OtherSubrs arguments.
     */
    pcxs->i_ctx_p = i_ctx_p;
    pcxs->num_args = num_args;
    memcpy(pcxs->save_args, osp - (num_args - 1), num_args * sizeof(ref));
    osp -= num_args;
    gs_type1_set_callback_data(&pcxs->cis, pcxs);
    code = pcxs->cis.pfont->data.interpret(&pcxs->cis, pcsd, &value);
    switch (code) {
	case type1_result_callothersubr: {
	    /*
	     * The Type 1 interpreter handles all known OtherSubrs,
	     * so this must be an unknown one.
	     */
	    const font_data *pfdata = pfont_data(gs_currentfont(igs));

	    code = array_get(imemory, &pfdata->u.type1.OtherSubrs, (long)value, pos);
	    if (code >= 0)
		return type1_result_callothersubr;
	}
    }
    /* Put back the arguments removed above. */
    memcpy(osp + 1, pcxs->save_args, num_args * sizeof(ref));
    osp += num_args;
    return code;
}

/*
 * Push a continuation, the arguments removed for the OtherSubr, and
 * the OtherSubr procedure.
 */
private int
type1_push_OtherSubr(i_ctx_t *i_ctx_p, const gs_type1exec_state *pcxs,
		     int (*cont)(i_ctx_t *), const ref *pos)
{
    int i, n = pcxs->num_args;

    push_op_estack(cont);
    /*
     * Push the saved arguments (in reverse order, so they will get put
     * back on the operand stack in the correct order) on the e-stack.
     */
    for (i = n; --i >= 0; ) {
	*++esp = pcxs->save_args[i];
	r_clear_attrs(esp, a_executable);  /* just in case */
    }
    ++esp;
    *esp = *pos;
    return o_push_estack;
}

/*
 * Do a callout to an OtherSubr implemented in PostScript.
 * The caller must have done a check_estack(4 + num_args).
 */
private int
type1_call_OtherSubr(i_ctx_t *i_ctx_p, const gs_type1exec_state * pcxs,
		     int (*cont) (i_ctx_t *),
		     const ref * pos)
{
    /* Move the Type 1 interpreter state to the heap. */
    gs_type1exec_state *hpcxs =
	ialloc_struct(gs_type1exec_state, &st_gs_type1exec_state,
		      "type1_call_OtherSubr");

    if (hpcxs == 0)
	return_error(e_VMerror);
    *hpcxs = *pcxs;
    gs_type1_set_callback_data(&hpcxs->cis, hpcxs);
    push_mark_estack(es_show, op_type1_cleanup);
    ++esp;
    make_istruct(esp, 0, hpcxs);
    return type1_push_OtherSubr(i_ctx_p, pcxs, cont, pos);
}

/* Continue from an OtherSubr callout while building the path. */
private int
type1_callout_dispatch(i_ctx_t *i_ctx_p, int (*cont)(i_ctx_t *),
		       int num_args)
{
    ref other_subr;
    gs_type1exec_state *pcxs = r_ptr(esp, gs_type1exec_state);
    int code;

  icont:
    code = type1_continue_dispatch(i_ctx_p, pcxs, NULL, &other_subr,
				   num_args);
    switch (code) {
	case 0:		/* callout done, cont is on e-stack */
	    return 0;
	default:		/* code < 0 or done, error */
	    op_type1_free(i_ctx_p);
	    return ((code < 0 ? code : gs_note_error(e_invalidfont)));
	case type1_result_callothersubr:	/* unknown OtherSubr */
	    return type1_push_OtherSubr(i_ctx_p, pcxs, cont, &other_subr);
	case type1_result_sbw:	/* [h]sbw, just continue */
	    goto icont;
    }
}

/* Clean up after a Type 1 callout. */
private int
op_type1_cleanup(i_ctx_t *i_ctx_p)
{
    ifree_object(r_ptr(esp + 2, void), "op_type1_cleanup");
    return 0;
}
private void
op_type1_free(i_ctx_t *i_ctx_p)
{
    ifree_object(r_ptr(esp, void), "op_type1_free");
    /*
     * In order to avoid popping from the e-stack and then pushing onto
     * it, which would violate an interpreter invariant, we simply
     * overwrite the two e-stack items being discarded (hpcxs and the
     * cleanup operator) with empty procedures.
     */
    make_empty_const_array(esp - 1, a_readonly + a_executable);
    make_empty_const_array(esp, a_readonly + a_executable);
}

/* -------- no-bbox case -------- */

private int
nobbox_continue(i_ctx_t *i_ctx_p)
{
    int code = type1_callout_dispatch(i_ctx_p, nobbox_continue, 4);

    if (code)
	return code;
    {
	gs_type1exec_state *pcxs = r_ptr(esp, gs_type1exec_state);
	gs_type1exec_state cxs;

	cxs = *pcxs;
	gs_type1_set_callback_data(&cxs.cis, &cxs);
	op_type1_free(i_ctx_p);
	return nobbox_finish(i_ctx_p, &cxs);
    }
}

/* Finish the no-FontBBox case after constructing the path. */
/* If we are oversampling for anti-aliasing, we have to go around again. */
/* <font> <code|name> <name> <charstring> %nobbox_continue - */
private int
nobbox_finish(i_ctx_t *i_ctx_p, gs_type1exec_state * pcxs)
{
    os_ptr op = osp;
    int code;
    gs_text_enum_t *penum = op_show_find(i_ctx_p);
    gs_font *pfont;

    if ((code = gs_pathbbox(igs, &pcxs->char_bbox)) < 0 ||
	(code = font_param(op - 3, &pfont)) < 0
	)
	return code;
    if (penum == 0 || !font_uses_charstrings(pfont))
	return_error(e_undefined);
    {
	gs_font_base *const pbfont = (gs_font_base *) pfont;
	gs_font_type1 *const pfont1 = (gs_font_type1 *) pfont;
	op_proc_t cont, exec_cont = 0;

	if (pcxs->present == metricsNone) {
	    gs_point endpt;

	    if ((code = gs_currentpoint(igs, &endpt)) < 0)
		return code;
	    pcxs->sbw[2] = endpt.x, pcxs->sbw[3] = endpt.y;
	    pcxs->present = metricsSideBearingAndWidth;
	}
	/*
	 * We only need to rebuild the path from scratch if we might
	 * oversample for anti-aliasing.
	 */
	if ((*dev_proc(igs->device, get_alpha_bits))(igs->device, go_text) > 1
	    ) {
	    gs_newpath(igs);
	    gs_moveto(igs, 0.0, 0.0);
	    code = type1_exec_init(&pcxs->cis, penum, igs, pfont1);
	    if (code < 0)
		return code;
	    code = type1exec_bbox(i_ctx_p, pcxs, pfont, &exec_cont);
	} else {
	    cont = (pbfont->PaintType == 0 ? nobbox_fill : nobbox_stroke), exec_cont = 0;
	    code = zchar_set_cache(i_ctx_p, pbfont, op - 1, NULL,
				   pcxs->sbw + 2,
				   &pcxs->char_bbox,
				   cont, &exec_cont,
				   (pcxs->use_FontBBox_as_Metrics2 ? pcxs->sbw : NULL));
	}
	if (code >= 0 && exec_cont != 0)
	    code = (*exec_cont)(i_ctx_p);
	return code;
    }
}
/* Finish by popping the operands and filling or stroking. */
private int
nobbox_draw(i_ctx_t *i_ctx_p, int (*draw)(gs_state *))
{
    int code = draw(igs);

    if (code >= 0)
	pop(4);
    return code;
}
private int
nobbox_fill(i_ctx_t *i_ctx_p)
{
    /* See above re GS_CHAR_FILL. */
    return nobbox_draw(i_ctx_p, GS_CHAR_FILL);
}
private int
nobbox_stroke(i_ctx_t *i_ctx_p)
{
    /* As a compatibility to Adobe, use the exact "StrokeWidth".
       Reset fill_adjust for that. */
    int code;
    gs_fixed_point fa = i_ctx_p->pgs->fill_adjust;

    i_ctx_p->pgs->fill_adjust.x = i_ctx_p->pgs->fill_adjust.y = 0;
    code = nobbox_draw(i_ctx_p, gs_stroke);
    i_ctx_p->pgs->fill_adjust = fa;
    return code;
}

/* <font> <array> .setweightvector - */
private int
zsetweightvector(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_font *pfont;
    int code = font_param(op - 1, &pfont);
    gs_font_type1 *pfont1;
    int size;

    if (code < 0) {
	/* The font was not defined yet. Just ignore. See lib/gs_type1.ps . */
	pop(2);
	return 0;
    }
    if (pfont->FontType != ft_encrypted && pfont->FontType != ft_encrypted2)
	return_error(e_invalidfont);
    pfont1 = (gs_font_type1 *)pfont;
    size = r_size(op);
    if (size != pfont1->data.WeightVector.count)
	return_error(e_invalidfont);
    code = process_float_array(imemory, op, size, pfont1->data.WeightVector.values);
    if (code < 0)
	return code;
    pop(2);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zchar1_op_defs[] =
{
    {"4.type1execchar", ztype1execchar},
		/* Internal operators */
    {"4%bbox_getsbw_continue", bbox_getsbw_continue},
    {"4%bbox_continue", bbox_continue},
    {"4%bbox_finish_fill", bbox_finish_fill},
    {"4%bbox_finish_stroke", bbox_finish_stroke},
    {"4%nobbox_continue", nobbox_continue},
    {"4%nobbox_fill", nobbox_fill},
    {"4%nobbox_stroke", nobbox_stroke},
    {"4.setweightvector", zsetweightvector},
    op_def_end(0)
};

/* ------ Auxiliary procedures for type 1 fonts ------ */

private int
z1_glyph_data(gs_font_type1 * pfont, gs_glyph glyph, gs_glyph_data_t *pgd)
{
    ref gref;

    glyph_ref(pfont->memory, glyph, &gref);
    return zchar_charstring_data((gs_font *)pfont, &gref, pgd);
}

private int
z1_subr_data(gs_font_type1 * pfont, int index, bool global,
	     gs_glyph_data_t *pgd)
{
    const font_data *pfdata = pfont_data(pfont);
    ref subr;
    int code;

    code = array_get(pfont->memory, (global ? &pfdata->u.type1.GlobalSubrs :
		      &pfdata->u.type1.Subrs),
		     index, &subr);
    if (code < 0)
	return code;
    check_type_only(subr, t_string);
    gs_glyph_data_from_string(pgd, subr.value.const_bytes, r_size(&subr),
			      NULL);
    return 0;
}

private int
z1_seac_data(gs_font_type1 *pfont, int ccode, gs_glyph *pglyph,
	     gs_const_string *gstr, gs_glyph_data_t *pgd)
{
    gs_glyph glyph = gs_c_known_encode((gs_char)ccode,
				       ENCODING_INDEX_STANDARD);
    int code;
    ref rglyph;

    if (glyph == GS_NO_GLYPH)
	return_error(e_rangecheck);
    if ((code = gs_c_glyph_name(glyph, gstr)) < 0 ||
	(code = name_ref(pfont->memory, gstr->data, gstr->size, &rglyph, 0)) < 0
	)
	return code;
    if (pglyph)
	*pglyph = name_index(pfont->memory, &rglyph);
    if (pgd)
	code = zchar_charstring_data((gs_font *)pfont, &rglyph, pgd);
    return code;
}

private int
z1_push(void *callback_data, const fixed * pf, int count)
{
    gs_type1exec_state *pcxs = callback_data;
    i_ctx_t *i_ctx_p = pcxs->i_ctx_p;
    const fixed *p = pf + count - 1;
    int i;

    check_ostack(count);
    for (i = 0; i < count; i++, p--) {
	osp++;
	make_real(osp, fixed2float(*p));
    }
    return 0;
}

private int
z1_pop(void *callback_data, fixed * pf)
{
    gs_type1exec_state *pcxs = callback_data;
    i_ctx_t *i_ctx_p = pcxs->i_ctx_p;
    double val;
    int code = real_param(osp, &val);

    if (code < 0)
	return code;
    *pf = float2fixed(val);
    osp--;
    return 0;
}

/* Define the Type 1 procedure vector. */
const gs_type1_data_procs_t z1_data_procs = {
    z1_glyph_data, z1_subr_data, z1_seac_data, z1_push, z1_pop
};

/* ------ Font procedures for Type 1 fonts ------ */

/*
 * Get a Type 1 or Type 2 glyph outline.  This is the glyph_outline
 * procedure for the font.
 */
int
zchar1_glyph_outline(gs_font *font, int WMode, gs_glyph glyph, const gs_matrix *pmat,
		     gx_path *ppath, double sbw[4])
{
    gs_font_type1 *const pfont1 = (gs_font_type1 *)font;
    ref gref;
    gs_glyph_data_t gdata;
    int code;

    glyph_ref(font->memory, glyph, &gref);
    gdata.memory = font->memory;
    code = zchar_charstring_data(font, &gref, &gdata);
    if (code < 0)
	return code;
    return zcharstring_outline(pfont1, WMode, &gref, &gdata, pmat, ppath, sbw);
}
/*
 * Get a glyph outline given a CharString.  The glyph_outline procedure
 * for CIDFontType 0 fonts uses this.
 */
int
zcharstring_outline(gs_font_type1 *pfont1, int WMode, const ref *pgref,
		    const gs_glyph_data_t *pgd_orig,
		    const gs_matrix *pmat, gx_path *ppath, double sbw[4])
{
    const gs_glyph_data_t *pgd = pgd_orig;
    int code;
    gs_type1exec_state cxs;
    gs_type1_state *const pcis = &cxs.cis;
    const gs_type1_data *pdata;
    int value;
    gs_imager_state gis;
    double wv[4];
    gs_point mpt;

    pdata = &pfont1->data;
    if (pgd->bits.size <= max(pdata->lenIV, 0))
	return_error(e_invalidfont);
#if 0 /* Ignore CDevProc for now. */   
    if (zchar_get_CDevProc((const gs_font_base *)pfont1, &pcdevproc))
	return_error(e_rangecheck); /* can't call CDevProc from here */
#endif
    switch (WMode) {
    default:
	code = zchar_get_metrics2((gs_font_base *)pfont1, pgref, wv);
	sbw[0] = wv[2];
	sbw[1] = wv[3];
	sbw[2] = wv[0];
	sbw[3] = wv[1];
	if (code)
	    break;
	/* falls through */
    case 0:
	code = zchar_get_metrics((gs_font_base *)pfont1, pgref, sbw);
    }
    if (code < 0)
	return code;
    cxs.present = code;
    /* Initialize just enough of the imager state. */
    if (pmat)
	gs_matrix_fixed_from_matrix(&gis.ctm, pmat);
    else {
	gs_matrix imat;

	gs_make_identity(&imat);
	gs_matrix_fixed_from_matrix(&gis.ctm, &imat);
    }
    gis.flatness = 0;
    code = gs_type1_interp_init(&cxs.cis, &gis, ppath, NULL, NULL, true, 0,
				pfont1);
    if (code < 0)
	return code;
    cxs.cis.no_grid_fitting = true;
    gs_type1_set_callback_data(pcis, &cxs);
    switch (cxs.present) {
    case metricsSideBearingAndWidth:
	mpt.x = sbw[0], mpt.y = sbw[1];
	gs_type1_set_lsb(pcis, &mpt);
	/* falls through */
    case metricsWidthOnly:
	mpt.x = sbw[2], mpt.y = sbw[3];
	gs_type1_set_width(pcis, &mpt);
    case metricsNone:
	;
    }
    /* Continue interpreting. */
icont:
    code = pfont1->data.interpret(pcis, pgd, &value);
    switch (code) {
    case 0:		/* all done */
	/* falls through */
    default:		/* code < 0, error */
	return code;
    case type1_result_callothersubr:	/* unknown OtherSubr */
	return_error(e_rangecheck); /* can't handle it */
    case type1_result_sbw:	/* [h]sbw, just continue */
	type1_cis_get_metrics(pcis, cxs.sbw);
	type1_cis_get_metrics(pcis, sbw);
	pgd = 0;
	goto icont;
    }
}

/*
 * Redefine glyph_info to take Metrics[2] and CDevProc into account (unless
 * GLYPH_INFO_OUTLINE_WIDTHS is set).  If CDevProc is present, return
 * e_rangecheck, since we can't call the interpreter from here.
 */
int
z1_glyph_info_generic(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
	      int members, gs_glyph_info_t *info, font_proc_glyph_info((*proc)), int wmode)
{
    ref gref;
    ref *pcdevproc;
    gs_font_base *const pbfont = (gs_font_base *)font;
    int width_members = members & (GLYPH_INFO_WIDTH0 << wmode);
    int outline_widths = members & GLYPH_INFO_OUTLINE_WIDTHS;
    bool modified_widths = false;
    int default_members = members & ~(width_members + outline_widths + 
                                      GLYPH_INFO_VVECTOR0 + GLYPH_INFO_VVECTOR1 + 
				      GLYPH_INFO_CDEVPROC);
    int done_members = 0;
    int code;

    if (!width_members)
	return (*proc)(font, glyph, pmat, members, info);
    if (!outline_widths && zchar_get_CDevProc(pbfont, &pcdevproc)) {
	done_members |= GLYPH_INFO_CDEVPROC;
	if (members & GLYPH_INFO_CDEVPROC) {
	    info->members = done_members;
	    return_error(e_rangecheck);
	} else {
	    /* Ignore CDevProc. Used to compure MissingWidth.*/
	}
    }
    glyph_ref(pbfont->memory, glyph, &gref);
    if (width_members == GLYPH_INFO_WIDTH1) {
	double wv[4];
	code = zchar_get_metrics2(pbfont, &gref, wv);
	if (code > 0) {
	    modified_widths = true;
	    info->width[1].x = wv[0];
	    info->width[1].y = wv[1];
	    info->v.x = wv[2];
	    info->v.y = wv[3];
	    done_members = width_members | GLYPH_INFO_VVECTOR1;
	    width_members = 0;
	}
    }
    if (width_members) {
	double sbw[4];
	code = zchar_get_metrics(pbfont, &gref, sbw);
	if (code > 0) {
	    modified_widths = true;
	    info->width[wmode].x = sbw[2];
	    info->width[wmode].y = sbw[3];
	    if (code == metricsSideBearingAndWidth) {
		info->v.x = sbw[0];
		info->v.y = sbw[1];
		width_members |= GLYPH_INFO_VVECTOR0;
	    } else {
		info->v.x = 0;
		info->v.y = 0;
	    }
	    done_members = width_members;
	    width_members = 0;
	}
    }

    if (outline_widths) {
	if (modified_widths || zchar_get_CDevProc(pbfont, &pcdevproc)) {
	    /* Discard the modified widths, but indicate they exist. */
	    width_members |= done_members;
	    done_members = outline_widths;
	}
    }
    default_members |= width_members;
    if (default_members) {
	code = (*proc)(font, glyph, pmat, default_members, info);

	if (code < 0)
	    return code;
    } else
	info->members = 0;
    info->members |= done_members;
    return 0;
}

int
z1_glyph_info(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
	      int members, gs_glyph_info_t *info)
{
    int wmode = font->WMode;

    return z1_glyph_info_generic(font, glyph, pmat, members, info, 
				    &gs_type1_glyph_info, wmode);
}

/* Get a Type 1 or Type 9 character metrics and set the cache device. */
int
z1_set_cache(i_ctx_t *i_ctx_p, gs_font_base *pbfont, ref *cnref, 
	    gs_glyph glyph, op_proc_t cont, op_proc_t *exec_cont)
{   /* This function is similar to zchar42_set_cache. */
    double sbw[4];
    gs_glyph_info_t info;
    int wmode = gs_rootfont(igs)->WMode;
    int code;
    gs_matrix id_matrix = { identity_matrix_body };

    code = gs_default_glyph_info((gs_font *)pbfont, glyph, &id_matrix,
		((GLYPH_INFO_WIDTH0 | GLYPH_INFO_VVECTOR0) << wmode) | GLYPH_INFO_BBOX,
	        &info);
    if (code < 0)
	return code;
    sbw[0] = info.v.x;
    sbw[1] = info.v.y;
    sbw[2] = info.width[wmode].x;
    sbw[3] = info.width[wmode].y;
    return zchar_set_cache(i_ctx_p, pbfont, cnref, NULL,
			   sbw + 2, &info.bbox,
			   cont, exec_cont,
			   wmode ? sbw : NULL);
}
