/* Copyright (C) 1992, 1995, 1996, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: main.h,v 1.1 2000/03/09 08:40:44 lpd Exp $ */
/* Backward-compatible interface to gsmain.c */

#ifndef main_INCLUDED
#  define main_INCLUDED

#include "imain.h"
#include "iminst.h"

/*
 * This file adds to imain.h some backward-compatible procedures and
 * data elements that assume there is only a single instance of
 * the interpreter.
 */

/* ================ Data elements ================ */

/* Clients should never access these directly. */

#define gs_user_errors (gs_main_instance_default()->user_errors)
#define gs_lib_path (gs_main_instance_default()->lib_path)
/* gs_lib_paths removed in release 3.65 */
/* gs_lib_env_path removed in release 3.65 */

/* ================ Exported procedures from gsmain.c ================ */

/* ---------------- Initialization ---------------- */

#define gs_init0(in, out, err, mlp)\
  gs_main_init0(gs_main_instance_default(), in, out, err, mlp)

#define gs_init1()\
  gs_main_init1(gs_main_instance_default())

#define gs_init2()\
  gs_main_init2(gs_main_instance_default())

#define gs_add_lib_path(path)\
  gs_main_add_lib_path(gs_main_instance_default(), path)

#define gs_set_lib_paths()\
  gs_main_set_lib_paths(gs_main_instance_default())

#define gs_lib_open(fname, pfile)\
  gs_main_lib_open(gs_main_instance_default(), fname, pfile)

/* ---------------- Execution ---------------- */

#define gs_run_file(fn, ue, pec, peo)\
  gs_main_run_file(gs_main_instance_default(), fn, ue, pec, peo)

#define gs_run_string(str, ue, pec, peo)\
  gs_main_run_string(gs_main_instance_default(), str, ue, pec, peo)

#define gs_run_string_with_length(str, len, ue, pec, peo)\
  gs_main_run_string_with_length(gs_main_instance_default(),\
				 str, len, ue, pec, peo)

#define gs_run_file_open(fn, pfref)\
  gs_main_run_file_open(gs_main_instance_default(), fn, pfref)

#define gs_run_string_begin(ue, pec, peo)\
  gs_main_run_string_begin(gs_main_instance_default(), ue, pec, peo)

#define gs_run_string_continue(str, len, ue, pec, peo)\
  gs_main_run_string_continue(gs_main_instance_default(),\
			      str, len, ue, pec, peo)

#define gs_run_string_end(ue, pec, peo)\
  gs_main_run_string_end(gs_main_instance_default(), ue, pec, peo)

/* ---------------- Debugging ---------------- */

/*
 * We should have the following definition:

#define gs_debug_dump_stack(code, peo)\
  gs_main_dump_stack(gs_main_instance_default(), code, peo)

 * but we make it a procedure instead so it can be called from debuggers.
 */
void gs_debug_dump_stack(P2(int code, ref * perror_object));

/* ---------------- Termination ---------------- */

#define gs_finit(status, code)\
  gs_main_finit(gs_main_instance_default(), status, code)

#endif /* main_INCLUDED */
