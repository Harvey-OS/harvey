/* Copyright (C) 1995, 1996, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: imain.h,v 1.9 2004/08/04 23:33:29 stefan Exp $ */
/* Interface to imain.c */
/* Requires <stdio.h>, stdpre.h, gsmemory.h, gstypes.h, iref.h */

#ifndef imain_INCLUDED
#  define imain_INCLUDED

#include "gsexit.h"		/* exported by imain.c */

/*
 * This file defines the intended API between client front ends
 * (such as imainarg.c, the command-line-driven front end)
 * and imain.c, which provides top-level control of the interpreter.
 */

/* ================ Types ================ */

/*
 * Currently, the interpreter has a lot of static variables, but
 * eventually it will have none, so that clients will be able to make
 * multiple instances of it.  In anticipation of this, many of the
 * top-level API calls take an interpreter instance (gs_main_instance *)
 * as their first argument.
 */
#ifndef gs_main_instance_DEFINED
#  define gs_main_instance_DEFINED
typedef struct gs_main_instance_s gs_main_instance;
#endif

/* ================ Exported procedures from imain.c ================ */

/* get minst from memory is a hack to allow parts of the system 
 * to reach minst, slightly better than a global
 */
gs_main_instance* get_minst_from_memory(const gs_memory_t *mem);

/* ---------------- Instance creation ---------------- */

/*
 * NB: multiple instances are not supported yet
 * 
 * // add usage documentation
 */
gs_main_instance *gs_main_alloc_instance(gs_memory_t *); 

/* ---------------- Initialization ---------------- */

/*
 * The interpreter requires three initialization steps, called init0,
 * init1, and init2.  These steps must be done in that order, but
 * init1 may be omitted.
 */

/*
 * init0 records the files to be used for stdio, and initializes the
 * graphics library, the file search paths, and other instance data.
 */
int gs_main_init0(gs_main_instance *minst, FILE *in, FILE *out, FILE *err,
		  int max_lib_paths);

/*
 * init1 initializes the memory manager and other internal data
 * structures such as the name table, the token scanner tables,
 * dictionaries such as systemdict, and the interpreter stacks.
 */
int gs_main_init1(gs_main_instance * minst);

/*
 * init2 finishes preparing the interpreter for use by running
 * initialization files with PostScript procedure definitions.
 */
int gs_main_init2(gs_main_instance * minst);

/*
 * The runlibfile operator uses a search path, as described in
 * Use.htm, for looking up file names.  Each interpreter instance has
 * its own search path.  The following call adds a directory or set of
 * directories to the search path; it is equivalent to the -I command
 * line switch.  It may be called any time after init0.
 */
int gs_main_add_lib_path(gs_main_instance * minst, const char *path);

/*
 * Under certain internal conditions, the search path may temporarily
 * be in an inconsistent state; gs_main_set_lib_paths takes care of
 * this.  Clients should never need to call this procedure, and
 * eventually it may be removed.
 */
int gs_main_set_lib_paths(gs_main_instance * minst);

/*
 * Open a PostScript file using the search path.  Clients should
 * never need to call this procedure, since gs_main_run_file opens the
 * file itself, and eventually the procedure may be removed.
 */
int gs_main_lib_open(gs_main_instance * minst, const char *fname,
		     ref * pfile);

/*
 * Here we summarize the C API calls that correspond to some of the
 * most common command line switches documented in Use.htm, to help
 * clients who are familiar with the command line and are starting to
 * use the API.
 *
 *      -d/D, -s/S (for setting device parameters like OutputFile)
 *              Use the C API for device parameters documented near the
 *              end of gsparam.h.
 *
 *      -d/D (for setting Boolean parameters like NOPAUSE)
 *              { ref vtrue;
 *                make_true(&vtrue);
 *                dict_put_string(systemdict, "NOPAUSE", &vtrue);
 *              }
 *      -I
 *              Use gs_main_add_lib_path, documented above.
 *
 *      -A, -A-
 *              Set gs_debug['@'] = 1 or 0 respectively.
 *      -E, -E-
 *              Set gs_debug['#'] = 1 or 0 respectively.
 *      -Z..., -Z-...
 *              Set gs_debug[c] = 1 or 0 respectively for each character
 *              c in the string.
 */

/* ---------------- Execution ---------------- */

/*
 * After initializing the interpreter, clients may pass it files or
 * strings to be interpreted.  There are four ways to do this:
 *      - Pass a file name (gs_main_run_file);
 *      - Pass a C string (gs_main_run_string);
 *      - Pass a string defined by pointer and length
 *              (gs_main_run_string_with_length);
 *      - Pass strings piece-by-piece
 *              (gs_main_run_string_begin/continue/end).
 *
 * The value returned by the first three of these calls is
 * 0 if the interpreter ran to completion, e_Quit for a normal quit,
 * or e_Fatal for a non-zero quit or a fatal error.
 * e_Fatal stores the exit code in the third argument.
 * The str argument of gs_main_run_string[_with_length] must be allocated
 * in non-garbage-collectable space (e.g., by malloc or gs_malloc,
 * or statically).
 */
int gs_main_run_file(gs_main_instance * minst, const char *fname,
		     int user_errors, int *pexit_code,
		     ref * perror_object);
int gs_main_run_string(gs_main_instance * minst, const char *str,
		       int user_errors, int *pexit_code,
		       ref * perror_object);
int gs_main_run_string_with_length(gs_main_instance * minst,
				   const char *str, uint length,
				   int user_errors, int *pexit_code,
				   ref * perror_object);

/*
 * Open the file for gs_main_run_file.  This is an internal routine
 * that is only exported for some special clients.
 */
int gs_main_run_file_open(gs_main_instance * minst,
			  const char *file_name, ref * pfref);

/*
 * The next 3 procedures provide for feeding input to the interpreter
 * in arbitrary chunks, unlike run_string, which requires that each string
 * be a properly formed PostScript program fragment.  To use them:
 *      Call run_string_begin.
 *      Call run_string_continue as many times as desired,
 *        stopping if it returns anything other than e_NeedInput.
 *      If run_string_continue didn't indicate an error or a quit
 *        (i.e., a return value other than e_NeedInput), call run_string_end
 *        to provide an EOF indication.
 * Note that run_string_continue takes a pointer and a length, like
 * run_string_with_length.
 */
int gs_main_run_string_begin(gs_main_instance * minst, int user_errors,
			     int *pexit_code, ref * perror_object);
int gs_main_run_string_continue(gs_main_instance * minst,
				const char *str, uint length,
				int user_errors, int *pexit_code,
				ref * perror_object);
int gs_main_run_string_end(gs_main_instance * minst, int user_errors,
			   int *pexit_code, ref * perror_object);

/* ---------------- Operand stack access ---------------- */

/*
 * The following procedures are not used in normal operation;
 * they exist only to allow clients driving the interpreter through the
 * gs_main_run_xxx procedures to push parameters quickly and to get results
 * back.  The push procedures return 0, e_stackoverflow, or e_VMerror;
 * the pop procedures return 0, e_stackunderflow, or e_typecheck.
 *
 * Procedures to push values on the operand stack:
 */
int gs_push_boolean(gs_main_instance * minst, bool value);
int gs_push_integer(gs_main_instance * minst, long value);
int gs_push_real(gs_main_instance * minst, floatp value);
int gs_push_string(gs_main_instance * minst, byte * chars, uint length,
		   bool read_only);

/*
 * Procedures to pop values from the operand stack:
 */
int gs_pop_boolean(gs_main_instance * minst, bool * result);
int gs_pop_integer(gs_main_instance * minst, long *result);
int gs_pop_real(gs_main_instance * minst, float *result);

/* gs_pop_string returns 1 if the string is read-only. */
int gs_pop_string(gs_main_instance * minst, gs_string * result);

/* ---------------- Debugging ---------------- */

/*
 * Print an error mesage including the error code, error object (if any),
 * and operand and execution stacks in hex.  Clients will probably
 * never call this.
 */
void gs_main_dump_stack(gs_main_instance *minst, int code,
			ref * perror_object);

/* ---------------- Console output ---------------- */




/* ---------------- Termination ---------------- */

/*
 * Terminate the interpreter by closing all devices and releasing all
 * allocated memory.  Currently, because of some technical problems
 * with statically initialized data, it is not possible to reinitialize
 * the interpreter after terminating it; we plan to fix this as soon as
 * possible.
 *
 * Note that calling gs_to_exit (defined in gsexit.h) automatically calls
 * gs_main_finit for the default instance.
 */
int gs_main_finit(gs_main_instance * minst, int exit_status, int code);

#endif /* imain_INCLUDED */
