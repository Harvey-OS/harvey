#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dict.h"
#include "kuten.h"

/*
 * Routines for handling dictionaries in the "Languages of the World"
 * format.  worldnextoff *must* be called with <address of valid entry>+1.
 */

#define	GSHORT(p)	(((p)[0]<<8)|(p)[1])

static void	putchar(int, int*);

#define	NONE	0xffff

/* adapted from jhelling@cs.ruu.nl (Jeroen Hellingman) */

static Rune chartab[] = {

/*00*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	L'\n',	L'æ',	L'ø',	L'å',	L'ä',	L'ö',
/*10*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	NONE,	L'Æ',	L'Ø',	L'Å',	L'Ä',	L'Ö',

/*20*/	L' ',	L'!',	L'"',	L'#',	L'$',	L'%',	L'&',	L'\'',
	L'(',	L')',	L'*',	L'+',	L',',	L'-',	L'.',	L'/',
/*30*/  L'0',	L'1',	L'2',	L'3',	L'4',	L'5',	L'6',	L'7',
	L'8',	L'9',	L':',	L';',	L'<',	L'=',	L'>',	L'?',
/*40*/  L'@',	L'A',	L'B',	L'C',	L'D',	L'E',	L'F',	L'G',
	L'H',	L'I',	L'J',	L'K',	L'L',	L'M',	L'N',	L'O',
/*50*/	L'P',	L'Q',	L'R',	L'S',	L'T',	L'U',	L'V',	L'W',
	L'X',	L'Y',	L'Z',	L'[',	L'\\',	L']',	L'^',	L'_',
/*60*/	L'`',	L'a',	L'b',	L'c',	L'd',	L'e',	L'f',	L'g',
	L'h',	L'i',	L'j',	L'k',	L'l',	L'm',	L'n',	L'o',
/*70*/	L'p',	L'q',	L'r',	L's',	L't',	L'u',	L'v',	L'w',
	L'x',	L'y',	L'z',	L'{',	L'|',	L'}',	L'~',	NONE,

/*80*/	L'Ç',	L'ü',	L'é',	L'â',	L'ä',	L'à',	L'å',	L'ç',
	L'ê',	L'ë',	L'è',	L'ï',	L'î',	L'ì',	L'Ä',	L'Å',
/*90*/	L'É',	L'æ',	L'Æ',	L'ô',	L'ö',	L'ò',	L'û',	L'ù',
	L'ÿ',	L'Ö',	L'Ü',	L'¢',	L'£',	L'¥',	L'₧',	L'ʃ',
/*a0*/	L'á',	L'í',	L'ó',	L'ú',	L'ñ',	L'Ñ',	L'ª',	L'º',
	L'¿',	L'⌐',	L'¬',	L'½',	L'¼',	L'¡',	L'«',	L'»',

/*b0*/	L'ɔ',	L'ə',	L'ð',	L'ʃ',	L'ʒ',	L'ŋ',	L'ɑ',	L'z',
	L'ɪ',	L'ð',	L'ʒ',	L'ã',	L'œ',	L'ũ',	L'ʌ',	L'ɥ',
/*c0*/	L'ʀ',	L'ë',	L'l',	L'ʌ',	L'õ',	L'ñ',	L'Œ',	NONE,
	NONE,	L'S',	L's',	L'Z',	L'z',	NONE,	NONE,	NONE,
/*d0*/	L'ß',	NONE,	NONE,	L'ā',	L'ī',	L'ū',	L'ē',	L'ō',	
	NONE,	NONE,	NONE,	L' ',	NONE,	NONE,	NONE,	NONE,

/*e0*/	L'α',	L'β',	L'γ',	L'π',	L'Σ',	L'σ',	L'µ',	L'τ',
	L'Φ',	L'Θ',	L'Ω',	L'δ',	L'∞',	L'Ø',	L'ε',	L'∩',
/*f0*/	L'≡',	L'±',	L'≥',	L'≤',	L'⌠',	L'⌡',	L'÷',	L'≈',
	L'°',	L'∙',	L'·',	NONE,	NONE,	NONE,	NONE,	NONE,
};

enum{ Utf, Kanahi, Kanalo=Kanahi+1, GBhi, GBlo=GBhi+1, };

void
worldprintentry(Entry e, int cmd)
{
	int nh, state[3];
	uchar *p, *pe;

	p = (uchar *)e.start;
	pe = (uchar *)e.end;
	nh = GSHORT(p);
	p += 6;
	if(cmd == 'h')
		pe = p+nh;
	state[0] = Utf;
	state[1] = 0;
	state[2] = 0;
	while(p < pe){
		if(cmd == 'r')
			outchar(*p++);
		else
			putchar(*p++, state);
	}
	outnl(0);
}

long
worldnextoff(long fromoff)
{
	int nh, np, nd;
	uchar buf[6];

	if(Bseek(bdict, fromoff-1, 0) < 0)
		return -1;
	if(Bread(bdict, buf, 6) != 6)
		return -1;
	nh = GSHORT(buf);
	np = GSHORT(buf+2);
	nd = GSHORT(buf+4);
	return fromoff-1 + 6 + nh + np + nd;
}

static void
putchar(int c, int *state)
{
	int xflag = 0;
	Rune r;
	int hi, lo;

	switch(state[0]){
	case Kanahi:
	case GBhi:
		if(CANS2JH(c) || c == 0xff){
			state[0]++;
			state[1] = c;
			break;
		}
		/* fall through */
	case Utf:
		if(c == 0xfe){
			state[0] = Kanahi;
			break;
		}else if(c == 0xff){
			state[0] = GBhi;
			break;
		}
		r = chartab[c];
		if(r < 0x80 && state[2] == 0)
			outchar(r);
		else if(r == NONE){
			switch(c){
			case 0xfb:
				if(!xflag){
					state[2] = 1;
					break;
				}
			case 0xfc:
				if(!xflag){
					state[2] = 0;
					break;
				}
			case 0x10:
			case 0xc7: case 0xc8:
			case 0xd8: case 0xd9: case 0xda:
			case 0xdc: case 0xdd: case 0xde: case 0xdf:
			case 0xfd:
				if(!xflag)
					break;
				/* fall through */
			default:
				outprint("\\%.2ux", c);
			}
		}else if(state[2] == 0)
			outrune(r);
		break;
	case Kanalo:
	case GBlo:
		if(state[1] == 0xff && c == 0xff){
			state[0] = Utf;
			break;
		}
		state[0]--;
		hi = state[1];
		lo = c;
		S2J(hi, lo);		/* convert to JIS */
		r = hi*100 + lo - 3232;	/* convert to jis208 */
		if(state[0] == Kanahi && r < JIS208MAX)
			r = tabjis208[r];
		else if(state[0] == GBhi && r < GB2312MAX)
			r = tabgb2312[r];
		else
			r = NONE;
		if(r == NONE)
			outprint("\\%.2ux\\%.2ux", state[1], c);
		else
			outrune(r);
		break;
	}
}

void
worldprintkey(void)
{
	Bprint(bout, "No pronunciation key.\n");
}
