#include "all.h"

IOQ *kq;	/* keyboard input queue */

enum
{
	/*
	 *  commands
	 */
	Krdcmd=		0x20,	/* read command byte */
	Kwrcmd=		0x60,	/* write command byte */
	Kselftest=	0xAA,	/* self test */
	Ktest=	0xAB,	/* keyboard test */
	Kdisable=	0xAD,	/* disable keyboard */
	Kenable=	0xAE,	/* enable keyboard */
	Krdin=		0xC0,	/* read input port */
	Krdout=		0xD0,	/* read output port */
	Kwrout=		0xD1,	/* write output port */
	Krdtest=	0xE0,	/* read test inputs */
	Kwrlights=	0xED,	/* set lights */
	Kreset=		0xF0,	/* soft reset */
	/*
	 *  magic characters
	 */
	Msetscan=	0xF0,	/* set scan type (0 == unix) */
	Menable=	0xF4,	/* enable the keyboard */
	Mdisable=	0xF5,	/* disable the keyboard */
	Mdefault=	0xF6,	/* set defaults */
	Mreset=		0xFF,	/* reset the keyboard */
	/*
	 *  responses from keyboard
	 */
	Rok=		0xAA,		/* self test OK */
	Recho=		0xEE,		/* ??? */
	Rack=		0xFA,		/* command acknowledged */
	Rfail=		0xFC,		/* self test failed */
	Rresend=	0xFE,		/* ??? */
	Rovfl=		0xFF,		/* input overflow */
	/*
	 *  command register bits
	 */
	Cintena=	1<<0,	/* enable output interrupt */
	Csystem=	1<<2,	/* set system */
	Cinhibit=	1<<3,	/* inhibit override */
	Cdisable=	1<<4,	/* disable keyboard */
	/*
	 *  output port bits
	 */
	Osoft=		1<<0,	/* soft reset bit (must be 1?) */
	Oparity=	1<<1,	/* force bad parity */
	Omemtype=	1<<2,	/* simm type (1 = 4Mb, 0 = 1Mb)	*/
	Obigendian=	1<<3,	/* big endian */
	Ointena=	1<<4,	/* enable interrupt */
	Oclear=		1<<5,	/* clear expansion slot interrupt */
	/*
	 *  status bits
	 */
	Sobf=		1<<0,	/* output buffer full */
	Sibf=		1<<1,	/* input buffer full */
	Ssys=		1<<2,	/* set by self-test */
	Slast=		1<<3,	/* last access was to data */
	Senabled=	1<<4,	/* keyboard is enabled */
	Stxtimeout=	1<<5,	/* transmit to kybd has timed out */
	Srxtimeout=	1<<6,	/* receive from kybd has timed out */
	Sparity=	1<<7,	/* parity on byte was even */
	/*
	 *  light bits
	 */
	L1=		1<<0,	/* light 1, network activity */
	L2=		1<<2,	/* light 2, caps lock */
	L3=		1<<1,	/* light 3, no label */
};

enum
{
	Spec=	0x80,

	PF=	Spec|0x20,	/* num pad function key */
	View=	Spec|0x00,	/* view (shift window up) */
	F=	Spec|0x40,	/* function key */
	Shift=	Spec|0x60,
	Break=	Spec|0x61,
	Ctrl=	Spec|0x62,
	Latin=	Spec|0x63,
	Up=	Spec|0x70,	/* key has come up */
	No=	Spec|0x7F,	/* no mapping */
};

uchar keymap[] = {
[0]	No,	No,	No,	No,	No,	No,	No,	F|1,
	'\033',	No,	No,	No,	No,	'\t',	'`',	F|2,
[0x10]	No,	Ctrl,	Shift,	Shift,	Shift,	'q',	'1',	F|3,
	No,	Shift,	'z',	's',	'a',	'w',	'2',	F|4,
[0x20]	No,	'c',	'x',	'd',	'e',	'4',	'3',	F|5,
	No,	' ',	'v',	'f',	't',	'r',	'5',	F|6,
[0x30]	No,	'n',	'b',	'h',	'g',	'y',	'6',	F|7,
	No,	View,	'm',	'j',	'u',	'7',	'8',	F|8,
[0x40]	No,	',',	'k',	'i',	'o',	'0',	'9',	F|9,
	No,	'.',	'/',	'l',	';',	'p',	'-',	F|10,
[0x50]	No,	No,	'\'',	No,	'[',	'=',	F|11,	'\n',
	Latin,	Shift,	'\r',	']',	'\\',	No,	F|12,	Break,
[0x60]	View,	View,	Break,	Shift,	'\177',	No,	'\b',	No,
	No,	'1',	View,	'4',	'7',	',',	No,	No,
[0x70]	'0',	'.',	'2',	'5',	'6',	'8',	PF|1,	PF|2,
	No,	'\r',	'3',	No,	PF|4,	'9',	PF|3,	No,
[0x80]	No,	No,	No,	No,	'-',	No,	No,	No,
	No,	No,	No,	No,	No,	No,	No,	No
};

uchar skeymap[] = {
[0]	No,	No,	No,	No,	No,	No,	No,	F|1,
	'\033',	No,	No,	No,	No,	'\t',	'~',	F|2,
[0x10]	No,	Ctrl,	Shift,	Shift,	Shift,	'Q',	'!',	F|3,
	No,	Shift,	'Z',	'S',	'A',	'W',	'@',	F|4,
[0x20]	No,	'C',	'X',	'D',	'E',	'$',	'#',	F|5,
	No,	' ',	'V',	'F',	'T',	'R',	'%',	F|6,
[0x30]	No,	'N',	'B',	'H',	'G',	'Y',	'^',	F|7,
	No,	View,	'M',	'J',	'U',	'&',	'*',	F|8,
[0x40]	No,	'<',	'K',	'I',	'O',	')',	'(',	F|9,
	No,	'>',	'?',	'L',	':',	'P',	'_',	F|10,
[0x50]	No,	No,	'"',	No,	'{',	'+',	F|11,	'\n',
	Latin,	Shift,	'\r',	'}',	'|',	No,	F|12,	Break,
[0x60]	View,	View,	Break,	Shift,	'\177',	No,	'\b',	No,
	No,	'1',	View,	'4',	'7',	',',	No,	No,
[0x70]	'0',	'.',	'2',	'5',	'6',	'8',	PF|1,	PF|2,
	No,	'\r',	'3',	No,	PF|4,	'9',	PF|3,	No,
[0x80]	No,	No,	No,	No,	'-',	No,	No,	No,
	No,	No,	No,	No,	No,	No,	No,	No
};

static void
outwait(void)
{
	KBDdev *k = KBD;

	while(k->ctl & Sibf)
		;
	delay(1);
}

static void
inwait(void)
{
	KBDdev *k = KBD;

	while(!(k->ctl & Sobf))
		;
	delay(1);
}

static void
ackwait(void)
{
	KBDdev *k = KBD;

	inwait();
	if(k->data != Rack)
		print("bad kbd response\n");
	delay(1);
}

void
kbdintr(void)
{
	KBDdev *k = KBD;
	static int shifted;
	static int ctrled;
	uchar ch, code;

	inwait();
	code = k->data;

	/*
	 *  key has gone up
	 */
	if(code == Up){
		inwait();
		ch = keymap[k->data];
		if(ch == Ctrl)
			ctrled = 0;
		else if(ch == Shift)
			shifted = 0;
		return;
	}

	/*
	 *  convert
	 */
	if(shifted)
		ch = skeymap[code];
	else
		ch = keymap[code];

	/*
 	 *  normal character
	 */
	if(!(ch & Spec)){
		if(ctrled)
			ch &= 0x1f;
		putc(&kbdq, ch);
		return;
	}

	/*
	 *  special character
	 */
	switch(ch){
	case Shift:
		shifted = 1;
		break;
	case Break:
		break;
	case Ctrl:
		ctrled = 1;
		break;
	default:
		putc(&kbdq, ch);
	}
}

IOQ	kbdq;

void
kbdinit(void)
{
	KBDdev *k = KBD;
	int i;

	initq(&kbdq);

	/*
	 *  empty the buffer
	 */
	while(k->ctl & Sobf){
		i = k->data;
		USED(i);
	}

	/*
	 *  disable the interface
	 */
	outwait();
	k->ctl = Kwrcmd;
	outwait();
	k->data = Csystem | Cinhibit | Cdisable | Cintena;

	/*
	 *  set unix scan on the keyboard
	 */
	outwait();
	k->data = Mdisable;
	inwait();
	/*
	 * if this fails, the keyboard is not there
	 */
	if(k->data != Rack){
		ttyscreen();
		return;
	}
	delay(1);
	outwait();
	k->data = Msetscan;
	ackwait();
	outwait();
	k->data = 0;
	ackwait();
	outwait();
	k->data = Menable;

	/*
	 *  enable the interface
	 */
	outwait();
	k->ctl = Kwrcmd;
	outwait();
	k->data = Csystem | Cinhibit | Cintena;
	outwait();
	k->ctl = Kenable;

	/*
	 *  empty the buffer
	 */
	while(k->ctl & Sobf){
		i = k->data;
		USED(i);
	}
}

#define IMR		IO(ushort, 0x8020002)
#define INTCLEAR	IO(ulong, 0x400000)
void
devinit(void)
{
	/*
	 * disable the lance
	 */
	*LANCERAP = 0;
	*LANCERDP = 0x4;

	/*
	 * disable atbus intr
	 */
	*IMR = 0xFF;
	*INTCLEAR = 1;

	/*
	 * disable the scc (clear master interrupt enable)
	 */
	SCCADDR->ptra = 9;
	wbflush();
	SCCADDR->ptra = 0;
	wbflush();
	SCCADDR->ptrb = 9;
	wbflush();
	SCCADDR->ptrb = 0;
	wbflush();

	/*
	 * initialize the clock
	 */
	*TCOUNT = 0;
	*TBREAK = COUNT;
	wbflush();
}
