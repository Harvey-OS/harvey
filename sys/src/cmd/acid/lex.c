#include <u.h>
#include <libc.h>
#include <bio.h>
#include <ctype.h>
#include <mach.h>
#define Extern extern
#include "acid.h"
#include "y.tab.h"

#define Ungetc(s)	{ if(s == '\n') line--; Bungetc(bin); }
int Lconv(void*, int, int, int, int);

struct keywd
{
	char	*name;
	int	terminal;
}
keywds[] =
{
	"do",		Tdo,
	"if",		Tif,
	"then",		Tthen,
	"else",		Telse,
	"while",	Twhile,
	"loop",		Tloop,
	"head",		Thead,
	"tail",		Ttail,
	"append",	Tappend,
	"defn",		Tfn,
	"return",	Tret,
	"local",	Tlocal,
	"struct",	Tcomplex,
	"union",	Tcomplex,
	"aggr",		Tcomplex,
	"adt",		Tcomplex,
	"complex",	Tcomplex,
	"delete",	Tdelete,
	"whatis",	Twhat,
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
				error("%d: <eof> in escape sequence", line);
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

void
eatstring(void)
{
	int esc, c, cnt;
	char buf[Strsize];

	esc = 0;
	for(cnt = 0;;) {
		c = Bgetc(bin);
		switch(c) {
		case Eof:
			error("%d: <eof> in string constant", line);

		case '\n':
			error("newline in string constant");
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
	buf[cnt] = '\0';
	yylval.string = strnode(buf);
}

void
eatnl(void)
{
	int c;

	line++;
	for(;;) {
		c = Bgetc(bin);
		if(c == Eof)
			error("eof in comment");
		if(c == '\n')
			return;
	}
}

int
yylex(void)
{
	int c;

loop:
	Bflush(bout);
	c = Bgetc(bin);
	switch(c) {
	case Eof:
		if(gotint) {
			gotint = 0;
			stacked = 0;
			Binit(bin, 0, OREAD);
			Bprint(bout, "\nacid: ");
			goto loop;
		}
		return Eof;

	case '"':
		eatstring();
		return Tstring;

	case ' ':
	case '\t':
		goto loop;

	case '\n':
		line++;
		if(interactive == 0)
			goto loop;
		if(stacked) {
			print("\t");
			goto loop;
		}
		return ';';

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
	case '*':
	case '@':
	case '^':
	case '%':
		return c;
	case '{':
		stacked++;
		return c;
	case '}':
		stacked--;
		return c;

	case '!':
		c = Bgetc(bin);
		if(c == '=')
			return Tneq;
		Ungetc(c);
		return '!';

	case '+':
		c = Bgetc(bin);
		if(c == '+')
			return Tinc;
		Ungetc(c);
		return '+';

	case '/':
		c = Bgetc(bin);
		if(c == '/') {
			eatnl();
			goto loop;
		}
		Ungetc(c);
		return '/';

	case '\'':
		c = Bgetc(bin);
		if(c == '\\')
			yylval.ival = escchar(Bgetc(bin));
		else
			yylval.ival = c;
		c = Bgetc(bin);
		if(c != '\'') {
			error("missing '");
			Ungetc(c);
		}
		return Tconst;

	case '&':
		c = Bgetc(bin);
		if(c == '&')
			return Tandand;
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
		Ungetc(c);
		return '|';

	case '<':
		c = Bgetc(bin);
		if(c == '=')
			return Tleq;
		if(c == '<')
			return Tlsh;
		Ungetc(c);
		return '<';

	case '>':
		c = Bgetc(bin);
		if(c == '=')
			return Tgeq;
		if(c == '>')
			return Trsh;
		Ungetc(c);
		return '>';

	case '-':
		c = Bgetc(bin);
		if(c == '>')
			return Tindir;
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
	Lsym *s;

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
				error("%d: <eof> eating symbols", line);

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
			error("%d <eof> eating symbols", line);
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

	s = look(symbol);
	if(s == 0)
		s = enter(symbol, Tid);

	yylval.sym = s;
	return s->lexval;
}

Lsym*
enter(char *name, int t)
{
	Lsym *s;
	ulong h;
	char *p;
	Value *v;

	h = 0;
	for(p = name; *p; p++)
		h = h*3 + *p;
	h %= Hashsize;

	s = gmalloc(sizeof(Lsym));
	memset(s, 0, sizeof(Lsym));
	s->name = strdup(name);

	s->hash = hash[h];
	hash[h] = s;
	s->lexval = t;

	v = gmalloc(sizeof(Value));
	s->v = v;

	v->fmt = 'X';
	v->type = TINT;
	memset(v, 0, sizeof(Value));

	return s;
}

Lsym*
look(char *name)
{
	Lsym *s;
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

Lsym*
mkvar(char *s)
{
	Lsym *l;

	l = look(s);
	if(l == 0)
		l = enter(s, Tid);
	return l;
}
