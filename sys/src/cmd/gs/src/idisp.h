/* Copyright (C) 2001, Ghostgum Software Pty Ltd.  All rights reserved.

   This file is part of AFPL Ghostscript.

   AFPL Ghostscript is distributed with NO WARRANTY OF ANY KIND.  No author
   or distributor accepts any responsibility for the consequences of using it,
   or for whether it serves any particular purpose or works at all, unless he
   or she says so in writing.  Refer to the Aladdin Ghostscript Free Public
   License (the "License") for full details.

   Every copy of AFPL Ghostscript must include a copy of the License,
   normally in a plain ASCII text file named PUBLIC.  The License grants you
   the right to copy, modify and redistribute AFPL Ghostscript, but only
   under certain conditions described in the License.  Among other things, the
   License requires that the copyright notice and this notice be preserved on
   all copies.
 */

/* $Id: idisp.h,v 1.1 2001/03/13 07:09:29 ghostgum Exp $ */

#ifndef display_callback_DEFINED
# define display_callback_DEFINED
typedef struct display_callback_s display_callback;
#endif

/* Called from imain.c to set the display callback in the device instance. */
int display_set_callback(gs_main_instance *minst, display_callback *callback);

