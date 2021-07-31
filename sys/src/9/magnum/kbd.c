#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#include	"devtab.h"

enum
{
	/*
	 *  commands
	 */
	Krdcmd=		0x20,	/* read command byte */
	Kwrcmd=		0x60,	/* write command byte */
	Kselftest=	0xAA,	/* self test */
	Ktest=		0xAB,	/* keyboard test */
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

#define OUTWAIT	while(k->ctl & Sibf); Xdelay(1)
#define INWAIT	while(!(k->ctl & Sobf)); Xdelay(1)
#define ACKWAIT INWAIT ; if(k->data != Rack) print("bad response\n"); Xdelay(1)

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

	Tmask=	Spec|0x60,
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
[0x50]	No,	No,	'\'',	No,	'[',	'=',	F|11,	'\r',
	Latin,	Shift,	'\n',	']',	'\\',	No,	F|12,	Break,
[0x60]	View,	View,	Break,	Shift,	'\177',	No,	'\b',	No,
	No,	'1',	View,	'4',	'7',	',',	No,	No,
[0x70]	'0',	'.',	'2',	'5',	'6',	'8',	PF|1,	PF|2,
	No,	'\n',	'3',	No,	PF|4,	'9',	PF|3,	No,
[0x80]	No,	No,	No,	No,	'-',	No,	No,	No,
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
[0x50]	No,	No,	'"',	No,	'{',	'+',	F|11,	'\r',
	Latin,	Shift,	'\n',	'}',	'|',	No,	F|12,	Break,
[0x60]	View,	View,	Break,	Shift,	'\177',	No,	'\b',	No,
	No,	'1',	View,	'4',	'7',	',',	No,	No,
[0x70]	'0',	'.',	'2',	'5',	'6',	'8',	PF|1,	PF|2,
	No,	'\n',	'3',	No,	PF|4,	'9',	PF|3,	No,
[0x80]	No,	No,	No,	No,	'-',	No,	No,	No,
};


struct Kbd
{
	Lock;
	int l;
} kbd;

/*
 *  wait for a keyboard event (or some max time)
 */
int
kbdwait(KBDdev *k)
{
	int tries;

	for(tries = 0; tries < 2000; tries++){
		if(k->ctl & Sobf){
			Xdelay(1);
			return 1;
		}
		Xdelay(1);
	}
	return 0;
}

/*
 *  wait for a keyboard acknowledge (or some max time)
 */
int
kbdackwait(KBDdev *k)
{
	if(kbdintr())
		return k->data;
	return 0;
}

int
kbdintr(void)
{
	KBDdev *k;
	static int shifted;
	static int ctrled;
	static int collecting, nk;
	static uchar kc[5];
	uchar ch, code;
	int c, i;

	k = KBD;
	kbdwait(k);
	code = k->data;

	/*
	 *  key has gone up
	 */
	if(code == Up){
		kbdwait(k);
		ch = keymap[k->data];
		if(ch == Ctrl)
			ctrled = 0;
		else if(ch == Shift)
			shifted = 0;
		return 0;
	}

	if(code > 0x87)
		return 1;

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
		if(!collecting){
			kbdputc(&kbdq, ch);
			return 0;
		}
		kc[nk++] = ch;
		c = latin1(kc, nk);
		if(c < -1)	/* need more keystrokes */
			return 0;
		if(c != -1)	/* valid sequence */
			kbdputc(&kbdq, c);
		else	/* dump characters */
			for(i=0; i<nk; i++)
				kbdputc(&kbdq, kc[i]);
		nk = 0;
		collecting = 0;
		return 0;
	}

	/*
	 *  filter out function keys (because ches doesn't like them)
	 */
	if((Tmask&ch) == (Spec|F))
		return 0;

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
	case Latin:
		collecting = 1;
		nk = 0;
		break;
	default:
		kbdputc(&kbdq, ch);
	}
	return 0;
}

void
lights(int l)
{
	KBDdev *k;
	int s;
	int tries;

	s = splhi();
	k = KBD;
	for(tries = 0; tries < 2000 && (k->ctl & Sibf); tries++)
		;
	Xdelay(1);
	k->data = Kwrlights;
	kbdackwait(k);
	for(tries = 0; tries < 2000 && (k->ctl & Sibf); tries++)
		;
	Xdelay(1);
	k->data = kbd.l = l;
	kbdackwait(k);
	splx(s);
}

struct Buzz {
	QLock;
	Rendez;
} buzzer;

void
buzz(int f, int d)
{
	qlock(&buzzer);
	if(waserror()){
		*CREG = (*CREG & ~Buzzbits)|Buzzdis;
		qunlock(&buzzer);
		nexterror();
	}
	if(f <= 190/2)
		tsleep(&buzzer, return0, 0, d);
	else {
		if(f < 190+190/2)
			f = 3;
		else if(f < 381+381/2)
			f = 2;
		else if(f < 762+762/2)
			f = 1;
		else
			f = 0;
		*CREG = (*CREG & ~Buzzbits)|((f&7)<<4);
		tsleep(&buzzer, return0, 0, d);
		*CREG = (*CREG & ~Buzzbits)|Buzzdis;
	}
	poperror();
	qunlock(&buzzer);
};

static void
empty(void)
{
	KBDdev *k = KBD;
	int i;

	/*
	 *  empty the buffer
	 */
	Xdelay(20);
	while(k->ctl & Sobf){
		i = k->data;
		USED(i);
		Xdelay(1);
	}
}

int
kbdinit(void)
{
	KBDdev *k;
	int i;

	k = KBD;

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
	OUTWAIT;
	k->ctl = Kwrcmd;
	OUTWAIT;
	k->data = Csystem | Cinhibit | Cdisable | Cintena;

	/*
	 *  set unix scan on the keyboard
	 */
	OUTWAIT;
	k->data = Mdisable;
	INWAIT;
	if(k->data != Rack)
		return 0;
	OUTWAIT;
	k->data = Msetscan;
	ACKWAIT;
	OUTWAIT;
	k->data = 0;
	ACKWAIT;
	OUTWAIT;
	k->data = Menable;

	/*
	 *  enable the interface
	 */
	OUTWAIT;
	k->ctl = Kwrcmd;
	OUTWAIT;
	k->data = Csystem | Cinhibit | Cintena;
	OUTWAIT;
	k->ctl = Kenable;
	empty();

	return 1;
}

int
setsimmtype(int type)
{
	KBDdev *k = KBD;
	uchar data;

	empty();

	/*
	 * read current value and mask out
	 * the SIMM type
	 */
	OUTWAIT;
	k->ctl = Krdout;
	INWAIT;
	data = (k->data & ~Omemtype);

	/*
	 * write out with the new value
	 */
	if(type == 4)
		data |= Omemtype;
	OUTWAIT;
	k->ctl = Kwrout;
	OUTWAIT;
	k->data = data;

	empty();
	/*
	 * let things settle
	 */
	delay(10);
	return data;
}
