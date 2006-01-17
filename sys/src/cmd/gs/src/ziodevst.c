/* Copyright (C) 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: ziodevst.c,v 1.1 2003/02/18 19:57:16 igor Exp $ */
/* %static% IODevice implementation */
#include "ghost.h"
#include "gserrors.h"
#include "gsstruct.h"
#include "gxiodev.h"
#include "istruct.h"
#include "idict.h"
#include "iconf.h"
#include "oper.h"
#include "store.h"
#include "stream.h"
#include "files.h"
#include "string_.h"
#include "memory_.h"



typedef struct iostatic_state_s {
    ref data;
} iostatic_state;

private 
CLEAR_MARKS_PROC(iostatic_state_clear_marks)
{   iostatic_state *const pptr = vptr;

    r_clear_attrs(&pptr->data, l_mark);
}
private 
ENUM_PTRS_WITH(iostatic_state_enum_ptrs, iostatic_state *pptr) return 0;
case 0:
ENUM_RETURN_REF(&pptr->data);
ENUM_PTRS_END
private RELOC_PTRS_WITH(iostatic_state_reloc_ptrs, iostatic_state *pptr);
RELOC_REF_VAR(pptr->data);
r_clear_attrs(&pptr->data, l_mark);
RELOC_PTRS_END

gs_private_st_complex_only(st_iostatic_state, iostatic_state,\
    "iostatic_state", iostatic_state_clear_marks, iostatic_state_enum_ptrs, 
    iostatic_state_reloc_ptrs, 0);

const char iodev_dtype_static[] = "Special";

private int
iostatic_init(gx_io_device * iodev, gs_memory_t * mem)
{
    iostatic_state *pstate = gs_alloc_struct(mem, iostatic_state, &st_iostatic_state,
			   			"iostatic_init");
    if (!pstate)
        return gs_error_VMerror;
    make_null(&pstate->data);
    iodev->state = pstate;
    return 0;
}

private int
iostatic_open_file(gx_io_device * iodev, const char *fname, uint namelen,
		   const char *access, stream ** ps, gs_memory_t * mem)
{
    const char *cat_name = fname + 1, *inst_name;
    int cat_name_len, inst_name_len, code;
    iostatic_state *state = (iostatic_state *)iodev->state;
    char buf[30];
    ref *cat, *inst, *beg_pos, *end_pos, file;

    if (fname[0] != '/')
	return_error(gs_error_undefinedfilename);
    inst_name = strchr(cat_name, '/');
    if (inst_name == NULL)
	return_error(gs_error_undefinedfilename);
    cat_name_len = inst_name - cat_name;
    inst_name++;
    inst_name_len = fname + namelen - inst_name;
    if (cat_name_len > sizeof(buf) - 1 || inst_name_len > sizeof(buf) - 1) {
	/* The operator 'file' doesn't allow rangecheck here. */
	return_error(gs_error_undefinedfilename);
    }
    memcpy(buf, cat_name, cat_name_len);
    buf[cat_name_len] = 0;
    code = dict_find_string(&state->data, buf, &cat);
    if (code < 0)
	return code;
    if (code == 0 || r_type(cat) != t_dictionary)
	return_error(gs_error_unregistered); /* Wrong gs_resst.ps . */
    memcpy(buf, inst_name, inst_name_len);
    buf[inst_name_len] = 0;
    code = dict_find_string(cat, buf, &inst);
    if (code < 0)
	return code;
    if (code == 0 || r_type(inst) != t_dictionary)
	return_error(gs_error_unregistered); /* Wrong gs_resst.ps . */
    code = dict_find_string(inst, "StaticFilePos", &beg_pos);
    if (code < 0)
	return code;
    if (code == 0 || r_type(beg_pos) != t_integer)
	return_error(gs_error_unregistered); /* Wrong gs_resst.ps . */
    code = dict_find_string(inst, "StaticFileEnd", &end_pos);
    if (code < 0)
	return code;
    if (code == 0 || r_type(end_pos) != t_integer)
	return_error(gs_error_unregistered); /* Wrong gs_resst.ps . */
    code = file_read_string(gs_init_string + beg_pos->value.intval,
			    end_pos->value.intval - beg_pos->value.intval, 
			    &file, (gs_ref_memory_t *)mem);
    if (code < 0)
	return code;
    *ps = file.value.pfile;
    return 0;
}

const gx_io_device gs_iodev_static = {
    "%static%", iodev_dtype_static,
	{ iostatic_init, iodev_no_open_device, iostatic_open_file,
	  iodev_no_fopen, iodev_no_fclose,
	  iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,
	  iodev_no_enumerate_files, NULL, NULL,
	  iodev_no_get_params, iodev_no_put_params
	}
};

/* <dict> .setup_io_static - */
private int
zsetup_io_static(i_ctx_t *i_ctx_p)
{
    os_ptr op = osp;
    gx_io_device *piodev;

    check_read_type(*op, t_dictionary);

    piodev = gs_findiodevice((const byte *)"%static%", 8);
    if (piodev == NULL)
	return_error(gs_error_unregistered); /* Must not happen. */
    ref_assign_new(&((iostatic_state *)piodev->state)->data, op);
    pop(1);
    return 0;
}

const op_def ziodevst_op_defs[] =
{
    {"0.setup_io_static", zsetup_io_static},
    op_def_end(0)
};


 


