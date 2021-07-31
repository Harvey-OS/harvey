/* Copyright (C) 1992, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* main.h */
/* Interface between gs.c and gsmain.c */

#include "gsexit.h"			/* exported by gsmain.c */

/* Exported data from gsmain.c */
extern char *gs_lib_env_path;
extern uint gs_memory_chunk_size;
extern int gs_user_errors;

/* Exported procedures from gsmain.c */
void	gs_init0(P4(FILE *, FILE *, FILE *, int)),
	gs_init1(P0()),
	gs_init2(P0());
void	gs_add_lib_path(P1(const char *)),
	gs_set_lib_paths(P0());
int	gs_lib_open(P2(const char *fname, ref *pfile));
/* The value returned by gs_run_file and gs_run_string[_with_length] is */
/* 0 if the interpreter ran to completion, e_Quit for a normal quit, */
/* e_Fatal for a non-zero quit or a fatal error. */
/* e_Fatal stores the exit code in the third argument. */
/* The str argument of gs_run_string[_with_length] must be allocated */
/* in non-garbage-collectable space (e.g., by malloc or gs_malloc, */
/* or statically). */
int	gs_run_file(P4(const char *fname, int user_errors, int *pexit_code, ref *perror_object)),
	gs_run_string(P4(const char *str, int user_errors, int *pexit_code, ref *perror_object)),
	gs_run_string_with_length(P5(const char *str, uint length, int user_errors, int *pexit_code, ref *perror_object));
void	gs_debug_dump_stack(P2(int code, ref *perror_object));
void	gs_finit(P2(int exit_status, int code));
/* Direct interface to the interpreter. */
/* Clients do not normally use this. */
int	gs_interpret(P4(ref *pref, int user_errors, int *pexit_code, ref *perror_object));
