/* Copyright (C) 1992, 2000 Aladdin Enterprises.  All rights reserved.
  
  This file is part of AFPL Ghostscript.
  
  AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author or
  distributor accepts any responsibility for the consequences of using it, or
  for whether it serves any particular purpose or works at all, unless he or
  she says so in writing.  Refer to the Aladdin Free Public License (the
  "License") for full details.
  
  Every copy of AFPL Ghostscript must include a copy of the License, normally
  in a plain ASCII text file named PUBLIC.  The License grants you the right
  to copy, modify and redistribute AFPL Ghostscript, but only under certain
  conditions described in the License.  Among other things, the License
  requires that the copyright notice and this notice be preserved on all
  copies.
*/

/*$Id: zcolor2.c,v 1.4 2001/06/07 14:48:24 alexcher Exp $ */
/* Level 2 color operators */
#include "ghost.h"
#include "oper.h"
#include "gscolor.h"
#include "gscssub.h"
#include "gsmatrix.h"
#include "gsstruct.h"
#include "gxcspace.h"
#include "gxfixed.h"		/* for gxcolor2.h */
#include "gxcolor2.h"
#include "gxdcolor.h"		/* for gxpcolor.h */
#include "gxdevice.h"
#include "gxdevmem.h"		/* for gxpcolor.h */
#include "gxpcolor.h"
#include "estack.h"
#include "ialloc.h"
#include "istruct.h"
#include "idict.h"
#include "iname.h"
#include "idparam.h"
#include "igstate.h"
#include "store.h"
#include <string.h>	/* RSC added include for strlen */

/* Forward references */
private int store_color_params(P3(os_ptr, const gs_paint_color *,
				  const gs_color_space *));
private int load_color_params(P3(os_ptr, gs_paint_color *,
				 const gs_color_space *));

/* Test whether a Pattern instance uses a base space. */
inline private bool
pattern_instance_uses_base_space(const gs_pattern_instance_t *pinst)
{
    return pinst->type->procs.uses_base_space(pinst->type->procs.get_pattern(pinst));
}

/* - currentcolor <param1> ... <paramN> */
private int
zcurrentcolor(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const gs_client_color *pc = gs_currentcolor(igs);
    const gs_color_space *pcs = gs_currentcolorspace(igs);
    int n;

    check_ostack(5);		/* Worst case: CMYK + pattern */
    if (pcs->type->index == gs_color_space_index_Pattern) {
	gs_pattern_instance_t *pinst = pc->pattern;

	n = 1;
	if (pinst != 0 && pattern_instance_uses_base_space(pinst))	/* uncolored */
	    n += store_color_params(op, &pc->paint,
		   (const gs_color_space *)&pcs->params.pattern.base_space);
	op[n] = istate->pattern;
    } else
	n = store_color_params(op, &pc->paint, pcs);
    push(n);
    return 0;
}

/* - .currentcolorspace <array|int> */
private int
zcurrentcolorspace(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    push(1);
    *op = istate->colorspace.array;
    if (r_has_type(op, t_null)) {
	/*
	 * Return the color space index.  This is only possible
	 * for the parameterless color spaces.
	 */
	gs_color_space_index csi = gs_currentcolorspace_index(igs);

	make_int(op, (int)(csi));
    }
    return 0;
}

private int
dev2ink_idx(const gx_device *dev, uint **pp_ref)
{ 
    int code=0;
    static uint sep_idx[8] = {0, 0, 0, 0, 0, 0, 0, 0}; 
    static const char * sep_names[] = {"Gray", "Red", "Green", "Blue", 
      "Cyan", "Magenta", "Yellow", "Black"};

    if (!sep_idx[7]) { 
        int i;
        for (i=0; i<8; i++) { 
            ref rname;
            int code = name_ref((const byte *)sep_names[i], strlen(sep_names[i]), &rname, 0);
            if (code < 0)
              return code;
            sep_idx[i]=name_index(&rname);
        }
    }
    switch(dev->color_info.num_components) {
        case 1:
          *pp_ref = sep_idx;
          break;
        case 3:
          *pp_ref = sep_idx + 1;
          break;
        case 4:
          *pp_ref = sep_idx + 4;
          break;
        default:  /* fixme: DeviceN color model not supported */
          *pp_ref = sep_idx;
          code = gs_note_error(e_rangecheck);
          break;
    }
    return code;
}

private int
is_subset_idx(uint *set, int set_len, gs_separation_name *subset, int sub_len)
{ 
    int i, j, is_subset = 1; 
  
    if(set_len < sub_len)
      return 0;
    for (i=0; i < sub_len && is_subset; ++i) { 
        for(j=0; j < set_len && set[j] != subset[i]; ++j)
          ; /* no-op */
        is_subset &= j < set_len;
    }
    return is_subset;   
}

private int
devicen_no_cleanup(i_ctx_t *i_ctx_p)
{ 
    return 0;
}

/* Discard the transformed color value. */
private int
devicen_setcolor_discard(i_ctx_t *i_ctx_p)
{
    gs_state *pgs = igs;
    const gs_color_space *pcs = gs_currentcolorspace(pgs);  /* always DeviceN */
    const gs_device_n_params *params = &pcs->params.device_n;
    const gs_color_space *pacs = (const gs_color_space *)&params->alt_space;
    int num_in = pcs->params.device_n.num_components; /* also on e-stack */
    int num_out = gs_color_space_num_components(pacs);
    
    esp -= num_in + 2;		/* mark, count, tint values */
    osp -= num_out;
    return o_pop_estack;
}

/* <param1> ... <paramN> setcolor - */
private int
zsetcolor(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_client_color c;
    const gs_color_space *pcs = gs_currentcolorspace(igs);
    int n, code;
    gs_pattern_instance_t *pinst = 0;

    if (pcs->type->index == gs_color_space_index_Pattern) {
	/* Make sure *op is a real Pattern. */
	ref *pImpl;

	if (r_has_type(op, t_null)) {
	    c.pattern = 0;
	    n = 1;
	} else {
	    check_type(*op, t_dictionary);
	    check_dict_read(*op);
	    /*
	     * We have no way to check for a subclass of st_pattern_instance,
	     * so just make sure the structure is large enough.
	     */
	    if (dict_find_string(op, "Implementation", &pImpl) <= 0 ||
		!r_is_struct(pImpl) ||
		gs_object_size(imemory, r_ptr(pImpl, const void)) <
		sizeof(gs_pattern_instance_t)
		)
		return_error(e_rangecheck);
	    pinst = r_ptr(pImpl, gs_pattern_instance_t);
	    c.pattern = pinst;
	    if (pattern_instance_uses_base_space(pinst)) {	/* uncolored */
		if (!pcs->params.pattern.has_base_space)
		    return_error(e_rangecheck);
		n = load_color_params(op - 1, &c.paint,
				      (const gs_color_space *)&pcs->params.pattern.base_space);
		if (n < 0)
		    return n;
		n++;
	    } else
		n = 1;
	}
    } else {
	n = load_color_params(op, &c.paint, pcs);
	c.pattern = 0;		/* for GC */
    }
    if (n < 0)
	return n;
    code = gs_setcolor(igs, &c);
    if (code < 0)
	return code;
    if (pinst != 0)
	istate->pattern = *op;
    if(pcs->type->index == gs_color_space_index_DeviceN) {
        /* 
         * Duotone code generated by Adobe PhotoShop 5+ expects that setcolor
         * (1) calls tint transform function (ttf). PLRM doesn't require this
         * (2) permits ttf to change op stack as side effect. PLRM forbids this
         * (3) assumes op stack is not changed when ttf is executed
         */
        gs_color_space_type const *pcst = pcs->type;
        int num_comp    = pcst->num_components(pcs);
        gx_device const *dev  = gs_currentdevice(igs);
        int num_ink  = dev->color_info.num_components;
        gs_separation_name *idx_comp=pcs->params.device_n.names;
        uint *idx_ink;

        code=dev2ink_idx(dev, &idx_ink);
        if(code >= 0 && !is_subset_idx(idx_ink, num_ink, idx_comp, num_comp)) { 
            int i;
            const gs_color_space *acsp = pcst->base_space(pcs);
            int alt_num_comp = acsp->type->num_components(acsp);

            check_estack(num_comp + 4);
            for (i = 0; i < num_comp; ++i)
	      *++esp = osp[i - num_comp + 1];
            ++esp;
            make_int(esp, num_comp);
            push_mark_estack(es_for, devicen_no_cleanup);
            push_op_estack(devicen_setcolor_discard);
            *++esp = istate->colorspace.procs.special.device_n.tint_transform;
            return o_push_estack;
        }
    }
    pop(n);
    return code;
}

/* <array> .setcolorspace - */
private int
zsetcolorspace(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;

    check_type(*op, t_array);
    istate->colorspace.array = *op;
    pop(1);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zcolor2_l2_op_defs[] =
{
    op_def_begin_level2(),
    {"0currentcolor", zcurrentcolor},
    {"0.currentcolorspace", zcurrentcolorspace},
    {"1setcolor", zsetcolor},
    {"1.setcolorspace", zsetcolorspace},
    {"0%.devicen_setcolor_discard", devicen_setcolor_discard},
    op_def_end(0)
};

/* ------ Internal procedures ------ */

/* Store non-pattern color values on the operand stack. */
/* Return the number of values stored. */
private int
store_color_params(os_ptr op, const gs_paint_color * pc,
		   const gs_color_space * pcs)
{
    int n = cs_num_components(pcs);

    if (pcs->type->index == gs_color_space_index_Indexed)
	make_int(op + 1, (int)pc->values[0]);
    else
	make_floats(op + 1, pc->values, n);
    return n;
}

/* Load non-pattern color values from the operand stack. */
/* Return the number of values stored. */
private int
load_color_params(os_ptr op, gs_paint_color * pc, const gs_color_space * pcs)
{
    int n = cs_num_components(pcs);
    int code = float_params(op, n, pc->values);

    if (code < 0)
	return code;
    return n;
}
