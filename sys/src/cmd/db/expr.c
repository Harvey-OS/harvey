/*
 *
 *	debugger
 *
 */

#include "defs.h"
#include "fns.h"

static long	round(long, long);

WORD	var[NVARS];

extern	char	lastc, peekc;

extern	ADDR	ditto;
extern	int	ditsp;
WORD	expv;
int	expsp;

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
	expsp = SEGNONE;
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

	switch ((int)readchar()) {

	case '*':
		term(a|1);
		get4(cormap, (ADDR)expv, SEGDATA, &expv);
		expsp = SEGNONE;
		chkerr();
		return(1);

	case '@':
		term(a|1);
		get4(symmap, (ADDR)expv, SEGTEXT, &expv);
		expsp = SEGNONE;
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

	case '%':
		term(a|1);
		expsp = SEGREGS;
		return(1);

	default:
		reread();
		return(item(a));
	}
}

item(int a)
{	/* name [ . local ] | number | . | ^ | <var | <register | 'x | | */
	int	base;
	char	savc;
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
				error(symerror);
			expsp = SEGNONE;
			return 1;
		}
		error("bad file location");
	} else if (symchar(0)) {	
		readsym(gsym);
		if (lastc=='.') {
			readchar();	/* ugh */
			if (!symchar(0))
				localaddr(gsym, 0);
			else {
				readsym(lsym);
				localaddr(gsym, lsym);
			}
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
		if (!symchar(0)) {
			expv = dot;
			expsp = dotsp;
		} else {
			readsym(lsym);
			localaddr(0, lsym);
		}	
		reread();
	} else if (lastc=='"') {
		expv=ditto;
		expsp = ditsp;
	} else if (lastc=='+') {
		expv=inkdot(dotinc);
		expsp = ditsp;
	} else if (lastc=='^') {
		expv=inkdot(-dotinc);
		expsp = ditsp;
	} else if (lastc=='<') {
		savc=rdc();
		base = getreg(savc);
		if (base != BADREG)
			expv = rget(base);
		else if ((base = varchk(savc)) != -1)
			expv = var[base];
		else
			error("bad variable");
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

	p = isymbol;
	do {
		if (p < &isymbol[MAXSYM-1])
			*p++ = lastc;
		readchar();
	} while (symchar(1));
	*p = 0;
}

void
readfname(char *filename)
{
	char	*p;
	char	c;

	/* snarf chars until un-escaped char in terminal char set */
	p = filename;
	do {
		if ((c = lastc) != '\\' && p < &filename[MAXSYM-1])
			*p++ = c;
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
	return(isalpha(lastc) || lastc=='_' || dig && isdigit(lastc));
}

varchk(int name)
{
	if (isdigit(name))
		return(name-'0');
	if (isalpha(name))
		return((name&037)-1+10);
	return(-1);
}

static long
round(long a, long b)
{
	long w;

	w = (a/b)*b;
	if (a!=w)
		w += b;
	return(w);
}
