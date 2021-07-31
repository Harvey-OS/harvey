#include	"cc.h"
#include	"y.tab.h"

/*
 * known debug flags
 *	-o file		output file
 *	-s tag		debug structure output
 *	-D name		define
 *	-I path		include
 *	-H sym		help symbol output
 *	-B
 *	-L
 *	-a
 *	-d
 *	-i
 *	-r
 *	-t
 *	-v
 *	-w
 */

void
main(int argc, char *argv[])
{
	char ofile[100], incfile[20], *p, *q, *r;
	int nproc, nout, status, i, c;
	Sym *s, *t;

	memset(debug, 0, sizeof(debug));
	cinit();
	ginit();

	outfile = 0;
	include[ninclude++] = ".";
	ARGBEGIN {
	default:
		c = ARGC();
		if(c >= 0 && c < sizeof(debug))
			debug[c]++;
		break;

	case 'o':
		outfile = ARGF();
		break;

	case 's':
		debug['H']++;
		suedebug = ARGF();
		break;

	case 'D':
		p = ARGF();
		if(p)
			dodefine(p);
		break;

	case 'I':
		p = ARGF();
		if(p)
			include[ninclude++] = p;
		break;

	case 'H':
		debug['H']++;
		p = ARGF();
		if(p) {
			i = 0;
			t = 0;
			q = utfrune(p, ':');
			if(q) {
				*q++ = 0;
				r = utfrune(q, ':');
				if(r) {
					*r++ = 0;
					i = atol(r);
					r = utfrrune(q, '/');
					if(r)
						q = r+1;
					t = slookup(q);
				} else
					i = atol(q);
			}
			s = slookup(p);
			if(!s->ref) {
				ALLOC(s->ref, Ref);
				s->ref->class = CHELP;
				s->ref->lineno = i;
				s->ref->sym = t;
			}
		}
		break;
	} ARGEND
	if(argc < 1 && outfile == 0) {
		print("usage: %cc [-options] files\n", thechar);
		errorexit();
	}
	nproc = 1;
	if(p = getenv("NPROC"))
		nproc = atol(p);	/* */
	if(argc > 1) {
		c = 0;
		nout = 0;
		for(;;) {
			while(nout < nproc && argc > 0) {
				i = fork();
				if(i < 0) {
					i = mywait(&status);
					if(i < 0) {
						print("cannot create a process\n");
						errorexit();
					}
					if(status)
						c++;
					nout--;
					continue;
				}
				if(i == 0)
					goto child;
				nout++;
				argc--;
				argv++;
			}
			i = mywait(&status);
			if(i < 0) {
				if(c)
					errorexit();
				exits(0);
			}
			if(status)
				c++;
			nout--;
		}
	}

child:
	if(argc < 1)
		strcpy(ofile, "stdin");
	else
		strcpy(ofile, *argv);
	if(p = utfrrune(ofile, '/')) {
		include[0] = ofile;
		*p++ = 0;
	} else
		p = ofile;
	if(outfile == 0) {
		outfile = p;
		if(p = utfrrune(outfile, '.'))
			if(p[1] == 'c' && p[2] == 0)
				p[0] = 0;
		p = utfrune(outfile, 0);
		p[0] = '.';
		p[1] = thechar;
		p[2] = 0;
	}
	if(unix()) {
		strcpy(incfile, "/usr/%include");
		p = utfrrune(incfile, '%');
		if(p)
			*p = thechar;
	} else {
		strcpy(incfile, "/");
		strcat(incfile, thestring);
		strcat(incfile, "/include");
	}
	if(p = getenv("INCLUDE")) {
		include[ninclude++] = p;
	} else {
		include[ninclude++] = incfile;
		include[ninclude++] = "/sys/include";
	}
	if(debug['H'])
		outfile = 0;
	else
		close(mycreat(outfile, 0664));
	newio();
	if(argc < 1)
		newfile("stdin", 0);
	else
		newfile(*argv, -1);
	yyparse();
	if(!debug['H'])
		gclean();
	reftrace();
	if(nerrors)
		errorexit();
	exits(0);
}

void
errorexit(void)
{
	if(outfile)
		remove(outfile);
	exits("error");
}

void
pushio(void)
{
	Io *i;

	i = iostack;
	if(i == I) {
		yyerror("botch in pushio");
		errorexit();
	}
	i->p = fi.p;
	i->c = fi.c;
}

void
newio(void)
{
	Io *i;
	static pushdepth = 0;

	i = iofree;
	if(i == I) {
		pushdepth++;
		if(pushdepth > 1000) {
			yyerror("macro/io expansion too deep");
			errorexit();
		}
		ALLOC(i, Io);
	} else
		iofree = i->link;
	i->c = 0;
	i->f = -1;
	ionext = i;
}

void
newfile(char *s, int f)
{
	Io *i;

	i = ionext;
	i->link = iostack;
	iostack = i;
	i->f = f;
	if(f < 0)
		i->f = open(s, 0);
	if(i->f < 0) {
		yyerror("%cc: %s: %s", thechar, myerrstr(i->f), s);
		errorexit();
	}
	fi.c = 0;
	linehist(s, 0);
}

Sym*
slookup(char *s)
{

	strcpy(symb, s);
	return lookup();
}

Sym*
lookup(void)
{
	Sym *s;
	Ref *r;
	ulong h;
	long g;
	char *p, c0;
	int i;

	g = 0;
	for(p=symb; *p;) {
		g = g * 3;
		g += *p++;
	}
	if(g < 0)
		g = ~g;
	h = g;
	if(p >= symb+CNNAME) {
		/*
		 * long names are first CNNAME/2-2 chars
		 * last CNNAME/2-2 chars and three pieces of hash
		 */
		memmove(symb+(CNNAME/2-2), p-(CNNAME/2-2), CNNAME/2-2);
		for(i=0; i<3; i++) {
			c0 = g % 62;
			g = g / 62;
			if(c0 < 26)
				c0 += 'a';
			else
			if(c0 < 52)
				c0 += 'A'-26;
			else
				c0 += '0'-52;
			symb[CNNAME-4+i] = c0;
		}
		symb[CNNAME-1] = 0;
	}
	h %= NHASH;
	c0 = symb[0];
	for(s = hash[h]; s != S; s = s->link) {
		if(s->name[0] != c0)
			continue;
		if(strcmp(s->name, symb) == 0) {
			if(s->ref) {
				ALLOC(r, Ref);
				r->link = s->ref;
				s->ref = r;
				r->lineno = lineno;
			}
			return s;
		}
	}
	ALLOC(s, Sym);
	strcpy(s->name, symb);
	s->link = hash[h];
	hash[h] = s;
	syminit(s);

	return s;
}

void
syminit(Sym *s)
{

	s->lexical = LNAME;
	s->block = 0;
	s->offset = 0;
	s->type = T;
	s->suetag = T;
	s->class = CXXX;
	s->ref = 0;
	s->aused = 0;
}

#define	EOF	(-1)
#define	IGN	(-2)
#define	ESC	(1<<20)
#define	GETC()	((--fi.c < 0)? filbuf(): (*fi.p++ & 0xff))

enum
{
	Numdec		= 1<<0,
	Numlong		= 1<<1,
	Numuns		= 1<<2,
	Numvlong	= 1<<3,
	Numflt		= 1<<4,
};

long
yylex(void)
{
	long c, c1;
	char *cp;
	Rune rune;
	Sym *s;

	if(peekc != IGN) {
		c = peekc;
		peekc = IGN;
		goto l1;
	}
l0:
	c = GETC();

l1:
	if(c >= Runeself) {
		/*
		 * extension --
		 *	all multibyte runes are alpha
		 */
		cp = symb;
		goto talph;
	}
	if(isspace(c)) {
		if(c == '\n')
			lineno++;
		goto l0;
	}
	if(isalpha(c)) {
		cp = symb;
		if(c != 'L')
			goto talph;
		*cp++ = c;
		c = GETC();
		if(c == '\'') {
			/* L'x' */
			c = escchar('\'', 1, 0);
			if(c == EOF)
				c = '\'';
			c1 = escchar('\'', 1, 0);
			if(c1 != EOF) {
				yyerror("missing '");
				peekc = c1;
			}
			yylval.lval = castto(c, TUSHORT);
			return LUCONST;
		}
		if(c == '"') {
			cp = hunk;
			c1 = 0;
			strcpy(symb, "\"<string>\"");
			goto caselq;
		}
		goto talph;
	}
	if(isdigit(c))
		goto tnum;
	switch(c)
	{

	case EOF:
		peekc = EOF;
		return -1;

	case '_':
		cp = symb;
		goto talph;

	case '#':
		domacro();
		goto l0;

	case '.':
		c1 = GETC();
		if(isdigit(c1)) {
			cp = symb;
			*cp++ = c;
			c = c1;
			c1 = 0;
			goto casedot;
		}
		break;

	case '"':
		strcpy(symb, "\"L<string>\"");
		cp = hunk;
		c1 = 0;

	caseq:
		/* "..." */
		for(;;) {
			c = escchar('"', 0, 1);
			if(c == EOF)
				break;
			if(c & ESC) {
				ALLOCN(cp, c1, 1);
				cp[c1] = c;
				c1++;
			} else {
				rune = c;
				c = runelen(rune);
				ALLOCN(cp, c1, c);
				runetochar(cp+c1, &rune);
				c1 += c;
			}
		}
		for(;;) {
			/* it takes 2 peekc's to skip comments */
			c = getc();
			if(isspace(c))
				continue;
			if(c == '"')
				goto caseq;
			unget(c);
			break;
		}
		lnstring = c1+1;
		do {
			ALLOCN(cp, c1, 1);
			cp[c1++] = 0;
		} while(c1 & 3);
		yylval.sval = cp;
		return LSTRING;

	caselq:
		/* L"..." */
		for(;;) {
			c = escchar('"', 1, 0);
			if(c == EOF)
				break;
			ALLOCN(cp, c1, sizeof(ushort));
			*(ushort*)(cp + c1) = c;
			c1 += sizeof(ushort);
		}
		for(;;) {
			/* it takes 2 peekc's to skip comments */
			c = getc();
			if(isspace(c))
				continue;
			if(c == '"')
				goto caselq;
			unget(c);
			break;
		}
		lnstring = c1+sizeof(ushort);
		do {
			ALLOCN(cp, c1, sizeof(ushort));
			*(ushort*)(cp + c1) = 0;
			c1 += sizeof(ushort);
		} while(c1 & 3);
		yylval.rval = (ushort*)cp;
		return LLSTRING;

	case '\'':
		/* '.' */
		c = escchar('\'', 0, 0);
		if(c == EOF)
			c = '\'';
		c1 = escchar('\'', 0, 0);
		if(c1 != EOF) {
			yyerror("missing '");
			peekc = c1;
		}
		if(ovflo(c, TUCHAR))
			yyerror("overflow in character constant: 0x%x", c);
		c = castto(c, TCHAR);
		yylval.lval = c;
		return LCONST;

	case '/':
		c1 = GETC();
		if(c1 == '*') {
			for(;;) {
				c = getr();
				while(c == '*') {
					c = getr();
					if(c == '/')
						goto l0;
				}
				if(c == EOF) {
					yyerror("eof in comment");
					errorexit();
				}
			}
		}
		if(c1 == '=')
			return LDVE;
		break;

	case '*':
		c1 = GETC();
		if(c1 == '=')
			return LMLE;
		break;

	case '%':
		c1 = GETC();
		if(c1 == '=')
			return LMDE;
		break;

	case '+':
		c1 = GETC();
		if(c1 == '+')
			return LPP;
		if(c1 == '=')
			return LPE;
		break;

	case '-':
		c1 = GETC();
		if(c1 == '-')
			return LMM;
		if(c1 == '=')
			return LME;
		if(c1 == '>')
			return LMG;
		break;

	case '>':
		c1 = GETC();
		if(c1 == '>') {
			c = LRSH;
			c1 = GETC();
			if(c1 == '=')
				return LRSHE;
			break;
		}
		if(c1 == '=')
			return LGE;
		break;

	case '<':
		c1 = GETC();
		if(c1 == '<') {
			c = LLSH;
			c1 = GETC();
			if(c1 == '=')
				return LLSHE;
			break;
		}
		if(c1 == '=')
			return LLE;
		break;

	case '=':
		c1 = GETC();
		if(c1 == '=')
			return LEQ;
		break;

	case '!':
		c1 = GETC();
		if(c1 == '=')
			return LNE;
		break;

	case '&':
		c1 = GETC();
		if(c1 == '&')
			return LANDAND;
		if(c1 == '=')
			return LANDE;
		break;

	case '|':
		c1 = GETC();
		if(c1 == '|')
			return LOROR;
		if(c1 == '=')
			return LORE;
		break;

	case '^':
		c1 = GETC();
		if(c1 == '=')
			return LXORE;
		break;

	default:
		return c;
	}
	peekc = c1;
	return c;

talph:
	/*
	 * cp is set to symb and some
	 * prefix has been stored
	 */
	for(;;) {
		if(c >= Runeself) {
			for(c1=0;;) {
				cp[c1++] = c;
				if(fullrune(cp, c1))
					break;
				c = GETC();
			}
			cp += c1;
			c = GETC();
			continue;
		}
		if(!isalnum(c) && c != '_')
			break;
		*cp++ = c;
		c = GETC();
	}
	*cp = 0;
	if(debug['L'])
		print("%L %s\n", lineno, symb);
	peekc = c;
	s = lookup();
	if(s->macro) {
		if(s->ref)
			s->ref->class = CMACRO;
		newio();
		cp = ionext->b;
		macexpand(s, cp);
		pushio();
		ionext->link = iostack;
		iostack = ionext;
		fi.p = cp;
		fi.c = strlen(cp);
		if(peekc != IGN) {
			cp[fi.c++] = peekc;
			cp[fi.c] = 0;
			peekc = IGN;
		}
		goto l0;
	}
	yylval.sym = s;
	if(s->class == CTYPEDEF)
		return LTYPE;
	if(s->lexical == LNAME)
		return LNAME;
	if(s->ref)
		s->ref->class = CLEXICAL;
	return s->lexical;

tnum:
	c1 = 0;
	cp = symb;
	if(c != '0') {
		c1 |= Numdec;
		for(;;) {
			*cp++ = c;
			c = GETC();
			if(isdigit(c))
				continue;
			goto dc;
		}
	}
	*cp++ = c;
	c = GETC();
	if(c == 'x' || c == 'X')
		for(;;) {
			*cp++ = c;
			c = GETC();
			if(isdigit(c))
				continue;
			if(c >= 'a' && c <= 'f')
				continue;
			if(c >= 'A' && c <= 'F')
				continue;
			if(cp == symb+2)
				yyerror("malformed hex constant");
			goto ncu;
		}
	if(c < '0' || c > '7')
		goto dc;
	for(;;) {
		if(c >= '0' && c <= '7') {
			*cp++ = c;
			c = GETC();
			continue;
		}
		goto ncu;
	}

dc:
	if(c == '.')
		goto casedot;
	if(c == 'e' || c == 'E')
		goto casee;

ncu:
	*cp = 0;
	if(c == 'U' || c == 'u') {
		c = GETC();
		c1 |= Numuns;
		if(c == 'L' || c == 'l') {
			c = GETC();
			c1 |= Numlong;
		}
	} else
	if(c == 'L' || c == 'l') {
		c = GETC();
		c1 |= Numlong;
		if(c == 'U' || c == 'u') {
			c = GETC();
			c1 |= Numuns;
		} else
		if(c == 'L' || c == 'l') {
			c = GETC();
			c1 |= Numvlong;
		}
	}
	peekc = c;
	if(c1 & Numvlong) {
		if(FPCHIP) {
			if(mpatof(symb, &yylval.dval)) {
				yyerror("overflow in vl constant");
				yylval.dval = 0;
			}
			return LVLCONST;
		}
		if(!fperror) {
			yyerror("compiler cannot interpret vl constants");
			fperror = 1;
		}
		yylval.lval = 1;
		return LCONST;
	}
	if(mpatol(symb, &yylval.lval))
		yyerror("overflow in integer constant");
	if(tint == types[TSHORT]) {
		/* does it fit in a short */
		if(!ovflo(yylval.lval, TSHORT))
			goto nret;
		/* does it fit in a ushort */
		if(!ovflo(yylval.lval, TUSHORT)) {
			if(c1 & (Numuns|Numlong))
				goto nret;
			if(c1 & Numdec) {
				c1 |= Numlong;
				goto ndiag;
			}
			c1 |= Numuns;
			goto nret;
		}
		/* does it fit in a long */
		if(!ovflo(yylval.lval, TLONG)) {
			if(c1 & Numlong)
				goto nret;
			c1 |= Numlong;
			goto ndiag;
		}
		/* then a ulong */
		if(c1 & Numlong) {
			if(c1 & Numuns)
				goto nret;
			c1 |= Numuns;
			if(c1 & Numdec)
				goto ndiag;
			goto nret;
		}
		c1 |= Numuns|Numlong;
		goto ndiag;
	}
	/* does it fit in a long */
	if(!ovflo(yylval.lval, TLONG))
		goto nret;
	/* then a ulong */
	if(c1 & Numuns)
		goto nret;
	c1 |= Numuns;
	if(c1 & Numdec)
		goto ndiag;
	goto nret;

casedot:
	for(;;) {
		*cp++ = c;
		c = GETC();
		if(!isdigit(c))
			break;
	}
	if(c != 'e' && c != 'E')
		goto caseout;

casee:
	*cp++ = 'e';
	c = GETC();
	if(c == '+' || c == '-') {
		*cp++ = c;
		c = GETC();
	}
	if(!isdigit(c))
		yyerror("malformed fp constant exponent");
	while(isdigit(c)) {
		*cp++ = c;
		c = GETC();
	}

caseout:
	if(c == 'L' || c == 'l') {
		c = GETC();
		c1 |= Numlong;
	} else
	if(c == 'F' || c == 'f') {
		c = GETC();
		c1 |= Numflt;
	}
	*cp = 0;
	peekc = c;
	if(FPCHIP) {
		if(mpatof(symb, &yylval.dval)) {
			yyerror("overflow in float constant");
			yylval.dval = 0;
		}
		if(c1 & Numflt)
			return LFCONST;
		return LDCONST;
	}
	if(!fperror) {
		yyerror("compiler cannot interpret fp constants");
		fperror = 1;
	}
	yylval.lval = 1;
	return LCONST;

ndiag:
	nearln = lineno;
	warn(Z, "constant promotion");

nret:
	if(c1 & Numlong) {
		if(c1 & Numuns)
			return LULCONST;
		return LLCONST;
	}
	if(c1 & Numuns)
		return LUCONST;
	return LCONST;
}

int
getc(void)
{
	int c;

	if(peekc != IGN) {
		c = peekc;
		peekc = IGN;
	} else
		c = GETC();
	if(c == '\n')
		lineno++;
	if(c == EOF) {
		yyerror("End of file");
		errorexit();
	}
	return c;
}

long
getr(void)
{
	int c, i;
	char str[UTFmax+1];
	Rune rune;


	c = getc();
	if(c < Runeself)
		return c;
	i = 0;
	str[i++] = c;

loop:
	c = getc();
	str[i++] = c;
	if(!fullrune(str, i))
		goto loop;
	c = chartorune(&rune, str);
	if(rune == Runeerror && c == 1) {
		nearln = lineno;
		diag(Z, "illegal rune in string");
		for(c=0; c<i; c++)
			print(" %.2x", *(uchar*)(str+c));
		print("\n");
	}
	return rune;
}

int
getnsc(void)
{
	int c;

	if(peekc != IGN) {
		c = peekc;
		peekc = IGN;
	} else
		c = GETC();
	for(;;) {
		if(!isspace(c))
			return c;
		if(c == '\n') {
			lineno++;
			return c;
		}
		c = GETC();
	}
	return 0;
}

void
unget(int c)
{

	peekc = c;
	if(c == '\n')
		lineno--;
}

long
escchar(long e, int longflg, int escflg)
{
	long c, l;
	int i;

loop:
	c = getr();
	if(c == '\n') {
		yyerror("newline in string");
		return EOF;
	}
	if(c != '\\') {
		if(c == e)
			c = EOF;
		return c;
	}
	c = getr();
	if(c == 'x') {
		/*
		 * note this is not ansi,
		 * supposed to only accept 2 hex
		 */
		i = 2;
		if(longflg)
			i = 4;
		l = 0;
		for(; i>0; i--) {
			c = getc();
			if(c >= '0' && c <= '7') {
				l = l*16 + c-'0';
				continue;
			}
			if(c >= 'a' && c <= 'f') {
				l = l*16 + c-'a' + 10;
				continue;
			}
			if(c >= 'A' && c <= 'F') {
				l = l*16 + c-'A' + 10;
				continue;
			}
			unget(c);
			break;
		}
		if(escflg)
			l |= ESC;
		return l;
	}
	if(c >= '0' && c <= '7') {
		/*
		 * note this is not ansi,
		 * supposed to only accept 3 oct
		 */
		i = 2;
		if(longflg)
			i = 5;
		l = c - '0';
		for(; i>0; i--) {
			c = getc();
			if(c >= '0' && c <= '7') {
				l = l*8 + c-'0';
				continue;
			}
			unget(c);
		}
		if(escflg)
			l |= ESC;
		return l;
	}
	switch(c)
	{
	case '\n':	goto loop;
	case 'n':	return '\n';
	case 't':	return '\t';
	case 'b':	return '\b';
	case 'r':	return '\r';
	case 'f':	return '\f';
	case 'a':	return '\a';
	case 'v':	return '\v';
	}
	return c;
}

struct
{
	char	*name;
	ushort	lexical;
} itab[] =
{
	"auto",		LAUTO,
	"break",	LBREAK,
	"case",		LCASE,
	"char",		LCHAR,
	"const",	LCONSTNT,
	"continue",	LCONTINUE,
	"default",	LDEFAULT,
	"do",		LDO,
	"double",	LDOUBLE,
	"else",		LELSE,
	"enum",		LENUM,
	"extern",	LEXTERN,
	"float",	LFLOAT,
	"for",		LFOR,
	"goto",		LGOTO,
	"if",		LIF,
	"int",		LINT,
	"long",		LLONG,
	"register",	LREGISTER,
	"return",	LRETURN,
	"SET",		LSET,
	"short",	LSHORT,
	"signed",	LSIGNED,
	"sizeof",	LSIZEOF,
	"static",	LSTATIC,
	"struct",	LSTRUCT,
	"switch",	LSWITCH,
	"typedef",	LTYPEDEF,
	"union",	LUNION,
	"unsigned",	LUNSIGNED,
	"USED",		LUSED,
	"void",		LVOID,
	"volatile",	LVOLATILE,
	"while",	LWHILE,
	0
};

void
cinit(void)
{
	Sym *s;
	int i;
	Type *t;

	nerrors = 0;
	lineno = 1;
	iostack = I;
	iofree = I;
	peekc = IGN;
	nhunk = 0;

	for(i=0; i<NHASH; i++)
		hash[i] = S;
	for(i=0; itab[i].name; i++) {
		s = slookup(itab[i].name);
		s->lexical = itab[i].lexical;
	}
	blockno = 0;
	autobn = 0;
	autoffset = 0;

	types[TXXX] = T;
	types[TCHAR] = typ(TCHAR, T);
	types[TUCHAR] = typ(TUCHAR, T);
	types[TSHORT] = typ(TSHORT, T);
	types[TUSHORT] = typ(TUSHORT, T);
	types[TLONG] = typ(TLONG, T);
	types[TULONG] = typ(TULONG, T);
	types[TVLONG] = typ(TVLONG, T);
	types[TFLOAT] = typ(TFLOAT, T);
	types[TDOUBLE] = typ(TDOUBLE, T);
	types[TVOID] = typ(TVOID, T);
	types[TENUM] = typ(TENUM, T);
	types[TFUNC] = typ(TFUNC, types[TLONG]);
	types[TIND] = typ(TIND, types[TVOID]);

	tint = types[TLONG];			/* assuming long, ginit knows */
	tuint = types[TULONG];

	t = typ(TARRAY, types[TCHAR]);
	t->width = 0;
	symstring = slookup(".string");
	symstring->class = CSTATIC;
	symstring->type = t;

	t = typ(TARRAY, types[TCHAR]);
	t->width = 0;

	nodproto = new(OPROTO, Z, Z);
	dclstack = D;

	fmtinstall('O', Oconv);
	fmtinstall('T', Tconv);
	fmtinstall('F', FNconv);
	fmtinstall('L', Lconv);
	fmtinstall('Q', Qconv);
	fmtinstall('|', VBconv);
}

int
filbuf(void)
{
	Io *i;

loop:
	i = iostack;
	if(i == I)
		return EOF;
	if(i->f < 0)
		goto pop;
	fi.c = read(i->f, i->b, BUFSIZ) - 1;
	if(fi.c < 0) {
		close(i->f);
		while((ulong)hunk & 3) {
			hunk++;
			nhunk--;
		}
		linehist(0, 0);
		goto pop;
	}
	fi.p = i->b + 1;
	return i->b[0] & 0xff;

pop:
	iostack = i->link;
	i->link = iofree;
	iofree = i;
	i = iostack;
	if(i == I)
		return EOF;
	fi.p = i->p;
	fi.c = i->c;
	if(--fi.c < 0)
		goto loop;
	return *fi.p++ & 0xff;
}

int
Oconv(void *o, Fconv *fp)
{
	int a;

	a = *(int*)o;
	if(a < OXXX || a > OEND)
		strconv(xOconv(a), fp);
	else
		strconv(onames[a], fp);
	return sizeof(a);
}

int
Lconv(void *o, Fconv *fp)
{
	char str[STRINGSZ], s[STRINGSZ];
	Hist *h;
	struct
	{
		Hist*	incl;	/* start of this include file */
		long	idel;	/* delta line number to apply to include */
		Hist*	line;	/* start of this #line directive */
		long	ldel;	/* delta line number to apply to #line */
	} a[HISTSZ];
	long l, d;
	int i, n;

	l = *(long*)o;
	n = 0;
	for(h = hist; h != H; h = h->link) {
		if(l < h->line)
			break;
		if(h->name) {
			if(h->offset != 0) {		/* #line directive, not #pragma */
				if(n > 0 && n < HISTSZ && h->offset >= 0) {
					a[n-1].line = h;
					a[n-1].ldel = h->line - h->offset + 1;
				}
			} else {
				if(n < HISTSZ) {	/* beginning of file */
					a[n].incl = h;
					a[n].idel = h->line;
					a[n].line = 0;
				}
				n++;
			}
			continue;
		}
		n--;
		if(n > 0 && n < HISTSZ) {
			d = h->line - a[n].incl->line;
			a[n-1].ldel += d;
			a[n-1].idel += d;
		}
	}
	if(n > HISTSZ)
		n = HISTSZ;
	str[0] = 0;
	for(i=n-1; i>=0; i--) {
		if(i != n-1) {
			if(fp->f3)
				break;
			strcat(str, " ");
		}
		if(a[i].line)
			sprint(s, "%s:%ld[%s:%ld]",
				a[i].line->name, l-a[i].ldel+1,
				a[i].incl->name, l-a[i].idel+1);
		else
			sprint(s, "%s:%ld",
				a[i].incl->name, l-a[i].idel+1);
		if(strlen(s)+strlen(str) >= STRINGSZ-10)
			break;
		strcat(str, s);
		l = a[i].incl->line - 1;	/* now print out start of this file */
	}
	if(n == 0)
		strcat(str, "<eof>");
	strconv(str, fp);
	return sizeof(l);
}

int
Tconv(void *o, Fconv *fp)
{
	char str[STRINGSZ], s[STRINGSZ];
	Type *t, *t1;
	int et;

	str[0] = 0;
	for(t = *(Type**)o; t != T; t = t->link) {
		et = t->etype;
		if(str[0])
			strcat(str, " ");
		sprint(s, "%s", tnames[et]);
		if(strlen(str) + strlen(s) < STRINGSZ)
			strcat(str, s);
		if(et == TFUNC && (t1 = t->down)) {
			sprint(s, "(%T", t1);
			strcat(str, s);
			while(t1 = t1->down) {
				sprint(s, ", %T", t1);
				strcat(str, s);
			}
			strcat(str, ")");
		}
		if(et == TARRAY) {
			sprint(s, "[%ld]", t->width);
			strcat(str, s);
		}
		if(t->nbits) {
			sprint(s, " %d:%d", t->shift, t->nbits);
			if(strlen(str) + strlen(s) < STRINGSZ)
				strcat(str, s);
		}
		if(typesu[et]) {
			if(t->tag) {
				strcat(str, " ");
				strcat(str, t->tag->name);
			} else
				strcat(str, " {}");
			break;
		}
	}
	strconv(str, fp);
	return sizeof(t);
}

int
FNconv(void *o, Fconv *fp)
{
	char str[STRINGSZ];
	Node *n;

	n = *(Node**)o;
	strcpy(str, "<indirect>");
	if(n != Z && (n->op == ONAME || n->op == ODOT))
		strcpy(str, n->sym->name);
	strconv(str, fp);
	return sizeof(n);
}

int
Qconv(void *o, Fconv *fp)
{
	char str[STRINGSZ], *s;
	long b;
	int i;

	str[0] = 0;
	for(b = *(long*)o; b;) {
		i = bitno(b);
		if(str[0])
			strcat(str, " ");
		s = qnames[i];
		if(strlen(str) + strlen(s) + 1 >= STRINGSZ)
			break;
		strcat(str, s);
		b &= ~(1L << i);
	}
	strconv(str, fp);
	return sizeof(b);
}

int
VBconv(void *o, Fconv *fp)
{
	char str[STRINGSZ];
	int i, n, t, pc;
	extern printcol;

	n = *(int*)o;
	pc = printcol;
	i = 0;
	while(pc < n) {
		t = (pc+8) & ~7;
		if(t <= n) {
			str[i++] = '\t';
			pc = t;
			continue;
		}
		str[i++] = ' ';
		pc++;
	}
	str[i] = 0;
	strconv(str, fp);
	return sizeof(n);
}

/*
 * fake malloc
 */
void*
malloc(long n)
{
	void *p;

	while(n & 3)
		n++;
	while(nhunk < n)
		gethunk();
	p = hunk;
	nhunk -= n;
	hunk += n;
	return p;
}

void
free(void *p)
{
	USED(p);
}

void*
calloc(long m, long n)
{
	void *p;

	n *= m;
	p = malloc(n);
	memset(p, 0, n);
	return p;
}

void*
realloc(void *p, long n)
{

	USED(p);
	USED(n);
	fprint(2, "realloc called\n");
	abort();
	return 0;
}
