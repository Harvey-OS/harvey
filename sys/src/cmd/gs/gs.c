/* Copyright (C) 1989, 1992, 1993, 1994 Aladdin Enterprises.  All rights reserved.
  
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

/* gs.c */
/* Driver program for Ghostscript */
/* Define PROGRAM_NAME before we include std.h */
#define PROGRAM_NAME gs_product
#include "ctype_.h"
#include "memory_.h"
#include "string_.h"
/* Capture stdin/out/err before gs.h redefines them. */
#include <stdio.h>
static FILE *real_stdin, *real_stdout, *real_stderr;
static void
get_real(void)
{	real_stdin = stdin, real_stdout = stdout, real_stderr = stderr;
}
#include "ghost.h"
#include "gscdefs.h"
#include "gxdevice.h"
#include "gxdevmem.h"
#include "gsdevice.h"
#include "stream.h"
#include "errors.h"
#include "estack.h"
#include "ialloc.h"
#include "strimpl.h"		/* for sfilter.h */
#include "sfilter.h"		/* for iscan.h */
#include "ostack.h"		/* must precede iscan.h */
#include "iscan.h"
#include "main.h"
#include "store.h"
#include "files.h"				/* requires stream.h */
#include "interp.h"
#include "iutil.h"
#include "ivmspace.h"

/* Import operator procedures */
extern int zflush (P1(os_ptr));
extern int zflushpage (P1(os_ptr));

#ifndef GS_LIB
#  define GS_LIB "GS_LIB"
#endif

#ifndef GS_OPTIONS
#  define GS_OPTIONS "GS_OPTIONS"
#endif

/* Library routines not declared in a standard header */
extern char *getenv(P1(const char *));
/* Note: sscanf incorrectly defines its first argument as char * */
/* rather than const char *.  This accounts for the ugly casts below. */

/* Other imported data */
extern const char **gs_lib_paths;
extern const char *gs_doc_directory;

/*
 * Help strings.  We have to break them up into parts, because
 * the Watcom compiler has a limit of 510 characters for a single token.
 * For PC displays, we want to limit the strings to 24 lines.
 */
private const char far_data gs_help1a[] = "\
Usage: gs [switches] [file1.ps file2.ps ...]\n\
Available devices:";
private const char far_data gs_help1b[] = "\n\
Search path:";
private const char far_data gs_help2a[] = "\n\
Most frequently used switches: (you can use # in place of =)\n\
    -d<name>[=<token>]   define name as token, or true if no token given\n\
    -dNOPAUSE            don't pause between pages\n\
    -g<width>x<height>   set width and height (`geometry'), in pixels\n\
    -q                   `quiet' mode, suppress most messages\n\
    -r<res>              set resolution, in pixels per inch\n";
private const char far_data gs_help2b[] = "\
    -s<name>=<string>    define name as string\n\
    -sDEVICE=<devname>   select initial device\n\
    -sOutputFile=<file>  select output file: embed %%d or %%ld for page #,\n\
                           - means stdout, use |command to pipe\n\
    -                    read from stdin (e.g., a pipe) non-interactively\n\
For more information, see the (plain text) file use.doc\n\
  in the directory %s.\n";

/* Forward references */
typedef struct arg_list_s arg_list;
private int swproc(P2(const char *, arg_list *));
private void argproc(P1(const char *));
private void print_revision(P0());
private int esc_strlen(P1(const char *));
private void esc_strcat(P2(char *, const char *));
private void runarg(P4(const char *, const char *, const char *, bool));
private void run_string(P2(const char *, bool));

/* Parameters set by swproc */
private bool quiet;
private bool batch;

/* ------ Argument management ------ */

/* We need to handle recursion into @-files. */
/* The following structures keep track of the state. */
typedef struct arg_source_s {
	int is_file;
	union _u {
		const char *str;
		FILE *file;
	} u;
} arg_source;
struct arg_list_s {
	bool expand_ats;	/* if true, expand @-files */
	const char **argp;
	int argn;
	int depth;		/* depth of @-files */
#define cstr_max 128
	char cstr[cstr_max + 1];
#define csource_max 10
	arg_source sources[csource_max];
};

/* Initialize an arg list. */
private void
arg_init(arg_list *pal, const char **argv, int argc)
{	pal->expand_ats = true;
	pal->argp = argv + 1;
	pal->argn = argc - 1;
	pal->depth = 0;
}

/* Push a string onto an arg list. */
/* (Only used at top level, so no need to check depth.) */
private void
arg_push_string(arg_list *pal, const char *str)
{	arg_source *pas = &pal->sources[pal->depth];
	pas->is_file = 0;
	pas->u.str = str;
	pal->depth++;
}

/* Clean up an arg list. */
private void
arg_finit(arg_list *pal)
{	while ( pal->depth )
	  if ( pal->sources[--(pal->depth)].is_file )
		fclose(pal->sources[pal->depth].u.file);
}

/* Get the next arg from a list. */
/* Note that these are not copied to the heap. */
private const char *
arg_next(arg_list *pal)
{	arg_source *pas;
	FILE *f;
	const char *astr = 0;		/* initialized only to pacify gcc */
	char *cstr;
	const char *result;
	int endc;
	register int c;
	register int i;
top:	pas = &pal->sources[pal->depth - 1];
	if ( pal->depth == 0 )
	{	if ( pal->argn == 0 )		/* all done */
			return 0;
		pal->argn--;
		result = *(pal->argp++);
		goto at;
	}
	if ( pas->is_file )
		f = pas->u.file, endc = EOF;
	else
		astr = pas->u.str, f = NULL, endc = 0;
	result = cstr = pal->cstr;
#define cfsgetc() (f == NULL ? (*astr ? *astr++ : 0) : fgetc(f))
	while ( isspace(c = cfsgetc()) ) ;
	if ( c == endc )
	{	if ( f != NULL )
			fclose(f);
		pal->depth--;
		goto top;
	}
	for ( i = 0; ; )
	{	if ( i == cstr_max - 1 )
		{	cstr[i] = 0;
			fprintf(stdout, "Command too long: %s\n", cstr);
			gs_exit(1);
		}
		cstr[i++] = c;
		c = cfsgetc();
		if ( c == endc || isspace(c) )
			break;
	}
	cstr[i] = 0;
	if ( f == NULL )
		pas->u.str = astr;
at:	if ( pal->expand_ats && result[0] == '@' )
	{	if ( pal->depth == csource_max )
		{	lprintf("Too much nesting of @-files.\n");
			gs_exit(1);
		}
		gs_set_lib_paths();
		result++;		/* skip @ */
		f = lib_fopen(result);
		if ( f == NULL )
		{	fprintf(stdout, "Unable to open command line file %s\n", result);
			gs_exit(1);
		}
		pal->depth++;
		pas++;
		pas->is_file = 1;
		pas->u.file = f;
		goto top;
	}
	return result;
}

/* Copy an argument string to the heap. */
private char *
arg_copy(const char *str)
{	char *sstr = gs_malloc(strlen(str) + 1, 1, "arg_copy");
	if ( sstr == 0 )
	{	lprintf("Out of memory!\n");
		gs_exit(1);
	}
	strcpy(sstr, str);
	return sstr;
}

/* ------ Main program ------ */

int
main(int argc, const char *argv[])
{	const char *arg;
	arg_list args;
	get_real();
	arg_init(&args, argv, argc);
	gs_init0(real_stdin, real_stdout, real_stderr, argc);
	   {	char *lib = getenv(GS_LIB);
		if ( lib != 0 ) 
		   {	int len = strlen(lib);
			gs_lib_env_path = gs_malloc(len + 1, 1, "GS_LIB");
			strcpy(gs_lib_env_path, lib);
		   }
	   }
	/* Execute files named in the command line, */
	/* processing options along the way. */
	/* Wait until the first file name (or the end */
	/* of the line) to finish initialization. */
	batch = false;
	quiet = false;
	{	const char *opts = getenv(GS_OPTIONS);
		if ( opts != 0 )
			arg_push_string(&args, opts);
	}
	while ( (arg = arg_next(&args)) != 0 )
	   {	switch ( *arg )
		{
		case '-':
			if ( swproc(arg, &args) < 0 )
			  fprintf(stdout,
				  "Unknown switch %s - ignoring\n", arg);
			break;
		default:
			argproc(arg);
		}
	   }
	gs_init2();
	if ( !batch )
	  run_string("systemdict /start get exec", true);
	gs_exit(0);			/* exit */
}

/* Process switches */
private int
swproc(const char *arg, arg_list *pal)
{	char sw = arg[1];
	arg += 2;		/* skip - and letter */
	switch ( sw )
	   {
	default:
		return -1;
	case 0:				/* read stdin as a file */
		batch = true;
		/* Set NOPAUSE so showpage won't try to read from stdin. */
		swproc("-dNOPAUSE=true", pal);
		gs_init2();		/* Finish initialization */
		/* We delete this only to make Ghostview work properly. */
		/**************** This is WRONG. ****************/
		/*gs_stdin_is_interactive = false;*/
		run_string("(%stdin) (r) file cvx execute0", true);
		break;
	case '-':			/* run with command line args */
	case '+':
		pal->expand_ats = false;
	case '@':			/* ditto with @-expansion */
	   {	const char *psarg = arg_next(pal);
		if ( psarg == 0 )
		{	fprintf(stdout, "Usage: gs ... -%c file.ps arg1 ... argn", sw);
			arg_finit(pal);
			gs_exit(1);
		}
		psarg = arg_copy(psarg);
		gs_init2();
		run_string("userdict/ARGUMENTS[", false);
		while ( (arg = arg_next(pal)) != 0 )
			runarg("", arg_copy(arg), "", false);
		runarg("]put{", psarg, ".runfile}execute", true);
		gs_exit(0);
	   }
	case 'A':			/* trace allocator */
		gs_alloc_debug = 1; break;
	case 'c':			/* code follows */
	  {	bool ats = pal->expand_ats;
		gs_init2();
		pal->expand_ats = false;
		while ( (arg = arg_next(pal)) != 0 )
		  {	char *sarg;
			if ( arg[0] == '@' ||
			     (arg[0] == '-' && !isdigit(arg[1]))
			   )
				break;
			sarg = arg_copy(arg);
			run_string(sarg, false);
		  }
		if ( arg != 0 )
			arg_push_string(pal, arg_copy(arg));
		pal->expand_ats = ats;
		break;
	  }
	case 'E':			/* log errors */
		gs_log_errors = 1; break;
	case 'f':			/* run file of arbitrary name */
		if ( *arg != 0 )
			argproc(arg);
		break;
	case 'h':			/* print help */
	case '?':			/* ditto */
		print_revision();
		fprintf(stdout, "%s", gs_help1a);
		{	int i;
			gx_device *pdev;
			for ( i = 0; (pdev = gs_getdevice(i)) != 0; i++ )
				fprintf(stdout, (i & 7 ? " %s" : "\n	%s"),
					gs_devicename(pdev));
		}
		fprintf(stdout, "%s", gs_help1b);
		gs_set_lib_paths();
		{	const char **ppath = gs_lib_paths;
			while ( *ppath != 0 )
				fprintf(stdout, "\n    %s", *ppath++);
		}
		fprintf(stdout, "%s", gs_help2a);
		fprintf(stdout, gs_help2b, gs_doc_directory);
		gs_exit(0);
	case 'I':			/* specify search path */
		gs_add_lib_path(arg_copy(arg));
		break;
	case 'q':			/* quiet startup */
	   {	ref vtrue;
		quiet = true;
		gs_init1();
		make_true(&vtrue);
		initial_enter_name("QUIET", &vtrue);
	   }	break;
	case 'D':			/* define name */
	case 'd':
	case 'S':			/* define name as string */
	case 's':
	   {	char *adef = arg_copy(arg);
		char *eqp = strchr(adef, '=');
		bool isd = (sw == 'D' || sw == 'd');
		ref value;

		if ( eqp == NULL )
			eqp = strchr(adef, '#');
		/* Initialize the object memory, scanner, and */
		/* name table now if needed. */
		gs_init1();
		if ( eqp == adef )
		   {	puts("Usage: -dname, -dname=token, -sname=string");
			gs_exit(1);
		   }
		if ( eqp == NULL )
		   {	if ( isd )
				make_true(&value);
			else
				make_empty_string(&value, a_readonly);
		   }
		else
		   {	int code;
			uint space = icurrent_space;

			*eqp++ = 0;
			ialloc_set_space(idmemory, avm_system);
			if ( isd )
			   {	stream astream;
				scanner_state state;
				sread_string(&astream,
					     (const byte *)eqp, strlen(eqp));
				scanner_state_init(&state, false);
				code = scan_token(&astream, &value, &state);
				if ( code )
				   {	puts("-dname= must be followed by a valid token");
					gs_exit(1);
				   }
			   }
			else
			   {	int len = strlen(eqp);
				char *str = gs_malloc((uint)len, 1, "-s");
				if ( str == 0 )
				   {	lprintf("Out of memory!\n");
					gs_exit(1);
				   }
				memcpy(str, eqp, len);
				make_const_string(&value,
					a_readonly | avm_foreign,
					len, (const byte *)str);
			   }
			ialloc_set_space(idmemory, space);
		   }
		/* Enter the name in systemdict. */
		initial_enter_name(adef, &value);
		break;
	   }
	case 'g':			/* define device geometry */
	   {	long width, height;
		ref value;
		gs_init1();
		if ( sscanf((char *)arg, "%ldx%ld", &width, &height) != 2 )
		   {	puts("-g must be followed by <width>x<height>");
			gs_exit(1);
		   }
		make_int(&value, width);
		initial_enter_name("DEVICEWIDTH", &value);
		make_int(&value, height);
		initial_enter_name("DEVICEHEIGHT", &value);
		break;
	   }
	case 'M':			/* set memory allocation increment */
	   {	unsigned msize = 0;
		sscanf((char *)arg, "%d", &msize);
		if ( msize <= 0 || msize >= 64 )
		   {	puts("-M must be between 1 and 63");
			gs_exit(1);
		   }
		gs_memory_chunk_size = msize << 10;
	   }
		break;
	case 'r':			/* define device resolution */
	   {	float xres, yres;
		ref value;
		gs_init1();
		switch ( sscanf((char *)arg, "%fx%f", &xres, &yres) )
		   {
		default:
			puts("-r must be followed by <res> or <xres>x<yres>");
			gs_exit(1);
		case 1:			/* -r<res> */
			yres = xres;
		case 2:			/* -r<xres>x<yres> */
			make_real(&value, xres);
			initial_enter_name("DEVICEXRESOLUTION", &value);
			make_real(&value, yres);
			initial_enter_name("DEVICEYRESOLUTION", &value);
		   }
		break;
	   }
	case 'v':			/* print revision */
		print_revision();
		gs_exit(0);
	case 'Z':
		while ( *arg )
			gs_debug[*arg++ & 127] = 0xff;
		break;
	   }
	return 0;
}

/* Define versions of strlen and strcat that encode strings in hex. */
/* This is so we can enter escaped characters regardless of whether */
/* the Level 1 convention of ignoring \s in strings-within-strings */
/* is being observed (sigh). */
private int
esc_strlen(const char *str)
{	return strlen(str) * 2 + 2;
}
private void
esc_strcat(char *dest, const char *src)
{	char *d = dest + strlen(dest);
	const char *p;
	static const char *hex = "0123456789abcdef";
	*d++ = '<';
	for ( p = src; *p; p++ )
	{	byte c = (byte)*p;
		*d++ = hex[c >> 4];
		*d++ = hex[c & 0xf];
	}
	*d++ = '>';
	*d = 0;
}

/* Process file names */
private void
argproc(const char *arg)
{	runarg("{", arg, ".runfile}execute", true);
}
private void
runarg(const char *pre, const char *arg, const char *post, bool flush)
{	int len = strlen(pre) + esc_strlen(arg) + strlen(post) + 1;
	char *line;
	gs_init2();	/* Finish initialization */
	line = gs_malloc(len, 1, "argproc");
	if ( line == 0 )
	{	lprintf("Out of memory!\n");
		gs_exit(1);
	}
	strcpy(line, pre);
	esc_strcat(line, arg);
	strcat(line, post);
	run_string(line, flush);
}
private void
run_string(const char *str, bool flush)
{	int exit_code;
	ref error_object;
	int code = gs_run_string(str, gs_user_errors, &exit_code, &error_object);
	if ( flush || code != 0 )
	{	zflush(osp);		/* flush stdout */
		zflushpage(osp);	/* force display update */
	}
	switch ( code )
	{
	case 0:
		break;
	case e_Quit:
		gs_exit(0);
	case e_Fatal:
		eprintf1("Unrecoverable error, exit code %d\n", exit_code);
		gs_exit(exit_code);
	default:
		gs_debug_dump_stack(code, &error_object);
		gs_exit_with_code(255, code);
	}
}

/* Print the revision and revision date. */
private void
print_revision(void)
{	fprintf(stdout, "%s %d.%02d",
		gs_product, (int)(gs_revision / 100),
		(int)(gs_revision % 100));
	fprintf(stdout, " (%d/%d/%d)\n%s\n",
		(int)(gs_revisiondate / 100 % 100),
		(int)(gs_revisiondate % 100), (int)(gs_revisiondate / 10000),
		gs_copyright);
}
