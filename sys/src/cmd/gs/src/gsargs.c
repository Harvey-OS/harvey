/* Copyright (C) 1997, 1998, 1999 Aladdin Enterprises.  All rights reserved.

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

/*$Id: gsargs.c,v 1.1 2000/03/09 08:40:42 lpd Exp $ */
/* Command line argument list management */
#include "ctype_.h"
#include "stdio_.h"
#include "string_.h"
#include "gsexit.h"
#include "gsmemory.h"
#include "gsargs.h"

/* Initialize an arg list. */
void
arg_init(arg_list * pal, const char **argv, int argc,
	 FILE * (*arg_fopen) (P2(const char *fname, void *fopen_data)),
	 void *fopen_data)
{
    pal->expand_ats = true;
    pal->arg_fopen = arg_fopen;
    pal->fopen_data = fopen_data;
    pal->argp = argv + 1;
    pal->argn = argc - 1;
    pal->depth = 0;
}

/* Push a string onto an arg list. */
void
arg_push_memory_string(arg_list * pal, char *str, gs_memory_t * mem)
{
    arg_source *pas;

    if (pal->depth == arg_depth_max) {
	lprintf("Too much nesting of @-files.\n");
	gs_exit(1);
    }
    pas = &pal->sources[pal->depth];
    pas->is_file = false;
    pas->u.s.chars = str;
    pas->u.s.memory = mem;
    pas->u.s.str = str;
    pal->depth++;
}

/* Clean up an arg list. */
void
arg_finit(arg_list * pal)
{
    while (pal->depth) {
	arg_source *pas = &pal->sources[--(pal->depth)];

	if (pas->is_file)
	    fclose(pas->u.file);
	else if (pas->u.s.memory)
	    gs_free_object(pas->u.s.memory, pas->u.s.chars, "arg_finit");
    }
}

/* Get the next arg from a list. */
/* Note that these are not copied to the heap. */
const char *
arg_next(arg_list * pal)
{
    arg_source *pas;
    FILE *f;
    const char *astr = 0;	/* initialized only to pacify gcc */
    char *cstr;
    const char *result;
    int endc;
    int c, i;
    bool in_quote, eol;

  top:pas = &pal->sources[pal->depth - 1];
    if (pal->depth == 0) {
	if (pal->argn <= 0)	/* all done */
	    return 0;
	pal->argn--;
	result = *(pal->argp++);
	goto at;
    }
    if (pas->is_file)
	f = pas->u.file, endc = EOF;
    else
	astr = pas->u.s.str, f = NULL, endc = 0;
    result = cstr = pal->cstr;
#define cfsgetc() (f == NULL ? (*astr ? *astr++ : 0) : fgetc(f))
#define is_eol(c) (c == '\r' || c == '\n')
    i = 0;
    in_quote = false;
    eol = true;
    c = cfsgetc();
    for (i = 0;;) {
	if (c == endc) {
	    if (in_quote) {
		cstr[i] = 0;
		fprintf(stdout, "Unterminated quote in @-file: %s\n", cstr);
		gs_exit(1);
	    }
	    if (i == 0) {
		/* EOF before any argument characters. */
		if (f != NULL)
		    fclose(f);
		else if (pas->u.s.memory)
		    gs_free_object(pas->u.s.memory, pas->u.s.chars,
				   "arg_next");
		pal->depth--;
		goto top;
	    }
	    break;
	}
	/* c != endc */
	if (isspace(c)) {
	    if (i == 0) {
		c = cfsgetc();
		continue;
	    }
	    if (!in_quote)
		break;
	}
	/* c isn't leading or terminating whitespace. */
	if (c == '#' && eol) {
	    /* Skip a comment. */
	    do {
		c = cfsgetc();
	    } while (!(c == endc || is_eol(c)));
	    if (c == '\r')
		c = cfsgetc();
	    if (c == '\n')
		c = cfsgetc();
	    continue;
	}
	if (c == '\\') {
	    /* Check for \ followed by newline. */
	    c = cfsgetc();
	    if (is_eol(c)) {
		if (c == '\r')
		    c = cfsgetc();
		if (c == '\n')
		    c = cfsgetc();
		eol = true;
		continue;
	    }
	    /* \ anywhere else is treated as a printing character. */
	    /* This is different from the Unix shells. */
	    if (i == arg_str_max - 1) {
		cstr[i] = 0;
		fprintf(stdout, "Command too long: %s\n", cstr);
		gs_exit(1);
	    }
	    cstr[i++] = '\\';
	    eol = false;
	    continue;
	}
	/* c will become part of the argument */
	if (i == arg_str_max - 1) {
	    cstr[i] = 0;
	    fprintf(stdout, "Command too long: %s\n", cstr);
	    gs_exit(1);
	}
	/* If input is coming from an @-file, allow quotes */
	/* to protect whitespace. */
	if (c == '"' && f != NULL)
	    in_quote = !in_quote;
	else
	    cstr[i++] = c;
	eol = is_eol(c);
	c = cfsgetc();
    }
    cstr[i] = 0;
    if (f == NULL)
	pas->u.s.str = astr;
  at:if (pal->expand_ats && result[0] == '@') {
	if (pal->depth == arg_depth_max) {
	    lprintf("Too much nesting of @-files.\n");
	    gs_exit(1);
	}
	result++;		/* skip @ */
	f = (*pal->arg_fopen) (result, pal->fopen_data);
	if (f == NULL) {
	    fprintf(stdout, "Unable to open command line file %s\n", result);
	    gs_exit(1);
	}
	pal->depth++;
	pas++;
	pas->is_file = true;
	pas->u.file = f;
	goto top;
    }
    return result;
}

/* Copy an argument string to the heap. */
char *
arg_copy(const char *str, gs_memory_t * mem)
{
    char *sstr = (char *)gs_alloc_bytes(mem, strlen(str) + 1, "arg_copy");

    if (sstr == 0) {
	lprintf("Out of memory!\n");
	gs_exit(1);
    }
    strcpy(sstr, str);
    return sstr;
}
