/* Copyright (C) 2001, Ghostgum Software Pty Ltd.  All rights reserved.

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

/* idisp.c */
/* $Id: idisp.c,v 1.7 2004/07/03 10:51:14 ghostgum Exp $ */

/*
 * This allows the interpreter to set the callback structure 
 * of the "display" device.  This must set at the end of 
 * initialization and before the device is used.
 * This is harmless if the 'display' device is not included.
 * If gsapi_set_display_callback() is not called, this code
 * won't even be used.
 */

#include "stdio_.h"
#include "stdpre.h"
#include "iapi.h"
#include "ghost.h"
#include "gp.h"
#include "gscdefs.h"
#include "gsmemory.h"
#include "gstypes.h"
#include "gsdevice.h"
#include "iref.h"
#include "imain.h"
#include "iminst.h"
#include "oper.h"
#include "ostack.h"
#include "gx.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "idisp.h"
#include "gdevdevn.h"
#include "gsequivc.h"
#include "gdevdsp.h"
#include "gdevdsp2.h"

int
display_set_callback(gs_main_instance *minst, display_callback *callback)
{
    i_ctx_t *i_ctx_p = minst->i_ctx_p;
    bool was_open;
    int code;
    int exit_code = 0;
    os_ptr op = osp;
    gx_device *dev;
    gx_device_display *ddev;

    /* If display device exists, copy prototype if needed and return
     *  device true
     * If it doesn't exist, return
     *  false
     */
    const char getdisplay[] = 
      "devicedict /display known dup { /display finddevice exch } if";
    code = gs_main_run_string(minst, getdisplay, 0, &exit_code, 
	&minst->error_object);
    if (code < 0)
       return code;

    op = osp;
    check_type(*op, t_boolean);
    if (op->value.boolval) {
	/* display device was included in Ghostscript so we need
	 * to set the callback structure pointer within it.
	 * If the device is already open, close it before
	 * setting callback, then reopen it.
	 */
	check_read_type(op[-1], t_device);
	dev = op[-1].value.pdevice;
	
	was_open = dev->is_open;
	if (was_open) {
	    code = gs_closedevice(dev);
	    if (code < 0)
		return_error(code);
	}

	ddev = (gx_device_display *) dev;
	ddev->callback = callback;

	if (was_open) {
	    code = gs_opendevice(dev);
	    if (code < 0) {
		dprintf("**** Unable to open the display device, quitting.\n");
		return_error(code);
	    }
	}
	pop(1);	/* device */
    }
    pop(1);	/* boolean */
    return 0;
}


