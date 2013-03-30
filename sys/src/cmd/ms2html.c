#include <u.h>
#include <libc.h>
#include <ctype.h>
#include <bio.h>

enum
{
	SSIZE = 10,

	Maxnh=	8,		/* highest NH level */
	HH=	4,		/* heading level used for SH and NH */
	Maxmstack=	10,	/* deepest macro/string nesting */
	Narg=	20,		/* max args to a macro */
	Maxsstack=	5,	/* deepest nesting of .so's */
	Nline=	1024,
	Maxget= 10,
	Maxif = 20,
	Maxfsp = 100,

	/* list types */
	Lordered = 1,
	Lunordered,
	Ldef,
	Lother,
};

char *delim = "$$";
char *basename;
char *title;
int eqnmode;

int 	quiet;
float	indent; /* from .in */
Biobuf	bout;
int	isup;
int	isdown;
int	debug;

int nh[Maxnh];
int ifwastrue[Maxif];

int list, listnum, example;
int hangingau, hangingdt, hanginghead, hangingcenter;
int indirective, paragraph, sol, titleseen, ignore_nl, weBref;
void dohangingcenter(void);

typedef struct Goobie Goobie;
typedef struct Goobieif Goobieif;
struct Goobie
{
	char *name;
	void (*f)(int, char**);
};

typedef void F(int, char**);
typedef void Fif(char*, char*);

struct Goobieif
{
	char *name;
	Fif *f;
};

/* if, ie */
Fif g_as, g_ds, g_el, g_ie, g_if;
Goobieif gtabif[] = {
	{ "as", g_as },
	{ "ds", g_ds },
	{ "if", g_if },
	{ "ie", g_ie },
	{ "el", g_el },
	{ nil, nil },
	};

/* pseudo ops */
F g_notyet, g_ignore, g_hrule, g_startgif;

/* ms macros */
F g_AU, g_B, g_BI, g_CW, g_I, g_IP, g_LP, g_PP, g_SH, g_NH;
F g_P1, g_P2, g_TL, g_R, g_AB, g_AE, g_EQ, g_TS, g_TE, g_FS, g_FE;
F g_PY, g_IH, g_MH, g_HO, g_BX, g_QS, g_QE, g_RS, g_RE;

/* pictures macro */
F g_BP;

/* real troff */
F g_br, g_ft, g_sp, g_de, g_lf, g_so, g_rm, g_in;
F g_nr, g_ig, g_RT, g_BS, g_BE, g_LB, g_ta;

/* macros to include ML in output */
F g__H, g__T;

Goobie gtab[] =
{
	{ "_T", g__T, },
	{ "_H", g__H, },
	{ "1C",	g_ignore, },
	{ "2C",	g_ignore, },
	{ "AB", g_AB, },
	{ "AE", g_AE, },
	{ "AI", g_ignore, },
	{ "AU", g_AU, },
	{ "B",	g_B, },
	{ "B1", g_hrule, },
	{ "B2", g_hrule, },
	{ "BI",	g_BI, },
	{ "BP",	g_BP, },
	{ "BT",	g_ignore, },
	{ "BX",	g_BX, },
	{ "CW",	g_CW, },
	{ "CT",	g_ignore, },
	{ "DA",	g_ignore, },
	{ "DE",	g_P2, },
	{ "DS",	g_P1, },
	{ "EG",	g_ignore, },
	{ "EN",	g_ignore, },
	{ "EQ",	g_startgif, },
	{ "FE",	g_FE, },
	{ "FP",	g_ignore, },
	{ "FS",	g_FS, },
	{ "HO",	g_HO, },
	{ "I",	g_I, },
	{ "IH",	g_IH, },
	{ "IM",	g_ignore, },
	{ "IP",	g_IP, },
	{ "KE",	g_ignore, },
	{ "KF",	g_ignore, },
	{ "KS",	g_ignore, },
	{ "LG",	g_ignore, },
	{ "LP",	g_LP, },
	{ "LT",	g_ignore, },
	{ "MF",	g_ignore, },
	{ "MH",	g_MH, },
	{ "MR",	g_ignore, },
	{ "ND",	g_ignore, },
	{ "NH",	g_NH, },
	{ "NL",	g_ignore, },
	{ "P1",	g_P1, },
	{ "P2",	g_P2, },
	{ "PE",	g_ignore, },
	{ "PF",	g_ignore, },
	{ "PP",	g_PP, },
	{ "PS",	g_startgif, },
	{ "PY",	g_PY, },
	{ "QE",	g_QE, },
	{ "QP",	g_QS, },
	{ "QS",	g_QS, },
	{ "R",		g_R, },
	{ "RE",	g_RE, },
	{ "RP",	g_ignore, },
	{ "RS",	g_RS, },
	{ "SG",	g_ignore, },
	{ "SH",	g_SH, },
	{ "SM",	g_ignore, },
	{ "TA",	g_ignore, },
	{ "TE",	g_ignore, },
	{ "TH",	g_TL, },
	{ "TL",	g_TL, },
	{ "TM",	g_ignore, },
	{ "TR",	g_ignore, },
	{ "TS",	g_startgif, },
	{ "UL",	g_I, },
	{ "UX",	g_ignore, },
	{ "WH",	g_ignore, },
	{ "RT", 	g_RT, },

	{ "br",	g_br, },
	{ "ti",		g_br, },
	{ "nf",	g_P1, },
	{ "fi",		g_P2, },
	{ "ft",		g_ft, },
	{ "sp", 	g_sp, },
	{ "rm", 	g_rm, },
	{ "de", 	g_de, },
	{ "am", 	g_de, },
	{ "lf", 	g_lf, },
	{ "so", 	g_so, },
	{ "ps", 	g_ignore },
	{ "vs", 	g_ignore },
	{ "nr", 	g_nr },
	{ "in", 	g_in },
	{ "ne", 	g_ignore },
	{ "ig", 	g_ig },
	{ "BS", 	g_BS },
	{ "BE", 	g_BE },
	{ "LB", 	g_LB },
	{ nil, nil },
};

typedef struct Entity Entity;
struct Entity
{
	char *name;
	int value;
};

Entity entity[] =
{
	{ "&#SPACE;",	L' ', },
	{ "&#RS;",		L'\n', },
	{ "&#RE;",		L'\r', },
	{ "&quot;",		L'"', },
	{ "&AElig;",	L'Æ', },
	{ "&Aacute;",	L'Á', },
	{ "&Acirc;",	L'Â', },
	{ "&Agrave;",	L'À', },
	{ "&Aring;",	L'Å', },
	{ "&Atilde;",	L'Ã', },
	{ "&Auml;",	L'Ä', },
	{ "&Ccedil;",	L'Ç', },
	{ "&ETH;",		L'Ð', },
	{ "&Eacute;",	L'É', },
	{ "&Ecirc;",	L'Ê', },
	{ "&Egrave;",	L'È', },
	{ "&Euml;",	L'Ë', },
	{ "&Iacute;",	L'Í', },
	{ "&Icirc;",		L'Î', },
	{ "&Igrave;",	L'Ì', },
	{ "&Iuml;",		L'Ï', },
	{ "&Ntilde;",	L'Ñ', },
	{ "&Oacute;",	L'Ó', },
	{ "&Ocirc;",	L'Ô', },
	{ "&Ograve;",	L'Ò', },
	{ "&Oslash;",	L'Ø', },
	{ "&Otilde;",	L'Õ', },
	{ "&Ouml;",	L'Ö', },
	{ "&THORN;",	L'Þ', },
	{ "&Uacute;",	L'Ú', },
	{ "&Ucirc;",	L'Û', },
	{ "&Ugrave;",	L'Ù', },
	{ "&Uuml;",	L'Ü', },
	{ "&Yacute;",	L'Ý', },
	{ "&aacute;",	L'á', },
	{ "&acirc;",	L'â', },
	{ "&aelig;",	L'æ', },
	{ "&agrave;",	L'à', },
	{ "&amp;",		L'&', },
	{ "&aring;",	L'å', },
	{ "&atilde;",	L'ã', },
	{ "&auml;",	L'ä', },
	{ "&ccedil;",	L'ç', },
	{ "&eacute;",	L'é', },
	{ "&ecirc;",	L'ê', },
	{ "&egrave;",	L'è', },
	{ "&eth;",		L'ð', },
	{ "&euml;",	L'ë', },
	{ "&gt;",		L'>', },
	{ "&iacute;",	L'í', },
	{ "&icirc;",		L'î', },
	{ "&igrave;",	L'ì', },
	{ "&iuml;",		L'ï', },
	{ "&lt;",		L'<', },
	{ "&ntilde;",	L'ñ', },
	{ "&oacute;",	L'ó', },
	{ "&ocirc;",	L'ô', },
	{ "&ograve;",	L'ò', },
	{ "&oslash;",	L'ø', },
	{ "&otilde;",	L'õ', },
	{ "&ouml;",	L'ö', },
	{ "&szlig;",		L'ß', },
	{ "&thorn;",	L'þ', },
	{ "&uacute;",	L'ú', },
	{ "&ucirc;",	L'û', },
	{ "&ugrave;",	L'ù', },
	{ "&uuml;",	L'ü', },
	{ "&yacute;",	L'ý', },
	{ "&yuml;",	L'ÿ', },
	{ "&#161;",	L'¡', },
	{ "&#162;",	L'¢', },
	{ "&#163;",	L'£', },
	{ "&#164;",	L'¤', },
	{ "&#165;",	L'¥', },
	{ "&#166;",	L'¦', },
	{ "&#167;",	L'§', },
	{ "&#168;",	L'¨', },
	{ "&#169;",	L'©', },
	{ "&#170;",	L'ª', },
	{ "&#171;",	L'«', },
	{ "&#172;",	L'¬', },
	{ "&#173;",	L'­', },
	{ "&#174;",	L'®', },
	{ "&#175;",	L'¯', },
	{ "&#176;",	L'°', },
	{ "&#177;",	L'±', },
	{ "&#178;",	L'²', },
	{ "&#179;",	L'³', },
	{ "&#180;",	L'´', },
	{ "&#181;",	L'µ', },
	{ "&#182;",	L'¶', },
	{ "&#183;",	L'·', },
	{ "&#184;",	L'¸', },
	{ "&#185;",	L'¹', },
	{ "&#186;",	L'º', },
	{ "&#187;",	L'»', },
	{ "&#188;",	L'¼', },
	{ "&#189;",	L'½', },
	{ "&#190;",	L'¾', },
	{ "&#191;",	L'¿', },

	{ "*",			L'•', },
	{ "&#164;",	L'□', },
	{ "&#186;",	L'◊', },
	{ "(tm)",		L'™', },
	{"&#913;",		L'Α',},
	{"&#914;",		L'Β',},
	{"&#915;",		L'Γ',},
	{"&#916;",		L'Δ',},
	{"&#917;",		L'Ε',},
	{"&#918;",		L'Ζ',},
	{"&#919;",		L'Η',},
	{"&#920;",		L'Θ',},
	{"&#921;",		L'Ι',},
	{"&#922;",		L'Κ',},
	{"&#923;",		L'Λ',},
	{"&#924;",		L'Μ',},
	{"&#925;",		L'Ν',},
	{"&#926;",		L'Ξ',},
	{"&#927;",		L'Ο',},
	{"&#928;",		L'Π',},
	{"&#929;",		L'Ρ',},
	{"&#930;",		L'΢',},
	{"&#931;",		L'Σ',},
	{"&#932;",		L'Τ',},
	{"&#933;",		L'Υ',},
	{"&#934;",		L'Φ',},
	{"&#935;",		L'Χ',},
	{"&#936;",		L'Ψ',},
	{"&#937;",		L'Ω',},
	{"&#945;",		L'α',},
	{"&#946;",		L'β',},
	{"&#947;",		L'γ',},
	{"&#948;",		L'δ',},
	{"&#949;",		L'ε',},
	{"&#950;",		L'ζ',},
	{"&#951;",		L'η',},
	{"&#952;",		L'θ',},
	{"&#953;",		L'ι',},
	{"&#954;",		L'κ',},
	{"&#955;",		L'λ',},
	{"&#956;",		L'μ',},
	{"&#957;",		L'ν',},
	{"&#958;",		L'ξ',},
	{"&#959;",		L'ο',},
	{"&#960;",		L'π',},
	{"&#961;",		L'ρ',},
	{"&#962;",		L'ς',},
	{"&#963;",		L'σ',},
	{"&#964;",		L'τ',},
	{"&#965;",		L'υ',},
	{"&#966;",		L'φ',},
	{"&#967;",		L'χ',},
	{"&#968;",		L'ψ',},
	{"&#969;",		L'ω',},

	{ "<-",		L'←', },
	{ "^",			L'↑', },
	{ "->",		L'→', },
	{ "v",			L'↓', },
	{ "!=",		L'≠', },
	{ "<=",		L'≤', },
	{ "...",		L'⋯', },
	{"&isin;",		L'∈', },

	{"&#8211;",	L'–', },
	{"&#8212;",	L'—', },

	{ "CYRILLIC XYZZY",	L'й', },
	{ "CYRILLIC XYZZY",	L'ъ', },
	{ "CYRILLIC Y",		L'ь', },
	{ "CYRILLIC YA",	L'я', },
	{ "CYRILLIC YA",	L'ё', },
	{ "&#191;",		L'ℱ', },

	{ nil, 0 },
};

typedef struct Troffspec Troffspec;
struct Troffspec
{
	char *name;
	char *value;
};

Troffspec tspec[] =
{
	{ "A*", "&Aring;", },
	{ "o\"", "&ouml;", },
	{ "ff", "ff", },
	{ "fi", "fi", },
	{ "fl", "fl", },
	{ "Fi", "ffi", },
	{ "ru", "_", },
	{ "em", "&#173;", },
	{ "14", "&#188;", },
	{ "12", "&#189;", },
	{ "co", "&#169;", },
	{ "de", "&#176;", },
	{ "dg", "&#161;", },
	{ "fm", "&#180;", },
	{ "rg", "&#174;", },
	{ "bu", "*", },
	{ "sq", "&#164;", },
	{ "hy", "-", },
	{ "pl", "+", },
	{ "mi", "-", },
	{ "mu", "&#215;", },
	{ "di", "&#247;", },
	{ "eq", "=", },
	{ "==", "==", },
	{ ">=", ">=", },
	{ "<=", "<=", },
	{ "!=", "!=", },
	{ "+-", "&#177;", },
	{ "no", "&#172;", },
	{ "sl", "/", },
	{ "ap", "&", },
	{ "~=", "~=", },
	{ "pt", "oc", },
	{ "gr", "GRAD", },
	{ "->", "->", },
	{ "<-", "<-", },
	{ "ua", "^", },
	{ "da", "v", },
	{ "is", "Integral", },
	{ "pd", "DIV", },
	{ "if", "oo", },
	{ "sr", "-/", },
	{ "sb", "(~", },
	{ "sp", "~)", },
	{ "cu", "U", },
	{ "ca", "(^)", },
	{ "ib", "(=", },
	{ "ip", "=)", },
	{ "mo", "C", },
	{ "es", "&Oslash;", },
	{ "aa", "&#180;", },
	{ "ga", "`", },
	{ "ci", "O", },
	{ "L1", "DEATHSTAR", },
	{ "sc", "&#167;", },
	{ "dd", "++", },
	{ "lh", "<=", },
	{ "rh", "=>", },
	{ "lt", "(", },
	{ "rt", ")", },
	{ "lc", "|", },
	{ "rc", "|", },
	{ "lb", "(", },
	{ "rb", ")", },
	{ "lf", "|", },
	{ "rf", "|", },
	{ "lk", "|", },
	{ "rk", "|", },
	{ "bv", "|", },
	{ "ts", "s", },
	{ "br", "|", },
	{ "or", "|", },
	{ "ul", "_", },
	{ "rn", " ", },
	{ "**", "*", },
	{ "tm", "&#153", },
	{ nil, nil, },
};

typedef struct Font Font;
struct Font
{
	char	*start;
	char	*end;
};
Font bfont = { "<B>", "</B>" };
Font ifont = { "<I>", "</I>" };
Font bifont = { "<B><I>", "</I></B>" };
Font cwfont = { "<TT>", "</TT>" };
Font *fstack[Maxfsp];
int fsp = -1;

typedef struct String String;
struct String
{
	String *next;
	char *name;
	char *val;
};
String *numregs, *strings;
char *strstack[Maxmstack];
char *mustfree[Maxmstack];
int strsp = -1;
int elsetop = -1;

typedef struct Mstack Mstack;
struct Mstack
{
	char *ptr;
	char *argv[Narg+1];
};
String *macros;
Mstack mstack[Maxmstack];
int msp = -1;

typedef struct Srcstack Srcstack;
struct Srcstack
{
	char	filename[256];
	int	fd;
	int	lno;
	int	rlno;
	Biobuf	in;
};
Srcstack sstack[Maxsstack];
Srcstack *ssp = &sstack[-1];

char token[128];

void	closel(void);
void	closefont(void);

void*
emalloc(uint n)
{
	void *p;

	p = mallocz(n, 1);
	if(p == nil){
		fprint(2, "ms2html: malloc failed: %r\n");
		exits("malloc");
	}
	return p;
}


/* define a string variable */
void
dsnr(char *name, char *val, String **l)
{
	String *s;

	for(s = *l; s != nil; s = *l){
		if(strcmp(s->name, name) == 0)
			break;
		l = &s->next;
	}
	if(s == nil){
		s = emalloc(sizeof(String));
		*l = s;
		s->name = strdup(name);
	} else
		free(s->val);
	s->val = strdup(val);
}

void
ds(char *name, char *val)
{
	dsnr(name, val, &strings);
}

/* look up a defined string */
char*
getds(char *name)
{
	String *s;

	for(s = strings; s != nil; s = s->next)
		if(strcmp(name, s->name) == 0)
			break;
	if(s != nil)
		return s->val;
	return "";
}

char *
getnr(char *name)
{
	String *s;

	for(s = numregs; s != nil; s = s->next)
		if(strcmp(name, s->name) == 0)
			break;
	if(s != nil)
		return s->val;
	return "0";
}

void
pushstr(char *p)
{
	if(p == nil)
		return;
	if(strsp >= Maxmstack - 1)
		return;
	strstack[++strsp] = p;
}


/* lookup a defined macro */
char*
getmacro(char *name)
{
	String *s;

	for(s = macros; s != nil; s = s->next)
		if(strcmp(name, s->name) == 0)
			return s->val;
	return nil;
}

enum
{
	Dstring,
	Macro,
	Input,
};
int lastsrc;

void
pushsrc(char *name)
{
	Dir *d;
	int fd;

	if(ssp == &sstack[Maxsstack-1]){
		fprint(2, "ms2html: .so's too deep\n");
		return;
	}
	d = nil;
	if(name == nil){
		d = dirfstat(0);
		if(d == nil){
			fprint(2, "ms2html: can't stat %s: %r\n", name);
			return;
		}
		name = d->name;
		fd = 0;
	} else {
		fd = open(name, OREAD);
		if(fd < 0){
			fprint(2, "ms2html: can't open %s: %r\n", name);
			return;
		}
	}
	ssp++;
	ssp->fd = fd;
	Binit(&ssp->in, fd, OREAD);
	snprint(ssp->filename, sizeof(ssp->filename), "%s", name);
	ssp->lno = ssp->rlno = 1;
	free(d);
}

/* get next logical byte.  from stdin or a defined string */
int
getrune(void)
{
	int i;
	Rune r;
	int c;
	Mstack *m;

	while(strsp >= 0){
		i = chartorune(&r, strstack[strsp]);
		if(r != 0){
			strstack[strsp] += i;
			lastsrc = Dstring;
			return r;
		}
		if (mustfree[strsp]) {
			free(mustfree[strsp]);
			mustfree[strsp] = nil;
		}
		strsp--;
 	}
	while(msp >= 0){
		m = &mstack[msp];
		i = chartorune(&r, m->ptr);
		if(r != 0){
			m->ptr += i;
			lastsrc = Macro;
			return r;
		}
		for(i = 0; m->argv[i] != nil; i++)
			free(m->argv[i]);
		msp--;
 	}

	lastsrc = Input;
	for(;;) {
		if(ssp < sstack)
			return -1;
		c = Bgetrune(&ssp->in);
		if(c >= 0){
			r = c;
			break;
		}
		close(ssp->fd);
		ssp--;
	}

	return r;
}

void
ungetrune(void)
{
	switch(lastsrc){
	case Dstring:
		if(strsp >= 0)
			strstack[strsp]--;
		break;
	case Macro:
		if(msp >= 0)
			mstack[msp].ptr--;
		break;
	case Input:
		if(ssp >= sstack)
			Bungetrune(&ssp->in);
		break;
	}
}

int vert;

char*
changefont(Font *f)
{
	token[0] = 0;
	if(fsp == Maxfsp)
		return token;
	if(fsp >= 0 && fstack[fsp])
		strcpy(token, fstack[fsp]->end);
	if(f != nil)
		strcat(token, f->start);
	fstack[++fsp] = f;
	return token;
}

char*
changebackfont(void)
{
	token[0] = 0;
	if(fsp >= 0){
		if(fstack[fsp])
			strcpy(token, fstack[fsp]->end);
		fsp--;
	}
	if(fsp >= 0 && fstack[fsp])
		strcat(token, fstack[fsp]->start);
	return token;
}

char*
changesize(int amount)
{
	static int curamount;
	static char buf[200];
	int i;

	buf[0] = 0;
	if (curamount >= 0)
		for (i = 0; i < curamount; i++)
			strcat(buf, "</big>");
	else
		for (i = 0; i < -curamount; i++)
			strcat(buf, "</small>");
	curamount = 0;
	if (amount >= 0)
		for (i = 0; i < amount; i++)
			strcat(buf, "<big>");
	else
		for (i = 0; i < -amount; i++)
			strcat(buf, "<small>");
	curamount = amount;
	return buf;
}

/* get next logical character.  expand it with escapes */
char*
getnext(void)
{
	int r;
	Entity *e;
	Troffspec *t;
	Rune R;
	char str[4];
	static char buf[8];

	r = getrune();
	if(r < 0)
		return nil;
	if(r > 128 || r == '<' || r == '>'){
		for(e = entity; e->name; e++)
			if(e->value == r)
				return e->name;
		sprint(buf, "&#%d;", r);
		return buf;
	}

	if (r == delim[eqnmode]){
		if (eqnmode == 0){
			eqnmode = 1;
			return changefont(&ifont);
		}
		eqnmode = 0;
		return changebackfont();
	}
	switch(r){
	case '\\':
		r = getrune();
		if(r < 0)
			return nil;
		switch(r){
		case ' ':
			return " ";

		/* chars to ignore */
		case '&':
		case '|':
		case '%':
			return "";

		/* small space in troff, nothing in nroff */
		case '^':
			return getnext();

		/* ignore arg */
		case 'k':
			getrune();
			return getnext();

		/* comment */
		case '"':
			while(getrune() != '\n')
				;
			return "\n";
		/* ignore line */
		case '!':
			while(getrune() != '\n')
				;
			ungetrune();
			return getnext();

		/* defined strings */
		case '*':
			r = getrune();
			if(r == '('){
				str[0] = getrune();
				str[1] = getrune();
				str[2] = 0;
			} else {
				str[0] = r;
				str[1] = 0;
			}
			pushstr(getds(str));
			return getnext();

		/* macro args */
		case '$':
			r = getrune();
			if(r < '1' || r > '9'){
				token[0] = '\\';
				token[1] = '$';
				token[2] = r;
				token[3] = 0;
				return token;
			}
			r -= '0';
			if(msp >= 0) 
				pushstr(mstack[msp].argv[r]);
			return getnext();

		/* special chars */
		case '(':
			token[0] = getrune();
			token[1] = getrune();
			token[2] = 0;
			for(t = tspec; t->name; t++)
				if(strcmp(token, t->name) == 0)
					return t->value;
			return "&#191;";

		/* ignore immediately following newline */
		case 'c':
			r = getrune();
			if (r == '\n') {
				sol = ignore_nl = 1;
				if (indirective)
					break;
				}
			else
				ungetrune();
			return getnext();

		/* escape backslash */
		case 'e':
			return "\\";

		/* font change */
		case 'f':
			r = getrune();
			switch(r){
			case '(':
				str[0] = getrune();
				str[1] = getrune();
				str[2] = 0;
				token[0] = 0;
				if(strcmp("BI", str) == 0)
					return changefont(&bifont);
				else if(strcmp("CW", str) == 0)
					return changefont(&cwfont);
				else
					return changefont(nil);
			case '3':
			case 'B':
				return changefont(&bfont);
			case '2':
			case 'I':
				return changefont(&ifont);
			case '4':
				return changefont(&bifont);
			case '5':
				return changefont(&cwfont);
			case 'P':
				return changebackfont();
			case 'R':
			default:
				return changefont(nil);
			}

		/* number register */
		case 'n':
			r = getrune();
			if (r == '(') /*)*/ {
				r = getrune();
				if (r < 0)
					return nil;
				str[0] = r;
				r = getrune();
				if (r < 0)
					return nil;
				str[1] = r;
				str[2] = 0;
				}
			else {
				str[0] = r;
				str[1] = 0;
				}
			pushstr(getnr(str));
			return getnext();

		/* font size */
		case 's':
			r = getrune();
			switch(r){
			case '0':
				return changesize(0);
			case '-':
				r = getrune();
				if (!isdigit(r))
					return getnext();
				return changesize(-(r - '0'));
			case '+':
				r = getrune();
				if (!isdigit(r))
					return getnext();
				return changesize(r - '0');
			}
			return getnext();
		/* vertical movement */
		case 'v':
			r = getrune();
			if(r != '\''){
				ungetrune();
				return getnext();
			}
			r = getrune();
			if(r != '-')
				vert--;
			else
				vert++;
			while(r != '\'' && r != '\n')
				r = getrune();
			if(r != '\'')
				ungetrune();
			
			if(vert > 0)
				return "^";
			return getnext();
			

		/* horizontal line */
		case 'l':
			r = getrune();
			if(r != '\''){
				ungetrune();
				return "<HR>";
			}
			while(getrune() != '\'')
				;
			return "<HR>";

		/* character height and slant */
		case 'S':
		case 'H':
			r = getrune();
			if(r != '\''){
				ungetrune();
				return "<HR>";
			}
			while(getrune() != '\'')
				;
			return getnext();

		/* digit-width space */
		case '0':
			return " ";

		/*for .if, .ie, .el */
		case '{':
			return "\\{"; /*}*/
		case '}':
			return "";
		/* up and down */
		case 'u':
			if (isdown) {
				isdown = 0;
				return "</sub>";
			}
			isup = 1;
			return "<sup>";
		case 'd':
			if (isup) {
				isup = 0;
				return "</sup>";
			}
			isdown = 1;
			return "<sub>";
		}
		break;
	case '&':
		if(msp >= 0 || strsp >= 0)
			return "&";
		return "&amp;";
	case '<':
		if(msp >= 0 || strsp >= 0)
			return "<";
		return "&#60;";
	case '>':
		if(msp >= 0 || strsp >= 0)
			return ">";
		return "&#62;";
	}
	if (r < Runeself) {
		token[0] = r;
		token[1] = 0;
		}
	else {
		R = r;
		token[runetochar(token,&R)] = 0;
	}
	return token;
}

/* if arg0 is set, read up to (and expand) to the next whitespace, else to the end of line */
char*
copyline(char *p, char *e, int arg0)
{
	int c;
	Rune r;
	char *p1;

	while((c = getrune()) == ' ' || c == '\t')
		;
	for(indirective = 1; p < e; c = getrune()) {
		if (c < 0)
			goto done;
		switch(c) {
		case '\\':
			break;
		case '\n':
			if (arg0)
				ungetrune();
			goto done;
		case ' ':
		case '\t':
			if (arg0)
				goto done;
		default:
			r = c;
			p += runetochar(p,&r);
			continue;
		}
		ungetrune();
		p1 = getnext();
		if (p1 == nil)
			goto done;
		if (*p1 == '\n') {
			if (arg0)
				ungetrune();
			break;
		}
		while((*p = *p1++) && p < e)
			p++;
	}
done:
	indirective = 0;
	*p++ = 0;
	return p;
}

char*
copyarg(char *p, char *e, int *nullarg)
{
	int c, quoted, last;
	Rune r;

	*nullarg = 0;
	quoted = 0;
	do{
		c = getrune();
	} while(c == ' ' || c == '\t');

	if(c == '"'){
		quoted = 1;
		*nullarg = 1;
		c = getrune();
	}

	if(c == '\n')
		goto done;

	last = 0;
	for(; p < e; c = getrune()) {
		if (c < 0)
			break;
		switch(c) {
		case '\n':
			ungetrune();
			goto done;
		case '\\':
			r = c;
			p += runetochar(p,&r);
			if(last == '\\')
				r = 0;
			break;
		case ' ':
		case '\t':
			if(!quoted && last != '\\')
				goto done;
			r = c;
			p += runetochar(p,&r);
			break;
		case '"':
			if(quoted && last != '\\')
				goto done;
			r = c;
			p += runetochar(p,&r);
			break;
		default:
			r = c;
			p += runetochar(p,&r);
			break;
		}
		last = r;
	}
done:
	*p++ = 0;
	return p;

}

int
parseargs(char *p, char *e, char **argv)
{
	int argc;
	char *np;
	int nullarg;

	indirective = 1;
	*p++ = 0;
	for(argc = 1; argc < Narg; argc++){
		np = copyarg(p, e, &nullarg);
		if(nullarg==0 && np == p+1)
			break;
		argv[argc] = p;
		p = np;
	}
	argv[argc] = nil;
	indirective = 0;


	return argc;
}

void
dodirective(void)
{
	char *p, *e;
	Goobie *g;
	Goobieif *gif;
	char line[Nline], *line1;
	int i, argc;
	char *argv[Narg];
	Mstack *m;

	/* read line, translate special bytes */
	e = line + sizeof(line) - UTFmax - 1;
	line1 = copyline(line, e, 1);
	if (!line[0])
		return;
	argv[0] = line;

	/* first look through user defined macros */
	p = getmacro(argv[0]);
	if(p != nil){
		if(msp == Maxmstack-1){
			fprint(2, "ms2html: macro stack overflow\n");
			return;
		}
		argc = parseargs(line1, e, argv);
		m = &mstack[++msp];
		m->ptr = p;
		memset(m->argv, 0, sizeof(m->argv));
		for(i = 0; i < argc; i++)
			m->argv[i] = strdup(argv[i]);
		return;
	}

	/* check for .if or .ie */
	for(gif = gtabif; gif->name; gif++)
		if(strcmp(gif->name, argv[0]) == 0){
			(*gif->f)(line1, e);
			return;
		}

	argc = parseargs(line1, e, argv);

	/* try standard ms macros */
	for(g = gtab; g->name; g++)
		if(strcmp(g->name, argv[0]) == 0){
			(*g->f)(argc, argv);
			return;
		}

	if(debug)
		fprint(2, "stdin %d(%s:%d): unknown directive %s\n",
			ssp->lno, ssp->filename, ssp->rlno, line);
}

void
printarg(char *a)
{
	char *e, *p;
	
	e = a + strlen(a);
	pushstr(a);
	while(strsp >= 0 && strstack[strsp] >= a && strstack[strsp] < e){
		p = getnext();
		if(p == nil)
			return;
		Bprint(&bout, "%s", p);
	}
}

void
printargs(int argc, char **argv)
{
	argc--;
	argv++;
	while(--argc > 0){
		printarg(*argv++);
		Bprint(&bout, " ");
	}
	if(argc == 0)
		printarg(*argv);
}

void
dohangingdt(void)
{
	switch(hangingdt){
	case 3:
		hangingdt--;
		break;
	case 2:
		Bprint(&bout, "<dd>");
		hangingdt = 0;
		break;
	}

}

void
dohangingau(void)
{
	if(hangingau == 0)
		return;
	Bprint(&bout, "</I></DL>\n");
	hangingau = 0;
}

void
dohanginghead(void)
{
	if(hanginghead == 0)
		return;
	Bprint(&bout, "</H%d>\n", hanginghead);
	hanginghead = 0;
}

/*
 *  convert a man page to html and output
 */
void
doconvert(void)
{
	char c, *p;
	Tm *t;

	pushsrc(nil);

	sol = 1;
	Bprint(&bout, "<html>\n");
	Bflush(&bout);
	for(;;){
		p = getnext();
		if(p == nil)
			break;
		c = *p;
		if(c == '.' && sol){
			dodirective();
			dohangingdt();
			ssp->lno++;
			ssp->rlno++;
			sol = 1;
		} else if(c == '\n'){
			if (ignore_nl)
				ignore_nl = 0;
			else {
				if(hangingau)
					Bprint(&bout, "<br>\n");
				else
					Bprint(&bout, "%s", p);
				dohangingdt();
				}
			ssp->lno++;
			ssp->rlno++;
			sol = 1;
		} else{
			Bprint(&bout, "%s", p);
			ignore_nl = sol = 0;
		}
	}
	dohanginghead();
	dohangingdt();
	closel();
	if(fsp >= 0 && fstack[fsp])
		Bprint(&bout, "%s", fstack[fsp]->end);
	Bprint(&bout, "<br>&#32;<br>\n");
	Bprint(&bout, "<A href=http://www.lucent.com/copyright.html>\n");
	t = localtime(time(nil));
	Bprint(&bout, "Copyright</A> &#169; %d Alcatel-Lucent Inc.  All rights reserved.\n",
			t->year+1900);
	Bprint(&bout, "</body></html>\n");
}

static void
usage(void)
{
	sysfatal("usage: ms2html [-q] [-b basename] [-d '$$'] [-t title]");
}

void
main(int argc, char **argv)
{
	quiet = 1;
	ARGBEGIN {
	case 't':
		title = EARGF(usage());
		break;
	case 'b':
		basename = EARGF(usage());
		break;
	case 'q':
		quiet = 0;
		break;
	case 'd':
		delim = EARGF(usage());
		break;
	case '?':
	default:
		usage();
	} ARGEND;

	Binit(&bout, 1, OWRITE);

	ds("R", "&#174;");

	doconvert();
	exits(nil);
}

void
g_notyet(int, char **argv)
{
	fprint(2, "ms2html: .%s not yet supported\n", argv[0]);
}

void
g_ignore(int, char **argv)
{
	if(quiet)
		return;
	fprint(2, "ms2html: line %d: ignoring .%s\n", ssp->lno, argv[0]);
}

void
g_PP(int, char**)
{
	dohanginghead();
	closel();
	closefont();
	Bprint(&bout, "<P>\n");
	paragraph = 1;
}

void
g_LP(int, char**)
{
	dohanginghead();
	closel();
	closefont();
	Bprint(&bout, "<br>&#32;<br>\n");
}

/* close a list */
void
closel(void)
{
	g_P2(1, nil);
	dohangingau();
	if(paragraph){
		Bprint(&bout, "</P>\n");
		paragraph = 0;
	}
	switch(list){
	case Lordered:
		Bprint(&bout, "</ol>\n");
		break;
	case Lunordered:
		Bprint(&bout, "</ul>\n");
		break;
	case Lother:
	case Ldef:
		Bprint(&bout, "</dl>\n");
		break;
	}
	list = 0;
	
}


void
g_IP(int argc, char **argv)
{
	dohanginghead();
	switch(list){
	default:
		closel();
		if(argc > 1){
			if(strcmp(argv[1], "1") == 0){
				list = Lordered;
				listnum = 1;
				Bprint(&bout, "<OL>\n");
			} else if(strcmp(argv[1], "\\(bu") == 0){
				list = Lunordered;
				Bprint(&bout, "<UL>\n");
			} else {
				list = Lother;
				Bprint(&bout, "<DL COMPACT>\n");
			}
		} else {
			list = Lother;
			Bprint(&bout, "<DL>\n");
		}
		break;
	case Lother:
	case Lordered:
	case Lunordered:
		break;
	}

	switch(list){
	case Lother:
		Bprint(&bout, "<DT>");
		if(argc > 1)
			printarg(argv[1]);
		else
			Bprint(&bout, "<DT>&#32;");
		Bprint(&bout, "<DD>\n");
		break;
	case Lordered:
	case Lunordered:
		Bprint(&bout, "<LI>\n");
		break;
	}
}

/*
 *  .5i is one <DL><DT><DD>
 */
void
g_in(int argc, char **argv)
{
	float	f;
	int	delta, x;
	char	*p;

	f = indent/0.5;
	delta = f;
	if(argc <= 1){
		indent = 0.0;
	} else {
		f = strtod(argv[1], &p);
		switch(*p){
		case 'i':
			break;
		case 'c':
			f = f / 2.54;
			break;
		case 'P':
			f = f / 6;
			break;
		default:
		case 'u':
		case 'm':
			f = f * (12 / 72);
			break;
		case 'n':
			f = f * (6 / 72);
			break;
		case 'p':
			f = f / 72.0;
			break;
		}
		switch(argv[1][0]){
		case '+':
		case '-':
			indent += f;
			break;
		default:
			indent = f;
			break;
		}
	}
	if(indent < 0.0)
		indent = 0.0;
	f = (indent/0.5);
	x = f;
	delta = x - delta;
	while(delta < 0){
		Bprint(&bout, "</DL>\n");
		delta++;
	}
	while(delta > 0){
		Bprint(&bout, "<DL><DT><DD>\n");
		delta--;
	}
}

void
g_HP(int, char**)
{
	switch(list){
	default:
		closel();
		list = Ldef;
		hangingdt = 1;
		Bprint(&bout, "<DL><DT>\n");
		break;
	case Ldef:
		if(hangingdt)
			Bprint(&bout, "<DD>");
		Bprint(&bout, "<DT>");
		hangingdt = 1;
		break;
	}
}

void
g_SH(int, char**)
{
	dohanginghead();
	dohangingcenter();
	closel();
	closefont();
	Bprint(&bout, "<H%d>", HH);
	hanginghead = HH;
}

void
g_NH(int argc, char **argv)
{
	int i, level;

	closel();
	closefont();

	dohanginghead();
	dohangingcenter();
	if(argc == 1)
		level = 0;
	else {
		level = atoi(argv[1])-1;
		if(level < 0 || level >= Maxnh)
			level = Maxnh - 1;
	}
	nh[level]++;

	Bprint(&bout, "<H%d>", HH);
	hanginghead = HH;

	Bprint(&bout, "%d", nh[0]);
	for(i = 1; i <= level; i++)
		Bprint(&bout, ".%d", nh[i]);
	Bprint(&bout, " ");

	for(i = level+1; i < Maxnh; i++)
		nh[i] = 0;
}

void
g_TL(int, char**)
{
	char *p, *np;
	char name[128];

	closefont();

	if(!titleseen){
		if(!title){
			/* get base part of filename */
			p = strrchr(ssp->filename, '/');
			if(p == nil)
				p = ssp->filename;
			else
				p++;
			strncpy(name, p, sizeof(name));
			name[sizeof(name)-1] = 0;
		
			/* dump any extensions */
			np = strchr(name, '.');
			if(np)
				*np = 0;
			title = p;
		}
		Bprint(&bout, "<title>\n");
		Bprint(&bout, "%s\n", title);
		Bprint(&bout, "</title>\n");
		Bprint(&bout, "<body BGCOLOR=\"#FFFFFF\" TEXT=\"#000000\" LINK=\"#0000FF\" VLINK=\"#330088\" ALINK=\"#FF0044\">\n");
		titleseen = 1;
	}

	Bprint(&bout, "<center>");
	hangingcenter = 1;
	Bprint(&bout, "<H%d>", 1);
	hanginghead = 1;
}

void
dohangingcenter(void)
{
	if(hangingcenter){
		Bprint(&bout, "</center>");
		hangingcenter = 1;
	}
}

void
g_AU(int, char**)
{
	closel();
	dohanginghead();
	Bprint(&bout, "<DL><DD><I>");
	hangingau = 1;
}

void
pushfont(Font *f)
{
	if(fsp == Maxfsp)
		return;
	if(fsp >= 0 && fstack[fsp])
		Bprint(&bout, "%s", fstack[fsp]->end);
	if(f != nil)
		Bprint(&bout, "%s", f->start);
	fstack[++fsp] = f;
}

void
popfont(void)
{
	if(fsp >= 0){
		if(fstack[fsp])
			Bprint(&bout, "%s", fstack[fsp]->end);
		fsp--;
	}
}

/*
 *  for 3 args print arg3 \fxarg1\fP arg2
 *  for 2 args print arg1 \fxarg2\fP
 *  for 1 args print \fxarg1\fP
 */
void
font(Font *f, int argc, char **argv)
{
	if(argc == 1){
		pushfont(nil);
		return;
	}
	if(argc > 3)
		printarg(argv[3]);
	pushfont(f);
	printarg(argv[1]);
	popfont();
	if(argc > 2)
		printarg(argv[2]);
	Bprint(&bout, "\n");
}

void
closefont(void)
{
	if(fsp >= 0 && fstack[fsp])
		Bprint(&bout, "%s", fstack[fsp]->end);
	fsp = -1;
}

void
g_B(int argc, char **argv)
{
	font(&bfont, argc, argv);
}

void
g_R(int argc, char **argv)
{
	font(nil, argc, argv);
}

void
g_BI(int argc, char **argv)
{
	font(&bifont, argc, argv);
}

void
g_CW(int argc, char **argv)
{
	font(&cwfont, argc, argv);
}

char*
lower(char *p)
{
	char *x;

	for(x = p; *x; x++)
		if(*x >= 'A' && *x <= 'Z')
			*x -= 'A' - 'a';
	return p;
}

void
g_I(int argc, char **argv)
{
	int anchor;
	char *p;

	anchor = 0;
	if(argc > 2){
		p = argv[2];
		if(p[0] == '(')
		if(p[1] >= '0' && p[1] <= '9')
		if(p[2] == ')'){
			anchor = 1;
			Bprint(&bout, "<A href=\"/magic/man2html/%c/%s\">",
				p[1], lower(argv[1]));
		}
	}
	font(&ifont, argc, argv);
	if(anchor)
		Bprint(&bout, "</A>");
}

void
g_br(int, char**)
{
	if(hangingdt){
		Bprint(&bout, "<dd>");
		hangingdt = 0;
	}else
		Bprint(&bout, "<br>\n");
}

void
g_P1(int, char**)
{
	if(example == 0){
		example = 1;
		Bprint(&bout, "<DL><DT><DD><TT><PRE>\n");
	}
}

void
g_P2(int, char**)
{
	if(example){
		example = 0;
		Bprint(&bout, "</PRE></TT></DL>\n");
	}
}

void
g_SM(int, char **argv)
{
	Bprint(&bout, "%s", argv[1]);
}

void
g_ft(int argc, char **argv)
{
	if(argc < 2){
		pushfont(nil);
		return;
	}

	switch(argv[1][0]){
	case '3':
	case 'B':
		pushfont(&bfont);
		break;
	case '2':
	case 'I':
		pushfont(&ifont);
		break;
	case '4':
		pushfont(&bifont);
		break;
	case '5':
		pushfont(&cwfont);
		break;
	case 'P':
		popfont();
		break;
	case 'R':
	default:
		pushfont(nil);
		break;
	}
}

void
g_sp(int argc, char **argv)
{
	int n;

	n = 1;
	if(argc > 1){
		n = atoi(argv[1]);
		if(n < 1)
			n = 1;
		if(argv[1][strlen(argv[1])-1] == 'i')
			n *= 4;
	}
	if(n > 5){
		Bprint(&bout, "<br>&#32;<br>\n");
		Bprint(&bout, "<HR>\n");
		Bprint(&bout, "<br>&#32;<br>\n");
	} else
		for(; n > 0; n--)
			Bprint(&bout, "<br>&#32;<br>\n");
}

 void
rm_loop(char *name, String **l)
{
	String *s;
	for(s = *l; s != nil; s = *l){
		if(strcmp(name, s->name) == 0){
			*l = s->next;
			free(s->name);
			free(s->val);
			free(s);
			break;
			}
		l = &s->next;
		}
	}

void
g_rm(int argc, char **argv)
{
	Goobie *g;
	char *name;
	int i;

	for(i = 1; i < argc; i++) {
		name = argv[i];
		rm_loop(name, &strings);
		rm_loop(name, &macros);
		for(g = gtab; g->name; g++)
			if (strcmp(g->name, name) == 0) {
				g->f = g_ignore;
				break;
				}
		}
	}

void
g_AB(int, char**)
{
	closel();
	dohangingcenter();
	Bprint(&bout, "<center><H4>ABSTRACT</H4></center><DL><DD>\n");
}

void
g_AE(int, char**)
{
	Bprint(&bout, "</DL>\n");
}

void
g_FS(int, char **)
{
	char *argv[3];

	argv[0] = "IP";
	argv[1] = nil;
	argv[2] = nil;
	g_IP(1, argv);
	Bprint(&bout, "NOTE:<I> ");
}

void
g_FE(int, char **)
{
	Bprint(&bout, "</I><DT>&#32;<DD>");
	closel();
	Bprint(&bout, "<br>\n");
}

void
g_de(int argc, char **argv)
{
	int r;
	char *p, *cp;
	String *m;
	int len;

	if(argc < 2)
		return;

	m = nil;
	len = 0;
	if(strcmp(argv[0], "am") == 0){
		for(m = macros; m != nil; m = m->next)
			if(strcmp(argv[1], m->name) == 0){
				len = strlen(m->val);
				break;
			}

		if(m == nil){
			/* nothing to append to */
			for(;;){
				p = Brdline(&ssp->in, '\n');
				if(p == nil)
					break;
				p[Blinelen(&ssp->in)-1] = 0;
				if(strcmp(p, "..") == 0)
					break;
			}
			return;
		}
	}

	if(m == nil){
		m = emalloc(sizeof(*m));
		m->next = macros;
		macros = m;
		m->name = strdup(argv[1]);
		m->val = nil;
		len = 0;
	}

	/* read up to a .. removing double backslashes */
	for(;;){
		p = Brdline(&ssp->in, '\n');
		if(p == nil)
			break;
		p[Blinelen(&ssp->in)-1] = 0;
		if(strcmp(p, "..") == 0)
			break;
		m->val = realloc(m->val, len + Blinelen(&ssp->in)+1);
		cp = m->val + len;
		while(*p){
			r = *p++;
			if(r == '\\' && *p == '\\')
				p++;
			*cp++ = r;
		}
		*cp++ = '\n';
		len = cp - m->val;
		*cp = 0;
	}
}

void
g_hrule(int, char**)
{
	Bprint(&bout, "<HR>\n");
}

void
g_BX(int argc, char **argv)
{
	Bprint(&bout, "<HR>\n");
	printargs(argc, argv);
	Bprint(&bout, "<HR>\n");
}

void
g_IH(int, char**)
{
	Bprint(&bout, "Bell Laboratories, Naperville, Illinois, 60540\n");
}

void
g_MH(int, char**)
{
	Bprint(&bout, "Bell Laboratories, Murray Hill, NJ, 07974\n");
}

void
g_PY(int, char**)
{
	Bprint(&bout, "Bell Laboratories, Piscataway, NJ, 08854\n");
}

void
g_HO(int, char**)
{
	Bprint(&bout, "Bell Laboratories, Holmdel, NJ, 07733\n");
}

void
g_QS(int, char**)
{
	Bprint(&bout, "<BLOCKQUOTE>\n");
}

void
g_QE(int, char**)
{
	Bprint(&bout, "</BLOCKQUOTE>\n");
}

void
g_RS(int, char**)
{
	Bprint(&bout, "<DL><DD>\n");
}

void
g_RE(int, char**)
{
	Bprint(&bout, "</DL>\n");
}

int gif;

void
g_startgif(int, char **argv)
{
	int fd;
	int pfd[2];
	char *e, *p;
	char name[32];
	Dir *d;

	if(strcmp(argv[0], "EQ") == 0)
		e = ".EN";
	else if(strcmp(argv[0], "TS") == 0)
		e = ".TE";
	else if(strcmp(argv[0], "PS") == 0)
		e = ".PE";
	else
		return;

	if(basename)
		p = basename;
	else{
		p = strrchr(sstack[0].filename, '/');
		if(p != nil)
			p++;
		else
			p = sstack[0].filename;
	}
	snprint(name, sizeof(name), "%s.%d.gif", p, gif++);
	fd = create(name, OWRITE, 0664);
	if(fd < 0){
		fprint(2, "ms2html: can't create %s: %r\n", name);
		return;
	}

	if(pipe(pfd) < 0){
		fprint(2, "ms2html: can't create pipe: %r\n");
		close(fd);
		return;
	}
	switch(rfork(RFFDG|RFPROC)){
	case -1:
		fprint(2, "ms2html: can't fork: %r\n");
		close(fd);
		return;
	case 0:
		dup(fd, 1);
		close(fd);
		dup(pfd[0], 0);
		close(pfd[0]);
		close(pfd[1]);
		execl("/bin/troff2gif", "troff2gif", nil);
		fprint(2, "ms2html: couldn't exec troff2gif: %r\n");
		_exits(nil);
	default:
		close(fd);
		close(pfd[0]);
		fprint(pfd[1], ".ll 7i\n");
	/*	fprint(pfd[1], ".EQ\ndelim %s\n.EN\n", delim); */
	/*	fprint(pfd[1], ".%s\n", argv[0]); */
		for(;;){
			p = Brdline(&ssp->in, '\n');
			if(p == nil)
				break;
			ssp->lno++;
			ssp->rlno++;
			if(write(pfd[1], p, Blinelen(&ssp->in)) < 0)
				break;
			if(strncmp(p, e, 3) == 0)
				break;
		}
		close(pfd[1]);
		waitpid();
		d = dirstat(name);
		if(d == nil)
			break;
		if(d->length == 0){
			remove(name);
			free(d);
			break;
		}
		free(d);
		fprint(2, "ms2html: created auxiliary file %s\n", name);
		Bprint(&bout, "<br><img src=\"%s\"><br>\n", name);
		break;
	}
}

void
g_lf(int argc, char **argv)
{
	if(argc > 2)
		snprint(ssp->filename, sizeof(ssp->filename), argv[2]);
	if(argc > 1)
		ssp->rlno = atoi(argv[1]);
}

void
g_so(int argc, char **argv)
{
	ssp->lno++;
	ssp->rlno++;
	if(argc > 1)
		pushsrc(argv[1]);
}


void
g_BP(int argc, char **argv)
{
	int fd;
	char *p, *ext;
	char name[32];
	Dir *d;

	if(argc < 2)
		return;

	p = strrchr(argv[1], '/');
	if(p != nil)
		p++;
	else
		p = argv[1];


	ext = strrchr(p, '.');
	if(ext){
		if(strcmp(ext, ".jpeg") == 0
		|| strcmp(ext, ".gif") == 0){
			Bprint(&bout, "<br><img src=\"%s\"><br>\n", argv[1]);
			return;
		}
	}


	snprint(name, sizeof(name), "%s.%d%d.gif", p, getpid(), gif++);
	fd = create(name, OWRITE, 0664);
	if(fd < 0){
		fprint(2, "ms2html: can't create %s: %r\n", name);
		return;
	}

	switch(rfork(RFFDG|RFPROC)){
	case -1:
		fprint(2, "ms2html: can't fork: %r\n");
		close(fd);
		return;
	case 0:
		dup(fd, 1);
		close(fd);
		execl("/bin/ps2gif", "ps2gif", argv[1], nil);
		fprint(2, "ms2html: couldn't exec ps2gif: %r\n");
		_exits(nil);
	default:
		close(fd);
		waitpid();
		d = dirstat(name);
		if(d == nil)
			break;
		if(d->length == 0){
			remove(name);
			free(d);
			break;
		}
		free(d);
		fprint(2, "ms2html: created auxiliary file %s\n", name);
		Bprint(&bout, "<br><img src=\"%s\"><br>\n", name);
		break;
	}
}

/* insert straight HTML into output */
void
g__H(int argc, char **argv)
{
	int i;

	for(i = 1; i < argc; i++)
		Bprint(&bout, "%s ", argv[i]);
	Bprint(&bout, "\n");
}

/* HTML page title */
void
g__T(int argc, char **argv)
{
	if(titleseen)
		return;

	Bprint(&bout, "<title>\n");
	printargs(argc, argv);
	Bprint(&bout, "</title></head><body>\n");
	titleseen = 1;
}

void
g_nr(int argc, char **argv)
{
	char *val;

	if (argc > 1) {
		if (argc == 2)
			val = "0";
		else
			val = argv[2];
		dsnr(argv[1], val, &numregs);
	}
}

void
zerodivide(void)
{
	fprint(2, "stdin %d(%s:%d): division by 0\n",
		ssp->lno, ssp->filename, ssp->rlno);
}

int
numval(char **pline, int recur)
{
	char *p;
	int neg, x, y;

	x = neg = 0;
	p = *pline;
	while(*p == '-') {
		neg = 1 - neg;
		p++;
	}
	if (*p == '(') {
		p++;
		x = numval(&p, 1);
		if (*p != ')')
			goto done;
		p++;
	}
	else while(*p >= '0' && *p <= '9')
		x = 10*x + *p++ - '0';
	if (neg)
		x = -x;
	if (recur)
	    for(;;) {
		switch(*p++) {
		case '+':
			x += numval(&p, 0);
			continue;
		case '-':
			x -= numval(&p, 0);
			continue;
		case '*':
			x *= numval(&p, 0);
			continue;
		case '/':
			y = numval(&p, 0);
			if (y == 0) {
				zerodivide();
				x = 0;
				goto done;
			}
			x /= y;
			continue;
		case '<':
			if (*p == '=') {
				p++;
				x = x <= numval(&p, 0);
				continue;
			}
			x = x < numval(&p, 0);
			continue;
		case '>':
			if (*p == '=') {
				p++;
				x = x >= numval(&p, 0);
				continue;
			}
			x = x > numval(&p, 0);
			continue;
		case '=':
			if (*p == '=')
				p++;
			x = x == numval(&p, 0);
			continue;
		case '&':
			x &= numval(&p, 0);
			continue;
		case ':':
			x |= numval(&p, 0);
			continue;
		case '%':
			y = numval(&p, 0);
			if (!y) {
				zerodivide();
				goto done;
			}
			x %= y;
			continue;
		}
		--p;
		break;
	}
 done:
	*pline = p;
	return x;
}

int
iftest(char *p, char **bp)
{
	char *p1;
	int c, neg, rv;

	rv = neg = 0;
	if (*p == '!') {
		neg = 1;
		p++;
	}
	c = *p;
	if (c >= '0' && c <= '9' || c == '+' || c == '-' || c == '('/*)*/) {
		if (numval(&p,1) >= 1)
			rv = 1;
		goto done;
	}
	switch(c) {
	case 't':
	case 'o':
		rv = 1;
	case 'n':
	case 'e':
		p++;
		goto done;
	}
	for(p1 = ++p; *p != c; p++)
		if (!*p)
			goto done;
	for(p++;;) {
		if (*p != *p1++) {
			while(*p && *p++ != c);
			goto done;
		}
		if (*p++ == c)
			break;
	}
	rv = 1;
 done:
	if (neg)
		rv = 1 - rv;
	while(*p == ' ' || *p == '\t')
		p++;
	*bp = p;
	return rv;
}

void
scanline(char *p, char *e, int wantnl)
{
	int c;
	Rune r;

	while((c = getrune()) == ' ' || c == '\t') ;
	while(p < e) {
		if (c < 0)
			break;
		if (c < Runeself) {
			if (c == '\n') {
				if (wantnl)
					*p++ = c;
				break;
			}
			*p++ = c;
		}
		else {
			r = c;
			p += runetochar(p, &r);
		}
		c = getrune();
	}
	*p = 0;
}

void
pushbody(char *line)
{
	char *b;

	if (line[0] == '\\' && line[1] == '{' /*}*/ )
		line += 2;
	if (strsp < Maxmstack - 1) {
		pushstr(b = strdup(line));
		mustfree[strsp] = b;
	}
}

void
skipbody(char *line)
{
	int c, n;

	if (line[0] != '\\' || line[1] != '{' /*}*/ )
		return;
	for(n = 1;;) {
		while((c = getrune()) != '\\')
			if (c < 0)
				return;
		c = getrune();
		if (c == '{')
			n++;
		else if ((c == '}' && (c = getrune()) == '\n' && !--n)
			|| c < 0)
			return;
	}
}

int
ifstart(char *line, char *e, char **bp)
{
	int it;
	char *b;

	b = copyline(line, e, 1);
	ungetrune();
	b[-1] = getrune();
	scanline(b, e, 1);
	it = iftest(line, bp);
	return it;
}

void
g_ie(char *line, char *e)
{
	char *b;

	if (elsetop >= Maxif-1) {
		fprint(2, "ms2html: .ie's too deep\n");
		return;
	}
	if (ifwastrue[++elsetop] = ifstart(line, e, &b))
		pushbody(b);
	else
		skipbody(b);
}

void
g_if(char *line, char *e)
{
	char *b;

	if (ifstart(line, e, &b))
		pushbody(b);
	else
		skipbody(b);
}

void
g_el(char *line, char *e)
{
	if (elsetop < 0)
		return;
	scanline(line, e, 1);
	if (ifwastrue[elsetop--])
		skipbody(line);
	else
		pushbody(line);
}

void
g_ig(int argc, char **argv)
{
	char *p, *s;

	s = "..";
	if (argc > 1)
		s = argv[1];
	for(;;) {
		p = Brdline(&ssp->in, '\n');
		if(p == nil)
			break;
		p[Blinelen(&ssp->in)-1] = 0;
		if(strcmp(p, s) == 0)
			break;
	}
}

void
g_ds(char *line, char *e)
{
	char *b;

	b = copyline(line, e, 1);
	if (b > line) {
		copyline(b, e, 0);
		if (*b == '"')
			b++;
		ds(line, b);
	}
}

void
g_as(char *line, char *e)
{
	String *s;
	char *b;

	b = copyline(line, e, 1);
	if (b == line)
		return;
	copyline(b, e, 0);
	if (*b == '"')
		b++;
	for(s = strings; s != nil; s = s->next)
		if(strcmp(line, s->name) == 0)
			break;

	if(s == nil){
		ds(line, b);
		return;
	}

	s->val = realloc(s->val, strlen(s->val) + strlen(b) + 1);
	strcat(s->val, b);
}

void
g_BS(int argc, char **argv)
{
	int i;

	if (argc > 1 && !weBref) {
		Bprint(&bout, "<a href=\"%s\"", argv[1]);
		for(i = 2; i < argc; i++)
			Bprint(&bout, " %s", argv[i]);
		Bprint(&bout, ">");
		weBref = 1;
	}
}

void
g_BE(int, char**)
{
	if (weBref) {
		Bprint(&bout, "</a>");
		weBref = 0;
	}
}

void
g_LB(int argc, char **argv)
{
	if (argc > 1) {
		if (weBref)
			g_BE(0,nil);
		Bprint(&bout, "<a name=\"%s\"></a>", argv[1]);
	}
}

void
g_RT(int, char**)
{
	g_BE(0,nil);
	dohanginghead();
	closel();
	closefont();
}
