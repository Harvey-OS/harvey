/* Copyright (C) 1992, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: zht2.c,v 1.1 2000/03/09 08:40:45 lpd Exp $ */
/* Level 2 sethalftone operator */
#include "ghost.h"
#include "oper.h"
#include "gsstruct.h"
#include "gxdevice.h"		/* for gzht.h */
#include "gzht.h"
#include "estack.h"
#include "ialloc.h"
#include "iddict.h"
#include "idparam.h"
#include "igstate.h"
#include "icolor.h"
#include "iht.h"
#include "store.h"

/* Forward references */
private int dict_spot_params(P4(const ref *, gs_spot_halftone *,
				ref *, ref *));
private int dict_spot_results(P3(i_ctx_t *, ref *, const gs_spot_halftone *));
private int dict_threshold_params(P3(const ref *, gs_threshold_halftone *,
				     ref *));
private int dict_threshold2_params(P4(const ref *, gs_threshold2_halftone *,
				      ref *, gs_memory_t *));

/* Dummy spot function */
private float
spot1_dummy(floatp x, floatp y)
{
    return (x + y) / 2;
}

/* <dict> <dict5> .sethalftone5 - */
private int sethalftone_finish(P1(i_ctx_t *));
private int sethalftone_cleanup(P1(i_ctx_t *));
private int
zsethalftone5(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint count;
    gs_halftone_component *phtc;
    gs_halftone_component *pc;
    int code = 0;
    int i, j;
    gs_halftone *pht;
    gx_device_halftone *pdht;
    static const char *const color_names[] = {
	gs_ht_separation_name_strings
    };
    ref sprocs[countof(color_names)];
    ref tprocs[countof(color_names)];
    gs_memory_t *mem;
    uint edepth = ref_stack_count(&e_stack);
    int npop = 2;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    check_type(op[-1], t_dictionary);
    check_dict_read(op[-1]);
    count = 0;
    for (i = 0; i < countof(color_names); i++) {
	ref *pvalue;

	if (dict_find_string(op, color_names[i], &pvalue) > 0)
	    count++;
	else if (i == gs_ht_separation_Default)
	    return_error(e_typecheck);
    }
    mem = (gs_memory_t *) idmemory->spaces_indexed[r_space_index(op - 1)];
    check_estack(5);		/* for sampling Type 1 screens */
    refset_null(sprocs, countof(sprocs));
    refset_null(tprocs, countof(tprocs));
    rc_alloc_struct_0(pht, gs_halftone, &st_halftone,
		      imemory, pht = 0, ".sethalftone5");
    phtc = gs_alloc_struct_array(mem, count, gs_halftone_component,
				 &st_ht_component_element,
				 ".sethalftone5");
    rc_alloc_struct_0(pdht, gx_device_halftone, &st_device_halftone,
		      imemory, pdht = 0, ".sethalftone5");
    if (pht == 0 || phtc == 0 || pdht == 0)
	code = gs_note_error(e_VMerror);
    else
	for (i = 0, j = 0, pc = phtc; i < countof(color_names); i++) {
	    int type;
	    ref *pvalue;

	    if (dict_find_string(op, color_names[i], &pvalue) > 0) {
		check_type_only(*pvalue, t_dictionary);
		check_dict_read(*pvalue);
		if (dict_int_param(pvalue, "HalftoneType", 1, 7, 0,
				   &type) < 0
		    ) {
		    code = gs_note_error(e_typecheck);
		    break;
		}
		switch (type) {
		    default:
			code = gs_note_error(e_rangecheck);
			break;
		    case 1:
			code = dict_spot_params(pvalue,
						&pc->params.spot, sprocs + j,
						tprocs + j);
			pc->params.spot.screen.spot_function = spot1_dummy;
			pc->type = ht_type_spot;
			break;
		    case 3:
			code = dict_threshold_params(pvalue,
					 &pc->params.threshold, tprocs + j);
			pc->type = ht_type_threshold;
			break;
		    case 7:
			code = dict_threshold2_params(pvalue,
					&pc->params.threshold2, tprocs + j,
					imemory);
			pc->type = ht_type_threshold2;
			break;
		}
		if (code < 0)
		    break;
		pc->cname = (gs_ht_separation_name) i;
		pc++, j++;
	    }
	}
    if (code >= 0) {
	/*
	 * We think that Type 2 and Type 4 halftones, like
	 * screens set by setcolorscreen, adapt automatically to
	 * the device color space, so we need to mark them
	 * with a different internal halftone type.
	 */
	int type = 0;

	dict_int_param(op - 1, "HalftoneType", 1, 5, 0, &type);
	pht->type =
	    (type == 2 || type == 4 ? ht_type_multiple_colorscreen :
	     ht_type_multiple);
	pht->params.multiple.components = phtc;
	pht->params.multiple.num_comp = count;
	code = gs_sethalftone_prepare(igs, pht, pdht);
    }
    if (code >= 0)
	for (j = 0, pc = phtc; j < count; j++, pc++) {
	    if (pc->type == ht_type_spot) {
		ref *pvalue;

		dict_find_string(op, color_names[pc->cname], &pvalue);
		code = dict_spot_results(i_ctx_p, pvalue, &pc->params.spot);
		if (code < 0)
		    break;
	    }
	}
    if (code >= 0) {
	/*
	 * Schedule the sampling of any Type 1 screens,
	 * and any (Type 1 or Type 3) TransferFunctions.
	 * Save the stack depths in case we have to back out.
	 */
	uint odepth = ref_stack_count(&o_stack);
	ref odict, odict5;

	odict = op[-1];
	odict5 = *op;
	pop(2);
	op = osp;
	esp += 5;
	make_mark_estack(esp - 4, es_other, sethalftone_cleanup);
	esp[-3] = odict;
	make_istruct(esp - 2, 0, pht);
	make_istruct(esp - 1, 0, pdht);
	make_op_estack(esp, sethalftone_finish);
	for (j = 0; j < count; j++) {
	    gx_ht_order *porder =
	    (pdht->components == 0 ? &pdht->order :
	     &pdht->components[j].corder);

	    switch (phtc[j].type) {
		case ht_type_spot:
		    code = zscreen_enum_init(i_ctx_p, porder,
					     &phtc[j].params.spot.screen,
					     &sprocs[j], 0, 0, mem);
		    if (code < 0)
			break;
		    /* falls through */
		case ht_type_threshold:
		    if (!r_has_type(tprocs + j, t__invalid)) {
			/* Schedule TransferFunction sampling. */
			/****** check_xstack IS WRONG ******/
			check_ostack(zcolor_remap_one_ostack);
			check_estack(zcolor_remap_one_estack);
			code = zcolor_remap_one(i_ctx_p, tprocs + j,
						porder->transfer, igs,
						zcolor_remap_one_finish);
			op = osp;
		    }
		    break;
		default:	/* not possible here, but to keep */
		    /* the compilers happy.... */
		    ;
	    }
	    if (code < 0) {	/* Restore the stack. */
		ref_stack_pop_to(&o_stack, odepth);
		ref_stack_pop_to(&e_stack, edepth);
		op = osp;
		op[-1] = odict;
		*op = odict5;
		break;
	    }
	    npop = 0;
	}
    }
    if (code < 0) {
	gs_free_object(mem, pdht, ".sethalftone5");
	gs_free_object(mem, phtc, ".sethalftone5");
	gs_free_object(mem, pht, ".sethalftone5");
	return code;
    }
    pop(npop);
    return (ref_stack_count(&e_stack) > edepth ? o_push_estack : 0);
}
/* Install the halftone after sampling. */
private int
sethalftone_finish(i_ctx_t *i_ctx_p)
{
    gx_device_halftone *pdht = r_ptr(esp, gx_device_halftone);
    int code;

    if (pdht->components)
	pdht->order = pdht->components[0].corder;
    code = gx_ht_install(igs, r_ptr(esp - 1, gs_halftone), pdht);
    if (code < 0)
	return code;
    istate->halftone = esp[-2];
    esp -= 4;
    sethalftone_cleanup(i_ctx_p);
    return o_pop_estack;
}
/* Clean up after installing the halftone. */
private int
sethalftone_cleanup(i_ctx_t *i_ctx_p)
{
    gx_device_halftone *pdht = r_ptr(&esp[4], gx_device_halftone);
    gs_halftone *pht = r_ptr(&esp[3], gs_halftone);

    gs_free_object(pdht->rc.memory, pdht,
		   "sethalftone_cleanup(device halftone)");
    gs_free_object(pht->rc.memory, pht,
		   "sethalftone_cleanup(halftone)");
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zht2_l2_op_defs[] =
{
    op_def_begin_level2(),
    {"2.sethalftone5", zsethalftone5},
		/* Internal operators */
    {"0%sethalftone_finish", sethalftone_finish},
    op_def_end(0)
};

/* ------ Internal routines ------ */

/* Extract frequency, angle, spot function, and accurate screens flag */
/* from a dictionary. */
private int
dict_spot_params(const ref * pdict, gs_spot_halftone * psp,
		 ref * psproc, ref * ptproc)
{
    int code;

    check_dict_read(*pdict);
    if ((code = dict_float_param(pdict, "Frequency", 0.0,
				 &psp->screen.frequency)) != 0 ||
	(code = dict_float_param(pdict, "Angle", 0.0,
				 &psp->screen.angle)) != 0 ||
      (code = dict_proc_param(pdict, "SpotFunction", psproc, false)) != 0 ||
	(code = dict_bool_param(pdict, "AccurateScreens",
				gs_currentaccuratescreens(),
				&psp->accurate_screens)) < 0 ||
      (code = dict_proc_param(pdict, "TransferFunction", ptproc, false)) < 0
	)
	return (code < 0 ? code : e_undefined);
    psp->transfer = (code > 0 ? (gs_mapping_proc) 0 : gs_mapped_transfer);
    psp->transfer_closure.proc = 0;
    psp->transfer_closure.data = 0;
    return 0;
}

/* Set actual frequency and angle in a dictionary. */
private int
dict_real_result(i_ctx_t *i_ctx_p, ref * pdict, const char *kstr, floatp val)
{
    int code = 0;
    ref *ignore;

    if (dict_find_string(pdict, kstr, &ignore) > 0) {
	ref rval;

	check_dict_write(*pdict);
	make_real(&rval, val);
	code = idict_put_string(pdict, kstr, &rval);
    }
    return code;
}
private int
dict_spot_results(i_ctx_t *i_ctx_p, ref * pdict, const gs_spot_halftone * psp)
{
    int code;

    code = dict_real_result(i_ctx_p, pdict, "ActualFrequency",
			    psp->screen.actual_frequency);
    if (code < 0)
	return code;
    return dict_real_result(i_ctx_p, pdict, "ActualAngle",
			    psp->screen.actual_angle);
}

/* Extract Width, Height, and TransferFunction from a dictionary. */
private int
dict_threshold_common_params(const ref * pdict,
			     gs_threshold_halftone_common * ptp,
			     ref **pptstring, ref *ptproc)
{
    int code;

    check_dict_read(*pdict);
    if ((code = dict_int_param(pdict, "Width", 1, 0x7fff, -1,
			       &ptp->width)) < 0 ||
	(code = dict_int_param(pdict, "Height", 1, 0x7fff, -1,
			       &ptp->height)) < 0 ||
	(code = dict_find_string(pdict, "Thresholds", pptstring)) <= 0 ||
      (code = dict_proc_param(pdict, "TransferFunction", ptproc, false)) < 0
	)
	return (code < 0 ? code : e_undefined);
    ptp->transfer_closure.proc = 0;
    ptp->transfer_closure.data = 0;
    return code;
}

/* Extract threshold common parameters + Thresholds. */
private int
dict_threshold_params(const ref * pdict, gs_threshold_halftone * ptp,
		      ref * ptproc)
{
    ref *tstring;
    int code =
	dict_threshold_common_params(pdict,
				     (gs_threshold_halftone_common *)ptp,
				     &tstring, ptproc);

    if (code < 0)
	return code;
    check_read_type_only(*tstring, t_string);
    if (r_size(tstring) != (long)ptp->width * ptp->height)
	return_error(e_rangecheck);
    ptp->thresholds.data = tstring->value.const_bytes;
    ptp->thresholds.size = r_size(tstring);
    ptp->transfer = (code > 0 ? (gs_mapping_proc) 0 : gs_mapped_transfer);
    return 0;
}

/* Extract threshold common parameters + Thresholds, Width2, Height2, */
/* BitsPerSample. */
private int
dict_threshold2_params(const ref * pdict, gs_threshold2_halftone * ptp,
		       ref * ptproc, gs_memory_t *mem)
{
    ref *tstring;
    int code =
	dict_threshold_common_params(pdict,
				     (gs_threshold_halftone_common *)ptp,
				     &tstring, ptproc);
    int bps;
    uint size;
    int cw2, ch2;

    if (code < 0 ||
	(code = cw2 = dict_int_param(pdict, "Width2", 0, 0x7fff, 0,
				     &ptp->width2)) < 0 ||
	(code = ch2 = dict_int_param(pdict, "Height2", 0, 0x7fff, 0,
				     &ptp->height2)) < 0 ||
	(code = dict_int_param(pdict, "BitsPerSample", 8, 16, -1, &bps)) < 0
	)
	return code;
    if ((bps != 8 && bps != 16) || cw2 != ch2 ||
	(!cw2 && (ptp->width2 == 0 || ptp->height2 == 0))
	)
	return_error(e_rangecheck);
    ptp->bytes_per_sample = bps / 8;
    switch (r_type(tstring)) {
    case t_string:
	size = r_size(tstring);
	gs_bytestring_from_string(&ptp->thresholds, tstring->value.const_bytes,
				  size);
	break;
    case t_astruct:
	if (gs_object_type(mem, tstring->value.pstruct) != &st_bytes)
	    return_error(e_typecheck);
	size = gs_object_size(mem, tstring->value.pstruct);
	gs_bytestring_from_bytes(&ptp->thresholds, r_ptr(tstring, byte),
				 0, size);
	break;
    default:
	return_error(e_typecheck);
    }
    check_read(*tstring);
    if (size != (ptp->width * ptp->height + ptp->width2 * ptp->height2) *
	ptp->bytes_per_sample)
	return_error(e_rangecheck);
    return 0;
}
