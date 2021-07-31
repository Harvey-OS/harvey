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

/* echogs.c */
/* 'echo'-like utility */
#include <stdio.h>
/* Some brain-damaged environments (e.g. Sun) don't include */
/* prototypes for fputc/fputs in stdio.h! */
extern int fputc(), fputs();
/* Some systems have time_t in sys/types.h rather than time.h. */
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <time.h>		/* for ctime */
/* The VMS environment uses different values for success/failure exits: */
#ifdef VMS
#include <stdlib.h>
#  define exit_OK 1
#  define exit_FAILED 18
#else
#  define exit_OK 0
#  define exit_FAILED 1
#endif

/*
 * This program exists solely to get around omissions, problems, and
 * incompatibilities in various shells and utility environments.
 * Don't count on it staying the same from one release to another!
 */

/*
 * Usage:
	echogs [-e .extn] [-(w|a)[b] file] [-h] [-n]
	  (-D | -x hexstring | -q string | -s | -i | -r file | -R file | -X)*
	  [-] string*
 * Echoes string(s), or the binary equivalent of hexstring(s).
 * If -w, writes to file; if -a, appends to file; if neither,
 *   writes to stdout.  -wb and -ab open the file in binary mode.
 * -e specifies an extension to be added to the file name.
 * If -h, write the output in hex instead of literally.
 * If -n, does not append a newline to the output.
 * -D means insert the date and time.
 * -q means write the next string literally.
 * -s means write a space.
 * -i means read from stdin, treating each line as an argument.
 * -r means read from a named file in the same way.
 * -R means read from a named file with no interpretation
 *   (but convert to hex if -h is in effect).
 * -X means treat any following literals as hex rather than string data.
 * - alone means treat the rest of the line as literal data,
 *   even if the first string begins with a -.
 * Inserts spaces automatically between the trailing strings,
 * but nowhere else; in particular,
	echogs -q a b
 * writes 'ab', in contrast to
	echogs -q a -s b
 * which writes 'a b'.
 */

static int hputc(), hputs();

main(argc, argv)
    int argc;
    char *argv[];
{	FILE *out = stdout;
	FILE *in;
	char *extn = "";
	char *fmode;
#define FNSIZE 100
	char fname[FNSIZE];
	int newline = 1;
	int interact = 0;
	int (*eputc)() = fputc, (*eputs)() = fputs;
#define LINESIZE 1000
	char line[LINESIZE];
	char sw = 0, sp = 0, hexx = 0;
	char **argp = argv + 1;
	int nargs = argc - 1;
	if ( nargs > 0 && !strcmp(*argp, "-e") )
	{	if ( nargs < 2 ) return 1;
		extn = argp[1];
		argp += 2, nargs -= 2;
	}
	if ( nargs > 0 && (*argp)[0] == '-' &&
	      ((*argp)[1] == 'w' || (*argp)[1] == 'a')
	   )
	{	if ( nargs < 2 ) return 1;
		fmode = *argp + 1;
		strcpy(fname, argp[1]);
		strcat(fname, extn);
		argp += 2, nargs -= 2;
	}
	else
		strcpy(fname, "");
	if ( nargs > 0 && !strcmp(*argp, "-h") )
	{	eputc = hputc, eputs = hputs;
		argp++, nargs--;
	}
	if ( nargs > 0 && !strcmp(*argp, "-n") )
	{	newline = 0;
		argp++, nargs--;
	}
	if ( strlen(fname) != 0 )
	{	out = fopen(fname, fmode);
		if ( out == 0 ) return 1;
	}
	while ( 1 )
	{	char *arg;
		if ( interact )
		{	if ( fgets(line, LINESIZE, in) == NULL )
			{	interact = 0;
				if ( in != stdin ) fclose(in);
				continue;
			}
			/* Remove the terminating \n. */
			line[strlen(line) - 1] = 0;
			arg = line;
		}
		else
		{	if ( nargs == 0 ) break;
			arg = *argp;
			argp++, nargs--;
		}
		if ( sw == 0 && arg[0] == '-' )
		{	sp = 0;
			switch ( arg[1] )
			{
			case 'q':		/* literal string */
			case 'r':		/* read from a file */
			case 'R':		/* insert file literally */
			case 'x':		/* hex string */
				sw = arg[1];
				break;
			case 's':		/* write a space */
				(*eputc)(' ', out);
				break;
			case 'i':		/* read interactively */
				interact = 1;
				in = stdin;
				break;
			case 'D':		/* insert date/time */
			{	time_t t;
				char str[26];
				time(&t);
				strcpy(str, ctime(&t));
				str[24] = 0;	/* remove \n */
				(*eputs)(str, out);
			}	break;
			case 'X':		/* treat literals as hex */
				hexx = 1;
				break;
			case 0:			/* just '-' */
				sw = '-';
				break;
			}
		}
		else
		  switch ( sw )
		{
		case 0:
		case '-':
			if ( hexx ) goto xx;
			if ( sp ) (*eputc)(' ', out);
			(*eputs)(arg, out);
			sp = 1;
			break;
		case 'q':
			sw = 0;
			(*eputs)(arg, out);
			break;
		case 'r':
			sw = 0;
			in = fopen(arg, "r");
			if ( in == NULL ) exit(exit_FAILED);
			interact = 1;
			break;
		case 'R':
			sw = 0;
			in = fopen(arg, "r");
			if ( in == NULL ) exit(exit_FAILED);
			{	int count;
				while ( (count = fread(line, 1, 1, in)) > 0 )
				  (*eputc)(line[0], out);
			}
			fclose(in);
			break;
		case 'x':
xx:		{	char *xp;
			unsigned int xchr = 1;
			for ( xp = arg; *xp; xp++ )
			{	char ch = *xp;
				if ( !isxdigit(ch) ) return 1;
				xchr <<= 4;
				xchr += (isdigit(ch) ? ch - '0' :
					 (isupper(ch) ? tolower(ch) : ch)
					  - 'a' + 10);
				if ( xchr >= 0x100 )
				{	(*eputc)(xchr & 0xff, out);
					xchr = 1;
				}
			}
		}	sw = 0;
			break;
		}
	}
	if ( newline ) (*eputc)('\n', out);
	if ( out != stdout ) fclose(out);
	return exit_OK;
}

static int
hputc(ch, out)
    int ch;
    FILE *out;
{	static char *hex = "0123456789abcdef";
	putc(hex[ch >> 4], out);
	putc(hex[ch & 0xf], out);
	return 0;
}

static int
hputs(str, out)
    char *str;
    FILE *out;
{	while ( *str ) hputc(*str++ & 0xff, out);
	return 0;
}
