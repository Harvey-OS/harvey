/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* zpcolor.c */
/* Pattern color */
#include "ghost.h"
#include "errors.h"
#include "oper.h"
#include "gscolor.h"
#include "gscspace.h"
#include "gsmatrix.h"
#include "gsstruct.h"
#include "gxfixed.h"		/* for gxcolor2.h */
#include "gxcolor2.h"
#include "gxdevice.h"
#include "gxdevmem.h"		/* for gxpcolor.h */
#include "gxpcolor.h"
#include "estack.h"
#include "ialloc.h"
#include "istruct.h"
#include "idict.h"
#include "idparam.h"
#include "igstate.h"
#include "store.h"

/* Imported from gspcolor.c */
extern const gs_color_space_type gs_color_space_type_Pattern;

/* Imported from zcolor2.c */
extern gs_memory_type_ptr_t zcolor2_st_pattern_instance_p;

/* Forward references */
private int zPaintProc(P2(const gs_client_color *, gs_state *));
private int pattern_paint_prepare(P1(os_ptr));
private int pattern_paint_finish(P1(os_ptr));

/*
 * Define the structure for remembering the pattern dictionary.
 * This is the "client data" in the template.
 * See zgstate.c (int_gstate) or zfont2.c (font_data) for information
 * as to why we define this as a structure rather than a ref array.
 */
typedef struct int_pattern_s {
	ref dict;
} int_pattern;
gs_private_st_ref_struct(st_int_pattern, int_pattern, "int_pattern");

/* Initialize the Pattern cache and the Pattern instance type. */
private void
zpcolor_init(void)
{	gstate_set_pattern_cache(igs,
				 gx_pattern_alloc_cache(imemory_system,
				     pattern_cache_default_max_tiles,
				     pattern_cache_default_max_bits));
	zcolor2_st_pattern_instance_p = &st_pattern_instance;
}

/* <pattern> <matrix> .buildpattern <pattern> <instance> */
int
zbuildpattern(os_ptr op)
{	os_ptr op1 = op - 1;
	int code;
	gs_matrix mat;
	int PatternType;
	float BBox[4];
	gs_client_pattern template;
	int_pattern *pdata;
	gs_client_color cc_instance;
	ref *pPaintProc;
	check_type(*op1, t_dictionary);
	check_dict_read(*op1);
	if ( (code = read_matrix(op, &mat)) < 0 ||
	     (code = dict_uid_param(op1, &template.uid, 1, imemory)) != 1 ||
	     (code = dict_int_param(op1, "PatternType", 1, 1, 0, &PatternType)) < 0 ||
	     (code = dict_int_param(op1, "PaintType", 1, 2, 0, &template.PaintType)) < 0 ||
	     (code = dict_int_param(op1, "TilingType", 1, 3, 0, &template.TilingType)) < 0 ||
	     (code = dict_float_array_param(op1, "BBox", 4, BBox, NULL)) != 4 ||
	     (code = dict_float_param(op1, "XStep", 0.0, &template.XStep)) != 0 ||
	     (code = dict_float_param(op1, "YStep", 0.0, &template.YStep)) != 0 ||
	     (code = dict_find_string(op1, "PaintProc", &pPaintProc)) <= 0
	   )
		return_error((code < 0 ? code : e_rangecheck));
	check_proc(*pPaintProc);
	template.BBox.p.x = BBox[0];
	template.BBox.p.y = BBox[1];
	template.BBox.q.x = BBox[2];
	template.BBox.q.y = BBox[3];
	template.PaintProc = zPaintProc;
	pdata = ialloc_struct(int_pattern, &st_int_pattern, "int_pattern");
	if ( pdata == 0 )
		return_error(e_VMerror);
	template.client_data = pdata;
	pdata->dict = *op1;
	code = gs_makepattern(&cc_instance, &template, &mat, igs);
	if ( code < 0 )
	  {	ifree_object(pdata, "int_pattern");
		return code;
	  }
	make_istruct(op, a_readonly, cc_instance.pattern);
	return code;
}

/* <array> .setpatternspace - */
/* In the case of uncolored patterns, the current color space is */
/* the base space for the pattern space. */
int
zsetpatternspace(register os_ptr op)
{	const ref *pcsa;
	gs_color_space cs;
	uint edepth = ref_stack_count(&e_stack);
	int code;

	check_read_type(*op, t_array);
	pcsa = op->value.const_refs + 1;
	cs = *gs_currentcolorspace(igs);
	if ( cs.type->num_components < 0 )	/* i.e., Pattern space */
	  return_error(e_rangecheck);
	switch ( r_size(op) )
	{
	case 1:		/* no base space */
		cs.params.pattern.has_base_space = false;
		break;
	default:
		return_error(e_rangecheck);
	case 2:
		/* We can't count on C compilers to recognize the aliasing */
		/* that would be involved in a direct assignment, so.... */
		{	gs_paint_color_space cs_paint;
			cs_paint = *(gs_paint_color_space *)&cs;
			cs.params.pattern.base_space = cs_paint;
		}
		cs.params.pattern.has_base_space = true;
	}
	cs.type = &gs_color_space_type_Pattern;
	code = gs_setcolorspace(igs, &cs);
	if ( code < 0 )
	{	ref_stack_pop_to(&e_stack, edepth);
		return code;
	}
	pop(1);
	return (ref_stack_count(&e_stack) == edepth ? 0 : o_push_estack);  /* installation will load the caches */
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(zpcolor_l2_op_defs) {
		op_def_begin_level2(),
	{"2.buildpattern", zbuildpattern},
	{"1.setpatternspace", zsetpatternspace},
		/* Internal operators */
	{"0%pattern_paint_prepare", pattern_paint_prepare},
	{"0%pattern_paint_finish", pattern_paint_finish},
END_OP_DEFS(zpcolor_init) }

/* ------ Internal procedures ------ */

/* Set up the pattern pointer in a client color for setcolor */
/* with a Pattern space. */


/* Render the pattern by calling the PaintProc. */
private int pattern_paint_cleanup(P1(os_ptr));
private int
zPaintProc(const gs_client_color *pcc, gs_state *pgs)
{	/* Just schedule a call on the real PaintProc. */
	check_estack(2);
	esp++;
	push_op_estack(pattern_paint_prepare);
	return e_InsertProc;
}
/* Prepare to run the PaintProc. */
private int
pattern_paint_prepare(os_ptr op)
{	gs_state *pgs = igs;
	gs_pattern_instance *pinst = gs_currentcolor(pgs)->pattern;
	ref *pdict = &((int_pattern *)pinst->template.client_data)->dict;
	gx_device_pattern_accum *pdev;
	int code;
	ref *ppp;
	check_estack(5);
	pdev = gx_pattern_accum_alloc(imemory, "pattern_paint_prepare");
	if ( pdev == 0 )
		return_error(e_VMerror);
	pdev->instance = pinst;
	pdev->bitmap_memory = gstate_pattern_cache(pgs)->memory;
	code = (*dev_proc(pdev, open_device))((gx_device *)pdev);
	if ( code < 0 )
	  { ifree_object(pdev, "pattern_paint_prepare");
	    return code;
	  }
	code = gs_gsave(pgs);
	if ( code < 0 )
		return code;
	code = gs_setgstate(pgs, pinst->saved);
	if ( code < 0 )
	{	gs_grestore(pgs);
		return code;
	}
	gx_set_device_only(pgs, (gx_device *)pdev);
	push_mark_estack(es_other, pattern_paint_cleanup);
	++esp;
	make_istruct(esp, 0, pdev);
	push_op_estack(pattern_paint_finish);
	dict_find_string(pdict, "PaintProc", &ppp); /* can't fail */
	*++esp = *ppp;
	*++esp = *pdict;	/* (push on ostack) */
	return o_push_estack;
}
/* Save the rendered pattern. */
private int
pattern_paint_finish(os_ptr op)
{	gx_device_pattern_accum *pdev = r_ptr(esp, gx_device_pattern_accum);
	gx_color_tile *ctile;
	int code = gx_pattern_cache_add_entry(igs, pdev, &ctile);
	if ( code < 0 )
	  return code;
	if ( ctile->bits.data != 0 )
	  pdev->bits->bitmap_memory = 0;	/* don't free the bits */
	if ( ctile->mask.data != 0 )
	  pdev->mask->bitmap_memory = 0;	/* ditto */
	esp -= 2;
	pattern_paint_cleanup(op);
	return o_pop_estack;
}
/* Clean up after rendering a pattern.  Note that iff the rendering */
/* succeeded, closing the accumulator won't free the bits. */
private int
pattern_paint_cleanup(os_ptr op)
{	gx_device_pattern_accum *pdev = r_ptr(esp + 2, gx_device_pattern_accum);
	gs_grestore(igs);
	(*dev_proc(pdev, close_device))((gx_device *)pdev);
	ifree_object(pdev, "pattern_paint_cleanup");
	return 0;
}
