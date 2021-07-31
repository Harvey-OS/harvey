#include <u.h>
#include <libc.h>
#include <bio.h>

enum{
	Nfont = 11,
	Wid = 20,	/* tmac.anhtml sets page width to 20" so we can recognize .nf text */
};

typedef ulong Char;
typedef struct Troffchar Troffchar;
typedef struct Htmlchar Htmlchar;
typedef struct Font Font;
typedef struct HTMLfont HTMLfont;

/* a Char is 32 bits. low 16 bits are the rune. higher are attributes */
enum
{
	Italic	=	16,
	Bold	=	17,
	CW	=	18,
	Indent1 =	19,
	Indent2 =	20,
	Indent3 =	21,
	Heading =	25,
	Anchor =	26,	/* must be last */
};

struct Troffchar
{
	char *name;
	char *value;
};

struct Htmlchar
{
	int value;
	char *name;
};

#include "chars.h"

struct Font{
	char		*name;
	HTMLfont	*htmlfont;
};

struct HTMLfont{
	char	*name;
	char	*htmlname;
	int	bit;
};

/* R must be first; it's the default representation for fonts we don't recognize */
HTMLfont htmlfonts[] =
{
	"R",			nil,		0,
	"LucidaSans",	nil,		0,
	"I",			"EM",	Italic,
	"LucidaSansI",	"EM",	Italic,
	"CW",		"TT",		CW,
	"LucidaCW",	"TT",		CW,
	nil,	nil,
};

char*
onattr[8*sizeof(ulong)] = {
	[Italic]	=	"<EM>",
	[Bold]	=	"<B>",
	[CW]		=	"<TT>",
	[Indent1]	=	"<DL><DT><DD>",
	[Indent2]	=	"<DL><DT><DD>",
	[Indent3]	=	"<DL><DT><DD>",
	[Heading] =	"<H4>",
	[Anchor] =	"<UNUSED>",
};

char*
offattr[8*sizeof(ulong)] = {
	[Italic]	=	"</EM>",
	[Bold]	=	"</B>",
	[CW]		=	"</TT>",
	[Indent1]	=	"</DL>",
	[Indent2]	=	"</DL>",
	[Indent3]	=	"</DL>",
	[Heading] =	"</H4>",
	[Anchor] =	"</A>",
};

Font *font[Nfont];

Biobuf bout;
int	debug = 0;

/* troff state */
int	page = 1;
int	ft = 1;
int	vp = 0;
int	hp = 0;
int	ps = 1;
int	res = 720;

int		didP = 0;
int		atnewline = 1;
int		prevlineH = 0;
ulong	attr = 0;	/* or'ed into each Char */

Char		*chars;
int		nchars;
int		nalloc;
char**	anchors;	/* allocated in order */
int		nanchors;

char	*filename;
int	cno;
char	buf[8192];
char	*title = "Plan 9 man page";

void	process(Biobuf*, char*);
void	mountfont(int, char*);
void	switchfont(int);
void	header(char*);
void	flush(void);
void	trailer(void);

void*
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		sysfatal("malloc failed: %r");
	return p;
}

void*
erealloc(void *p, ulong n)
{

	p = realloc(p, n);
	if(p == nil)
		sysfatal("realloc failed: %r");
	return p;
}

char*
estrdup(char *s)
{
	char *t;

	t = strdup(s);
	if(t == nil)
		sysfatal("strdup failed: %r");
	return t;
}

void
usage(void)
{
	fprint(2, "usage: dhtml [-d] [-t title] [file ...]\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	int i;
	Biobuf in, *inp;

	ARGBEGIN{
	case 't':
		title = ARGF();
		if(title == nil)
			usage();
		break;
	case 'd':
		debug++;
		break;
	default:
		usage();
	}ARGEND
	Binit(&bout, 1, OWRITE);
	if(argc == 0){
		header(title);
		Binit(&in, 0, OREAD);
		process(&in, "<stdin>");
	}else{
		header(title);
		for(i=0; i<argc; i++){
			inp = Bopen(argv[i], OREAD);
			if(inp == nil)
				sysfatal("can't open %s: %r", argv[i]);
			process(inp, argv[i]);
			Bterm(inp);
		}
	}
	flush();
	trailer();
	exits(nil);
}

void
emitul(ulong ul)
{
	if(nalloc == nchars){
		nalloc += 10000;
		chars = realloc(chars, nalloc*sizeof(chars[0]));
		if(chars == nil)
			sysfatal("malloc failed: %r");
	}
	chars[nchars++] = ul;
}

void
emit(Rune r)
{
	emitul(r | attr);
}

void
emitstr(char *s)
{
	emitul(0);
	emitul((ulong)s);
}

void
flush(void)
{
	int i, anchor;
	ulong c, oattr, off, on, a, top;

	anchor = 0;
	oattr = 0;
	for(i=0; i<nchars; i++){
		c = chars[i];
		if(c == 0){
			/* next word is string to print */
			Bprint(&bout, "%s", (char*)chars[++i]);
			continue;
		}
		attr = c & ~0xFFFF;
		/* clear old attributes */
		off = (oattr^attr) & oattr;
		if(off){
			/* do fonts first, since they tend to nest best */
			for(a=16; a<=Anchor; a++)
				if(off & (1<<a))
					Bprint(&bout, "%s", offattr[a]);
		}
		/* set new attributes */
		on = (oattr^attr) & attr;
		if(on){
			/* before we turn on an attribute, we need to hold off all lower ones to maintain nesting */
			for(top=Anchor-1; top>=16; top--)
				if(on & (1<<top))
					break;
			for(a=16; a<=top; a++)
				if((oattr^off) & (1<<a))
					Bprint(&bout, "%s", offattr[a]);
			a = Anchor;
			if(on & (1<<a))	/* anchors are special */
				Bprint(&bout, "%s", anchors[anchor++]);
			while(--a >= 16)
				if(on & (1<<a))
					Bprint(&bout, "%s", onattr[a]);
			/* turn the 'held' ones back on */
			for(a=top; a>=16; --a)
				if((oattr^off) & (1<<a))
					Bprint(&bout, "%s", onattr[a]);
		}
		oattr = attr;
		Bputrune(&bout, c & 0xFFFF);
	}
}

void
header(char *s)
{
	Bprint(&bout, "<HEAD>\n");
	Bprint(&bout, "<TITLE>%s</TITLE>\n", s);
	Bprint(&bout, "<META content=\"text/html; charset=utf-8\" http-equiv=Content-Type>\n");
	Bprint(&bout, "</HEAD>\n");
	Bprint(&bout, "<BODY BGCOLOR=WHITE>\n");
}

void
trailer(void)
{
	Tm *t;

	t = localtime(time(nil));
	Bprint(&bout, "<BR><FONT size=1><A HREF=\"http://www.lucent.com/copyright.html\">\n");
	Bprint(&bout, "Copyright</A> &#169; %d Lucent Technologies.  All rights reserved.</FONT>\n", t->year+1900);
	Bprint(&bout, "\n</BODY></HTML>\n");
}

int
getc(Biobuf *b)
{
	cno++;
	return Bgetrune(b);
}

void
ungetc(Biobuf *b)
{
	cno--;
	Bungetrune(b);
}

char*
getline(Biobuf *b)
{
	int i, c;

	for(i=0; i<sizeof buf; i++){
		c = getc(b);
		if(c == Beof)
			return nil;
		buf[i] = c;
		if(c == '\n'){
			buf[i] = '\0';
			break;
		}
	}
	return buf;
}

int
getnum(Biobuf *b)
{
	int i, c;

	i = 0;
	for(;;){
		c = getc(b);
		if(c<'0' || '9'<c){
			ungetc(b);
			break;
		}
		i = i*10 + (c-'0');
	}
	return i;
}

char*
getstr(Biobuf *b)
{
	int i, c;

	for(i=0; i<sizeof buf; i++){
		/* must get bytes not runes */
		cno++;
		c = Bgetc(b);
		if(c == Beof)
			return nil;
		buf[i] = c;
		if(c == '\n' || c==' ' || c=='\t'){
			ungetc(b);
			buf[i] = '\0';
			break;
		}
	}
	return buf;
}

int
setnum(Biobuf *b, char *name, int min, int max)
{
	int i;

	i = getnum(b);
	if(debug > 2)
		fprint(2, "set %s = %d\n", name, i);
	if(min<=i && i<max)
		return i;
	sysfatal("value of %s is %d; min %d max %d at %s:#%d", name, i, min, max, filename, cno);
	return i;
}

void
xcmd(Biobuf *b)
{
	char *p, *fld[16], buf[256];
	int i, nfld;

	p = getline(b);
	if(p == nil)
		sysfatal("xcmd error: %r");
	if(debug)
		fprint(2, "x command '%s'\n", p);
	nfld = tokenize(p, fld, nelem(fld));
	if(nfld == 0)
		return;
	switch(fld[0][0]){
	case 'f':
		/* mount font */
		if(nfld != 3)
			break;
		i = atoi(fld[1]);
		if(i<0 || Nfont<=i)
			sysfatal("font %d out of range at %s:#%d", i, filename, cno);
		mountfont(i, fld[2]);
		return;
	case 'i':
		/* init */
		return;
	case 'r':
		if(nfld<2 || atoi(fld[1])!=res)
			sysfatal("typesetter has unexpected resolution %s", fld[1]? fld[1] : "<unspecified>");
		return;
	case 's':
		/* stop */
		return;
	case 't':
		/* trailer */
		return;
	case 'T':
		if(nfld!=2 || strcmp(fld[1], "utf")!=0)
			sysfatal("output for unknown typesetter type %s", fld[1]);
		return;
	case 'X':
		if(nfld<3 || strcmp(fld[1], "html")!=0)
			break;
		/* is it a man reference of the form cp(1)? */
		/* X manref start/end cp (1) */
		if(nfld==6 && strcmp(fld[2], "manref")==0){
			/* was the right macro; is it the right form? */
			if(strlen(fld[5])>=3 &&
			   fld[5][0]=='(' && fld[5][2]==')' &&
			   '0'<=fld[5][1] && fld[5][1]<='9'){
				if(strcmp(fld[3], "start") == 0){
					/* set anchor attribute and remember string */
					attr |= (1<<Anchor);
					snprint(buf, sizeof buf,
						"<A HREF=\"/magic/man2html/%c/%s\">",
						fld[5][1], fld[4]);
					nanchors++;
					anchors = erealloc(anchors, nanchors*sizeof(char*));
					anchors[nanchors-1] = estrdup(buf);
				}else if(strcmp(fld[3], "end") == 0)
					attr &= ~(1<<Anchor);
			}
		}else if(nfld<4 || strcmp(fld[2], "manref")!=0){
			if(nfld>2 && strcmp(fld[2], "<P>")==0){	/* avoid triggering extra <br> */
				didP = 1;
				/* clear all font attributes before paragraph */
				emitul(' ' | (attr & ~(0xFFFF|((1<<Italic)|(1<<Bold)|(1<<CW)))));
				emitstr("<P>");
				/* next emittec char will turn font attributes back on */
			}else if(nfld>2 && strcmp(fld[2], "<H4>")==0)
				attr |= (1<<Heading);
			else if(nfld>2 && strcmp(fld[2], "</H4>")==0)
				attr &= ~(1<<Heading);
			else if(debug)
				fprint(2, "unknown in-line html %s... at %s:%#d\n",
					fld[2], filename, cno);
		}
		return;
	}
	if(debug)
		fprint(2, "unknown or badly formatted x command %s\n", fld[0]);
}

int
lookup(int c, Htmlchar tab[], int ntab)
{
	int low, high, mid;

	low = 0;
	high = ntab - 1;
	while(low <= high){
		mid = (low+high)/2;
		if(c < tab[mid].value)
			high = mid - 1;
		else if(c > tab[mid].value)
			low = mid + 1;
		else
			return mid;
	}
	return -1;	/* no match */
}

void
emithtmlchar(int r)
{
	static char buf[10];
	int i;

	i = lookup(r, htmlchars, nelem(htmlchars));
	if(i >= 0)
		emitstr(htmlchars[i].name);
	else
		emit(r);
}

char*
troffchar(char *s)
{
	int i;

	for(i=0; troffchars[i].name!=nil; i++)
		if(strcmp(s, troffchars[i].name) == 0)
			return troffchars[i].value;
	return "??";
}

void
indent(void)
{
	int nind;

	didP = 0;
	if(atnewline){
		if(hp != prevlineH){
			prevlineH = hp;
			/* these most peculiar numbers appear in the troff -man output */
			nind = ((prevlineH-1*res)+323)/324;
			attr &= ~((1<<Indent1)|(1<<Indent2)|(1<<Indent3));
			if(nind >= 1)
				attr |= (1<<Indent1);
			if(nind >= 2)
				attr |= (1<<Indent2);
			if(nind >= 2)
				attr |= (1<<Indent3);
		}
		atnewline = 0;
	}
}

void
process(Biobuf *b, char *name)
{
	int c, r, v, i;
	char *p;

	cno = 0;
	prevlineH = res;
	filename = name;
	for(;;){
		c = getc(b);
		switch(c){
		case Beof:
			/* go to ground state */
			attr = 0;
			emit('\n');
			return;
		case '\n':
			break;
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			v = c-'0';
			c = getc(b);
			if(c<'0' || '9'<c)
				sysfatal("illegal character motion at %s:#%d", filename, cno);
			v = v*10 + (c-'0');
			hp += v;
			/* fall through to character case */
		case 'c':
			indent();
			r = getc(b);
			emithtmlchar(r);
			break;
		case 'f':
			v = setnum(b, "font", 0, Nfont);
			switchfont(v);
			break;
		case 'h':
			v = setnum(b, "hpos", -20000, 20000);
			/* generate spaces if motion is large and within a line */
			if(!atnewline && v>2*72)
				for(i=0; i<v; i+=72)
					emitstr("&nbsp;");
			hp += v;
			break;
		case 'n':
			setnum(b, "n1", -10000, 10000);
			//Bprint(&bout, " N1=%d", v);
			getc(b);	/* space separates */
			setnum(b, "n2", -10000, 10000);
			atnewline = 1;
			if(!didP && hp < (Wid-1)*res)	/* if line is less than 19" long, probably need a line break */
				emitstr("<br>");
			emit('\n');
			break;
		case 'p':
			page = setnum(b, "ps", -10000, 10000);
			break;
		case 's':
			ps = setnum(b, "ps", 1, 1000);
			break;
		case 'v':
			vp += setnum(b, "vpos", -10000, 10000);
			/* BUG: ignore motion */
			break;
		case 'x':
			xcmd(b);
			break;
		case 'w':
			emit(' ');
			break;
		case 'C':
			indent();
			p = getstr(b);
			emitstr(troffchar(p));
			break;
		case 'H':
			hp = setnum(b, "hpos", 0, 20000);
			//Bprint(&bout, " H=%d ", hp);
			break;
		case 'V':
			vp = setnum(b, "vpos", 0, 10000);
			break;
		default:
			fprint(2, "dhtml: unknown directive %c(0x%.2ux) at %s:#%d\n", c, c, filename, cno);
			return;
		}
	}
}

HTMLfont*
htmlfont(char *name)
{
	int i;

	for(i=0; htmlfonts[i].name!=nil; i++)
		if(strcmp(name, htmlfonts[i].name) == 0)
			return &htmlfonts[i];
	return &htmlfonts[0];
}

void
mountfont(int pos, char *name)
{
	if(debug)
		fprint(2, "mount font %s on %d\n", name, pos);
	if(font[pos] != nil){
		free(font[pos]->name);
		free(font[pos]);
	}
	font[pos] = emalloc(sizeof(Font));
	font[pos]->name = estrdup(name);
	font[pos]->htmlfont = htmlfont(name);
}

void
switchfont(int pos)
{
	HTMLfont *hf;

	if(debug)
		fprint(2, "font change from %d (%s) to %d (%s)\n", ft, font[ft]->name, pos, font[pos]->name);
	if(pos == ft)
		return;
	hf = font[ft]->htmlfont;
	if(hf->bit != 0)
		attr &= ~(1<<hf->bit);
	ft = pos;
	hf = font[ft]->htmlfont;
	if(hf->bit != 0)
		attr |= (1<<hf->bit);
}
