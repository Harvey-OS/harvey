/* Copyright (C) 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* ziodev2.c */
/* (Level 2) IODevice operators */
#include "string_.h"
#include "ghost.h"
#include "gp.h"
#include "errors.h"
#include "oper.h"
#include "stream.h"
#include "gxiodev.h"
#include "files.h"				/* for file_open_stream */
#include "iparam.h"
#include "iutil2.h"
#include "store.h"

/* ------ %null% ------ */

/* This represents the null output file. */
private iodev_proc_open_device(null_open);
gx_io_device gs_iodev_null =
  { "%null%", "FileSystem",
     { iodev_no_init, null_open, iodev_no_open_file,
       iodev_os_fopen, iodev_os_fclose,
       iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,
       iodev_no_enumerate_files, NULL, NULL,
       iodev_no_get_params, iodev_no_put_params
     }
  };

private int
null_open(gx_io_device *iodev, const char *access, stream **ps,
  gs_memory_t *mem)
{	if ( !streq1(access, 'w') )
		return_error(e_invalidfileaccess);
	return file_open_stream(gp_null_file_name,
				strlen(gp_null_file_name),
				access, 256 /* arbitrary */, ps,
				iodev->procs.fopen);
}

/* ------ %ram% ------ */

/* This is an IODevice with no interesting parameters for the moment. */
gx_io_device gs_iodev_ram =
  { "%ram%", "Special",
     { iodev_no_init, iodev_no_open_device, iodev_no_open_file,
       iodev_no_fopen, iodev_no_fclose,
       iodev_no_delete_file, iodev_no_rename_file, iodev_no_file_status,
       iodev_no_enumerate_files, NULL, NULL,
       iodev_no_get_params, iodev_no_put_params
     }
  };

/* ------ Operators ------ */

/* <iodevice> .getdevparams <mark> <name> <value> ... */
int
zgetdevparams(os_ptr op)
{	gx_io_device *iodev;
	stack_param_list list;
	gs_param_string ts;
	int code;
	ref *pmark;
	check_read_type(*op, t_string);
	iodev = gs_findiodevice(op->value.bytes, r_size(op));
	if ( iodev == 0 )
	  return_error(e_undefinedfilename);
	stack_param_list_write(&list, &o_stack, NULL);
#define plist ((gs_param_list *)&list)
	param_string_from_string(ts, iodev->dtype);
	if ( (code = param_write_name(plist, "Type", &ts)) < 0 ||
	     (code = (*iodev->procs.get_params)(iodev, plist)) < 0
	   )
	{	ref_stack_pop(&o_stack, list.count * 2);
		return code;
	}
#undef plist
	pmark = ref_stack_index(&o_stack, list.count * 2);
	make_mark(pmark);
	return 0;
}

/* <mark> <name> <value> ... <iodevice> .putdevparams */
int
zputdevparams(os_ptr op)
{	gx_io_device *iodev;
	stack_param_list list;
	int code;
	check_read_type(*op, t_string);
	iodev = gs_findiodevice(op->value.bytes, r_size(op));
	if ( iodev == 0 )
	  return_error(e_undefinedfilename);
	code = stack_param_list_read(&list, &o_stack, 1, NULL, false);
	if ( code < 0 )
	  return code;
#define plist ((gs_param_list *)&list)
	code = param_check_password(plist, &SystemParamsPassword);
	if ( code != 0 )
	  return_error(code < 0 ? code : e_invalidaccess);
	code = (*iodev->procs.put_params)(iodev, plist);
	if ( code < 0 )
	  return code;
#undef plist
	ref_stack_pop(&o_stack, list.count * 2 + 2);
	return 0;
}

/* ------ Initialization procedure ------ */

BEGIN_OP_DEFS(ziodev2_l2_op_defs) {
		op_def_begin_level2(),
	{"1.getdevparams", zgetdevparams},
	{"2.putdevparams", zputdevparams},
END_OP_DEFS(0) }
