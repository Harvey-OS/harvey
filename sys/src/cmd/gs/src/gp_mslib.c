/* Copyright (C) 1997 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gp_mslib.c,v 1.1 2000/03/09 08:40:41 lpd Exp $ */
/*
 * Microsoft Windows 3.n platform support for Graphics Library
 *
 * This file just stubs out a few references from gp_mswin.c, where the
 * real action is. This was done to avoid restructuring Windows support,
 * though that would be the right thing to do.
 *
 * Created 6/25/97 by JD
 */

#include <setjmp.h>

/// Export dummy longjmp environment
jmp_buf gsdll_env;


/// Export dummy interpreter status
int gs_exit_status = 0;


/// Dummy callback routine & export pointer to it
int
gsdll_callback(int a, char *b, unsigned long c)
{
    return 0;
}

int (*pgsdll_callback) () = gsdll_callback;
