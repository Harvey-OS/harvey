/* Copyright (C) 1993, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zchar1.c,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
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
#include "store.h"

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
    return gs_type1_interp_init(pcis, (gs_imager_state *)pgs, pgs->path,
				&penum->log2_scale,
				(penum->text.operation & TEXT_DO_ANY_CHARPATH) != 0,
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
    /*
     * The following elements are only used locally to make the stack clean
     * for OtherSubrs: they don't need to be declared for the garbage
     * collector.
     */
    ref save_args[6];
    int num_args;
} gs_type1exec_state;

gs_private_st_suffix_add1(st_gs_type1exec_state, gs_type1exec_state,
			  "gs_type1exec_state", gs_type1exec_state_enum_ptrs,
			  gs_type1exec_state_reloc_ptrs, st_gs_type1_state,
			  i_ctx_p);

/* Forward references */
private int bbox_continue(P1(i_ctx_t *));
private int nobbox_continue(P1(i_ctx_t *));
private int type1_push_OtherSubr(P4(i_ctx_t *, const gs_type1exec_state *,
				    int (*)(P1(i_ctx_t *)), const ref *));
private int type1_call_OtherSubr(P4(i_ctx_t *, const gs_type1exec_state *,
				    int (*)(P1(i_ctx_t *)), const ref *));
private int type1_callout_dispatch(P3(i_ctx_t *, int (*)(P1(i_ctx_t *)),
				      int));
private int type1_continue_dispatch(P5(i_ctx_t *, gs_type1exec_state *,
				       const ref *, ref *, int));
private int op_type1_cleanup(P1(i_ctx_t *));
private void op_type1_free(P1(i_ctx_t *));
private void
     type1_cis_get_metrics(P2(const gs_type1_state * pcis, double psbw[4]));
private int bbox_getsbw_continue(P1(i_ctx_t *));
private int type1exec_bbox(P3(i_ctx_t *, gs_type1exec_state *, gs_font *));
private int bbox_finish_fill(P1(i_ctx_t *));
private int bbox_finish_stroke(P1(i_ctx_t *));
private int bbox_fill(P1(i_ctx_t *));
private int bbox_stroke(P1(i_ctx_t *));
private int nobbox_finish(P2(i_ctx_t *, gs_type1exec_state *));
private int nobbox_draw(P2(i_ctx_t *, int (*)(P1(gs_state *))));
private int nobbox_fill(P1(i_ctx_t *));
private int nobbox_stroke(P1(i_ctx_t *));

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
    code = zchar_get_metrics(pbfont, op - 1, cxs.sbw);
    if (code < 0)
	return code;
    cxs.present = code;
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
	cxs.char_bbox = pfont1->FontBBox;
	return type1exec_bbox(i_ctx_p, &cxs, pfont);
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
private int
type1exec_bbox(i_ctx_t *i_ctx_p, gs_type1exec_state * pcxs,
	       gs_font * pfont)
{
    os_ptr op = osp;
    gs_type1_state *const pcis = &pcxs->cis;
    gs_font_base *const pbfont = (gs_font_base *) pfont;

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
			       bbox_finish_fill, bbox_finish_stroke);
    } else {
	/* We have the width and bounding box: */
	/* set up the cache device now. */
	return zchar_set_cache(i_ctx_p, pbfont, op - 1,
			       (pcxs->present ==
				metricsSideBearingAndWidth ?
				pcxs->sbw : NULL),
			       pcxs->sbw + 2,
			       &pcxs->char_bbox,
			       bbox_finish_fill, bbox_finish_stroke);
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

	    /* Get the metrics before freeing the state. */
	    type1_cis_get_metrics(pcis, sbw);
	    bbox = pcxs->char_bbox;
	    op_type1_free(i_ctx_p);
	    return zchar_set_cache(i_ctx_p, pbfont, op, sbw, sbw + 2, &bbox,
				   bbox_finish_fill, bbox_finish_stroke);
	}
    }
}

/* <font> <code|name> <name> <charstring> <sbx> <sby> %bbox_{fill|stroke} - */
/* <font> <code|name> <name> <charstring> %bbox_{fill|stroke} - */
private int bbox_finish(P2(i_ctx_t *, int (*)(P1(i_ctx_t *))));
private int
bbox_finish_fill(i_ctx_t *i_ctx_p)
{
    return bbox_finish(i_ctx_p, bbox_fill);
}
private int
bbox_finish_stroke(i_ctx_t *i_ctx_p)
{
    return bbox_finish(i_ctx_p, bbox_stroke);
}
private int
bbox_finish(i_ctx_t *i_ctx_p, int (*cont) (P1(i_ctx_t *)))
{
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
	    return (*cont)(i_ctx_p);
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
 */
private int
bbox_draw(i_ctx_t *i_ctx_p, int (*draw)(P1(gs_state *)))
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
    if ((code = gs_pathbbox(igs, &bbox)) < 0 ||
	(code = font_param(op - 3, &pfont)) < 0
	)
	return code;
    penum = op_show_find(i_ctx_p);
    if (penum == 0 || !font_uses_charstrings(pfont))
	return_error(e_undefined);
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
    code = zchar_get_metrics(pbfont, op - 1, cxs.sbw);
    if (code < 0)
	return code;
    cxs.present = code;
    code = type1_exec_init(&cxs.cis, penum, igs, pfont1);
    if (code < 0)
	return code;
    cxs.char_bbox = pfont1->FontBBox;
    return type1exec_bbox(i_ctx_p, &cxs, pfont);
}
private int
bbox_fill(i_ctx_t *i_ctx_p)
{
    /* See nobbox_fill for why we use eofill here. */
    return bbox_draw(i_ctx_p, gs_eofill);
}
private int
bbox_stroke(i_ctx_t *i_ctx_p)
{
    return bbox_draw(i_ctx_p, gs_stroke);
}

/* -------- Common code -------- */

/* Get the metrics (l.s.b. and width) from the Type 1 interpreter. */
private void
type1_cis_get_metrics(const gs_type1_state * pcis, double psbw[4])
{
    psbw[0] = fixed2float(pcis->lsb.x);
    psbw[1] = fixed2float(pcis->lsb.y);
    psbw[2] = fixed2float(pcis->width.x);
    psbw[3] = fixed2float(pcis->width.y);
}

/* Handle the results of interpreting the CharString. */
/* pcref points to a t_string ref. */
private int
type1_continue_dispatch(i_ctx_t *i_ctx_p, gs_type1exec_state *pcxs,
			const ref * pcref, ref *pos, int num_args)
{
    int value;
    int code;
    gs_const_string charstring;
    gs_const_string *pchars;

    if (pcref == 0) {
	pchars = 0;
    } else {
	charstring.data = pcref->value.const_bytes;
	charstring.size = r_size(pcref);
	pchars = &charstring;
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
    code = pcxs->cis.pfont->data.interpret(&pcxs->cis, pchars, &value);
    switch (code) {
	case type1_result_callothersubr: {
	    /*
	     * The Type 1 interpreter handles all known OtherSubrs,
	     * so this must be an unknown one.
	     */
	    const font_data *pfdata = pfont_data(gs_currentfont(igs));

	    code = array_get(&pfdata->u.type1.OtherSubrs, (long)value, pos);
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
		     int (*cont)(P1(i_ctx_t *)), const ref *pos)
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
		     int (*cont) (P1(i_ctx_t *)),
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
type1_callout_dispatch(i_ctx_t *i_ctx_p, int (*cont)(P1(i_ctx_t *)),
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
	    return type1exec_bbox(i_ctx_p, pcxs, pfont);
	}
	return zchar_set_cache(i_ctx_p, pbfont, op, NULL, pcxs->sbw + 2,
			       &pcxs->char_bbox,
			       nobbox_fill, nobbox_stroke);
    }
}
/* Finish by popping the operands and filling or stroking. */
private int
nobbox_draw(i_ctx_t *i_ctx_p, int (*draw)(P1(gs_state *)))
{
    int code = draw(igs);

    if (code >= 0)
	pop(4);
    return code;
}
private int
nobbox_fill(i_ctx_t *i_ctx_p)
{
    /*
     * Properly designed fonts, which have no self-intersecting outlines
     * and in which outer and inner outlines are drawn in opposite
     * directions, aren't affected by choice of filling rule; but some
     * badly designed fonts in the Genoa test suite seem to require
     * using the even-odd rule to match Adobe interpreters.
     */
    return nobbox_draw(i_ctx_p, gs_eofill);
}
private int
nobbox_stroke(i_ctx_t *i_ctx_p)
{
    return nobbox_draw(i_ctx_p, gs_stroke);
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
    op_def_end(0)
};

/* ------ Auxiliary procedures for type 1 fonts ------ */

private int
z1_glyph_data(gs_font_type1 * pfont, gs_glyph glyph, gs_const_string * pstr)
{
    ref gref;

    glyph_ref(glyph, &gref);
    return zchar_charstring_data((gs_font *)pfont, &gref, pstr);
}

private int
z1_subr_data(gs_font_type1 * pfont, int index, bool global,
	     gs_const_string * pstr)
{
    const font_data *pfdata = pfont_data(pfont);
    ref subr;
    int code;

    code = array_get((global ? &pfdata->u.type1.GlobalSubrs :
		      &pfdata->u.type1.Subrs),
		     index, &subr);
    if (code < 0)
	return code;
    check_type_only(subr, t_string);
    pstr->data = subr.value.const_bytes;
    pstr->size = r_size(&subr);
    return 0;
}

private int
z1_seac_data(gs_font_type1 *pfont, int ccode, gs_glyph *pglyph,
	     gs_const_string *pstr)
{
    ref std_glyph;
    int code = array_get(&StandardEncoding, (long)ccode, &std_glyph);

    if (code < 0)
	return code;
    if (pglyph) {
	switch (r_type(&std_glyph)) {
	case t_name:
	    *pglyph = name_index(&std_glyph);
	    break;
	case t_integer:
	    *pglyph = gs_min_cid_glyph + std_glyph.value.intval;
	    if (*pglyph < gs_min_cid_glyph || *pglyph > gs_max_glyph)
		*pglyph = gs_no_glyph;
	    break;
	default:
	    return_error(e_typecheck);
	}
    }
    if (pstr)
	code = zchar_charstring_data((gs_font *)pfont, &std_glyph, pstr);
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

int
zcharstring_glyph_outline(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
			  gx_path *ppath)
{
    gs_font_type1 *const pfont1 = (gs_font_type1 *)font;
    gs_const_string charstring;
    ref gref;
    gs_const_string *pchars = &charstring;
    int code;
    gs_type1exec_state cxs;
    gs_type1_state *const pcis = &cxs.cis;
    static const gs_log2_scale_point no_scale = {0, 0};
    const gs_type1_data *pdata;
    const ref *pfdict;
    ref *pcdevproc;
    int value;
    gs_imager_state gis;
    double sbw[4];
    gs_point mpt;

    glyph_ref(glyph, &gref);
    code = zchar_charstring_data(font, &gref, &charstring);
    if (code < 0)
	return code;
    pdata = &pfont1->data;
    if (charstring.size <= max(pdata->lenIV, 0))
	return_error(e_invalidfont);
    pfdict = &pfont_data(pfont1)->dict;
    if (dict_find_string(pfdict, "CDevProc", &pcdevproc) > 0)
	return_error(e_rangecheck); /* can't call CDevProc from here */
    switch (font->WMode) {
    default:
	code = zchar_get_metrics2((gs_font_base *)pfont1, &gref, sbw);
	if (code)
	    break;
	/* falls through */
    case 0:
	code = zchar_get_metrics((gs_font_base *)pfont1, &gref, sbw);
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
    code = gs_type1_interp_init(&cxs.cis, &gis, ppath, &no_scale, true, 0,
				pfont1);
    if (code < 0)
	return code;
    cxs.cis.charpath_flag = true;	/* suppress hinting */
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
    code = pfont1->data.interpret(pcis, pchars, &value);
    switch (code) {
    case 0:		/* all done */
	/* falls through */
    default:		/* code < 0, error */
	return code;
    case type1_result_callothersubr:	/* unknown OtherSubr */
	return_error(e_rangecheck); /* can't handle it */
    case type1_result_sbw:	/* [h]sbw, just continue */
	type1_cis_get_metrics(pcis, cxs.sbw);
	pchars = 0;
	goto icont;
    }
}

/*
 * Redefine glyph_info to take Metrics[2] and CDevProc into account.
 * (If CDevProc is present, return e_rangecheck, since we can't call the
 * interpreter from here.)
 */
int
z1_glyph_info(gs_font *font, gs_glyph glyph, const gs_matrix *pmat,
	      int members, gs_glyph_info_t *info)
{
    ref gref;
    ref *pcdevproc;
    gs_font_type1 *const pfont = (gs_font_type1 *)font;
    gs_font_base *const pbfont = (gs_font_base *)font;
    int wmode = pfont->WMode;
    const ref *pfdict = &pfont_data(pbfont)->dict;
    double sbw[4];
    int width_members = members & (GLYPH_INFO_WIDTH0 << wmode);
    int default_members = members - width_members;
    int done_members = 0;
    int code;

    if (!width_members)
	return gs_type1_glyph_info(font, glyph, pmat, members, info);
    if (dict_find_string(pfdict, "CDevProc", &pcdevproc) > 0)
	return_error(e_rangecheck); /* can't handle it */
    glyph_ref(glyph, &gref);
    if (width_members == GLYPH_INFO_WIDTH1) {
	code = zchar_get_metrics2(pbfont, &gref, sbw);
	if (code > 0) {
	    info->width[1].x = sbw[2];
	    info->width[1].y = sbw[3];
	    done_members = width_members;
	    width_members = 0;
	}
    }
    if (width_members) {
	code = zchar_get_metrics(pbfont, &gref, sbw);
	if (code > 0) {
	    info->width[wmode].x = sbw[2];
	    info->width[wmode].y = sbw[3];
	    done_members = width_members;
	    width_members = 0;
	}
    }
    default_members |= width_members;
    if (default_members) {
	code = gs_type1_glyph_info(font, glyph, pmat, default_members, info);

	if (code < 0)
	    return code;
    } else
	info->members = 0;
    info->members |= done_members;
    return 0;
}
