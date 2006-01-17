/* Copyright (C) 1992, 1995-2004 artofcode LLC. All rights reserved.
  
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

/* $Id: echogs.c,v 1.7 2004/01/07 19:50:35 giles Exp $ */
/* 'echo'-like utility */

#include "stdpre.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(__sun__) && !defined(const)
/* Apparently, there are archaic Sun platforms which don't include
 * prototypes for fputc/fputs in stdio.h. The following prototype can
 * cause type mismatches if const has been defined (usually with
 * -Dconst=), so it's only included if const is undefined.
 */
extern int fputc(int, FILE *), fputs(const char *, FILE *);
#endif

/* Some systems have time_t in sys/types.h rather than time.h. */
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <time.h>		/* for ctime */

/*
 * This program exists solely to get around omissions, problems, and
 * incompatibilities in various shells and utility environments.
 * Don't count on it staying the same from one release to another!
 */

/*
 * Usage:
 echogs [-e .extn] [-(w|a)[b][-] file] [-h] [-n]
 (-b|-B | -d|-D | -f|-F | -x hexstring | -(l|q|Q) string | -(l|q|Q)string |
 -s | -u string | -i | -r file | -R file | -X)*
 [-] string*
 * Echoes string(s), or the binary equivalent of hexstring(s).
 * If -w, writes to file; if -a, appends to file; if neither,
 *   writes to stdout.  -wb and -ab open the file in binary mode.
 *   -w and -a search forward for the next argument that is not a switch.
 *   An appended - means the same as - alone, taking effect after the file
 *   argument.
 * -e specifies an extension to be added to the file name.
 * If -h, write the output in hex instead of literally.
 * If -n, does not append a newline to the output.
 * -b or -B means insert the base part (minus directories) of the file name
 *   passed as the argument of -w or -a.
 * -d or -D means insert the date and time.
 * -f or -F means insert the file name passed as the argument of -w or -a.
 * -q means write the next string literally.
 * -l or -Q means the same as -q followed by -s.
 * -s means write a space.
 * -u means convert the next string to upper case.
 * -i means read from stdin, treating each line as an argument.
 * -r means read from a named file in the same way.
 * -R means copy a named file with no interpretation
 *   (but convert to hex if -h is in effect).
 * -X means treat any following literals as hex rather than string data.
 * - or -+ alone means treat the rest of the line as literal data,
 *   even if the first string begins with a -.
 * -+<letter> is equivalent to -<Letter>, i.e., it upper-cases the letter.
 * Inserts spaces automatically between the trailing strings,
 * but nowhere else; in particular,
 echogs -q a b
 * writes 'ab', in contrast to
 echogs -q a -s b
 * which writes 'a b'.
 */

static int hputc(int, FILE *), hputs(const char *, FILE *);

int
main(int argc, char *argv[])
{
    FILE *out = stdout;
    /*
     * The initialization in = 0 is unnecessary: in is only referenced if
     * interact = 1, in which case in has always been initialized.
     * We initialize in = 0 solely to pacify stupid compilers.
     */
    FILE *in = 0;
    const char *extn = "";
    char fmode[4];
#define FNSIZE 100
    char *fnparam;
    char fname[FNSIZE];
    int newline = 1;
    int interact = 0;
    int (*eputc)(int, FILE *) = fputc;
    int (*eputs)(const char *, FILE *) = fputs;
#define LINESIZE 1000
    char line[LINESIZE];
    char sw = 0, sp = 0, hexx = 0;
    char **argp = argv + 1;
    int nargs = argc - 1;

    if (nargs > 0 && !strcmp(*argp, "-e")) {
	if (nargs < 2)
	    return 1;
	extn = argp[1];
	argp += 2, nargs -= 2;
    }
    if (nargs > 0 && (*argp)[0] == '-' &&
	((*argp)[1] == 'w' || (*argp)[1] == 'a')
	) {
	size_t len = strlen(*argp);
	int i;

	if (len > 4)
	    return 1;
	for (i = 1; i < nargs; i++)
	    if (argp[i][0] != '-')
		break;
	if (i == nargs)
	    return 1;
	fnparam = argp[i];
	strcpy(fmode, *argp + 1);
	strcpy(fname, fnparam);
	strcat(fname, extn);
	if (fmode[len - 2] == '-') {
	    /*
	     * The referents of argp are actually const, but they can't be
	     * declared that way, so we have to make a writable constant.
	     */
	    static char dash[2] = { '-', 0 };

	    fmode[len - 2] = 0;
	    argp[i] = dash;
	    argp++, nargs--;
	} else {
	    for (; i > 1; i--)
		argp[i] = argp[i - 1];
	    argp += 2, nargs -= 2;
	}
    } else
	strcpy(fname, "");
    if (nargs > 0 && !strcmp(*argp, "-h")) {
	eputc = hputc, eputs = hputs;
	argp++, nargs--;
    }
    if (nargs > 0 && !strcmp(*argp, "-n")) {
	newline = 0;
	argp++, nargs--;
    }
    if (strlen(fname) != 0) {
	out = fopen(fname, fmode);
	if (out == 0)
	    return 1;
    }
    while (1) {
	char *arg;

	if (interact) {
	    if (fgets(line, LINESIZE, in) == NULL) {
		interact = 0;
		if (in != stdin)
		    fclose(in);
		continue;
	    }
	    /* Remove the terminating \n. */
	    line[strlen(line) - 1] = 0;
	    arg = line;
	} else {
	    if (nargs == 0)
		break;
	    arg = *argp;
	    argp++, nargs--;
	}
	if (sw == 0 && arg[0] == '-') {
	    char chr = arg[1];

	    sp = 0;
	  swc:switch (chr) {
		case 'l':	/* literal string, then -s */
		    chr = 'Q';
		    /* falls through */
		case 'q':	/* literal string */
		case 'Q':	/* literal string, then -s */
		    if (arg[2] != 0) {
			(*eputs) (arg + 2, out);
			if (chr == 'Q')
			    (*eputc) (' ', out);
			break;
		    }
		    /* falls through */
		case 'r':	/* read from a file */
		case 'R':
		case 'u':	/* upper-case string */
		case 'x':	/* hex string */
		    sw = chr;
		    break;
		case 's':	/* write a space */
		    (*eputc) (' ', out);
		    break;
		case 'i':	/* read interactively */
		    interact = 1;
		    in = stdin;
		    break;
		case 'b':	/* insert base file name */
		case 'B':
		    arg = fnparam + strlen(fnparam);
		    while (arg > fnparam &&
			   (isalnum(arg[-1]) || arg[-1] == '_'))
			--arg;
		    (*eputs) (arg, out);
		    break;
		case 'd':	/* insert date/time */
		case 'D':
		    {
			time_t t;
			char str[26];

			time(&t);
			strcpy(str, ctime(&t));
			str[24] = 0;	/* remove \n */
			(*eputs) (str, out);
		    } break;
		case 'f':	/* insert file name */
		case 'F':
		    (*eputs) (fnparam, out);
		    break;
		case 'X':	/* treat literals as hex */
		    hexx = 1;
		    break;
		case '+':	/* upper-case command */
		    if (arg[1]) {
			++arg;
			chr = toupper(arg[1]);
			goto swc;
		    }
		    /* falls through */
		case 0:		/* just '-' */
		    sw = '-';
		    break;
	    }
	} else
	    switch (sw) {
		case 0:
		case '-':
		    if (hexx)
			goto xx;
		    if (sp)
			(*eputc) (' ', out);
		    (*eputs) (arg, out);
		    sp = 1;
		    break;
		case 'q':
		    sw = 0;
		    (*eputs) (arg, out);
		    break;
		case 'Q':
		    sw = 0;
		    (*eputs) (arg, out);
		    (*eputc) (' ', out);
		    break;
		case 'r':
		    sw = 0;
		    in = fopen(arg, "r");
		    if (in == NULL)
			exit(exit_FAILED);
		    interact = 1;
		    break;
		case 'R':
		    sw = 0;
		    in = fopen(arg, "r");
		    if (in == NULL)
			exit(exit_FAILED);
		    while (fread(line, 1, 1, in) > 0)
			(*eputc) (line[0], out);
		    fclose(in);
		    break;
		case 'u':
		    {
			char *up;

			for (up = arg; *up; up++)
			    (*eputc) (toupper(*up), out);
		    }
		    sw = 0;
		    break;
		case 'x':
		  xx:{
			char *xp;
			unsigned int xchr = 1;

			for (xp = arg; *xp; xp++) {
			    char ch = *xp;

			    if (!isxdigit(ch))
				return 1;
			    xchr <<= 4;
			    xchr += (isdigit(ch) ? ch - '0' :
				     (isupper(ch) ? tolower(ch) : ch)
				     - 'a' + 10);
			    if (xchr >= 0x100) {
				(*eputc) (xchr & 0xff, out);
				xchr = 1;
			    }
			}
		    }
		    sw = 0;
		    break;
	    }
    }
    if (newline)
	(*eputc) ('\n', out);
    if (out != stdout)
	fclose(out);
    return exit_OK;
}

static int
hputc(int ch, FILE * out)
{
    static const char *hex = "0123456789abcdef";

    /* In environments where char is signed, ch may be negative (!). */
    putc(hex[(ch >> 4) & 0xf], out);
    putc(hex[ch & 0xf], out);
    return 0;
}

static int
hputs(const char *str, FILE * out)
{
    while (*str)
	hputc(*str++ & 0xff, out);
    return 0;
}
