#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dict.h"

/*
 * Robert Électronique.
 */

enum
{
	CIT = MULTIE+1,	/* citation ptr followed by long int and ascii label */
	BROM,		/* bold roman */
	ITON,		/* start italic */
	ROM,		/* roman */
	SYM,		/* symbol font? */
	HEL,		/* helvetica */
	BHEL,		/* helvetica bold */
	SMALL,		/* smaller? */
	ITOFF,		/* end italic */
	SUP,		/* following character is superscript */
	SUB		/* following character is subscript */
};

static Rune intab[256] = {
	/*0*/	/*1*/	/*2*/	/*3*/	/*4*/	/*5*/	/*6*/	/*7*/
/*00*/	NONE,	L'☺',	L'☻',	L'♥',	L'♦',	L'♣',	L'♠',	L'•',
	0x25d8,	L'ʘ',	L'\n',	L'♂',	L'♀',	L'♪',	0x266b,	L'※',
/*10*/	L'⇨',	L'⇦',	L'↕',	L'‼',	L'¶',	L'§',	L'⁃',	L'↨',
	L'↑',	L'↓',	L'→',	L'←',	L'⌙',	L'↔',	0x25b4,	0x25be,
/*20*/	L' ',	L'!',	L'"',	L'#',	L'$',	L'%',	L'&',	L''',
	L'(',	L')',	L'*',	L'+',	L',',	L'-',	L'.',	L'/',
/*30*/	L'0',	L'1',	L'2',	L'3',	L'4',	L'5',	L'6',	L'7',
	L'8',	L'9',	L':',	L';',	L'<',	L'=',	L'>',	L'?',
/*40*/	L'@',	L'A',	L'B',	L'C',	L'D',	L'E',	L'F',	L'G',
	L'H',	L'I',	L'J',	L'K',	L'L',	L'M',	L'N',	L'O',
/*50*/	L'P',	L'Q',	L'R',	L'S',	L'T',	L'U',	L'V',	L'W',
	L'X',	L'Y',	L'Z',	L'[',	L'\\',	L']',	L'^',	L'_',
/*60*/	L'`',	L'a',	L'b',	L'c',	L'd',	L'e',	L'f',	L'g',
	L'h',	L'i',	L'j',	L'k',	L'l',	L'm',	L'n',	L'o',
/*70*/	L'p',	L'q',	L'r',	L's',	L't',	L'u',	L'v',	L'w',
	L'x',	L'y',	L'z',	L'{',	L'|',	L'}',	L'~',	L'',
/*80*/	L'Ç',	L'ü',	L'é',	L'â',	L'ä',	L'à',	L'å',	L'ç',
	L'ê',	L'ë',	L'è',	L'ï',	L'î',	L'ì',	L'Ä',	L'Å',
/*90*/	L'É',	L'æ',	L'Æ',	L'ô',	L'ö',	L'ò',	L'û',	L'ù',
	L'ÿ',	L'Ö',	L'Ü',	L'¢',	L'£',	L'¥',	L'₧',	L'ʃ',
/*a0*/	L'á',	L'í',	L'ó',	L'ú',	L'ñ',	L'Ñ',	L'ª',	L'º',
	L'¿',	L'⌐',	L'¬',	L'½',	L'¼',	L'¡',	L'«',	L'»',
/*b0*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
/*c0*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	CIT,	BROM,	NONE,	ITON,	ROM,	SYM,	HEL,	BHEL,
/*d0*/	NONE,	SMALL,	ITOFF,	SUP,	SUB,	NONE,	NONE,	NONE,
	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
/*e0*/	L'α',	L'ß',	L'γ',	L'π',	L'Σ',	L'σ',	L'µ',	L'τ',
	L'Φ',	L'Θ',	L'Ω',	L'δ',	L'∞',	L'Ø',	L'ε',	L'∩',
/*f0*/	L'≡',	L'±',	L'≥',	L'≤',	L'⌠',	L'⌡',	L'÷',	L'≈',
	L'°',	L'∙',	L'·',	L'√',	L'ⁿ',	L'²',	L'∎',	L' ',
};

static Rune suptab[] = {
	['0'] L'⁰',	['1'] L'ⁱ',	['2'] L'⁲',	['3'] L'⁳',
	['4'] L'⁴',	['5'] L'⁵',	['6'] L'⁶',	['7'] L'⁷',
	['8'] L'⁸',	['9'] L'⁹',	['+'] L'⁺',	['-'] L'⁻',
	['='] L'⁼',	['('] L'⁽',	[')'] L'⁾',	['a'] L'ª',
	['n'] L'ⁿ',	['o'] L'º'
};

static Rune subtab[] = {
	['0'] L'₀',	['1'] L'₁',	['2'] L'₂',	['3'] L'₃',
	['4'] L'₄',	['5'] L'₅',	['6'] L'₆',	['7'] L'₇',
	['8'] L'₈',	['9'] L'₉',	['+'] L'₊',	['-'] L'₋',
	['='] L'₌',	['('] L'₍',	[')'] L'₎'
};

#define	GSHORT(p)	(((p)[0]<<8) | (p)[1])
#define	GLONG(p)	(((p)[0]<<24) | ((p)[1]<<16) | ((p)[2]<<8) | (p)[3])

static char	cfile[] = "/lib/dict/robert/cits.rob";
static char	dfile[] = "/lib/dict/robert/defs.rob";
static char	efile[] = "/lib/dict/robert/etym.rob";
static char	kfile[] = "/lib/dict/robert/_phon";

static Biobuf *	cb;
static Biobuf *	db;
static Biobuf *	eb;

static Biobuf *	Bouvrir(char*);
static void	citation(int, int);
static void	robertprintentry(Entry*, Entry*, int);

void
robertindexentry(Entry e, int cmd)
{
	uchar *p = (uchar *)e.start;
	long ea, el, da, dl, fa;
	Entry def, etym;

	ea = GLONG(&p[0]);
	el = GSHORT(&p[4]);
	da = GLONG(&p[6]);
	dl = GSHORT(&p[10]);
	fa = GLONG(&p[12]);
	USED(fa);

	if(db == 0)
		db = Bouvrir(dfile);
	def.start = malloc(dl+1);
	def.end = def.start + dl;
	def.doff = da;
	Bseek(db, da, 0);
	Bread(db, def.start, dl);
	*def.end = 0;
	if(cmd == 'h'){
		robertprintentry(&def, 0, cmd);
	}else{
		if(eb == 0)
			eb = Bouvrir(efile);
		etym.start = malloc(el+1);
		etym.end = etym.start + el;
		etym.doff = ea;
		Bseek(eb, ea, 0);
		Bread(eb, etym.start, el);
		*etym.end = 0;
		robertprintentry(&def, &etym, cmd);
		free(etym.start);
	}
	free(def.start);
}

static void
robertprintentry(Entry *def, Entry *etym, int cmd)
{
	uchar *p, *pe;
	Rune r; int c, n;
	int baseline = 0;
	int lineno = 0;
	int cit = 0;

	p = (uchar *)def->start;
	pe = (uchar *)def->end;
	while(p < pe){
		if(cmd == 'r'){
			outchar(*p++);
			continue;
		}
		c = *p++;
		switch(r = intab[c]){	/* assign = */
		case BROM:
		case ITON:
		case ROM:
		case SYM:
		case HEL:
		case BHEL:
		case SMALL:
		case ITOFF:
		case NONE:
			if(debug)
				outprint("\\%.2ux", c);
			baseline = 0;
			break;

		case SUP:
			baseline = 1;
			break;

		case SUB:
			baseline = -1;
			break;

		case CIT:
			n = p[0] | (p[1]<<8) | (p[2]<<16) | (p[3]<<24);
			p += 4;
			if(debug)
				outprint("[%d]", n);
			while(*p == ' ' || ('0'<=*p && *p<='9') || *p == '.'){
				if(debug)
					outchar(*p);
				++p;
			}
			++cit;
			outnl(2);
			citation(n, cmd);
			baseline = 0;
			break;

		case '\n':
			outnl(0);
			baseline = 0;
			++lineno;
			break;

		default:
			if(baseline > 0 && r < nelem(suptab))
				r = suptab[r];
			else if(baseline < 0 && r < nelem(subtab))
				r = subtab[r];
			if(cit){
				outchar('\n');
				cit = 0;
			}
			outrune(r);
			baseline = 0;
			break;
		}
		if(r == '\n'){
			if(cmd == 'h')
				break;
			if(lineno == 1 && etym)
				robertprintentry(etym, 0, cmd);
		}
	}
	outnl(0);
}

static void
citation(int addr, int cmd)
{
	Entry cit;

	if(cb == 0)
		cb = Bouvrir(cfile);
	Bseek(cb, addr, 0);
	cit.start = Brdline(cb, 0xc8);
	cit.end = cit.start + Blinelen(cb) - 1;
	cit.doff = addr;
	*cit.end = 0;
	robertprintentry(&cit, 0, cmd);
}

long
robertnextoff(long fromoff)
{
	return (fromoff & ~15) + 16;
}

void
robertprintkey(void)
{
	Biobuf *db;
	char *l;

	db = Bouvrir(kfile);
	while(l = Brdline(db, '\n'))	/* assign = */
		Bwrite(bout, l, Blinelen(db));
	Bterm(db);
}

void
robertflexentry(Entry e, int cmd)
{
	uchar *p, *pe;
	Rune r; int c;
	int lineno = 1;

	p = (uchar *)e.start;
	pe = (uchar *)e.end;
	while(p < pe){
		if(cmd == 'r'){
			Bputc(bout, *p++);
			continue;
		}
		c = *p++;
		r = intab[c];
		if(r == '$')
			r = '\n';
		if(r == '\n'){
			++lineno;
			if(cmd == 'h' && lineno > 2)
				break;
		}
		if(cmd == 'h' && lineno < 2)
			continue;
		if(r > MULTIE){
			if(debug)
				Bprint(bout, "\\%.2ux", c);
			continue;
		}
		if(r < Runeself)
			Bputc(bout, r);
		else
			Bputrune(bout, r);
	}
	outnl(0);
}

long
robertnextflex(long fromoff)
{
	int c;

	if(Bseek(bdict, fromoff, 0) < 0)
		return -1;
	while((c = Bgetc(bdict)) >= 0){
		if(c == '$')
			return Boffset(bdict);
	}
	return -1;
}

static Biobuf *
Bouvrir(char *fichier)
{
	Biobuf *db;

	db = Bopen(fichier, OREAD);
	if(db == 0){
		fprint(2, "%s: impossible d'ouvrir %s: %r\n", argv0, fichier);
		exits("ouvrir");
	}
	return db;
}
