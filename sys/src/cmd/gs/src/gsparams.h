/* Copyright (C) 1998 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsparams.h,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Serializer/expander for gs_parm_list's */

#ifndef gsparams_INCLUDED
#  define gsparams_INCLUDED

/* Initial version 2/1/98 by John Desrosiers (soho@crl.com) */
/* 8/8/98 L. Peter Deutsch (ghost@aladdin.com) Completely redesigned
   to use stream rather than buffer API (but retained former API for
   compatibility as well). */

#include "stream.h"
#include "gsparam.h"

#if 0	/****************/

/* ------ Future interface, implemented in gsparam2.c ------ */

/*
 * Serialize the contents of a gs_param_list, including sub-collections,
 * onto a stream.  The list must be in READ mode.
 */
int gs_param_list_puts(P2(stream *dest, gs_param_list *list));

/*
 * Unserialize a parameter list, including sub-collections, from a stream.
 * The list must be in WRITE mode.
 */
int gs_param_list_gets(P3(stream *src, gs_param_list *list, gs_memory_t *mem));

#else	/****************/

/* ------ Present interface, implemented in gsparams.c ------ */

/*
 * Serialize a parameter list into a buffer.  Return the actual number
 * of bytes required to store the list, or a negative error code.
 * The list was stored successfully iff the return value is positive and
 * less than or equal to the buffer size.  Note that the buffer may be
 * NULL, in which case nothing is stored (but the size is still returned).
 */
int gs_param_list_serialize(P3(gs_param_list *list, byte *buf, int buf_size));

/*
 * Unserialize a parameter list from a buffer.  Return the actual number
 * of bytes occupied by the list, or a negative error code.  The buffer
 * must be void * aligned.
 */
int gs_param_list_unserialize(P2(gs_param_list *list, const byte *buf));

#endif	/****************/

#endif /* gsparams_INCLUDED */
