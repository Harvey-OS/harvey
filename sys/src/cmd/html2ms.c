#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>

enum
{
	SSIZE = 10,

	/* list types */
	Lordered = 0,
	Lunordered,
	Lmenu,
	Ldir,

};

Biobuf in, out;
int lastc = '\n';
int inpre = 0;

/* stack for fonts */
char *fontstack[SSIZE];
char *font = "R";
int fsp;

/* stack for lists */
struct
{
	int	type;
	int	ord;
} liststack[SSIZE];
int lsp;

int quoting;

typedef struct Goobie Goobie;
struct Goobie
{
	char *name;
	void (*f)(Goobie*, char*);
	void (*ef)(Goobie*, char*);
};

void	eatwhite(void);
void	escape(void);

typedef void Action(Goobie*, char*);

Action	g_ignore;
Action	g_unexpected;
Action	g_title;
Action	g_p;
Action	g_h;
Action	g_li;
Action	g_list, g_listend;
Action	g_pre;
Action	g_fpush, g_fpop;
Action	g_indent, g_exdent;
Action	g_dt;
Action	g_display;
Action	g_displayend;
Action	g_table, g_tableend, g_caption, g_captionend;
Action	g_br, g_hr;

Goobie gtab[] =
{
	"!--",		g_ignore,	g_unexpected,
	"!doctype",	g_ignore,	g_unexpected,
	"a",		g_ignore,	g_ignore,
	"address",	g_display,	g_displayend,
	"b",		g_fpush,	g_fpop,
	"base",		g_ignore,	g_unexpected,
	"blink",	g_ignore,	g_ignore,
	"blockquote",	g_ignore,	g_ignore,
	"body",		g_ignore,	g_ignore,
	"br",		g_br,		g_unexpected,
	"caption",	g_caption,	g_captionend,
	"center",	g_ignore,	g_ignore,
	"cite",		g_ignore,	g_ignore,
	"code",		g_ignore,	g_ignore,
	"dd",		g_ignore,	g_unexpected,
	"dfn",		g_ignore,	g_ignore,
	"dir",		g_list,		g_listend,
	"dl",		g_indent,	g_exdent,
	"dt",		g_dt,		g_unexpected,
	"em",		g_ignore,	g_ignore,
	"font",		g_ignore,	g_ignore,
	"form",		g_ignore,	g_ignore,
	"h1",		g_h,		g_p,
	"h2",		g_h,		g_p,
	"h3",		g_h,		g_p,
	"h4",		g_h,		g_p,
	"h5",		g_h,		g_p,
	"h6",		g_h,		g_p,
	"head",		g_ignore,	g_ignore,
	"hr",		g_hr,		g_unexpected,
	"html",		g_ignore,	g_ignore,
	"i",		g_fpush,	g_fpop,
	"input",	g_ignore,	g_unexpected,
	"img",		g_ignore,	g_unexpected,
	"isindex",	g_ignore,	g_unexpected,
	"kbd",		g_fpush,	g_fpop,
	"key",		g_ignore,	g_ignore,
	"li",		g_li,		g_unexpected,
	"link",		g_ignore,	g_unexpected,
	"listing",	g_ignore,	g_ignore,
	"menu",		g_list,		g_listend,
	"meta",		g_ignore,	g_unexpected,
	"nextid",	g_ignore,	g_unexpected,
	"ol",		g_list,		g_listend,
	"option",	g_ignore,	g_unexpected,
	"p",		g_p,		g_ignore,
	"plaintext",	g_ignore,	g_unexpected,
	"pre",		g_pre,		g_displayend,
	"samp",		g_ignore,	g_ignore,
	"select",	g_ignore,	g_ignore,
	"strong",	g_ignore,	g_ignore,
	"table",	g_table,	g_tableend,
	"textarea",	g_ignore,	g_ignore,
	"title",	g_title,	g_ignore,
	"tt",		g_fpush,	g_fpop,
	"u",		g_ignore,	g_ignore,
	"ul",		g_list,		g_listend,
	"var",		g_ignore,	g_ignore,
	"xmp",		g_ignore,	g_ignore,
	0,		0,	0,
};

typedef struct Entity Entity;
struct Entity
{
	char *name;
	Rune value;
};

Entity pl_entity[]=
{
"#SPACE", L' ', "#RS",   L'\n', "#RE",   L'\r', "quot",   L'"',
"AElig",  L'Æ', "Aacute", L'Á', "Acirc",  L'Â', "Agrave", L'À', "Aring",  L'Å',
"Atilde", L'Ã', "Auml",   L'Ä', "Ccedil", L'Ç', "ETH",    L'Ð', "Eacute", L'É',
"Ecirc",  L'Ê', "Egrave", L'È', "Euml",   L'Ë', "Iacute", L'Í', "Icirc",  L'Î',
"Igrave", L'Ì', "Iuml",   L'Ï', "Ntilde", L'Ñ', "Oacute", L'Ó', "Ocirc",  L'Ô',
"Ograve", L'Ò', "Oslash", L'Ø', "Otilde", L'Õ', "Ouml",   L'Ö', "THORN",  L'Þ',
"Uacute", L'Ú', "Ucirc",  L'Û', "Ugrave", L'Ù', "Uuml",   L'Ü', "Yacute", L'Ý',
"aacute", L'á', "acirc",  L'â', "aelig",  L'æ', "agrave", L'à', "amp",    L'&',
"aring",  L'å', "atilde", L'ã', "auml",   L'ä', "ccedil", L'ç', "eacute", L'é',
"ecirc",  L'ê', "egrave", L'è', "eth",    L'ð', "euml",   L'ë', "gt",     L'>',
"iacute", L'í', "icirc",  L'î', "igrave", L'ì', "iuml",   L'ï', "lt",     L'<',
"ntilde", L'ñ', "oacute", L'ó', "ocirc",  L'ô', "ograve", L'ò', "oslash", L'ø',
"otilde", L'õ', "ouml",   L'ö', "szlig",  L'ß', "thorn",  L'þ', "uacute", L'ú',
"ucirc",  L'û', "ugrave", L'ù', "uuml",   L'ü', "yacute", L'ý', "yuml",   L'ÿ',
0
};

int
cistrcmp(char *a, char *b)
{
	int c, d;

	for(;; a++, b++){
		d = tolower(*a);
		c = d - tolower(*b);
		if(c)
			break;
		if(d == 0)
			break;
	}
	return c;
}

int
readupto(char *buf, int n, char d, char notme)
{
	char *p;
	int c;

	buf[0] = 0;
	for(p = buf;; p++){
		c = Bgetc(&in);
		if(c < 0){
			*p = 0;
			return -1;
		}
		if(c == notme){
			Bungetc(&in);
			return -1;
		}
		if(c == d){
			*p = 0;
			return 0;
		}
		*p = c;
		if(p == buf + n){
			*p = 0;
			Bprint(&out, "<%s", buf);
			return -1;
		}
	}
}

void
dogoobie(void)
{
	char *arg, *type;
	Goobie *g;
	char buf[1024];
	int closing;

	if(readupto(buf, sizeof(buf), '>', '<') < 0){
		Bprint(&out, "<%s", buf);
		return;
	}
	type = buf;
	if(*type == '/'){
		type++;
		closing = 1;
	} else
		closing = 0;
	arg = strchr(type, ' ');
	if(arg == 0)
		arg = strchr(type, '\r');
	if(arg == 0)
		arg = strchr(type, '\n');
	if(arg)
		*arg++ = 0;
	for(g = gtab; g->name; g++)
		if(cistrcmp(type, g->name) == 0){
			if(closing){
				if(g->ef){
					(*g->ef)(g, arg);
					return;
				}
			} else {
				if(g->f){
					(*g->f)(g, arg);
					return;
				}
			}
		}
	if(closing)
		type--;
	if(arg)
		Bprint(&out, "<%s %s>\n", type, arg);
	else
		Bprint(&out, "<%s>\n", type);
}

void
main(void)
{
	int c, pos;

	Binit(&in, 0, OREAD);
	Binit(&out, 1, OWRITE);

	pos = 0;
	for(;;){
		c = Bgetc(&in);
		if(c < 0)
			return;
		switch(c){
		case '<':
			dogoobie();
			break;
		case '&':
			escape();
			break;
		case '\r':
			pos = 0;
			break;
		case '\n':
			if(quoting){
				Bputc(&out, '"');
				quoting = 0;
			}
			if(lastc != '\n')
				Bputc(&out, '\n');
			/* can't emit leading spaces in filled troff docs */
			if (!inpre)
				eatwhite();
			lastc = c;
			break;
		default:
			++pos;
			if(!inpre && isascii(c) && isspace(c) && pos > 80){
				Bputc(&out, '\n');
				eatwhite();
				pos = 0;
			}else
				Bputc(&out, c);
			lastc = c;
			break;
		}
	}
}

void
escape(void)
{
	int c;
	Entity *e;
	char buf[8];

	if(readupto(buf, sizeof(buf), ';', '\n') < 0){
		Bprint(&out, "&%s", buf);
		return;
	}
	for(e = pl_entity; e->name; e++)
		if(strcmp(buf, e->name) == 0){
			Bprint(&out, "%C", e->value);
			return;
		}
	if(*buf == '#'){
		c = atoi(buf+1);
		if(isascii(c) && isprint(c)){
			Bputc(&out, c);
			return;
		}
	}
	Bprint(&out, "&%s;", buf);
}

/*
 * whitespace is not significant to HTML, but newlines
 * and leading spaces are significant to troff.
 */
void
eatwhite(void)
{
	int c;

	for(;;){
		c = Bgetc(&in);
		if(c < 0)
			break;
		if(!isspace(c)){
			Bungetc(&in);
			break;
		}
	}
}

/*
 *  print at start of line
 */
void
printsol(char *fmt, ...)
{
	va_list arg;

	if(quoting){
		Bputc(&out, '"');
		quoting = 0;
	}
	if(lastc != '\n')
		Bputc(&out, '\n');
	va_start(arg, fmt);
	Bvprint(&out, fmt, arg);
	va_end(arg);
	lastc = '\n';
}

void
g_ignore(Goobie *g, char *arg)
{
	USED(g, arg);
}

void
g_unexpected(Goobie *g, char *arg)
{
	USED(arg);
	fprint(2, "unexpected %s ending\n", g->name);
}

void
g_title(Goobie *g, char *arg)
{
	USED(arg);
	printsol(".TL\n", g->name);
}

void
g_p(Goobie *g, char *arg)
{
	USED(arg);
	printsol(".LP\n", g->name);
}

void
g_h(Goobie *g, char *arg)
{
	USED(arg);
	printsol(".SH %c\n", g->name[1]);
}

void
g_list(Goobie *g, char *arg)
{
	USED(arg);

	if(lsp != SSIZE){
		switch(g->name[0]){
		case 'o':
			liststack[lsp].type  = Lordered;
			liststack[lsp].ord = 0;
			break;
		default:
			liststack[lsp].type = Lunordered;
			break;
		}
	}
	lsp++;
}

void
g_br(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".br\n");
}

void
g_li(Goobie *g, char *arg)
{
	USED(g, arg);
	if(lsp <= 0 || lsp > SSIZE){
		printsol(".IP \\(bu\n");
		return;
	}
	switch(liststack[lsp-1].type){
	case Lunordered:
		printsol(".IP \\(bu\n");
		break;
	case Lordered:
		printsol(".IP %d\n", ++liststack[lsp-1].ord);
		break;
	}
}

void
g_listend(Goobie *g, char *arg)
{
	USED(g, arg);
	if(--lsp < 0)
		lsp = 0;
	printsol(".LP\n");
}

void
g_display(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".DS\n");
}

void
g_pre(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".DS L\n");
	inpre = 1;
}

void
g_displayend(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".DE\n");
	inpre = 0;
}

void
g_fpush(Goobie *g, char *arg)
{
	USED(arg);
	if(fsp < SSIZE)
		fontstack[fsp] = font;
	fsp++;
	switch(g->name[0]){
	case 'b':
		font = "B";
		break;
	case 'i':
		font = "I";
		break;
	case 'k':		/* kbd */
	case 't':		/* tt */
		font = "(CW";
		break;
	}
	Bprint(&out, "\\f%s", font);
}

void
g_fpop(Goobie *g, char *arg)
{
	USED(g, arg);
	fsp--;
	if(fsp < SSIZE)
		font = fontstack[fsp];
	else
		font = "R";

	Bprint(&out, "\\f%s", font);
}

void
g_indent(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".RS\n");
}

void
g_exdent(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".RE\n");
}

void
g_dt(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".IP \"");
	quoting = 1;
}

void
g_hr(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".br\n");
	printsol("\\l'5i'\n");
}


/*
<table border>
<caption><font size="+1"><b>Cumulative Class Data</b></font></caption>
<tr><th rowspan=2>DOSE<br>mg/kg</th><th colspan=2>PARALYSIS</th><th colspan=2>DEATH</th>
</tr>
<tr><th width=80>Number</th><th width=80>Percent</th><th width=80>Number</th><th width=80>Percent</th>
</tr>
<tr align=center>
<td>0.1</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
<tr align=center>
<td>0.2</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
<tr align=center>
<td>0.3</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
<tr align=center>
<td>0.4</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
<tr align=center>
<td>0.5</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
<tr align=center>
<td>0.6</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
<tr align=center>
<td>0.7</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
<tr align=center>
<td>0.8</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
<tr align=center>
<td>0.8 oral</td><td><br></td> <td><br></td> <td><br></td> <td><br></td>
</tr>
</table>
*/

void
g_table(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".TS\ncenter ;\n");
}

void
g_tableend(Goobie *g, char *arg)
{
	USED(g, arg);
	printsol(".TE\n");
}

void
g_caption(Goobie *g, char *arg)
{
	USED(g, arg);
}

void
g_captionend(Goobie *g, char *arg)
{
	USED(g, arg);
}
