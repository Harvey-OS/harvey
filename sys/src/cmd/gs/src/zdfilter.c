/*
  Copyright (C) 2001 artofcode LLC.
  
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

  Author: Raph Levien <raph@artofcode.com>
*/
/*$Id: zdfilter.c,v 1.2 2001/04/01 00:30:41 raph Exp $ */
/* PostScript operators for managing the device filter stack */

/* We probably don't need all of these, they were copied from zdevice.c. */
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
#include "gsdfilt.h"
#include "gdevp14.h"

#ifdef DFILTER_TEST
private int
/* - .pushtestdevicefilter - */
zpushtestdevicefilter(i_ctx_t *i_ctx_p)
{
    gs_device_filter_t *df;
    int code;
    gs_memory_t *mem = gs_memory_stable(imemory);

    code = gs_test_device_filter(&df, mem);
    if (code < 0)
	return code;
    code = gs_push_device_filter(mem, igs, df);
    return code;
}
#endif

private int
/* depth .pushpdf14devicefilter - */
zpushpdf14devicefilter(i_ctx_t *i_ctx_p)
{
    gs_device_filter_t *df;
    int code;
    gs_memory_t *mem = gs_memory_stable(imemory);
    os_ptr op = osp;

    check_type(*op, t_integer);
    code = gs_pdf14_device_filter(&df, op->value.intval, mem);
    if (code < 0)
	return code;
    code = gs_push_device_filter(mem, igs, df); 
    if (code < 0)
	return code;
    pop(1);
    return 0;
}

/* - .popdevicefilter - */
private int
zpopdevicefilter(i_ctx_t *i_ctx_p)
{
    gs_memory_t *mem = gs_memory_stable(imemory);

    return gs_pop_device_filter(mem, igs);
}

const op_def zdfilter_op_defs[] =
{
#ifdef DFILTER_TEST
    {"0.pushtestdevicefilter", zpushtestdevicefilter},
#endif
    {"1.pushpdf14devicefilter", zpushpdf14devicefilter},
    {"0.popdevicefilter", zpopdevicefilter},
    op_def_end(0)
};
