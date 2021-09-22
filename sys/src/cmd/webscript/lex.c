#include <u.h>
#include <libc.h>
#include <bio.h>
#include <draw.h>
#include <html.h>
#include "dat.h"
#include "y.tab.h"

Biobuf	*bscript;
char	*filename;
int		lineno;
int		lexerrors;
int		yylexdebug;

static	char		*linebuf;

static char*
skipspace(char *s)
{
	while(*s == ' ' || *s == '\t' || *s == '\r')
		s++;
	return s;
}

static char escaper[] = "abnrtv\"";
static char escapee[] = "\a\b\n\r\t\v\"";

static int
parsequote(char *p, char **pp)
{
	char *op, *q, *w;
	
	op = p;
	w = yylval.s;
	for(p++; *p && *p != '\"' && w<yylval.s+sizeof yylval.s; p++){
		if(*p == '\\'){
			if(*(p+1) == 0){
				fprint(2, "%s:%d: quoted string ends in \\\n", filename, lineno);
				lexerrors++;
				p++;
				goto out;
			}
			p++;
			if((q = strchr(escaper, *p)) != nil)
				*w++ = escapee[q-escaper];
			else{
				fprint(2, "%s:%d: unknown escape \\%c\n", filename, lineno, *p);
				lexerrors++;
				continue;
			}
		}else
			*w++ = *p;
	}
	if(*p == '\"')
		p++;
	else if(w >= yylval.s+sizeof yylval.s){
		w = yylval.s+sizeof yylval.s-1;
		fprint(2, "%s:%d: string too long: %.*s...\n", 
			filename, lineno, utfnlen(op, p-op), op);
		lexerrors++;
		while(*p != 0 && *p != '\"'){
			if(*p == '\\' && *(p+1))
				p++;
			p++;
		}
	}else{
		fprint(2, "%s:%d: missing terminating \"\n", filename, lineno);
		lexerrors++;
	}

out:
	*w = 0;
	*pp = p;
	return STRING;
}

struct 
{
	char *s;
	int i;
} keywords[] =
{
	"cell",		CELL,
	"checkbox",	CHECKBOX,
	"else",		ELSE,
	"find",		FIND,
	"form",		FORM,
	"if",		IF,
	"inner",	INNER,
	"input",	INPUT,
	"load",		LOAD,
	"next",		NEXT,
	"outer",	OUTER,
	"page",		PAGE,
	"password",	PASSWORD,
	"prev",		PREV,
	"print",	PRINT,
	"radiobutton",	RADIOBUTTON,
	"row",		ROW,
	"select",		SELECT,
	"string",	STRING,
	"submit",	SUBMIT,
	"table",	TABLE,
	"textbox",	TEXTBOX,
	"textarea",	TEXTAREA,
};

static int
iskeyword(char *s)
{
	int i;
	
	for(i=0; i<nelem(keywords); i++)
		if(strcmp(keywords[i].s, s) == 0)
			return keywords[i].i;
	return 0;
}

int
_yylex(void)
{
	char *p, *q;
	int i;
	
next:
	if(linebuf == nil){
		linebuf = Brdline(bscript, '\n');
		if(linebuf == nil){
			if(Blinelen(bscript))
				fprint(2, "%s:%d: very long line\n", filename, lineno);
			strcpy(yylval.s, "EOF");
			return 0;
		}
		linebuf[Blinelen(bscript)-1] = 0;
		lineno++;
	}
	p = skipspace(linebuf);
	switch(*p){
	case 0:
		linebuf = nil;
		return '\n';
	case '#':
		linebuf = nil;
		return '\n';
	case ';':
		*p = '\n';
	case '\n':
	case '!':
	case '(':
	case ')':
	case '{':
	case '}':
		linebuf = p+1;
		yylval.s[0] = *p;
		yylval.s[1] = 0;
		return *p;
//	case '/':
//		return parseregexp(p, &line);
	case '\"':
		return parsequote(p, &linebuf);
	default:
		q = strpbrk(p, " \t\r\n({}),;!");
		if(q == nil)
			q = p+strlen(p);
		linebuf = q;
		if(q-p >= sizeof yylval.s){
			fprint(2, "%s:%d: very long keyword: %.*s\n",
				filename, lineno, utfnlen(p, q-p), p);
			lexerrors++;
			goto next;
		}
		memmove(yylval.s, p, q-p);
		yylval.s[q-p] = 0;
		if((i=iskeyword(yylval.s)) == 0){
			fprint(2, "%s:%d: ignoring unexpected keyword: %s\n", 
				filename, lineno, yylval.s);
			lexerrors++;
			goto next;
		}
		return i;
	}
}

int
yylex(void)
{
	int i;
	
	i = _yylex();
	if(yylexdebug){
		if(i == '\n')
			print("<\\n>\n");
		else
			print("<%s>", yylval.s);
	}
	return i;
}

int
yyerror(char *msg)
{
	sysfatal("%s:%d: %s", filename, lineno, msg);
	return 0;
}

