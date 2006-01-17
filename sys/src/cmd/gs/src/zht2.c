/* Copyright (C) 1992, 1995, 1996, 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zht2.c,v 1.14 2005/10/11 10:04:28 leonardo Exp $ */
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
#include "iname.h"
#include "zht2.h"

/* Forward references */
private int dict_spot_params(const ref *, gs_spot_halftone *, ref *, ref *);
private int dict_spot_results(i_ctx_t *, ref *, const gs_spot_halftone *);
private int dict_threshold_params(const ref *, gs_threshold_halftone *,
				  ref *);
private int dict_threshold2_params(const ref *, gs_threshold2_halftone *,
				   ref *, gs_memory_t *);

/*
 * This routine translates a gs_separation_name value into a character string
 * pointer and a string length.
 */
int
gs_get_colorname_string(const gs_memory_t *mem, gs_separation_name colorname_index,
			unsigned char **ppstr, unsigned int *pname_size)
{
    ref nref;

    name_index_ref(mem, colorname_index, &nref);
    name_string_ref(mem, &nref, &nref);
    return obj_string_data(mem, &nref, (const unsigned char**) ppstr, pname_size);
}

/* Dummy spot function */
private float
spot1_dummy(floatp x, floatp y)
{
    return (x + y) / 2;
}

/* <dict> <dict5> .sethalftone5 - */
private int sethalftone_finish(i_ctx_t *);
private int sethalftone_cleanup(i_ctx_t *);
private int
zsethalftone5(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    uint count;
    gs_halftone_component *phtc;
    gs_halftone_component *pc;
    int code = 0;
    int j;
    gs_halftone *pht;
    gx_device_halftone *pdht;
    ref sprocs[GS_CLIENT_COLOR_MAX_COMPONENTS + 1];
    ref tprocs[GS_CLIENT_COLOR_MAX_COMPONENTS + 1];
    gs_memory_t *mem;
    uint edepth = ref_stack_count(&e_stack);
    int npop = 2;
    int dict_enum = dict_first(op);
    ref rvalue[2];
    int cname, colorant_number;
    byte * pname;
    uint name_size;
    int halftonetype, type = 0;
    gs_state *pgs = igs;
    int space_index = r_space_index(op - 1);

    mem = (gs_memory_t *) idmemory->spaces_indexed[space_index];

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    check_type(op[-1], t_dictionary);
    check_dict_read(op[-1]);
 
    /*
     * We think that Type 2 and Type 4 halftones, like
     * screens set by setcolorscreen, adapt automatically to
     * the device color space, so we need to mark them
     * with a different internal halftone type.
     */
    dict_int_param(op - 1, "HalftoneType", 1, 5, 0, &type);
    halftonetype = (type == 2 || type == 4)
    			? ht_type_multiple_colorscreen
			: ht_type_multiple;

    /* Count how many components that we will actually use. */

    for (count = 0; ;) {
	bool have_default = false;

	/* Move to next element in the dictionary */
	if ((dict_enum = dict_next(op, dict_enum, rvalue)) == -1)
	    break;
	/*
	 * Verify that we have a valid component.  We may have a
	 * /HalfToneType entry.
	 */
  	if (!r_has_type(&rvalue[1], t_dictionary))
	    continue;

	/* Get the name of the component  verify that we will use it. */
	cname = name_index(mem, &rvalue[0]);
	code = gs_get_colorname_string(mem, cname, &pname, &name_size);
	if (code < 0)
	    break;
	colorant_number = gs_cname_to_colorant_number(pgs, pname, name_size,
						halftonetype);
	if (colorant_number < 0)
	    continue;
	else if (colorant_number == GX_DEVICE_COLOR_MAX_COMPONENTS) {
	    /* If here then we have the "Default" component */
	    if (have_default)
		return_error(e_rangecheck);
	    have_default = true;
	}

	count++;
	/*
	 * Check to see if we have already reached the legal number of
	 * components.
	 */
	if (count > GS_CLIENT_COLOR_MAX_COMPONENTS + 1) {
	    code = gs_note_error(e_rangecheck);
	    break;
        }
    }

    check_estack(5);		/* for sampling Type 1 screens */
    refset_null(sprocs, count);
    refset_null(tprocs, count);
    rc_alloc_struct_0(pht, gs_halftone, &st_halftone,
		      imemory, pht = 0, ".sethalftone5");
    phtc = gs_alloc_struct_array(mem, count, gs_halftone_component,
				 &st_ht_component_element,
				 ".sethalftone5");
    rc_alloc_struct_0(pdht, gx_device_halftone, &st_device_halftone,
		      imemory, pdht = 0, ".sethalftone5");
    if (pht == 0 || phtc == 0 || pdht == 0) {
	j = 0; /* Quiet the compiler: 
	          gs_note_error isn't necessarily identity, 
		  so j could be left ununitialized. */
	code = gs_note_error(e_VMerror);
    } else {
        dict_enum = dict_first(op);
	for (j = 0, pc = phtc; ;) {
	    int type;

	    /* Move to next element in the dictionary */
	    if ((dict_enum = dict_next(op, dict_enum, rvalue)) == -1)
	        break;
	    /*
	     * Verify that we have a valid component.  We may have a
	     * /HalfToneType entry.
	     */
  	    if (!r_has_type(&rvalue[1], t_dictionary))
		continue;

	    /* Get the name of the component */
	    cname = name_index(mem, &rvalue[0]);
	    code = gs_get_colorname_string(mem, cname, &pname, &name_size);
	    if (code < 0)
	        break;
	    colorant_number = gs_cname_to_colorant_number(pgs, pname, name_size,
						halftonetype);
	    if (colorant_number < 0)
		continue;		/* Do not use this component */
	    pc->cname = cname;
	    pc->comp_number = colorant_number;

	    /* Now process the component dictionary */
	    check_dict_read(rvalue[1]);
	    if (dict_int_param(&rvalue[1], "HalftoneType", 1, 7, 0, &type) < 0) {
		code = gs_note_error(e_typecheck);
		break;
	    }
	    switch (type) {
		default:
		    code = gs_note_error(e_rangecheck);
		    break;
		case 1:
		    code = dict_spot_params(&rvalue[1], &pc->params.spot,
		    				sprocs + j, tprocs + j);
		    pc->params.spot.screen.spot_function = spot1_dummy;
		    pc->type = ht_type_spot;
		    break;
		case 3:
		    code = dict_threshold_params(&rvalue[1], &pc->params.threshold,
		    					tprocs + j);
		    pc->type = ht_type_threshold;
		    break;
		case 7:
		    code = dict_threshold2_params(&rvalue[1], &pc->params.threshold2,
		    					tprocs + j, imemory);
		    pc->type = ht_type_threshold2;
		    break;
	    }
	    if (code < 0)
		break;
	    pc++;
	    j++;
	}
    }
    if (code >= 0) {
	pht->type = halftonetype;
	pht->params.multiple.components = phtc;
	pht->params.multiple.num_comp = j;
	pht->params.multiple.get_colorname_string = gs_get_colorname_string;
	code = gs_sethalftone_prepare(igs, pht, pdht);
    }
    if (code >= 0) {
	/*
	 * Put the actual frequency and angle in the spot function component dictionaries.
	 */
	dict_enum = dict_first(op);
	for (pc = phtc; ; ) {
	    /* Move to next element in the dictionary */
	    if ((dict_enum = dict_next(op, dict_enum, rvalue)) == -1)
		break;

	    /* Verify that we have a valid component */
	    if (!r_has_type(&rvalue[1], t_dictionary))
		continue;

	    /* Get the name of the component and verify that we will use it. */
	    cname = name_index(mem, &rvalue[0]);
	    code = gs_get_colorname_string(mem, cname, &pname, &name_size);
	    if (code < 0)
	        break;
	    colorant_number = gs_cname_to_colorant_number(pgs, pname, name_size,
						halftonetype);
	    if (colorant_number < 0)
		continue;

	    if (pc->type == ht_type_spot) {
		code = dict_spot_results(i_ctx_p, &rvalue[1], &pc->params.spot);
		if (code < 0)
		    break;
	    }
	    pc++;
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
	    gx_ht_order *porder = NULL;

	    if (pdht->components == 0)
		porder = &pdht->order;
	    else {
		/* Find the component in pdht that matches component j in
		   the pht; gs_sethalftone_prepare() may permute these. */
		int k;
		int comp_number = phtc[j].comp_number;
		for (k = 0; k < count; k++) {
		    if (pdht->components[k].comp_number == comp_number) {
			porder = &pdht->components[k].corder;
			break;
		    }
		}
	    }
	    switch (phtc[j].type) {
	    case ht_type_spot:
		code = zscreen_enum_init(i_ctx_p, porder,
					 &phtc[j].params.spot.screen,
					 &sprocs[j], 0, 0, space_index);
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
