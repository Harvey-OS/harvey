/*
Copyright (c) 1989 AT&T
	All Rights Reserved

THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T.

The copyright notice above does not evidence any
actual or intended publication of such source code.
*/

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "awk.h"
#include "y.tab.h"
#include "regexp.h"

	/* This file provides the interface between the main body of
	 * awk and the pattern matching package.  It preprocesses
	 * patterns prior to compilation to provide awk-like semantics
	 * to character sequences not supported by the pattern package.
	 * The following conversions are performed:
	 *
	 *	"()"		->	"[]"
	 *	"[-"		->	"[\-"
	 *	"[^-"		->	"[^\-"
	 *	"-]"		->	"\-]"
	 *	"[]"		->	"[]*"
	 *	"\xdddd"	->	"\z" where 'z' is the UTF sequence
	 *					for the hex value
	 *	"\ddd"		->	"\o" where 'o' is a char octal value
	 *	"\b"		->	"\B"	where 'B' is backspace
	 *	"\t"		->	"\T"	where 'T' is tab
	 *	"\f"		->	"\F"	where 'F' is form feed
	 *	"\n"		->	"\N"	where 'N' is newline
	 *	"\r"		->	"\r"	where 'C' is cr
	 */

#define	MAXRE	512

static uchar	re[MAXRE];	/* copy buffer */

uchar	*patbeg;
int	patlen;			/* number of chars in pattern */

#define	NPATS	20		/* number of slots in pattern cache */

static struct pat_list		/* dynamic pattern cache */
{
	char	*re;
	int	use;
	Reprog	*program;
} pattern[NPATS];

static int npats;		/* cache fill level */

	/* Compile a pattern */
void
*compre(uchar *pat)
{
	int i, j, inclass;
	uchar c, *p, val, *s;
	Reprog *program;

	if (!compile_time) {	/* search cache for dynamic pattern */
		for (i = 0; i < npats; i++)
			if (!strcmp(pat, pattern[i].re)) {
				pattern[i].use++;
				return((void *) pattern[i].program);
			}
	}
		/* Preprocess Pattern for compilation */
	p = re;
	s = pat;
	inclass = 0;
	while (c = *s++) {
		if (c == '\\') {
			quoted(&s, &p, re+MAXRE);
			continue;
		}
		else if (!inclass && c == '(' && *s == ')') {
			if (p < re+MAXRE-2) {	/* '()' -> '[]*' */
				*p++ = '[';
				*p++ = ']';
				c = '*';
				s++;
			}
			else overflow();
		}
		else if (c == '['){			/* '[-' -> '[\-' */
			inclass = 1;
			if (*s == '-') {
				if (p < re+MAXRE-2) {
					*p++ = '[';
					*p++ = '\\';
					c = *s++;
				}
				else overflow();
			}				/* '[^-' -> '[^\-'*/
			else if (*s == '^' && s[1] == '-'){
				if (p < re+MAXRE-3) {
					*p++ = '[';
					*p++ = *s++;
					*p++ = '\\';
					c = *s++;
				}
				else overflow();
			}
			else if (*s == '['){		/* skip '[[' */
				if (p < re+MAXRE-1)
					*p++ = c;
				else overflow();
				c = *s++;
			}
			else if (*s == '^' && s[1] == '[') {	/* skip '[^['*/
				if (p < re+MAXRE-2) {
					*p++ = c;
					*p++ = *s++;
					c = *s++;
				}
				else overflow();
			}
			else if (*s == ']') {		/* '[]' -> '[]*' */
				if (p < re+MAXRE-2) {
					*p++ = c;
					*p++ = *s++;
					c = '*';
					inclass = 0;
				}
				else overflow();
			}
		}
		else if (c == '-' && *s == ']') {	/* '-]' -> '\-]' */
			if (p < re+MAXRE-1)
				*p++ = '\\';
			else overflow();
		}
		else if (c == ']')
			inclass = 0;
		if (p < re+MAXRE-1)
			*p++ = c;
		else overflow();
	}
	*p = 0;
	program = regcomp(re);		/* compile pattern */
	if (!compile_time) {
		if (npats < NPATS)	/* Room in cache */
			i = npats++;
		else {			/* Throw out least used */
			int use = pattern[0].use;
			i = 0;
			for (j = 1; j < NPATS; j++) {
				if (pattern[j].use < use) {
					use = pattern[j].use;
					i = j;
				}
			}
			xfree(pattern[i].program);
			xfree(pattern[i].re);
		}
		pattern[i].re = tostring(pat);
		pattern[i].program = program;
		pattern[i++].use = 1;
	}
	return((void *) program);
}

	/* T/F match indication - matched string not exported */
int
match(void *p, uchar *s, uchar *start)
{
	return regexec((Reprog *) p, (char *) s, 0, 0);
}

	/* match and delimit the matched string */
int
pmatch(void *p, uchar *s, uchar *start)
{
	Resub m;

	m.s.sp = start;
	m.e.ep = 0;
	if (regexec((Reprog *) p, (char *) s, &m, 1)) {
		patbeg = m.s.sp;
		patlen = m.e.ep-m.s.sp;
		return 1;
	}
	patlen = -1;
	patbeg = start;
	return 0;
}

	/* perform a non-empty match */
int
nematch(void *p, uchar *s, uchar *start)
{
	if (pmatch(p, s, start) == 1 && patlen > 0)
		return 1;
	patlen = -1;
	patbeg = start; 
	return 0;
}
/* in the parsing of regular expressions, metacharacters like . have */
/* to be seen literally;  \056 is not a metacharacter. */

hexstr(char **pp)	/* find and eval hex string at pp, return new p */
{
	char c;
	int n = 0;
	int i;

	for (i = 0, c = (*pp)[i]; i < 4 && isxdigit(c); i++, c = (*pp)[i]) {
		if (isdigit(c))
			n = 16 * n + c - '0';
		else if ('a' <= c && c <= 'f')
			n = 16 * n + c - 'a' + 10;
		else if ('A' <= c && c <= 'F')
			n = 16 * n + c - 'A' + 10;
	}
	*pp += i;
	return n;
}

	/* look for awk-specific escape sequences */

#define isoctdigit(c) ((c) >= '0' && (c) <= '8') /* multiple use of arg */

void
quoted(uchar **s, uchar **to, uchar *end)	/* handle escaped sequence */
{
	char *p = *s;
	char *t = *to;
	wchar_t c;

	switch(c = *p++) {
	case 't':
		c = '\t';
		break;
	case 'n':
		c = '\n';
		break;
	case 'f':
		c = '\f';
		break;
	case 'r':
		c = '\r';
		break;
	case 'b':
		c = '\b';
		break;
	default:
		if (t < end-1)		/* all else must be escaped */
			*t++ = '\\';
		if (c == 'x') {		/* hexadecimal goo follows */
			c = hexstr(&p);
			if (t < end-MB_CUR_MAX)
				t += wctomb(t, c);
			else overflow();
			*to = t;
			*s = p;
			return;
		} else if (isoctdigit(c)) {	/* \d \dd \ddd */
			c -= '0';
			if (isoctdigit(*p)) {
				c = 8 * c + *p++ - '0';
				if (isoctdigit(*p))
					c = 8 * c + *p++ - '0';
			}
		}
		break;
	}
	if (t < end-1)
		*t++ = c;
	*s = p;
	*to = t;
}
	/* count rune positions */
int
countposn(uchar *s, int n)
{
	int i;
	uchar *end;

	for (i = 0, end = s+n; *s && s < end; i++)
		s += mblen(s, n); 
	return(i);
}

	/* pattern package error handler */

void
regerror(char *s)
{
	ERROR "%s", s FATAL;
}

void
overflow(void)
{
	ERROR "%s", "regular expression too big" FATAL;
}
