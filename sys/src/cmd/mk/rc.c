/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

#include "mk.h"

char* termchars = "'= \t"; /*used in parse.c to isolate assignment attribute*/
char* shflags = "-I";      /* rc flag to force non-interactive mode */
int IWS = '\1'; /* inter-word separator in env - not used in plan 9 */

/*
 *	This file contains functions that depend on rc's syntax.  Most
 *	of the routines extract strings observing rc's escape conventions
 */

/*
 *	skip a token in single quotes.
 */
static char*
squote(char* cp)
{
	Rune r;
	int n;

	while(*cp) {
		n = chartorune(&r, cp);
		if(r == '\'') {
			n += chartorune(&r, cp + n);
			if(r != '\'')
				return (cp);
		}
		cp += n;
	}
	SYNERR(-1); /* should never occur */
	fprint(2, "missing closing '\n");
	return 0;
}

/*
 *	search a string for characters in a pattern set
 *	characters in quotes and variable generators are escaped
 */
char*
charin(char* cp, char* pat)
{
	Rune r;
	int n, vargen;

	vargen = 0;
	while(*cp) {
		n = chartorune(&r, cp);
		switch(r) {
		case '\'':                   /* skip quoted string */
			cp = squote(cp + 1); /* n must = 1 */
			if(!cp)
				return 0;
			break;
		case '$':
			if(*(cp + 1) == '{')
				vargen = 1;
			break;
		case '}':
			if(vargen)
				vargen = 0;
			else if(utfrune(pat, r))
				return cp;
			break;
		default:
			if(vargen == 0 && utfrune(pat, r))
				return cp;
			break;
		}
		cp += n;
	}
	if(vargen) {
		SYNERR(-1);
		fprint(2, "missing closing } in pattern generator\n");
	}
	return 0;
}

/*
 *	extract an escaped token.  Possible escape chars are single-quote,
 *	double-quote,and backslash.  Only the first is valid for rc. the
 *	others are just inserted into the receiving buffer.
 */
char*
expandquote(char* s, Rune r, Bufblock* b)
{
	if(r != '\'') {
		rinsert(b, r);
		return s;
	}

	while(*s) {
		s += chartorune(&r, s);
		if(r == '\'') {
			if(*s == '\'')
				s++;
			else
				return s;
		}
		rinsert(b, r);
	}
	return 0;
}

/*
 *	Input an escaped token.  Possible escape chars are single-quote,
 *	double-quote and backslash.  Only the first is a valid escape for
 *	rc; the others are just inserted into the receiving buffer.
 */
int
escapetoken(Biobuf* bp, Bufblock* buf, int preserve, int esc)
{
	int c, line;

	if(esc != '\'')
		return 1;

	line = mkinline;
	while((c = nextrune(bp, 0)) > 0) {
		if(c == '\'') {
			if(preserve)
				rinsert(buf, c);
			c = Bgetrune(bp);
			if(c < 0)
				break;
			if(c != '\'') {
				Bungetrune(bp);
				return 1;
			}
		}
		rinsert(buf, c);
	}
	SYNERR(line);
	fprint(2, "missing closing %c\n", esc);
	return 0;
}

/*
 *	copy a single-quoted string; s points to char after opening quote
 */
static char*
copysingle(char* s, Bufblock* buf)
{
	Rune r;

	while(*s) {
		s += chartorune(&r, s);
		rinsert(buf, r);
		if(r == '\'')
			break;
	}
	return s;
}
/*
 *	check for quoted strings.  backquotes are handled here; single quotes
 *above.
 *	s points to char after opening quote, q.
 */
char*
copyq(char* s, Rune q, Bufblock* buf)
{
	if(q == '\'') /* copy quoted string */
		return copysingle(s, buf);

	if(q != '`') /* not quoted */
		return s;

	while(*s) { /* copy backquoted string */
		s += chartorune(&q, s);
		rinsert(buf, q);
		if(q == '}')
			break;
		if(q == '\'')
			s = copysingle(s, buf); /* copy quoted string */
	}
	return s;
}
