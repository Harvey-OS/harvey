/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: gsargs.h,v 1.7 2002/06/16 08:45:42 lpd Exp $ */
/* Command line argument list management */

#ifndef gsargs_INCLUDED
#  define gsargs_INCLUDED

/*
 * We need to handle recursion into @-files.
 * The following structures keep track of the state.
 * Defining a maximum argument length and a maximum nesting depth
 * decreases generality, but eliminates the need for dynamic allocation.
 */
#define arg_str_max 2048
#define arg_depth_max 10
typedef struct arg_source_s {
    bool is_file;
    union _u {
	struct _su {
	    char *chars;	/* original string */
	    gs_memory_t *memory;  /* if non-0, free chars when done with it */
	    const char *str;	/* string being read */
	} s;
	FILE *file;
    } u;
} arg_source;
typedef struct arg_list_s {
    bool expand_ats;		/* if true, expand @-files */
    FILE *(*arg_fopen) (const char *fname, void *fopen_data);
    void *fopen_data;
    const char **argp;
    int argn;
    int depth;			/* depth of @-files */
    char cstr[arg_str_max + 1];
    arg_source sources[arg_depth_max];
} arg_list;

/* Initialize an arg list. */
void arg_init(arg_list * pal, const char **argv, int argc,
	      FILE * (*arg_fopen) (const char *fname, void *fopen_data),
	      void *fopen_data);

/*
 * Push a string onto an arg list.
 * This may also be used (once) to "unread" the last argument.
 * If mem != 0, it is used to free the string when we are done with it.
 * Return 0 on success, non-zero on failure
 */
int arg_push_memory_string(arg_list * pal, char *str, gs_memory_t * mem);

#define arg_push_string(pal, str)\
  arg_push_memory_string(pal, str, (gs_memory_t *)0);

/* Clean up an arg list before exiting. */
void arg_finit(arg_list * pal);

/*
 * Get the next arg from a list.
 * Note that these are not copied to the heap.
 */
const char *arg_next(arg_list * pal, int *code);

/* Copy an argument string to the heap. */
char *arg_copy(const char *str, gs_memory_t * mem);

#endif /* gsargs_INCLUDED */
