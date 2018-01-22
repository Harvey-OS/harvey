/*
 * This file is part of the UCB release of Plan 9. It is subject to the license
 * terms in the LICENSE file found in the top-level directory of this
 * distribution and at http://akaros.cs.berkeley.edu/files/Plan9License. No
 * part of the UCB release of Plan 9, including this file, may be copied,
 * modified, propagated, or distributed except according to the terms contained
 * in the LICENSE file.
 */

/*
 *
 *	debugger
 *
 */

#include "defs.h"
#include "fns.h"

static int32_t	round(int32_t, int32_t);

extern	ADDR	ditto;
uint64_t	expv;

static WORD
ascval(void)
{
	Rune r;

	if (readchar() == 0)
		return (0);
	r = lastc;
	while(quotchar())	/*discard chars to ending quote */
		;
	return((WORD) r);
}

/*
 * read a floating point number
 * the result must fit in a WORD
 */

static WORD
fpin(char *buf)
{
	union {
		WORD w;
		float f;
	} x;

	x.f = atof(buf);
	return (x.w);
}

WORD
defval(WORD w)
{
	if (expr(0))
		return (expv);
	else
		return (w);
}

expr(int a)
{	/* term | term dyadic expr |  */
	int	rc;
	WORD	lhs;

	rdc();
	reread();
	rc=term(a);
	while (rc) {
		lhs = expv;
		switch ((int)readchar()) {

		case '+':
			term(a|1);
			expv += lhs;
			break;

		case '-':
			term(a|1);
			expv = lhs - expv;
			break;

		case '#':
			term(a|1);
			expv = round(lhs,expv);
			break;

		case '*':
			term(a|1);
			expv *= lhs;
			break;

		case '%':
			term(a|1);
			if(expv != 0)
				expv = lhs/expv;
			else{
				if(lhs)
					expv = 1;
				else
					expv = 0;
			}
			break;

		case '&':
			term(a|1);
			expv &= lhs;
			break;

		case '|':
			term(a|1);
			expv |= lhs;
			break;

		case ')':
			if ((a&2)==0)
				error("unexpected `)'");

		default:
			reread();
			return(rc);
		}
	}
	return(rc);
}

term(int a)
{	/* item | monadic item | (expr) | */
	ADDR e;

	switch ((int)readchar()) {

	case '*':
		term(a|1);
		if (geta(cormap, expv, &e) < 0)
			error("%r");
		expv = e;
		return(1);

	case '@':
		term(a|1);
		if (geta(symmap, expv, &e) < 0)
			error("%r");
		expv = e;
		return(1);

	case '-':
		term(a|1);
		expv = -expv;
		return(1);

	case '~':
		term(a|1);
		expv = ~expv;
		return(1);

	case '(':
		expr(2);
		if (readchar()!=')')
			error("syntax error: `)' expected");
		return(1);

	default:
		reread();
		return(item(a));
	}
}

item(int a)
{	/* name [ . local ] | number | . | ^  | <register | 'x | | */
	char	*base;
	char	savc;
	uvlong e;
	Symbol s;
	char gsym[MAXSYM], lsym[MAXSYM];

	readchar();
	if (isfileref()) {
		readfname(gsym);
		rdc();			/* skip white space */
		if (lastc == ':') {	/* it better be */
			rdc();		/* skip white space */
			if (!getnum(readchar))
				error("bad number");
			if (expv == 0)
				expv = 1;	/* file begins at line 1 */
			expv = file2pc(gsym, expv);
			if (expv == -1)
				error("%r");
			return 1;
		}
		error("bad file location");
	} else if (symchar(0)) {
		readsym(gsym);
		if (lastc=='.') {
			readchar();	/* ugh */
			if (lastc == '.') {
				lsym[0] = '.';
				readchar();
				readsym(lsym+1);
			} else if (symchar(0)) {
				readsym(lsym);
			} else
				lsym[0] = 0;
			if (localaddr(cormap, gsym, lsym, &e, rget) < 0)
				error("%r");
			expv = e;
		}
		else {
			if (lookup(0, gsym, &s) == 0)
				error("symbol not found");
			expv = s.value;
		}
		reread();
	} else if (getnum(readchar)) {
		;
	} else if (lastc=='.') {
		readchar();
		if (!symchar(0) && lastc != '.') {
			expv = dot;
		} else {
			if (findsym(rget(cormap, mach->pc), CTEXT, &s) == 0)
				error("no current function");
			if (lastc == '.') {
				lsym[0] = '.';
				readchar();
				readsym(lsym+1);
			} else
				readsym(lsym);
			if (localaddr(cormap, s.name, lsym, &e, rget) < 0)
				error("%r");
			expv = e;
		}
		reread();
	} else if (lastc=='"') {
		expv=ditto;
	} else if (lastc=='+') {
		expv=inkdot(dotinc);
	} else if (lastc=='^') {
		expv=inkdot(-dotinc);
	} else if (lastc=='<') {
		savc=rdc();
		base = regname(savc);
		expv = rget(cormap, base);
	}
	else if (lastc=='\'')
		expv = ascval();
	else if (a)
		error("address expected");
	else {
		reread();
		return(0);
	}
	return(1);
}

#define	MAXBASE	16

/* service routines for expression reading */
getnum(int (*rdf)(void))
{
	char *cp;
	int base, d;
	BOOL fpnum;
	char num[MAXLIN];

	base = 0;
	fpnum = FALSE;
	if (lastc == '#') {
		base = 16;
		(*rdf)();
	}
	if (convdig(lastc) >= MAXBASE)
		return (0);
	if (lastc == '0')
		switch ((*rdf)()) {
		case 'x':
		case 'X':
			base = 16;
			(*rdf)();
			break;

		case 't':
		case 'T':
			base = 10;
			(*rdf)();
			break;

		case 'o':
		case 'O':
			base = 8;
			(*rdf)();
			break;
		default:
			if (base == 0)
				base = 8;
			break;
		}
	if (base == 0)
		base = 10;
	expv = 0;
	for (cp = num, *cp = lastc; ;(*rdf)()) {
		if ((d = convdig(lastc)) < base) {
			expv *= base;
			expv += d;
			*cp++ = lastc;
		}
		else if (lastc == '.') {
			fpnum = TRUE;
			*cp++ = lastc;
		} else {
			reread();
			break;
		}
	}
	if (fpnum)
		expv = fpin(num);
	return (1);
}

void
readsym(char *isymbol)
{
	char	*p;
	Rune r;

	p = isymbol;
	do {
		if (p < &isymbol[MAXSYM-UTFmax-1]){
			r = lastc;
			p += runetochar(p, &r);
		}
		readchar();
	} while (symchar(1));
	*p = 0;
}

void
readfname(char *filename)
{
	char	*p;
	Rune	c;

	/* snarf chars until un-escaped char in terminal char set */
	p = filename;
	do {
		if ((c = lastc) != '\\' && p < &filename[MAXSYM-UTFmax-1])
			p += runetochar(p, &c);
		readchar();
	} while (c == '\\' || strchr(CMD_VERBS, lastc) == 0);
	*p = 0;
	reread();
}

convdig(int c)
{
	if (isdigit(c))
		return(c-'0');
	else if (!isxdigit(c))
		return(MAXBASE);
	else if (isupper(c))
		return(c-'A'+10);
	else
		return(c-'a'+10);
}

symchar(int dig)
{
	if (lastc=='\\') {
		readchar();
		return(TRUE);
	}
	return(isalpha(lastc) || lastc>0x80 || lastc=='_' || dig && isdigit(lastc));
}

static int32_t
round(int32_t a, int32_t b)
{
	int32_t w;

	w = (a/b)*b;
	if (a!=w)
		w += b;
	return(w);
}
