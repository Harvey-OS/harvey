/* Copyright (C) 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: iinit.h,v 1.1 2000/03/09 08:40:43 lpd Exp $ */
/* (Internal) interface to iinit.c */

#ifndef iinit_INCLUDED
#  define iinit_INCLUDED

/*
 * Declare initialization procedures exported by iinit.c for imain.c.
 * These must be executed in the order they are declared below.
 */
int obj_init(P2(i_ctx_t **, gs_dual_memory_t *));
int zop_init(P1(i_ctx_t *));
int op_init(P1(i_ctx_t *));

/*
 * Test whether there are any Level 2 operators in the executable.
 * (This is different from the language level in which the interpreter is
 * actually running: it is only tested during initialization.)
 */
bool gs_have_level2(P0());

#endif /* iinit_INCLUDED */
