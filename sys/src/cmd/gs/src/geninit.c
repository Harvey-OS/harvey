/* Copyright (C) 1995, 2000 Aladdin Enterprises.  All rights reserved.
  
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

/* $Id: geninit.c,v 1.8 2003/07/01 04:37:20 alexcher Exp $ */
/*
 * Utility for merging all the Ghostscript initialization files (gs_*.ps)
 * into a single file, optionally converting them to C data.  Usage:
 *    geninit [-(I|i) <prefix>] <init-file.ps> <gconfig.h> <merged-init-file.ps>
 *    geninit [-(I|i) <prefix>] <init-file.ps> <gconfig.h> -c <merged-init-file.c>
 *
 * The following special constructs are recognized in the input files:
 *	%% Replace[%| ]<#lines> (<psfile>)
 *	%% Replace[%| ]<#lines> INITFILES
 * '%' after Replace means that the file to be included intact.
 * If the first non-comment, non-blank line in a file ends with the
 * LanguageLevel 2 constructs << or <~, its section of the merged file
 * will begin with
 *	currentobjectformat 1 setobjectformat
 * and end with
 *	setobjectformat
 * and if any line then ends with with <, the following ASCIIHex string
 * will be converted to a binary token.
 */
#include "stdpre.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>		/* for exit() */
#include <string.h>
#include <memory.h>

/* Forward references */
private FILE *prefix_open(const char *prefix, const char *inname);
private void merge_to_c(const char *prefix, const char *inname, FILE * in,
			FILE * config, FILE * out);
private void merge_to_ps(const char *prefix, const char *inname, FILE * in,
			 FILE * config, FILE * out);

#define LINE_SIZE 128

int
main(int argc, char *argv[])
{
    int arg_c = argc;
    char **arg_v = argv;
    const char *fin;
    FILE *in;
    const char *fconfig;
    FILE *config;
    const char *fout;
    FILE *out;
    const char *prefix = "";
    bool to_c = false;

    if (arg_c >= 2 && (!strcmp(arg_v[1], "-I") || !strcmp(arg_v[1], "-i"))) {
	prefix = arg_v[2];
	arg_c -= 2;
	arg_v += 2;
    }
    if (arg_c == 4)
	fin = arg_v[1], fconfig = arg_v[2], fout = arg_v[3];
    else if (arg_c == 5 && !strcmp(arg_v[3], "-c"))
	fin = arg_v[1], fconfig = arg_v[2], fout = arg_v[4], to_c = true;
    else {
	fprintf(stderr, "\
Usage: geninit [-(I|i) lib/] gs_init.ps gconfig.h gs_xinit.ps\n\
or     geninit [-(I|i) lib/] gs_init.ps gconfig.h -c gs_init.c\n");
	exit(1);
    }
    in = prefix_open(prefix, fin);
    if (in == 0)
	exit(1);
    config = fopen(fconfig, "r");
    if (config == 0) {
	fprintf(stderr, "Cannot open %s for reading.\n", fconfig);
	fclose(in);
	exit(1);
    }
    out = fopen(fout, "w");
    if (out == 0) {
	fprintf(stderr, "Cannot open %s for writing.\n", fout);
	fclose(config);
	fclose(in);
	exit(1);
    }
    if (to_c)
	merge_to_c(prefix, fin, in, config, out);
    else
	merge_to_ps(prefix, fin, in, config, out);
    fclose(out);
    return 0;
}

/* Translate a path from Postscript notation to platform notation. */

private void
translate_path(char *path)
{
    /* 
     * It looks that we only need to do for Mac OS. 
     * We don't support 16-bit DOS/Windows, which wants '\'.
     * Win32 API supports '/'.
     */
#ifdef __MACOS__
    if (path[0] == '.' && path[1] == '.' && path[2] == '/') {
	char *p;

	path[0] = path[1] = ':';
	for (p = path + 2; p[1] ; p++)
	    *p = (p[1] == '/' ? ':' : p[1]);
	*p = 0;
    }
#endif
}

/* Open a file with a name prefix. */
private FILE *
prefix_open(const char *prefix, const char *inname)
{
    char fname[LINE_SIZE + 1];
    int prefix_length = strlen(prefix);
    FILE *f;

    if (prefix_length + strlen(inname) > LINE_SIZE) {
	fprintf(stderr, "File name > %d characters, too long.\n",
		LINE_SIZE);
	exit(1);
    }
    strcpy(fname, prefix);
    strcat(fname, inname);
    translate_path(fname + prefix_length);
    f = fopen(fname, "r");
    if (f == NULL)
	fprintf(stderr, "Cannot open %s for reading.\n", fname);
    return f;
}

/* Read a line from the input. */
private bool
rl(FILE * in, char *str, int len)
{
    /*
     * Unfortunately, we can't use fgets here, because the typical
     * implementation only recognizes the EOL convention of the current
     * platform.
     */
    int i = 0, c = getc(in);

    if (c < 0)
	return false;
    while (i < len - 1) {
	if (c < 0 || c == 10)		/* ^J, Unix EOL */
	    break;
	if (c == 13) {		/* ^M, Mac EOL */
	    c = getc(in);
	    if (c != 10 && c >= 0)	/* ^M^J, PC EOL */
		ungetc(c, in);
	    break;
	}
	str[i++] = c;
	c = getc(in);
    }
    str[i] = 0;
    return true;
}

/* Write a string or a line on the output. */
private void
wsc(FILE * out, const byte *str, int len)
{
    int n = 0;
    const byte *p = str;
    int i;

    for (i = 0; i < len; ++i) {
	int c = str[i];

	fprintf(out,
		(c < 32 || c >= 127 ? "%d," :
		 c == '\'' || c == '\\' ? "'\\%c'," : "'%c',"),
		c);
	if (++n == 15) {
	    fputs("\n", out);
	    n = 0;
	}
    }
    if (n != 0)
	fputc('\n', out);
}
private void
ws(FILE * out, const byte *str, int len, bool to_c)
{
    if (to_c)
	wsc(out, str, len);
    else
	fwrite(str, 1, len, out);
}
private void
wl(FILE * out, const char *str, bool to_c)
{
    ws(out, (const byte *)str, strlen(str), to_c);
    ws(out, (const byte *)"\n", 1, to_c);
}

/*
 * Strip whitespace and comments from a line of PostScript code as possible.
 * Return a pointer to any string that remains, or NULL if none.
 * Note that this may store into the string.
 */
private char *
doit(char *line, bool intact)
{
    char *str = line;
    char *from;
    char *to;
    int in_string = 0;

    if (intact)
	return str;
    while (*str == ' ' || *str == '\t')		/* strip leading whitespace */
	++str;
    if (*str == 0)		/* all whitespace */
	return NULL;
    if (!strncmp(str, "%END", 4))	/* keep these for .skipeof */
	return str;
    if (str[0] == '%')    /* comment line */
	return NULL;
    /*
     * Copy the string over itself removing:
     *  - All comments not within string literals;
     *  - Whitespace adjacent to '[' ']' '{' '}';
     *  - Whitespace before '/' '(' '<';
     *  - Whitespace after ')' '>'.
     */
    for (to = from = str; (*to = *from) != 0; ++from, ++to) {
	switch (*from) {
	    case '%':
		if (!in_string)
		    break;
		continue;
	    case ' ':
	    case '\t':
		if (to > str && !in_string && strchr(" \t>[]{})", to[-1]))
		    --to;
		continue;
	    case '(':
	    case '<':
	    case '/':
	    case '[':
	    case ']':
	    case '{':
	    case '}':
		if (to > str && !in_string && strchr(" \t", to[-1]))
		    *--to = *from;
                if (*from == '(')
                    ++in_string;
              	continue;
	    case ')':
		--in_string;
		continue;
	    case '\\':
		if (from[1] == '\\' || from[1] == '(' || from[1] == ')')
		    *++to = *++from;
		continue;
	    default:
		continue;
	}
	break;
    }
    /* Strip trailing whitespace. */
    while (to > str && (to[-1] == ' ' || to[-1] == '\t'))
	--to;
    *to = 0;
    return str;
}

/* Copy a hex string to the output as a binary token. */
private void
hex_string_to_binary(FILE *out, FILE *in, bool to_c)
{
#define MAX_STR 0xffff	/* longest possible PostScript string token */
    byte *strbuf = (byte *)malloc(MAX_STR);
    byte *q = strbuf;
    int c, digit;
    bool which = false;
    int len;
    byte prefix[3];

    if (strbuf == 0) {
	fprintf(stderr, "Unable to allocate string token buffer.\n");
	exit(1);
    }
    while ((c = getc(in)) >= 0) {
	if (isxdigit(c)) {
	    c -= (isdigit(c) ? '0' : islower(c) ? 'a' : 'A');
	    if ((which = !which)) {
		if (q == strbuf + MAX_STR) {
		    fprintf(stderr, "Can't handle string token > %d bytes.\n",
			    MAX_STR);
		    exit(1);
		}
		*q++ = c << 4;
	    } else
		q[-1] += c;
	} else if (isspace(c))
	    continue;
	else if (c == '>')
	    break;
	else
	    fprintf(stderr, "Unknown character in ASCIIHex string: %c\n", c);
    }
    len = q - strbuf;
    if (len <= 255) {
	prefix[0] = 142;
	prefix[1] = (byte)len;
	ws(out, prefix, 2, to_c);
    } else {
	prefix[0] = 143;
	prefix[1] = (byte)(len >> 8);
	prefix[2] = (byte)len;
	ws(out, prefix, 3, to_c);
    }
    ws(out, strbuf, len, to_c);
    free((char *)strbuf);
#undef MAX_STR
}

/* Merge a file from input to output. */
private void
flush_buf(FILE * out, char *buf, bool to_c)
{
    if (buf[0] != 0) {
	wl(out, buf, to_c);
	buf[0] = 0;
    }
}
private void
mergefile(const char *prefix, const char *inname, FILE * in, FILE * config,
	  FILE * out, bool to_c, bool intact)
{
    char line[LINE_SIZE + 1];
    char buf[LINE_SIZE + 1];
    char *str;
    int level = 1;
    bool first = true;

    buf[0] = 0;
    while (rl(in, line, LINE_SIZE)) {
	char psname[LINE_SIZE + 1];
	int nlines;

	if (!strncmp(line, "%% Replace", 10) &&
	    sscanf(line + 11, "%d %s", &nlines, psname) == 2
	    ) {
	    bool do_intact = (line[10] == '%');

	    flush_buf(out, buf, to_c);
	    while (nlines-- > 0)
		rl(in, line, LINE_SIZE);
	    if (psname[0] == '(') {
		FILE *ps;
		
		psname[strlen(psname) - 1] = 0;
		ps = prefix_open(prefix, psname + 1);
		if (ps == 0)
		    exit(1);
		mergefile(prefix, psname + 1, ps, config, out, to_c, intact || do_intact);
	    } else if (!strcmp(psname, "INITFILES")) {
		/*
		 * We don't want to bind config.h into geninit, so
		 * we parse it ourselves at execution time instead.
		 */
		rewind(config);
		while (rl(config, psname, LINE_SIZE))
		    if (!strncmp(psname, "psfile_(\"", 9)) {
			FILE *ps;
			char *quote = strchr(psname + 9, '"');

			*quote = 0;
			ps = prefix_open(prefix, psname + 9);
			if (ps == 0)
			    exit(1);
			mergefile(prefix, psname + 9, ps, config, out, to_c, false);
		    }
	    } else {
		fprintf(stderr, "Unknown %%%% Replace %d %s\n",
			nlines, psname);
		exit(1);
	    }
	} else if (!strcmp(line, "currentfile closefile")) {
	    /* The rest of the file is debugging code, stop here. */
	    break;
	} else {
	    int len;

	    str = doit(line, intact);
	    if (str == 0)
		continue;
	    len = strlen(str);
	    if (first && len >= 2 && str[len - 2] == '<' &&
		(str[len - 1] == '<' || str[len - 1] == '~')
		) {
		wl(out, "currentobjectformat 1 setobjectformat\n", to_c);
		level = 2;
	    }
	    if (level > 1 && len > 0 && str[len - 1] == '<' &&
		(len < 2 || str[len - 2] != '<')
		) {
		/* Convert a hex string to a binary token. */
		flush_buf(out, buf, to_c);
		str[len - 1] = 0;
		wl(out, str, to_c);
		hex_string_to_binary(out, in, to_c);
		continue;
	    }
	    if (buf[0] != '%' &&	/* i.e. not special retained comment */
		strlen(buf) + len < LINE_SIZE &&
		(strchr("(/[]{}", str[0]) ||
		 (buf[0] != 0 && strchr(")[]{}", buf[strlen(buf) - 1])))
		)
		strcat(buf, str);
	    else {
		flush_buf(out, buf, to_c);
		strcpy(buf, str);
	    }
	    first = false;
	}
    }
    flush_buf(out, buf, to_c);
    if (level > 1)
	wl(out, "setobjectformat", to_c);
    fprintf(stderr, "%s: %ld bytes, output pos = %ld\n",
	    inname, ftell(in), ftell(out));
    fclose(in);
}

/* Merge and produce a C file. */
private void
merge_to_c(const char *prefix, const char *inname, FILE * in, FILE * config,
	   FILE * out)
{
    char line[LINE_SIZE + 1];

    fputs("/*\n", out);
    while ((rl(in, line, LINE_SIZE), line[0]))
	fprintf(out, "%s\n", line);
    fputs("*/\n", out);
    fputs("\n", out);
    fputs("/* Pre-compiled interpreter initialization string. */\n", out);
    fputs("\n", out);
    fputs("const unsigned char gs_init_string[] = {\n", out);
    mergefile(prefix, inname, in, config, out, true, false);
    fputs("10};\n", out);
    fputs("const unsigned int gs_init_string_sizeof = sizeof(gs_init_string);\n", out);
}

/* Merge and produce a PostScript file. */
private void
merge_to_ps(const char *prefix, const char *inname, FILE * in, FILE * config,
	    FILE * out)
{
    char line[LINE_SIZE + 1];

    while ((rl(in, line, LINE_SIZE), line[0]))
	fprintf(out, "%s\n", line);
    mergefile(prefix, inname, in, config, out, false, false);
}
