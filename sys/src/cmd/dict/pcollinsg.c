#include <u.h>
#include <libc.h>
#include <bio.h>
#include "dict.h"

/*
 * Routines for handling dictionaries in the "Paperback Collins"
 * `German' format (with tags surrounded by \5⋯\6 and \xba⋯\xba)
 */

/*
 *	\5⋯\6 escapes (fonts, mostly)
 *
 *	h	headword (helvetica 7 pt)
 *	c	clause (helvetica 7 pt)
 *	3	helvetica 7 pt
 *	4	helvetica 6.5 pt
 *	s	helvetica 8 pt
 *	x	helvetica 8 pt
 *	y	helvetica 5 pt
 *	m	helvetica 30 pt
 *	1	roman 6 pt
 *	9	roman 4.5 pt
 *	p	roman 7 pt
 *	q	roman 4.5 pt
 *	2	italic 6 pt
 *	7	italic 4.5 pt
 *	b	bold 6 pt
 *	a	`indent 0:4 left'
 *	k	`keep 9'
 *	l	`size 12'
 */

enum {
	IBASE=L'i',	/* dotless i */
	Taglen=32,
};

static Rune intab[256] = {
	/*0*/	/*1*/	/*2*/	/*3*/	/*4*/	/*5*/	/*6*/	/*7*/
/*00*/	NONE,	NONE,	NONE,	NONE,	NONE,	TAGS,	TAGE,	NONE,
	NONE,	NONE,	NONE,	NONE,	NONE,	L' ',	NONE,	NONE,
/*10*/	NONE,	L'-',	L' ',	L' ',	NONE,	NONE,	NONE,	NONE,
	L' ',	NONE,	NONE,	NONE,	L' ',	NONE,	NONE,	L'-',
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
/*80*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	L' ',	NONE,	NONE,	NONE,	NONE,	NONE,
/*90*/	L'ß',	L'æ',	NONE,	MOE,	NONE,	NONE,	NONE,	L'ø',
	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
/*A0*/	NONE,	NONE,	L'"',	L'£',	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
/*B0*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	L'~',
	NONE,	IBASE,	SPCS,	NONE,	NONE,	NONE,	NONE,	NONE,
/*C0*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
/*D0*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
/*E0*/	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
/*F0*/	L' ',	L' ',	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,	NONE,
};

static Nassoc numtab[] = {
	{1,	L'+'},
	{4,	L'='},
	{7,	L'°'},
	{11,	L'≈'},
	{69,	L'♦'},
	{114,	L'®'},
	{340,	L'ɛ'},
	{341,	L'ɔ'},
	{342,	L'ʌ'},
	{343,	L'ə'},
	{345,	L'ʒ'},
	{346,	L'ʃ'},
	{347,	L'ɵ'},
	{348,	L'ʊ'},
	{349,	L'ˈ'},
	{351,	L'ɪ'},
	{352,	L'ɜ'},
	{354,	L'ɑ'},
	{355,	L'~'},
	{356,	L'ɒ'},
	{384,	L'ɳ'},
	{445,	L'ð'},	/* BUG -- should be script eth */
};

static Nassoc overtab[] = {
	{L',',	LCED},
	{L'/',	LACU},
	{L':',	LUML},
	{L'\\',	LGRV},
	{L'^',	LFRN},
	{L'~',	LTIL},
};

static uchar *reach(uchar*, int);

static Entry	curentry;
static char	tag[Taglen];

void
pcollgprintentry(Entry e, int cmd)
{
	uchar *p, *pe;
	int r, rprev = NONE, rx, over = 0, font;
	char buf[16];

	p = (uchar *)e.start;
	pe = (uchar *)e.end;
	curentry = e;
	if(cmd == 'h')
		outinhibit = 1;
	while(p < pe){
		if(cmd == 'r'){
			outchar(*p++);
			continue;
		}
		switch(r = intab[*p++]){	/* assign = */
		case TAGS:
			if(rprev != NONE){
				outrune(rprev);
				rprev = NONE;
			}
			p = reach(p, 0x06);
			font = tag[0];
			if(cmd == 'h')
				outinhibit = (font != 'h');
			break;

		case TAGE:	/* an extra one */
			break;
	
		case SPCS:
			p = reach(p, 0xba);
			r = looknassoc(numtab, asize(numtab), strtol(tag,0,0));
			if(r < 0){
				if(rprev != NONE){
					outrune(rprev);
					rprev = NONE;
				}
				sprint(buf, "\\N'%s'", tag);
				outchars(buf);
				break;
			}
			/* else fall through */

		default:
			if(over){
				rx = looknassoc(overtab, asize(overtab), r);
				if(rx > 0)
					rx = liglookup(rx, rprev);
				if(rx > 0 && rx != NONE)
					outrune(rx);
				else{
					outrune(rprev);
					if(r == ':')
						outrune(L'¨');
					else{
						outrune(L'^');
						outrune(r);
					}
				}
				over = 0;
				rprev = NONE;
			}else if(r == '^'){
				over = 1;
			}else{
				if(rprev != NONE)
					outrune(rprev);
				rprev = r;
			}
		}
		
	}
	if(rprev != NONE)
		outrune(rprev);
	if(cmd == 'h')
		outinhibit = 0;
	outnl(0);
}

long
pcollgnextoff(long fromoff)
{
	int c, state = 0, defoff = -1;

	if(Bseek(bdict, fromoff, 0) < 0)
		return -1;
	while((c = Bgetc(bdict)) >= 0){
		if(c == '\r')
			defoff = Boffset(bdict);
		switch(state){
		case 0:
			if(c == 0x05)
				state = 1;
			break;
		case 1:
			if(c == 'h')
				state = 2;
			else
				state = 0;
			break;
		case 2:
			if(c == 0x06)
				return (Boffset(bdict)-3);
			else
				state = 0;
			break;
		}
	}
	return defoff;
}

void
pcollgprintkey(void)
{
	Bprint(bout, "No pronunciation key yet\n");
}

static uchar *
reach(uchar *p, int tagchar)
{
	int c; char *q=tag;

	while(p < (uchar *)curentry.end){
		c = *p++;
		if(c == tagchar)
			break;
		*q++ = c;
		if(q >= &tag[sizeof tag-1])
			break;
	}
	*q = 0;
	return p;
}
