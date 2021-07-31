#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dict.h"

Dict dicts[] = {
	{"oed",		"Oxford English Dictionary, 2nd Ed.",
	 "/lib/dict/oed2",	"/lib/dict/oed2index",
	 oednextoff,	oedprintentry,		oedprintkey},
	{"ahd",		"American Heritage Dictionary, 2nd College Ed.",
	 "/lib/ahd/DICT.DB",	"/lib/ahd/index",
	 ahdnextoff,	ahdprintentry,		ahdprintkey},
	{"pgw",		"Project Gutenberg Webster Dictionary",
	 "/lib/dict/pgw",	"/lib/dict/pgwindex",
	 pgwnextoff,	pgwprintentry,		pgwprintkey},
	{"thesaurus",	"Collins Thesaurus",
	 "/lib/dict/thesaurus",	"/lib/dict/thesindex",
	 thesnextoff,	thesprintentry,	thesprintkey},
	{"roget",		"Project Gutenberg Roget's Thesaurus",
	 "/lib/dict/roget", "/lib/dict/rogetindex",
	 rogetnextoff,	rogetprintentry,	rogetprintkey},

	{"ce",		"Gendai Chinese->English",
	 "/lib/dict/world/sansdata/sandic24.dat",
	 "/lib/dict/world/sansdata/ceindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"ceh",		"Gendai Chinese->English (Hanzi index)",
	 "/lib/dict/world/sansdata/sandic24.dat",
	 "/lib/dict/world/sansdata/cehindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"ec",		"Gendai English->Chinese",
	 "/lib/dict/world/sansdata/sandic24.dat",
	 "/lib/dict/world/sansdata/ecindex",
	 worldnextoff,	worldprintentry,	worldprintkey},

	{"dae",		"Gyldendal Danish->English",
	 "/lib/dict/world/gylddata/sandic30.dat",
	 "/lib/dict/world/gylddata/daeindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"eda",		"Gyldendal English->Danish",
	 "/lib/dict/world/gylddata/sandic29.dat",
	 "/lib/dict/world/gylddata/edaindex",
	 worldnextoff,	worldprintentry,	worldprintkey},

	{"due",		"Wolters-Noordhoff Dutch->English",
	 "/lib/dict/world/woltdata/sandic07.dat",
	 "/lib/dict/world/woltdata/deindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"edu",		"Wolters-Noordhoff English->Dutch",
	 "/lib/dict/world/woltdata/sandic06.dat",
	 "/lib/dict/world/woltdata/edindex",
	 worldnextoff,	worldprintentry,	worldprintkey},

	{"fie",		"WSOY Finnish->English",
	 "/lib/dict/world/werndata/sandic32.dat",
	 "/lib/dict/world/werndata/fieindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"efi",		"WSOY English->Finnish",
	 "/lib/dict/world/werndata/sandic31.dat",
	 "/lib/dict/world/werndata/efiindex",
	 worldnextoff,	worldprintentry,	worldprintkey},

	{"fe",		"Collins French->English",
	 "/lib/dict/fe",	"/lib/dict/feindex",
	 pcollnextoff,	pcollprintentry,	pcollprintkey},
	{"ef",		"Collins English->French",
	 "/lib/dict/ef",	"/lib/dict/efindex",
	 pcollnextoff,	pcollprintentry,	pcollprintkey},

	{"ge",		"Collins German->English",
	 "/lib/dict/ge",	"/lib/dict/geindex",
	 pcollgnextoff,	pcollgprintentry,	pcollgprintkey},
	{"eg",		"Collins English->German",
	 "/lib/dict/eg",	"/lib/dict/egindex",
	 pcollgnextoff,	pcollgprintentry,	pcollgprintkey},

	{"ie",		"Collins Italian->English",
	 "/lib/dict/ie",	"/lib/dict/ieindex",
	 pcollnextoff,	pcollprintentry,	pcollprintkey},
	{"ei",		"Collins English->Italian",
	 "/lib/dict/ei",	"/lib/dict/eiindex",
	 pcollnextoff,	pcollprintentry,	pcollprintkey},

	{"je",		"Sanshusha Japanese->English",
	 "/lib/dict/world/sansdata/sandic18.dat",
	 "/lib/dict/world/sansdata/jeindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"jek",		"Sanshusha Japanese->English (Kanji index)",
	 "/lib/dict/world/sansdata/sandic18.dat",
	 "/lib/dict/world/sansdata/jekindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"ej",		"Sanshusha English->Japanese",
	 "/lib/dict/world/sansdata/sandic18.dat",
	 "/lib/dict/world/sansdata/ejindex",
	 worldnextoff,	worldprintentry,	worldprintkey},

	{"tjeg",	"Sanshusha technical Japanese->English,German",
	 "/lib/dict/world/sansdata/sandic16.dat",
	 "/lib/dict/world/sansdata/tjegindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"tjegk",	"Sanshusha technical Japanese->English,German (Kanji index)",
	 "/lib/dict/world/sansdata/sandic16.dat",
	 "/lib/dict/world/sansdata/tjegkindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"tegj",	"Sanshusha technical English->German,Japanese",
	 "/lib/dict/world/sansdata/sandic16.dat",
	 "/lib/dict/world/sansdata/tegjindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"tgje",	"Sanshusha technical German->Japanese,English",
	 "/lib/dict/world/sansdata/sandic16.dat",
	 "/lib/dict/world/sansdata/tgjeindex",
	 worldnextoff,	worldprintentry,	worldprintkey},

	{"ne",		"Kunnskapforlaget Norwegian->English",
	 "/lib/dict/world/kunndata/sandic28.dat",
	 "/lib/dict/world/kunndata/neindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"en",		"Kunnskapforlaget English->Norwegian",
	 "/lib/dict/world/kunndata/sandic27.dat",
	 "/lib/dict/world/kunndata/enindex",
	 worldnextoff,	worldprintentry,	worldprintkey},

	{"re",		"Leon Ungier Russian->English",
	 "/lib/dict/re",	"/lib/dict/reindex",
	 simplenextoff,	simpleprintentry,	simpleprintkey},
	{"er",		"Leon Ungier English->Russian",
	 "/lib/dict/re",	"/lib/dict/erindex",
	 simplenextoff,	simpleprintentry,	simpleprintkey},

	{"se",		"Collins Spanish->English",
	 "/lib/dict/se",	"/lib/dict/seindex",
	 pcollnextoff,	pcollprintentry,	pcollprintkey},
	{"es",		"Collins English->Spanish",
	 "/lib/dict/es",	"/lib/dict/esindex",
	 pcollnextoff,	pcollprintentry,	pcollprintkey},

	{"swe",		"Esselte Studium Swedish->English",
	 "/lib/dict/world/essedata/sandic34.dat",
	 "/lib/dict/world/essedata/sweindex",
	 worldnextoff,	worldprintentry,	worldprintkey},
	{"esw",		"Esselte Studium English->Swedish",
	 "/lib/dict/world/essedata/sandic33.dat",
	 "/lib/dict/world/essedata/eswindex",
	 worldnextoff,	worldprintentry,	worldprintkey},

	{"movie",	"Movies -- by title",
	 "/lib/movie/data",	"/lib/dict/movtindex",
	 movienextoff,	movieprintentry,	movieprintkey},
	{"moviea",	"Movies -- by actor",
	 "/lib/movie/data",	"/lib/dict/movaindex",
	 movienextoff,	movieprintentry,	movieprintkey},
	{"movied",	"Movies -- by director",
	 "/lib/movie/data",	"/lib/dict/movdindex",
	 movienextoff,	movieprintentry,	movieprintkey},

	{"slang",	"English Slang",
	 "/lib/dict/slang",	"/lib/dict/slangindex",
	 slangnextoff,	slangprintentry,	slangprintkey},

	{"robert",	"Robert Électronique",
	 "/lib/dict/robert/_pointers",	"/lib/dict/robert/_index",
	 robertnextoff,	robertindexentry,	robertprintkey},
	{"robertv",	"Robert Électronique - formes des verbes",
	 "/lib/dict/robert/flex.rob",	"/lib/dict/robert/_flexindex",
	 robertnextflex,	robertflexentry,	robertprintkey},

	{0, 0, 0, 0, 0}
};

typedef struct Lig Lig;
struct Lig {
	Rune	start;		/* accent rune */
	Rune	*pairs;		/* <char,accented version> pairs */
};

static Lig ligtab[Nligs] = {
[LACU-LIGS]	{L'´',	L"AÁaáCĆcćEÉeégģIÍiíıíLĹlĺNŃnńOÓoóRŔrŕSŚsśUÚuúYÝyýZŹzź"},
[LGRV-LIGS]	{L'ˋ',	L"AÀaàEÈeèIÌiìıìOÒoòUÙuù"},
[LUML-LIGS]	{L'¨',	L"AÄaäEËeëIÏiïOÖoöUÜuüYŸyÿ"},
[LCED-LIGS]	{L'¸',	L"CÇcçGĢKĶkķLĻlļNŅnņRŖrŗSŞsşTŢtţ"},
[LTIL-LIGS]	{L'˜',	L"AÃaãIĨiĩıĩNÑnñOÕoõUŨuũ"},
[LBRV-LIGS]	{L'˘',	L"AĂaăEĔeĕGĞgğIĬiĭıĭOŎoŏUŬuŭ"},
[LRNG-LIGS]	{L'˚',	L"AÅaåUŮuů"},
[LDOT-LIGS]	{L'˙',	L"CĊcċEĖeėGĠgġIİLĿlŀZŻzż"},
[LDTB-LIGS]	{L'.',	L""},
[LFRN-LIGS]	{L'⌢',	L"AÂaâCĈcĉEÊeêGĜgĝHĤhĥIÎiîıîJĴjĵOÔoôSŜsŝUÛuûWŴwŵYŶyŷ"},
[LFRB-LIGS]	{L'̯',	L""},
[LOGO-LIGS]	{L'˛',	L"AĄaąEĘeęIĮiįıįUŲuų"},
[LMAC-LIGS]	{L'¯',	L"AĀaāEĒeēIĪiīıīOŌoōUŪuū"},
[LHCK-LIGS]	{L'ˇ',	L"CČcčDĎdďEĚeěLĽlľNŇnňRŘrřSŠsšTŤtťZŽzž"},
[LASP-LIGS]	{L'ʽ',	L""},
[LLEN-LIGS]	{L'ʼ',	L""},
[LBRB-LIGS]	{L'̮',	L""}
};

Rune *multitab[Nmulti] = {
[MAAS-MULTI]	L"ʽα",
[MALN-MULTI]	L"ʼα",
[MAND-MULTI]	L"and",
[MAOQ-MULTI]	L"a/q",
[MBRA-MULTI]	L"<|",
[MDD-MULTI]	L"..",
[MDDD-MULTI]	L"...",
[MEAS-MULTI]	L"ʽε",
[MELN-MULTI]	L"ʼε",
[MEMM-MULTI]	L"——",
[MHAS-MULTI]	L"ʽη",
[MHLN-MULTI]	L"ʼη",
[MIAS-MULTI]	L"ʽι",
[MILN-MULTI]	L"ʼι",
[MLCT-MULTI]	L"ct",
[MLFF-MULTI]	L"ff",
[MLFFI-MULTI]	L"ffi",
[MLFFL-MULTI]	L"ffl",
[MLFL-MULTI]	L"fl",
[MLFI-MULTI]	L"fi",
[MLLS-MULTI]	L"ɫɫ",
[MLST-MULTI]	L"st",
[MOAS-MULTI]	L"ʽο",
[MOLN-MULTI]	L"ʼο",
[MOR-MULTI]	L"or",
[MRAS-MULTI]	L"ʽρ",
[MRLN-MULTI]	L"ʼρ",
[MTT-MULTI]	L"~~",
[MUAS-MULTI]	L"ʽυ",
[MULN-MULTI]	L"ʼυ",
[MWAS-MULTI]	L"ʽω",
[MWLN-MULTI]	L"ʼω",
[MOE-MULTI]	L"oe",
[MES-MULTI]	L"  ",
};

#define	risupper(r)	(L'A' <= (r) && (r) <= L'Z')
#define	rislatin1(r)	(0xC0 <= (r) && (r) <= 0xFF)
#define	rtolower(r)	((r)-'A'+'a')

static Rune latin_fold_tab[] =
{
/*	Table to fold latin 1 characters to ASCII equivalents
			based at Rune value 0xc0

	 À    Á    Â    Ã    Ä    Å    Æ    Ç
	 È    É    Ê    Ë    Ì    Í    Î    Ï
	 Ð    Ñ    Ò    Ó    Ô    Õ    Ö    ×
	 Ø    Ù    Ú    Û    Ü    Ý    Þ    ß
	 à    á    â    ã    ä    å    æ    ç
	 è    é    ê    ë    ì    í    î    ï
	 ð    ñ    ò    ó    ô    õ    ö    ÷
	 ø    ù    ú    û    ü    ý    þ    ÿ
*/
	'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c',
	'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
	'd', 'n', 'o', 'o', 'o', 'o', 'o',  0 ,
	'o', 'u', 'u', 'u', 'u', 'y',  0 ,  0 ,
	'a', 'a', 'a', 'a', 'a', 'a', 'a', 'c',
	'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
	'd', 'n', 'o', 'o', 'o', 'o', 'o',  0 ,
	'o', 'u', 'u', 'u', 'u', 'y',  0 , 'y',
};

static Rune 	*ttabstack[20];
static int	ntt;

/*
 * tab is an array of n Assoc's, sorted by key.
 * Look for key in tab, and return corresponding val
 * or -1 if not there
 */
long
lookassoc(Assoc *tab, int n, char *key)
{
	Assoc *q;
	long i, low, high;
	int r;

	for(low = -1, high = n; high > low+1; ){
		i = (high+low)/2;
		q = &tab[i];
		if((r=strcmp(key, q->key))<0)
			high = i;
		else if(r == 0)
			return q->val;
		else
			low=i;
	}
	return -1;
}

long
looknassoc(Nassoc *tab, int n, long key)
{
	Nassoc *q;
	long i, low, high;

	for(low = -1, high = n; high > low+1; ){
		i = (high+low)/2;
		q = &tab[i];
		if(key < q->key)
			high = i;
		else if(key == q->key)
			return q->val;
		else
			low=i;
	}
	return -1;
}

void
err(char *fmt, ...)
{
	char buf[1000];
	va_list v;

	va_start(v, fmt);
	vsnprint(buf, sizeof(buf), fmt, v);
	va_end(v);
	fprint(2, "%s: %s\n", argv0, buf);
}

/*
 * Write the rune r to bout, keeping track of line length
 * and breaking the lines (at blanks) when they get too long
 */
void
outrune(long r)
{
	if(outinhibit)
		return;
	if(++linelen > breaklen && r == L' ') {
		Bputc(bout, '\n');
		linelen = 0;
	} else
		Bputrune(bout, r);
}

void
outrunes(Rune *rp)
{
	Rune r;

	while((r = *rp++) != 0)
		outrune(r);
}

/* like outrune, but when arg is know to be a char */
void
outchar(int c)
{
	if(outinhibit)
		return;
	if(++linelen > breaklen && c == ' ') {
		c ='\n';
		linelen = 0;
	}
	Bputc(bout, c);
}

void
outchars(char *s)
{
	char c;

	while((c = *s++) != 0)
		outchar(c);
}

void
outprint(char *fmt, ...)
{
	char buf[1000];
	va_list v;

	va_start(v, fmt);
	vsnprint(buf, sizeof(buf), fmt, v);
	va_end(v);
	outchars(buf);
}

void
outpiece(char *b, char *e)
{
	int c, lastc;

	lastc = 0;
	while(b < e) {
		c = *b++;
		if(c == '\n')
			c = ' ';
		if(!(c == ' ' && lastc == ' '))
			outchar(c);
		lastc = c;
	}
}

/*
 * Go to new line if not already there; indent if ind != 0.
 * If ind > 1, leave a blank line too.
 * Slight hack: assume if current line is only one or two
 * characters long, then they were spaces.
 */
void
outnl(int ind)
{
	if(outinhibit)
		return;
	if(ind) {
		if(ind > 1) {
			if(linelen > 2)
				Bputc(bout, '\n');
			Bprint(bout, "\n  ");
		} else if(linelen == 0)
			Bprint(bout, "  ");
		else if(linelen == 1)
			Bputc(bout, ' ');
		else if(linelen != 2)
			Bprint(bout, "\n  ");
		linelen = 2;
	} else {
		if(linelen) {
			Bputc(bout, '\n');
			linelen = 0;
		}
	}
}

/*
 * Fold the runes in null-terminated rp.
 * Use the sort(1) definition of folding (uppercase to lowercase,
 * latin1-accented characters to corresponding unaccented chars)
 */
void
fold(Rune *rp)
{
	Rune r;

	while((r = *rp) != 0) {
		if (rislatin1(r) && latin_fold_tab[r-0xc0])
				r = latin_fold_tab[r-0xc0];
		if(risupper(r))
			r = rtolower(r);
		*rp++ = r;
	}
}

/*
 * Like fold, but put folded result into new
 * (assumed to have enough space).
 * old is a regular expression, but we know that
 * metacharacters aren't affected
 */
void
foldre(char *new, char *old)
{
	Rune r;

	while(*old) {
		old += chartorune(&r, old);
		if (rislatin1(r) && latin_fold_tab[r-0xc0])
				r = latin_fold_tab[r-0xc0];
		if(risupper(r))
			r = rtolower(r);
		new += runetochar(new, &r);
	}
	*new = 0;
}

/*
 *	acomp(s, t) returns:
 *		-2 if s strictly precedes t
 *		-1 if s is a prefix of t
 *		0 if s is the same as t
 *		1 if t is a prefix of s
 *		2 if t strictly precedes s
 */

int
acomp(Rune *s, Rune *t)
{
	int cs, ct;

	for(;;) {
		cs = *s;
		ct = *t;
		if(cs != ct)
			break;
		if(cs == 0)
			return 0;
		s++;
		t++;
	}
	if(cs == 0)
		return -1;
	if(ct == 0)
		return 1;
	if(cs < ct)
		return -2;
	return 2;
}

/*
 * Copy null terminated Runes from 'from' to 'to'.
 */
void
runescpy(Rune *to, Rune *from)
{
	while((*to++ = *from++) != 0)
		continue;
}

/*
 * Conversion of unsigned number to long, no overflow detection
 */
long
runetol(Rune *r)
{
	int c;
	long n;

	n = 0;
	for(;; r++){
		c = *r;
		if(L'0'<=c && c<=L'9')
			c -= '0';
		else
			break;
		n = n*10 + c;
	}
	return n;
}

/*
 * See if there is a rune corresponding to the accented
 * version of r with accent acc (acc in [LIGS..LIGE-1]),
 * and return it if so, else return NONE.
 */
Rune
liglookup(Rune acc, Rune r)
{
	Rune *p;

	if(acc < LIGS || acc >= LIGE)
		return NONE;
	for(p = ligtab[acc-LIGS].pairs; *p; p += 2)
		if(*p == r)
			return *(p+1);
	return NONE;
}

/*
 * Maintain a translation table stack (a translation table
 * is an array of Runes indexed by bytes or 7-bit bytes).
 * If starting is true, push the curtab onto the stack
 * and return newtab; else pop the top of the stack and
 * return it.
 * If curtab is 0, initialize the stack and return.
 */
Rune *
changett(Rune *curtab, Rune *newtab, int starting)
{
	if(curtab == 0) {
		ntt = 0;
		return 0;
	}
	if(starting) {
		if(ntt >= asize(ttabstack)) {
			if(debug)
				err("translation stack overflow");
			return curtab;
		}
		ttabstack[ntt++] = curtab;
		return newtab;
	} else {
		if(ntt == 0) {
			if(debug)
				err("translation stack underflow");
			return curtab;
		}
		return ttabstack[--ntt];
	}
}
