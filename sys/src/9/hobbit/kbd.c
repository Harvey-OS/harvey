#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

#define	No	(-1)	/* key not used */
#define	None	(-1)	/* key can't happen */

enum
{
	Up=	0x80,	/* flag */
	View=	0x80,	/* 8½ convention */
};

enum
{
	/* unmapped special functions */

	Lctrl=	2,
	Rctrl=	85,
	Lshift=	5,
	Rshift=	4,
	Cplock=	3,
	Lalt=	83,
	Ralt=	84,
	Numlck=	106,
	Config=	110,

	/* function key row */

	Escape=	0x1b,
	F1=	No,
	F2=	No,
	F3=	No,
	F4=	No,
	F5=	No,
	F6=	No,
	F7=	No,
	F8=	No,
	F9=	No,
	F10=	No,
	F11=	No,
	F12=	No,
	Pr_scr=	No,
	Scrlck=	No,
	Pause=	No,

	/* numeric pad */

	P0=	'0',
	P1=	'1',
	P2=	'2',
	P3=	'3',
	P4=	'4',
	P5=	'5',
	P6=	'6',
	P7=	'7',
	P8=	'8',
	P9=	'9',
	Pdot=	'.',
	Penter=	'\n',
	Pminus=	'-',
	Pplus=	'+',
	Pslash=	'/',
	Pstar=	'*',

	/* edit pad */

	Insert=	No,
	Home=	No,
	Pageup=	No,
	Delete=	0x7f,
	End=	No,
	Pagedn=	No,

	Uarrow=	No,
	Larrow=	View,
	Darrow=	View,
	Rarrow=	View,

	/* reserved */

	XBreak=	0,
	XSetup=	1,
	XNscrl=	12,
	XLf=	59,
	XPF2=	70,
	XPF1=	71,
	XPcom=	76,
	XPF4=	77,
	XPF3=	78,
};

static short keymap[] = {
	XBreak,	XSetup,	Lctrl,	Cplock,	Rshift,	Lshift,	Escape,	'1',
	'\t',	'q',	'a',	's',	XNscrl,	'2',	'3',	'w',
	'e',	'd',	'f',	'z',	'x',	'4',	'5',	'r',
	't',	'g',	'h',	'c',	'v',	'6',	'7',	'y',
	'u',	'j',	'k',	'b',	'n',	'8',	'9',	'i',
	'o',	'l',	';',	'm',	',',	'0',	'-',	'p',
	'[',	'\'',	'\n',	'.',	'/',	'=',	'`',	']',
	'\\',	P1,	P0,	XLf,	'\b',	Delete,	P4,	P2,
	P3,	Pdot,	P7,	P8,	P5,	P6,	XPF2,	XPF1,
	Larrow,	Darrow,	P9,	Pminus,	XPcom,	XPF4,	XPF3,	Rarrow,
	Uarrow,	Penter,	' ',	Lalt,	Ralt,	Rctrl,	F1,	F2,
	F3,	F4,	F5,	F6,	F7,	F8,	F9,	F10,
	F11,	F12,	Pr_scr,	Scrlck,	Pause,	Insert,	Home,	Pageup,
	End,	Pagedn,	Numlck,	Pslash,	Pstar,	Pplus,	Config,	None,
	None,	None,	None,	None,	None,	None,	None,	None,
	None,	None,	None,	None,	None,	None,	None,	None,
};

static short skeymap[] = {
	XBreak,	XSetup,	Lctrl,	Cplock,	Rshift,	Lshift,	Escape,	'!',
	'\t',	'Q',	'A',	'S',	XNscrl,	'@',	'#',	'W',
	'E',	'D',	'F',	'Z',	'X',	'$',	'%',	'R',
	'T',	'G',	'H',	'C',	'V',	'^',	'&',	'Y',
	'U',	'J',	'K',	'B',	'N',	'*',	'(',	'I',
	'O',	'L',	':',	'M',	'<',	')',	'_',	'P',
	'{',	'"',	'\n',	'>',	'?',	'+',	'~',	'}',
	'|',	P1,	P0,	XLf,	'\b',	Delete,	P4,	P2,
	P3,	Pdot,	P7,	P8,	P5,	P6,	XPF2,	XPF1,
	Larrow,	Darrow,	P9,	Pminus,	XPcom,	XPF4,	XPF3,	Rarrow,
	Uarrow,	Penter,	' ',	Lalt,	Ralt,	Rctrl,	F1,	F2,
	F3,	F4,	F5,	F6,	F7,	F8,	F9,	F10,
	F11,	F12,	Pr_scr,	Scrlck,	Pause,	Insert,	Home,	Pageup,
	End,	Pagedn,	Numlck,	Pslash,	Pstar,	Pplus,	Config,	None,
	None,	None,	None,	None,	None,	None,	None,	None,
	None,	None,	None,	None,	None,	None,	None,	None,
};

enum
{
	Beep=	0x02<<8,
	Beeeep=	0x04<<8,
	Noclick=0x08
};

static IOQ	liteq;

static struct Lite
{
	Lock;
	QLock;
	Rendez;
	ulong	bits;
} lites;

void
lightbits(int clr, int set)
{
	char c[2];
	int s;

	s = splhi();
	lock(&lites);
	lites.bits &= ~clr;
	lites.bits |= set;
	c[0] = ((lites.bits&0x03)<<5)|Noclick|((lites.bits>>8)&0x06);
	c[1] =  (lites.bits&0x3c)|1;
	(*liteq.puts)(&liteq, c, 2);
	unlock(&lites);
	splx(s);
}

static int
rawkbdputc(IOQ *q, int code)
{
	static int shifted;
	static int ctrled;
	static int capslock;
	static int numlock;
	static int lstate;
	static uchar kc[5];
	int ch, c, i, nk;

	USED(q);
	code &= 0xff;
	switch(code){
	case Lctrl:
	case Rctrl:
		ctrled = 1;
		return 0;
	case Up|Lctrl:
	case Up|Rctrl:
		ctrled = 0;
		return 0;
	case Lshift:
	case Rshift:
		shifted = 1;
		return 0;
	case Up|Lshift:
	case Up|Rshift:
		shifted = 0;
		return 0;
	case Numlck:
		numlock ^= 1;
		lightbits(1, numlock);
	case Up|Numlck:
		return 0;
	case Cplock:
		capslock ^= 2;
		lightbits(2, capslock);
	case Up|Cplock:
		return 0;
	case Lalt:
	case Ralt:
		lstate = 1;
	case Up|Lalt:
	case Up|Ralt:
		return 0;
	}
	if(code&Up)
		return 0;

	/*
	 *  convert
	 */
	if(shifted)
		ch = skeymap[code];
	else
		ch = keymap[code];
	if(ch < 0)
		return 0;
	if(capslock && ch >= 'a' && ch <= 'z')
		ch &= ~0x20;
	if(ctrled)
		ch &= 0x1f;
	switch(lstate){
	case 1:
		kc[0] = ch;
		lstate = 2;
		if(ch == 'X')
			lstate = 3;
		break;
	case 2:
		kc[1] = ch;
		c = latin1(kc);
		nk = 2;
	putit:
		lstate = 0;
		if(c != -1)
			kbdputc(&kbdq, c);
		else for(i=0; i<nk; i++)
			kbdputc(&kbdq, kc[i]);
		break;
	case 3:
	case 4:
	case 5:
		kc[lstate-2] = ch;
		lstate++;
		break;
	case 6:
		kc[4] = ch;
		c = unicode(kc);
		nk = 5;
		goto putit;
	default:
		kbdputc(&kbdq, ch);
		break;
	}
	return 0;
}

extern void eiasetrts(int, int);
extern void eiasetparity(int, uchar);
extern void eiarawput(int, int);

int
kbdinit(int portno)
{
	initq(&liteq);
	eiaspecial(portno, &liteq, &kbdq, 600);
	eiasetrts(portno, 0);			/* 0 turns LED on */
	eiasetparity(portno, 'o');
	kbdq.putc = rawkbdputc;
	eiarawput(portno, Noclick);
	return 1;
}
