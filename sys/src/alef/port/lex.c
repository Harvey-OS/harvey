#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#define Extern extern
#include "parl.h"
#include "globl.h"
#include "y.tab.h"

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
	"become",	Tbecome,
	"break",	Tbreak,
	"byte",		Tchar,
	"case",		Tcase,
	"chan",		Tchannel,
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
	"typeof",	Ttypeof,
	"uint",		Tuint,
	"unalloc",	Tunalloc,
	"union",	Tunion,
	"usint",	Tsuint,
	"void",		Tvoid,
	"while",	Twhile,
	"tuple",	Ttuple,
	"zerox",	Tzerox,
	0,		0
};

static char cmap[256];

void
kinit(void)
{
	int i;
	
	for(i = 0; keywds[i].name; i++) 
		enter(keywds[i].name, keywds[i].terminal);

	polyptr = enter("ptr", Tid);
	polysig = enter("sig", Tid);

	cmap['0'] = '\0'+1;
	cmap['n'] = '\n'+1;
	cmap['r'] = '\r'+1;
	cmap['t'] = '\t'+1;
	cmap['b'] = '\b'+1;
	cmap['f'] = '\f'+1;
	cmap['a'] = '\a'+1;
	cmap['v'] = '\v'+1;
	cmap['\\'] = '\\'+1;
	cmap['"'] = '"'+1;

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
				Bungetc(bin);
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

void
polytab(char *line)
{
	char *p;
	Node *n, *i;

	p = line;
	while(*p && *p != ' ' && *p != '\t')
		p++;
	if(*p == '\0')
		goto bad;
	*p++ = '\0';
	while(*p && (*p == ' ' || *p == '\t'))
		p++;
	if(*p == '\0')
		goto bad;

	n = rtnode(line, nil, at(TIND, builtype[TFUNC]));
	i = rtnode(p, nil, builtype[TINT]);
	dynt(n, i);
	return;
bad:
	diag(0, "bad #pragma dynamic");
}

void
pragma(char *buf)
{
	char *p, *e;

	p = buf;
	while(*p && *p != ' ' && *p != '\t')
		p++;
	if(*p == '\0')
		return;

	while(*p && (*p == ' ' || *p == '\t'))
		p++;
	if (*p == '\0')
		return;

	e = p;
	while(*e && *e != ' ' && *e != '\t')
		e++;

	if(*e == '\0')
		return;
	*e++ = '\0';

	if(strcmp(p, "lib") == 0) {
		p = strrchr(e, '"');
		if(p == 0)
			fatal("%L: malformed #pragma", line);
		*p = '\0';
		p = strchr(e, '"')+1;
		linehist(strdup(p), -1);
	}
	else
	if(strcmp(p, "noprofile") == 0)
		txtprof = 1;	
	else
	if(strcmp(p, "profile") == 0)
		txtprof = 0;
	else
	if(strcmp(p, "dynamic") == 0)
		polytab(e);
}

/* Eat preprocessor directives */
void
preprocessor(void)
{
	int i, c, off;
	static char *basef;
	char buf[Strsize], *p, *q;

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

	p = buf;
	while (*p == ' ' || *p == '\t')
		p++;

	if(strncmp(p, "line", 4) == 0) {
		p += 4;
		while (*p == ' ' || *p == '\t')
			 p++;
	}

	if(*p >= '0' && *p <= '9') {
		off = strtoul(p, &p, 10);
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == 0)
			fatal("%L: malformed preprocessor statement", line);

		q = strrchr(p+1, '"');
		if (*p != '"' || q == 0)
			fatal("%L: malformed #line", line);

		*q = '\0';
		p = strdup(p+1);
		if(basef == 0)
			basef = p;
		inbase = 0;
		if(strcmp(p, basef) == 0)
			inbase = 1;
		linehist(p, off);
		return;
	}
	if(strncmp(p, "pragma", 6) == 0)
		pragma(p+6);
}

String*
mkstring(char *fmt, ...)
{
	String *s;
	char buf[Strsize];

	doprint(buf, buf+sizeof(buf), fmt, (&fmt+1));
	s = malloc(sizeof(String));
	s->string = strdup(buf);
	s->len = strlen(buf)+1;

	return s;
}

void
eatstring(void)
{
	String *s;
	char buf[Strsize];
	int esc, c, cnt;

	esc = 0;
	for(cnt = 0;;) {
		c = Bgetc(bin);
		switch(c) {
		case Eof:
			fatal("%L: <eof> in string constant", line);

		case '\n':
			line++;
			diag(0, "newline in string constant");
			goto done;

		case '\\':
			if(esc) {
				buf[cnt++] = c;
				esc = 0;
				break;
			}
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
	buf[cnt] = '\0';
	s = malloc(sizeof(String));
	s->len = strlen(buf)+1;
	s->string = strdup(buf);
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
		if(opt('L'))
			print("%L\n", line);
		line++;
		goto loop;

	case '.':
		c = Bgetc(bin);
		Bungetc(bin);
		if(isdigit(c))
			return numsym('.');
		return '.';

 	case ':':
		c = Bgetc(bin);
		if(c == ':')
			return Titer;
		if(c == '=')
			return Tvasgn;
		Bungetc(bin);
		return ':';

	case '(':
	case ')':
	case '[':
	case ']':
	case ';':
	case ',':
	case '~':
	case '?':
	case '}':
	case '{':
	case '$':
		return c;

	case '+':
		c = Bgetc(bin);
		if(c == '=')
			return Taddeq;
		if(c == '+')
			return Tinc;
		Bungetc(bin);
		return '+';

	case '/':
		c = Bgetc(bin);
		if(c == '=')
			return Tdiveq;
		Bungetc(bin);
		return '/';

	case '%':
		c = Bgetc(bin);
		if(c == '=')
			return Tmodeq;
		Bungetc(bin);
		return '%';

	case '^':
		c = Bgetc(bin);
		if(c == '=')
			return Txoreq;
		Bungetc(bin);
		return '^';

	case '*':
		c = Bgetc(bin);
		if(c == '=')
			return Tmuleq;
		Bungetc(bin);
		return '*';

	case '\'':
		c = Bgetrune(bin);
		if(c == '\\')
			yylval.ival = escchar(Bgetc(bin));
		else
			yylval.ival = c;
		c = Bgetc(bin);
		if(c != '\'') {
			diag(0, "missing '");
			Bungetc(bin);
		}
		return Tconst;

	case '&':
		c = Bgetc(bin);
		if(c == '&')
			return Tandand;
		if(c == '=')
			return Tandeq;
		Bungetc(bin);
		return '&';

	case '=':
		c = Bgetc(bin);
		if(c == '=')
			return Teq;
		Bungetc(bin);
		return '=';

	case '|':
		c = Bgetc(bin);
		if(c == '|')
			return Toror;
		if(c == '=')
			return Toreq;
		Bungetc(bin);
		return '|';

	case '<':
		c = Bgetc(bin);
		if(c == '=')
			return Tleq;
		if(c == '<') {
			c = Bgetc(bin);
			if(c == '=')
				return Tlsheq;
			Bungetc(bin);
			return Tlsh;
		}
		if(c == '-')
			return Tcomm;
		Bungetc(bin);
		return '<';

	case '>':
		c = Bgetc(bin);
		if(c == '=')
			return Tgeq;
		if(c == '>') {
			c = Bgetc(bin);
			if(c == '=')
				return Trsheq;
			Bungetc(bin);
			return Trsh;
		}
		Bungetc(bin);
		return '>';

	case '!':
		c = Bgetc(bin);
		if(c == '=')
			return Tneq;
		if(c == '{')
			return Tguard;
		Bungetc(bin);
		return '!';

	case '-':
		c = Bgetc(bin);
		if(c == '>')
			return Tindir;
		if(c == '=')
			return Tsubeq;
		if(c == '-')
			return Tdec;
		Bungetc(bin);
		return '-';

	default:
		return numsym(c);
	}
}

int
numsym(char first)
{
	int c;
	char *p;
	Sym *s;
	Type *t;
	enum { Int, Hex, Frac, Expsign, Exp } state;

	symbol[0] = first;
	p = symbol;

	if(first == '.')
		state = Frac;
	else
		state = Int;

	if(isdigit(*p++) || state == Frac) {
		for(;;) {
			c = Bgetc(bin);
			if(c < 0)
				fatal("%L: <eof> eating numeric", line);

			if(c == '\n')
				line++;

			switch(state) {
			case Int:
				if(strchr("01234567890", c))
					break;
				switch(c) {
				case 'x':
				case 'X':
					state = Hex;
					break;
				case '.':
					state = Frac;
					break;
				case 'e':
				case 'E':
					state = Expsign;
					break;
				default:
					goto done;
				}
				break;
			case Hex:
				if(strchr("01234567890abcdefABCDEF", c) == 0)
					goto done;
				break;
			case Frac:
				if(strchr("01234567890", c))
					break;
				if(c == 'e' || c == 'E')
					state = Expsign;
				else
					goto done;
				break;
			case Expsign:
				state = Exp;
				if(c == '-' || c == '+')
					break;
				/* else fall through */
			case Exp:
				if(strchr("01234567890", c) == 0)
					goto done;
				break;
			}
			*p++ = c;
		}
	done:
		Bungetc(bin);
		*p = '\0';
		switch(state) {
		default:
			yylval.ival = strtoul(symbol, 0, 0);
			return Tconst;
		case Frac:
		case Expsign:
		case Exp:
			if(mpatof(symbol, &yylval.fval))
				diag(0, "floating point conversion overflow %s", symbol);
			return Tfconst;
		}

	}

	for(;;) {
		c = Bgetc(bin);
		if(c < 0)
			fatal("%L <eof> eating symbols", line);
		if(c != '_')
		if(!isalnum(c)) {
			Bungetc(bin);
			break;
		}
		*p++ = c;
	}

	*p = '\0';

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

	s = lookup(name);
	if(s != nil)
		return s;

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

	p = t->tag;
	if(p && p->ltype && typecmp(p->ltype, t, 5))
		return p;

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
			if(h->offset != 0) {	 
				/* #line directive, not #pragma */
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
		sprint(s, "%s:%d:", a[i].line->name, l-a[i].ldel+1);
		if(strlen(s)+strlen(str) >= Strsize-10)
			break;
		strcat(str, s);
		l = a[i].incl->line - 1;	
	}

	if(n == 0)
		strcat(str, "<eof> ");

	n = strlen(wd);
	if(strncmp(str, wd, n) != 0)
		n = 0;

	strconv(str+n, f);
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

void
umap(char *s)
{
	s = strchr(s, '_');
	if(s == 0)
		return;
	if(s[-1] == '\0' || s[1] == '\0')
		return;
	if(!isalnum(s[-1]) || !isalnum(s[1]))
		return;

	*s = '.';
}
