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

/*$Id: iht.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* Procedures exported by zht.c for zht1.c and zht2.c */

#ifndef iht_INCLUDED
#  define iht_INCLUDED

int zscreen_params(P2(os_ptr op, gs_screen_halftone * phs));

int zscreen_enum_init(P7(i_ctx_t *i_ctx_p, const gx_ht_order * porder,
			 gs_screen_halftone * phs, ref * pproc, int npop,
			 op_proc_t finish_proc, gs_memory_t * mem));

#endif /* iht_INCLUDED */
