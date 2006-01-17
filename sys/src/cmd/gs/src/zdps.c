/* Copyright (C) 1997, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: zdps.c,v 1.8 2004/08/04 19:36:13 stefan Exp $ */
/* Display PostScript extensions */
#include "ghost.h"
#include "oper.h"
#include "gsstate.h"
#include "gsdps.h"
#include "gsimage.h"
#include "gsiparm2.h"
#include "gxalloc.h"		/* for names_array in allocator */
#include "gxfixed.h"		/* for gxpath.h */
#include "gxpath.h"
#include "btoken.h"		/* for user_names_p */
#include "iddict.h"
#include "idparam.h"
#include "igstate.h"
#include "iimage2.h"
#include "iname.h"
#include "store.h"

/* Import the procedure for constructing user paths. */
extern int make_upath(i_ctx_t *, ref *, const gs_state *, gx_path *, bool);

/* ------ Graphics state ------ */

/* <screen_index> <x> <y> .setscreenphase - */
private int
zsetscreenphase(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;
    long x, y;

    check_type(op[-2], t_integer);
    check_type(op[-1], t_integer);
    check_type(*op, t_integer);
    x = op[-1].value.intval;
    y = op->value.intval;
    if (x != (int)x || y != (int)y ||
	op[-2].value.intval < -1 ||
	op[-2].value.intval >= gs_color_select_count
	)
	return_error(e_rangecheck);
    code = gs_setscreenphase(igs, (int)x, (int)y,
			     (gs_color_select_t) op[-2].value.intval);
    if (code >= 0)
	pop(3);
    return code;
}

/* <screen_index> .currentscreenphase <x> <y> */
private int
zcurrentscreenphase(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gs_int_point phase;
    int code;

    check_type(*op, t_integer);
    if (op->value.intval < -1 ||
	op->value.intval >= gs_color_select_count
	)
	return_error(e_rangecheck);
    code = gs_currentscreenphase(igs, &phase,
				 (gs_color_select_t)op->value.intval);
    if (code < 0)
	return code;
    push(1);
    make_int(op - 1, phase.x);
    make_int(op, phase.y);
    return 0;
}

/* ------ Device-source images ------ */

/* <dict> .image2 - */
private int
zimage2(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    int code;

    check_type(*op, t_dictionary);
    check_dict_read(*op);
    {
	gs_image2_t image;
	ref *pDataSource;

	gs_image2_t_init(&image);
	if ((code = dict_matrix_param(imemory, op, "ImageMatrix",
				      &image.ImageMatrix)) < 0 ||
	    (code = dict_find_string(op, "DataSource", &pDataSource)) < 0 ||
	    (code = dict_float_param(op, "XOrigin", 0.0,
				     &image.XOrigin)) != 0 ||
	    (code = dict_float_param(op, "YOrigin", 0.0,
				     &image.YOrigin)) != 0 ||
	    (code = dict_float_param(op, "Width", 0.0,
				     &image.Width)) != 0 ||
	    image.Width <= 0 ||
	    (code = dict_float_param(op, "Height", 0.0,
				     &image.Height)) != 0 ||
	    image.Height <= 0 ||
	    (code = dict_bool_param(op, "PixelCopy", false,
				    &image.PixelCopy)) < 0
	    )
	    return (code < 0 ? code : gs_note_error(e_rangecheck));
	check_stype(*pDataSource, st_igstate_obj);
	image.DataSource = igstate_ptr(pDataSource);
	{
	    ref *ignoref;

	    if (dict_find_string(op, "UnpaintedPath", &ignoref) > 0) {
		check_dict_write(*op);
		image.UnpaintedPath = gx_path_alloc(imemory,
						    ".image2 UnpaintedPath");
		if (image.UnpaintedPath == 0)
		    return_error(e_VMerror);
	    } else
		image.UnpaintedPath = 0;
	}
	code = process_non_source_image(i_ctx_p,
					(const gs_image_common_t *)&image,
					".image2");
	if (image.UnpaintedPath) {
	    ref rupath;

	    if (code < 0)
		return code;
	    if (gx_path_is_null(image.UnpaintedPath))
		make_null(&rupath);
	    else
		code = make_upath(i_ctx_p, &rupath, igs, image.UnpaintedPath,
				  false);
	    gx_path_free(image.UnpaintedPath, ".image2 UnpaintedPath");
	    if (code < 0)
		return code;
	    code = idict_put_string(op, "UnpaintedPath", &rupath);
	}
    }
    if (code >= 0)
	pop(1);
    return code;
}

/* ------ View clipping ------ */

/* - viewclip - */
private int
zviewclip(i_ctx_t *i_ctx_p)
{
    return gs_viewclip(igs);
}

/* - eoviewclip - */
private int
zeoviewclip(i_ctx_t *i_ctx_p)
{
    return gs_eoviewclip(igs);
}

/* - initviewclip - */
private int
zinitviewclip(i_ctx_t *i_ctx_p)
{
    return gs_initviewclip(igs);
}

/* - viewclippath - */
private int
zviewclippath(i_ctx_t *i_ctx_p)
{
    return gs_viewclippath(igs);
}

/* ------ User names ------ */

/* <index> <name> defineusername - */
private int
zdefineusername(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    ref uname;

    check_int_ltu(op[-1], max_array_size);
    check_type(*op, t_name);
    if (user_names_p == 0) {
	int code = create_names_array(&user_names_p, imemory_local,
				      "defineusername");

	if (code < 0)
	    return code;
    }
    if (array_get(imemory, user_names_p, 
		  op[-1].value.intval, &uname) >= 0) {
	switch (r_type(&uname)) {
	    case t_null:
		break;
	    case t_name:
		if (name_eq(&uname, op))
		    goto ret;
		/* falls through */
	    default:
		return_error(e_invalidaccess);
	}
    } else {			/* Expand the array. */
	ref new_array;
	uint old_size = r_size(user_names_p);
	uint new_size = (uint) op[-1].value.intval + 1;

	if (new_size < 100)
	    new_size = 100;
	else if (new_size > max_array_size / 2)
	    new_size = max_array_size;
	else if (new_size >> 1 < old_size)
	    new_size = (old_size > max_array_size / 2 ? max_array_size :
			old_size << 1);
	else
	    new_size <<= 1;
	/*
	 * The user name array is allocated in stable local VM,
	 * because it must be immune to save/restore.
	 */
	{
	    gs_ref_memory_t *slmem =
		(gs_ref_memory_t *)gs_memory_stable(imemory_local);
	    int code;

	    code = gs_alloc_ref_array(slmem, &new_array, a_all, new_size,
				      "defineusername(new)");
	    if (code < 0)
		return code;
	    refcpy_to_new(new_array.value.refs, user_names_p->value.refs,
			  old_size, idmemory);
	    refset_null(new_array.value.refs + old_size,
			new_size - old_size);
	    if (old_size)
		gs_free_ref_array(slmem, user_names_p, "defineusername(old)");
	}
	ref_assign(user_names_p, &new_array);
    }
    ref_assign(user_names_p->value.refs + op[-1].value.intval, op);
  ret:
    pop(2);
    return 0;
}

/* ------ Initialization procedure ------ */

const op_def zdps_op_defs[] =
{
		/* Graphics state */
    {"1.currentscreenphase", zcurrentscreenphase},
    {"3.setscreenphase", zsetscreenphase},
		/* Device-source images */
    {"1.image2", zimage2},
		/* View clipping */
    {"0eoviewclip", zeoviewclip},
    {"0initviewclip", zinitviewclip},
    {"0viewclip", zviewclip},
    {"0viewclippath", zviewclippath},
		/* User names */
    {"2defineusername", zdefineusername},
    op_def_end(0)
};
