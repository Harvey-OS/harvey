#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"
#include "y.tab.h"

#define Ungetc(s)	{ if(s == '\n') line--; Bungetc(bin); }
int Lconv(void*, Fconv*);

struct keywd
{
	char	*name;
	int	terminal;
}
keywds[] =
{
	"adt",		Tadt,
	"aggr",		Taggregate,
	"alloc",	Talloc,
	"alt",		Tselect,
	"break",	Tbreak,
	"case",		Tcase,
	"chan",		Tchannel,
	"char",		Tchar,
	"check",	Tcheck,
	"continue",	Tcontinue,
	"default",	Tdefault,
	"do",		Tdo,
	"else",		Telse,
	"enum",		Tset,
	"extern",	Textern,
	"float",	Tfloat,
	"for",		Tfor,
	"goto",		Tgoto,
	"if",		Tif,
	"int",		Tint,
	"intern",	Tintern,
	"nil",		Tnil,
	"par",		Tpar,
	"private",	Tprivate,
	"proc",		Tprocess,
	"raise",	Traise,
	"rescue",	Trescue,
	"return",	Treturn,
	"sint",		Tsint,
	"sizeof",	Tstorage,
	"switch",	Tswitch,
	"task",		Ttask,		
	"typedef",	Tnewtype,
	"uint",		Tuint,
	"unalloc",	Tunalloc,
	"union",	Tunion,
	"usint",	Tsuint,
	"void",		Tvoid,
	"while",	Twhile,
	0,		0
};

char cmap[256] =
{
	['0']	'\0'+1,
	['n']	'\n'+1,
	['r']	'\r'+1,
	['t']	'\t'+1,
	['b']	'\b'+1,
	['f']	'\f'+1,
	['a']	'\a'+1,
	['v']	'\v'+1,
	['\\']	'\\'+1,
	['"']	'"'+1,
};

void
kinit(void)
{
	int i;
	
	for(i = 0; keywds[i].name; i++) 
		enter(keywds[i].name, keywds[i].terminal);

	fmtinstall('L', Lconv);
	fmtinstall('m', Lconv);
}

int
escchar(char c)
{
	int n;
	char buf[Strsize];

	if(c >= '0' && c <= '9') {
		n = 1;
		buf[0] = c;
		for(;;) {
			c = Bgetc(bin);
			if(c == Eof)
				fatal("%L: <eof> in escape sequence", line);
			if(strchr("0123456789xX", c) == 0) {
				Ungetc(c);
				break;
			}
			buf[n++] = c;
		}
		buf[n] = '\0';
		return strtol(buf, 0, 0);
	}

	n = cmap[c];
	if(n == 0)
		return c;
	return n-1;
}

/* Eat preprocessor directives */
void
preprocessor(void)
{
	char buf[Strsize], *p;
	int i, c, off;

	SET(c);
	p = buf;
	for(i = sizeof(buf); i; i--) {
		c = Bgetc(bin);
		if(c == Eof)
			fatal("%L: <eof> in preprocessor directive", line);

		if(c == '\n')
			break;
		*p++ = c;
	}
	if(c != '\n')
		fatal("%L: malformed #", line);

	line++;
	*p = '\0';
	if(opt('p'))
		print("%s\n", buf);

	if(strncmp(buf, "line", 4) == 0) {
		off = strtoul(buf+5, 0, 10);
		p = strrchr(buf, '"');
		if(p == 0)
			fatal("%L: malformed #line", line);
		*p = '\0';
		p = strchr(buf, '"');
		linehist(strdup(p+1), off);
	}
	if(strncmp(buf, "pragma", 6) == 0) {
		p = strstr(buf+7, "lib");
		if(p == 0)
			return;
		p = strrchr(buf, '"');
		if(p == 0)
			fatal("%L: malformed #pragma", line);
		*p = '\0';
		p = strchr(buf, '"');
		linehist(strdup(p+1), -1);
	}
}

void
eatstring(void)
{
	String *s;
	int esc, c, cnt;
	char buf[Strsize];

	esc = 0;
	for(cnt = 0;;) {
		c = Bgetc(bin);
		switch(c) {
		case Eof:
			fatal("%L: <eof> in string constant", line);

		case '\n':
			diag(0, "newline in string constant");
			goto done;

		case '\\':
			esc = 1;
			break;

		case '"':
			if(esc == 0)
				goto done;

			/* Fall through */
		default:
			if(esc) {
				c = escchar(c);
				esc = 0;
			}
			buf[cnt++] = c;
			break;
		}
	}
done:
	buf[cnt++] = '\0';
	s = malloc(sizeof(String));
	s->string = malloc(cnt);
	s->len = cnt;
	memcpy(s->string, buf, cnt);
	yylval.string = s;
}

int
yylex(void)
{
	int c, last;

	c = '\n';

loop:
	last = c;
	c = Bgetc(bin);
	switch(c) {
	case Eof:
		linehist(0, 0);
		return Eof;

	case '"':
		eatstring();
		return Tstring;

	case '#':
		if(last == '\n') {
			preprocessor();
			c = '\n';
		}
		goto loop;

	case ' ':
	case '\t':
		goto loop;

	case '\n':
		line++;
		if(opt('L'))
			print("%L\n", line);
		goto loop;

	case '.':
		c = Bgetc(bin);
		Ungetc(c);
		if(isdigit(c))
			return numsym('.');

		return '.';
 
	case '(':
	case ')':
	case '[':
	case ']':
	case ';':
	case ':':
	case ',':
	case '~':
	case '?':
	case '}':
	case '{':
		return c;

	case '+':
		c = Bgetc(bin);
		if(c == '=')
			return Taddeq;
		if(c == '+')
			return Tinc;
		Ungetc(c);
		return '+';

	case '/':
		c = Bgetc(bin);
		if(c == '=')
			return Tdiveq;
		Ungetc(c);
		return '/';

	case '%':
		c = Bgetc(bin);
		if(c == '=')
			return Tmodeq;
		Ungetc(c);
		return '%';

	case '^':
		c = Bgetc(bin);
		if(c == '=')
			return Txoreq;
		Ungetc(c);
		return '^';

	case '*':
		c = Bgetc(bin);
		if(c == '=')
			return Tmuleq;
		Ungetc(c);
		return '*';

	case '\'':
		c = Bgetc(bin);
		if(c == '\\')
			yylval.ival = escchar(Bgetc(bin));
		else
			yylval.ival = c;
		c = Bgetc(bin);
		if(c != '\'') {
			diag(0, "missing '");
			Ungetc(c);
		}
		return Tconst;

	case '&':
		c = Bgetc(bin);
		if(c == '&')
			return Tandand;
		if(c == '=')
			return Tandeq;
		Ungetc(c);
		return '&';

	case '=':
		c = Bgetc(bin);
		if(c == '=')
			return Teq;
		Ungetc(c);
		return '=';

	case '|':
		c = Bgetc(bin);
		if(c == '|')
			return Toror;
		if(c == '=')
			return Toreq;
		Ungetc(c);
		return '|';

	case '<':
		c = Bgetc(bin);
		if(c == '=')
			return Tleq;
		if(c == '<') {
			c = Bgetc(bin);
			if(c == '=')
				return Tlsheq;
			Ungetc(c);
			return Tlsh;
		}
		if(c == '-')
			return Tcomm;
		Ungetc(c);
		return '<';

	case '>':
		c = Bgetc(bin);
		if(c == '=')
			return Tgeq;
		if(c == '>') {
			c = Bgetc(bin);
			if(c == '=')
				return Trsheq;
			Ungetc(c);
			return Trsh;
		}
		Ungetc(c);
		return '>';

	case '!':
		c = Bgetc(bin);
		if(c == '=')
			return Tneq;
		if(c == '{')
			return Tguard;
		Ungetc(c);
		return '!';

	case '-':
		c = Bgetc(bin);
		if(c == '>')
			return Tindir;
		if(c == '=')
			return Tsubeq;
		if(c == '-')
			return Tdec;
		Ungetc(c);
		return '-';

	default:
		return numsym(c);
	}
}

int
numsym(char first)
{
	int c, isfloat, ishex;
	char *sel, *p;
	Sym *s;
	Type *t;

	symbol[0] = first;
	p = symbol;

	ishex = 0;
	isfloat = 0;
	if(first == '.')
		isfloat = 1;

	if(isdigit(*p++) || isfloat) {
		for(;;) {
			c = Bgetc(bin);
			if(c < 0)
				fatal("%L: <eof> eating symbols", line);

			if(c == '\n')
				line++;
			sel = "01234567890.x";
			if(ishex)
				sel = "01234567890abcdefABCDEF";

			if(strchr(sel, c) == 0) {
				Ungetc(c);
				break;
			}
			if(c == '.')
				isfloat = 1;
			if(c == 'x')
				ishex = 1;
			*p++ = c;
		}
		*p = '\0';
		if(isfloat) {
			yylval.fval = atof(symbol);
			return Tfconst;
		}

		yylval.ival = strtoul(symbol, 0, 0);
		return Tconst;
	}

	for(;;) {
		c = Bgetc(bin);
		if(c < 0)
			fatal("%L <eof> eating symbols", line);
		if(c == '\n')
			line++;
		if(c != '_')
		if(!isalnum(c)) {
			Ungetc(c);
			break;
		}
		*p++ = c;
	}

	*p = '\0';

	s = lookup(symbol);
	if(s == 0) 
		s = enter(symbol, Tid);

	switch(s->lexval) {
	case Tid:
		yylval.sym = s;
		break;

	case Tsname:
		yylval.ival = s->sval;
		break;

	case Ttypename:
		t = abt(TXXX);
		if(s->ltype)
			*t = *(s->ltype);
		else
			t->sym = s;

		yylval.ltype.t = t;
		yylval.ltype.s = s;
		break;
	}
	return s->lexval;
}

static Sym *hash[Hashsize];

Sym *
enter(char *name, int type)
{
	Sym *s;
	ulong h;
	char *p;

	h = 0;
	for(p = name; *p; p++)
		h = h*3 + *p;
	h %= Hashsize;

	s = malloc(sizeof(Sym));
	memset(s, 0, sizeof(Sym));
	s->name = strdup(name);
	s->lexval = type;
	s->hash = hash[h];
	hash[h] = s;
	return s;
}

Sym *
lookup(char *name)
{
	Sym *s;
	ulong h;
	char *p;

	h = 0;
	for(p = name; *p; p++)
		h = h*3 + *p;
	h %= Hashsize;

	for(s = hash[h]; s; s = s->hash)
		if(strcmp(name, s->name) == 0)
			return s;
	return 0;
}

Sym*
ltytosym(Type *t)
{
	int i;
	Sym *p;

	for(i = 0; i < Hashsize; i++) {
		for(p = hash[i]; p; p = p->hash)
			if(typecmp(p->ltype, t, 5))
				return p;
	}
	return ZeroS;
}

/* History algorithm from kens compiler */
int
Lconv(void *o, Fconv *f)
{
	char str[Strsize], s[Strsize];
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
		if(h->offset == -1)
			continue;
		if(l < h->line)
			break;
		if(h->name) {
			if(h->offset != 0) {	 /* #line directive, not #pragma */
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
		if(i != n-1)
			strcat(str, " ");
			sprint(s, "%s %3ld:", a[i].line->name, l-a[i].ldel+1);
		if(strlen(s)+strlen(str) >= Strsize-10)
			break;
		strcat(str, s);
		l = a[i].incl->line - 1;	/* now print out start of this file */
	}
	if(n == 0)
		strcat(str, "<eof> ");
	strconv(str, f);
	return sizeof(l);
}

void
linehist(char *f, int offset)
{
	Hist *h;

	if(opt('p')) {
		if(f) {
			if(offset)
				print("%4d: %s (#line %d)\n", line, f, offset);
			else
				print("%4d: %s\n", line, f);
		} else
			print("%4d: <pop>\n", line);
	}

	/*
	 * overwrite the last #line directive if
	 * no alloc has happened since the last one
	 */
	if(newflag == 0 && ehist != H && offset != 0 && ehist->offset != 0)
		if(f && ehist->name && strcmp(f, ehist->name) == 0) {
			ehist->line = line;
			ehist->offset = offset;
			return;
		}
	newflag = 0;

	h = malloc(sizeof(Hist));
	h->name = f;
	h->line = line;
	h->offset = offset;
	h->link = H;
	if(ehist == H) {
		hist = h;
		ehist = h;
		return;
	}
	ehist->link = h;
	ehist = h;
}
