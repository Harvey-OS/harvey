/* Copyright (C) 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* istream.h */
/* Interpreter-level stream procedures */
/* Requires scommon.h, ostack.h */

/* Procedures exported by zfproc.c */
	/* for zfilter.c - procedure stream initialization */
int sread_proc(P2(ref *, stream **));
int swrite_proc(P2(ref *, stream **));
	/* for interp.c, zfileio.c, zpaint.c - handle a procedure */
	/* callback or an interrupt */
int s_handle_read_exception(P4(int, const ref *, const ref *,
  int (*)(P1(os_ptr))));
int s_handle_write_exception(P4(int, const ref *, const ref *,
  int (*)(P1(os_ptr))));
