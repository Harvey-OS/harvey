/* Copyright (C) 1989, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zdevice.c,v 1.10 2004/09/15 19:41:01 ray Exp $ */
/* Device-related operators */
#include "string_.h"
#include "ghost.h"
#include "oper.h"
#include "ialloc.h"
#include "idict.h"
#include "igstate.h"
#include "iname.h"
#include "interp.h"
#include "iparam.h"
#include "ivmspace.h"
#include "gsmatrix.h"
#include "gsstate.h"
#include "gxdevice.h"
#include "gxgetbit.h"
#include "store.h"

/* <device> <keep_open> .copydevice2 <newdevice> */
private int
zcopydevice2(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gx_device *new_dev;
    int code;

    check_read_type(op[-1], t_device);
    check_type(*op, t_boolean);
    code = gs_copydevice2(&new_dev, op[-1].value.pdevice, op->value.boolval,
			  imemory);
    if (code < 0)
	return code;
    new_dev->memory = imemory;
    make_tav(op - 1, t_device, icurrent_space | a_all, pdevice, new_dev);
    pop(1);
    return 0;
}

/* - currentdevice <device> */
int
zcurrentdevice(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gx_device *dev = gs_currentdevice(igs);
    gs_ref_memory_t *mem = (gs_ref_memory_t *) dev->memory;

    push(1);
    make_tav(op, t_device,
	     (mem == 0 ? avm_foreign : imemory_space(mem)) | a_all,
	     pdevice, dev);
    return 0;
}

/* <device> .devicename <string> */
private int
zdevicename(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const char *dname;

    check_read_type(*op, t_device);
    dname = op->value.pdevice->dname;
    make_const_string(op, avm_foreign | a_readonly, strlen(dname),
		      (const byte *)dname);
    return 0;
}

/* - .doneshowpage - */
private int
zdoneshowpage(i_ctx_t *i_ctx_p)
{
    gx_device *dev = gs_currentdevice(igs);
    gx_device *tdev = (*dev_proc(dev, get_page_device)) (dev);

    if (tdev != 0)
	tdev->ShowpageCount++;
    return 0;
}

/* - flushpage - */
int
zflushpage(i_ctx_t *i_ctx_p)
{
    return gs_flushpage(igs);
}

/* <device> <x> <y> <width> <max_height> <alpha?> <std_depth|null> <string> */
/*   .getbitsrect <height> <substring> */
private int
zgetbitsrect(i_ctx_t *i_ctx_p)
{	/*
	 * alpha? is 0 for no alpha, -1 for alpha first, 1 for alpha last.
	 * std_depth is null for native pixels, depth/component for
	 * standard color space.
	 */
    os_ptr op = osp;
    gx_device *dev;
    gs_int_rect rect;
    gs_get_bits_params_t params;
    int w, h;
    gs_get_bits_options_t options =
	GB_ALIGN_ANY | GB_RETURN_COPY | GB_OFFSET_0 | GB_RASTER_STANDARD |
	GB_PACKING_CHUNKY;
    int depth;
    uint raster;
    int num_rows;
    int code;

    check_read_type(op[-7], t_device);
    dev = op[-7].value.pdevice;
    check_int_leu(op[-6], dev->width);
    rect.p.x = op[-6].value.intval;
    check_int_leu(op[-5], dev->height);
    rect.p.y = op[-5].value.intval;
    check_int_leu(op[-4], dev->width);
    w = op[-4].value.intval;
    check_int_leu(op[-3], dev->height);
    h = op[-3].value.intval;
    check_type(op[-2], t_integer);
    /*
     * We use if/else rather than switch because the value is long,
     * which is not supported as a switch value in pre-ANSI C.
     */
    if (op[-2].value.intval == -1)
	options |= GB_ALPHA_FIRST;
    else if (op[-2].value.intval == 0)
	options |= GB_ALPHA_NONE;
    else if (op[-2].value.intval == 1)
	options |= GB_ALPHA_LAST;
    else
	return_error(e_rangecheck);
    if (r_has_type(op - 1, t_null)) {
	options |= GB_COLORS_NATIVE;
	depth = dev->color_info.depth;
    } else {
	static const gs_get_bits_options_t depths[17] = {
	    0, GB_DEPTH_1, GB_DEPTH_2, 0, GB_DEPTH_4, 0, 0, 0, GB_DEPTH_8,
	    0, 0, 0, GB_DEPTH_12, 0, 0, 0, GB_DEPTH_16
	};
	gs_get_bits_options_t depth_option;
	int std_depth;

	check_int_leu(op[-1], 16);
	std_depth = (int)op[-1].value.intval;
	depth_option = depths[std_depth];
	if (depth_option == 0)
	    return_error(e_rangecheck);
	options |= depth_option | GB_COLORS_NATIVE;
	depth = (dev->color_info.num_components +
		 (options & GB_ALPHA_NONE ? 0 : 1)) * std_depth;
    }
    if (w == 0)
	return_error(e_rangecheck);
    raster = (w * depth + 7) >> 3;
    check_write_type(*op, t_string);
    num_rows = r_size(op) / raster;
    h = min(h, num_rows);
    if (h == 0)
	return_error(e_rangecheck);
    rect.q.x = rect.p.x + w;
    rect.q.y = rect.p.y + h;
    params.options = options;
    params.data[0] = op->value.bytes;
    code = (*dev_proc(dev, get_bits_rectangle))(dev, &rect, &params, NULL);
    if (code < 0)
	return code;
    make_int(op - 7, h);
    op[-6] = *op;
    r_set_size(op - 6, h * raster);
    pop(6);
    return 0;
}

/* <int> .getdevice <device> */
private int
zgetdevice(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    const gx_device *dev;

    check_type(*op, t_integer);
    if (op->value.intval != (int)(op->value.intval))
	return_error(e_rangecheck);	/* won't fit in an int */
    dev = gs_getdevice((int)(op->value.intval));
    if (dev == 0)		/* index out of range */
	return_error(e_rangecheck);
    /* Device prototypes are read-only; */
    /* the cast is logically unnecessary. */
    make_tav(op, t_device, avm_foreign | a_readonly, pdevice,
	     (gx_device *) dev);
    return 0;
}

/* Common functionality of zgethardwareparms & zgetdeviceparams */
private int
zget_device_params(i_ctx_t *i_ctx_p, bool is_hardware)
{
    os_ptr op = osp;
    ref rkeys;
    gx_device *dev;
    stack_param_list list;
    int code;
    ref *pmark;

    check_read_type(op[-1], t_device);
    rkeys = *op;
    dev = op[-1].value.pdevice;
    pop(1);
    stack_param_list_write(&list, &o_stack, &rkeys, iimemory);
    code = gs_get_device_or_hardware_params(dev, (gs_param_list *) & list,
					    is_hardware);
    if (code < 0) {
	/* We have to put back the top argument. */
	if (list.count > 0)
	    ref_stack_pop(&o_stack, list.count * 2 - 1);
	else
	    ref_stack_push(&o_stack, 1);
	*osp = rkeys;
	return code;
    }
    pmark = ref_stack_index(&o_stack, list.count * 2);
    make_mark(pmark);
    return 0;
}
/* <device> <key_dict|null> .getdeviceparams <mark> <name> <value> ... */
private int
zgetdeviceparams(i_ctx_t *i_ctx_p)
{
    return zget_device_params(i_ctx_p, false);
}
/* <device> <key_dict|null> .gethardwareparams <mark> <name> <value> ... */
private int
zgethardwareparams(i_ctx_t *i_ctx_p)
{
    return zget_device_params(i_ctx_p, true);
}

/* <matrix> <width> <height> <palette> <word?> makewordimagedevice <device> */
private int
zmakewordimagedevice(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    os_ptr op1 = op - 1;
    gs_matrix imat;
    gx_device *new_dev;
    const byte *colors;
    int colors_size;
    int code;

    check_int_leu(op[-3], max_uint >> 1);	/* width */
    check_int_leu(op[-2], max_uint >> 1);	/* height */
    check_type(*op, t_boolean);
    if (r_has_type(op1, t_null)) {	/* true color */
	colors = 0;
	colors_size = -24;	/* 24-bit true color */
    } else if (r_has_type(op1, t_integer)) {
	/*
	 * We use if/else rather than switch because the value is long,
	 * which is not supported as a switch value in pre-ANSI C.
	 */
	if (op1->value.intval != 16 && op1->value.intval != 24 &&
	    op1->value.intval != 32
	    )
	    return_error(e_rangecheck);
	colors = 0;
	colors_size = -op1->value.intval;
    } else {
	check_type(*op1, t_string);	/* palette */
	if (r_size(op1) > 3 * 256)
	    return_error(e_rangecheck);
	colors = op1->value.bytes;
	colors_size = r_size(op1);
    }
    if ((code = read_matrix(imemory, op - 4, &imat)) < 0)
	return code;
    /* Everything OK, create device */
    code = gs_makewordimagedevice(&new_dev, &imat,
				  (int)op[-3].value.intval,
				  (int)op[-2].value.intval,
				  colors, colors_size,
				  op->value.boolval, true, imemory);
    if (code == 0) {
	new_dev->memory = imemory;
	make_tav(op - 4, t_device, imemory_space(iimemory) | a_all,
		 pdevice, new_dev);
	pop(4);
    }
    return code;
}

/* - nulldevice - */
/* Note that nulldevice clears the current pagedevice. */
private int
znulldevice(i_ctx_t *i_ctx_p)
{
    gs_nulldevice(igs);
    clear_pagedevice(istate);
    return 0;
}

/* <num_copies> <flush_bool> .outputpage - */
private int
zoutputpage(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_type(op[-1], t_integer);
    check_type(*op, t_boolean);
    code = gs_output_page(igs, (int)op[-1].value.intval,
			  op->value.boolval);
    if (code < 0)
	return code;
    pop(2);
    return 0;
}

/* <device> <policy_dict|null> <require_all> <mark> <name> <value> ... */
/*      .putdeviceparams */
/*   (on success) <device> <eraseflag> */
/*   (on failure) <device> <policy_dict|null> <require_all> <mark> */
/*       <name1> <error1> ... */
/* For a key that simply was not recognized, if require_all is true, */
/* the result will be an /undefined error; if require_all is false, */
/* the key will be ignored. */
/* Note that .putdeviceparams clears the current pagedevice. */
private int
zputdeviceparams(i_ctx_t *i_ctx_p)
{
    uint count = ref_stack_counttomark(&o_stack);
    ref *prequire_all;
    ref *ppolicy;
    ref *pdev;
    gx_device *dev;
    stack_param_list list;
    int code;
    int old_width, old_height;
    int i, dest;

    if (count == 0)
	return_error(e_unmatchedmark);
    prequire_all = ref_stack_index(&o_stack, count);
    ppolicy = ref_stack_index(&o_stack, count + 1);
    pdev = ref_stack_index(&o_stack, count + 2);
    if (pdev == 0)
	return_error(e_stackunderflow);
    check_type_only(*prequire_all, t_boolean);
    check_write_type_only(*pdev, t_device);
    dev = pdev->value.pdevice;
    code = stack_param_list_read(&list, &o_stack, 0, ppolicy,
				 prequire_all->value.boolval, iimemory);
    if (code < 0)
	return code;
    old_width = dev->width;
    old_height = dev->height;
    code = gs_putdeviceparams(dev, (gs_param_list *) & list);
    /* Check for names that were undefined or caused errors. */
    for (dest = count - 2, i = 0; i < count >> 1; i++)
	if (list.results[i] < 0) {
	    *ref_stack_index(&o_stack, dest) =
		*ref_stack_index(&o_stack, count - (i << 1) - 2);
	    gs_errorname(i_ctx_p, list.results[i],
			 ref_stack_index(&o_stack, dest - 1));
	    dest -= 2;
	}
    iparam_list_release(&list);
    if (code < 0) {		/* There were errors reported. */
	ref_stack_pop(&o_stack, dest + 1);
	return 0;
    }
    if (code > 0 || (code == 0 && (dev->width != old_width || dev->height != old_height))) {
	/*
	 * The device was open and is now closed, or its dimensions have
	 * changed.  If it was the current device, call setdevice to
	 * reinstall it and erase the page.
	 */
	/****** DOESN'T FIND ALL THE GSTATES THAT REFERENCE THE DEVICE. ******/
	if (gs_currentdevice(igs) == dev) {
	    bool was_open = dev->is_open;

	    code = gs_setdevice_no_erase(igs, dev);
	    /* If the device wasn't closed, setdevice won't erase the page. */
	    if (was_open && code >= 0)
		code = 1;
	}
    }
    if (code < 0)
	return code;
    ref_stack_pop(&o_stack, count + 1);
    make_bool(osp, code);
    clear_pagedevice(istate);
    return 0;
}

/* <device> .setdevice <eraseflag> */
/* Note that .setdevice clears the current pagedevice. */
int
zsetdevice(i_ctx_t *i_ctx_p)
{
    gx_device *dev = gs_currentdevice(igs);
    os_ptr op = osp;
    int code;

    check_write_type(*op, t_device);
    if (dev->LockSafetyParams) {	  /* do additional checking if locked  */
        if(op->value.pdevice != dev) 	  /* don't allow a different device    */
	    return_error(e_invalidaccess);
    }
    code = gs_setdevice_no_erase(igs, op->value.pdevice);
    if (code < 0)
	return code;
    make_bool(op, code != 0);	/* erase page if 1 */
    clear_pagedevice(istate);
    return code;
}

/* ------ Initialization procedure ------ */

const op_def zdevice_op_defs[] =
{
    {"1.copydevice2", zcopydevice2},
    {"0currentdevice", zcurrentdevice},
    {"1.devicename", zdevicename},
    {"0.doneshowpage", zdoneshowpage},
    {"0flushpage", zflushpage},
    {"7.getbitsrect", zgetbitsrect},
    {"1.getdevice", zgetdevice},
    {"2.getdeviceparams", zgetdeviceparams},
    {"2.gethardwareparams", zgethardwareparams},
    {"5makewordimagedevice", zmakewordimagedevice},
    {"0nulldevice", znulldevice},
    {"2.outputpage", zoutputpage},
    {"3.putdeviceparams", zputdeviceparams},
    {"1.setdevice", zsetdevice},
    op_def_end(0)
};
