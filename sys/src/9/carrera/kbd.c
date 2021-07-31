#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"
#include	"../port/error.h"

#define	Image	IMAGE
#include	<draw.h>
#include	<cursor.h>
#include	"screen.h"

enum
{
	/* controller command byte */
	Cscs1=		(1<<6),		/* scan code set 1 */
	Cmousedis=	(1<<5),		/* mouse disable */
	Ckbddis=	(1<<4),		/* kbd disable */
	Csf=		(1<<2),		/* system flag */
	Cmouseint=	(1<<1),		/* mouse interrupt enable */
	Ckbdint=	(1<<0),		/* kbd interrupt enable */

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

	Spec=		0x80,

	PF=		Spec|0x20,	/* num pad function key */
	View=		Spec|0x00,	/* view (shift window up) */
	KF=		0xF000,	/* function key (begin Unicode private space) */
	Shift=		Spec|0x60,
	Break=		Spec|0x61,
	Ctrl=		Spec|0x62,
	Latin=		Spec|0x63,
	Caps=		Spec|0x64,
	Num=		Spec|0x65,
	Middle=		Spec|0x66,
	No=		0x00,		/* peter */

	Home=		KF|13,
	Up=		KF|14,
	Pgup=		KF|15,
	Print=		KF|16,
	Left=		KF|17,
	Right=		KF|18,
	End=		'\r',
	Down=		View,
	Pgdown=		KF|19,
	Ins=		KF|20,
	Del=		0x7F,
};

#define KBDCTL	(*(uchar*)(KeyboardIO+Keyctl))
#define KBDDAT	(*(uchar*)(KeyboardIO+Keydat))
#define OUTWAIT	while(KBDCTL & Sibf); kdbdly(1)
#define INWAIT	while(!(KBDCTL & Sobf)); kdbdly(1)
#define ACKWAIT INWAIT ; if(KBDDAT != Rack) print("bad response\n"); kdbdly(1)

Rune kbtab[] = 
{
[0x00]	No,	0x1b,	'1',	'2',	'3',	'4',	'5',	'6',
[0x08]	'7',	'8',	'9',	'0',	'-',	'=',	'\b',	'\t',
[0x10]	'q',	'w',	'e',	'r',	't',	'y',	'u',	'i',
[0x18]	'o',	'p',	'[',	']',	'\n',	Ctrl,	'a',	's',
[0x20]	'd',	'f',	'g',	'h',	'j',	'k',	'l',	';',
[0x28]	'\'',	'`',	Shift,	'\\',	'z',	'x',	'c',	'v',
[0x30]	'b',	'n',	'm',	',',	'.',	'/',	Shift,	'*',
[0x38]	Latin,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	KF|12,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
};

Rune kbtabshift[] =
{
[0x00]	No,	0x1b,	'!',	'@',	'#',	'$',	'%',	'^',
[0x08]	'&',	'*',	'(',	')',	'_',	'+',	'\b',	'\t',
[0x10]	'Q',	'W',	'E',	'R',	'T',	'Y',	'U',	'I',
[0x18]	'O',	'P',	'{',	'}',	'\n',	Ctrl,	'A',	'S',
[0x20]	'D',	'F',	'G',	'H',	'J',	'K',	'L',	':',
[0x28]	'"',	'~',	Shift,	'|',	'Z',	'X',	'C',	'V',
[0x30]	'B',	'N',	'M',	'<',	'>',	'?',	Shift,	'*',
[0x38]	Latin,	' ',	Ctrl,	KF|1,	KF|2,	KF|3,	KF|4,	KF|5,
[0x40]	KF|6,	KF|7,	KF|8,	KF|9,	KF|10,	Num,	KF|12,	'7',
[0x48]	'8',	'9',	'-',	'4',	'5',	'6',	'+',	'1',
[0x50]	'2',	'3',	'0',	'.',	No,	No,	No,	KF|11,
[0x58]	KF|12,	No,	No,	No,	No,	No,	No,	No,
};

Rune kbtabesc1[] =
{
[0x00]	No,	No,	No,	No,	No,	No,	No,	No,
[0x08]	No,	No,	No,	No,	No,	No,	No,	No,
[0x10]	No,	No,	No,	No,	No,	No,	No,	No,
[0x18]	No,	No,	No,	No,	'\n',	Ctrl,	No,	No,
[0x20]	No,	No,	No,	No,	No,	No,	No,	No,
[0x28]	No,	No,	Shift,	No,	No,	No,	No,	No,
[0x30]	No,	No,	No,	No,	No,	'/',	No,	Print,
[0x38]	Latin,	No,	No,	No,	No,	No,	No,	No,
[0x40]	No,	No,	No,	No,	No,	No,	Break,	Home,
[0x48]	Up,	Pgup,	No,	Left,	No,	Right,	No,	End,
[0x50]	Down,	Pgdown,	Ins,	Del,	No,	No,	No,	No,
[0x58]	No,	No,	No,	No,	No,	No,	No,	No,
};

struct Kbd
{
	Lock;
	int l;
} kbd;
static uchar ccc;

void
kdbdly(int l)
{
	int i;

	l *= 21;	/* experimentally determined */
	for(i=0; i<l; i++)
		;
}

/*
 *  wait for a keyboard event (or some max time)
 */
int
kbdwait(void)
{
	int tries;

	for(tries = 0; tries < 2000; tries++){
		if(KBDCTL & Sobf)
			return 1;
		kdbdly(1);
	}
	return 0;
}

void
mouseintr(void)
{
	uchar c;
	static int nb;
	int buttons, dx, dy;
	static short msg[3];
	static uchar b[] = {0, 1, 4, 5, 2, 3, 6, 7, 0, 1, 2, 5, 2, 3, 6, 7 };

	kbdwait();
	c = KBDDAT;

	/* 
	 *  check byte 0 for consistency
	 */
	if(nb==0 && (c&0xc8)!=0x08)
		return;

	msg[nb] = c;
	if(++nb == 3){
		nb = 0;
		if(msg[0] & 0x10)
			msg[1] |= 0xFF00;
		if(msg[0] & 0x20)
			msg[2] |= 0xFF00;

		buttons = b[msg[0]&7];
		dx = msg[1];
		dy = -msg[2];
		mousetrack(buttons, dx, dy);
	}
}

void
kbdintr(void)
{
	int c, i;
	static int esc1, esc2;
	static int caps;
	static int ctl;
	static int num;
	static int collecting, nk;
	static Rune kc[5];
	static int shift;
	int keyup;

	kbdwait();
	c = KBDDAT;

	/*
	 *  e0's is the first of a 2 character sequence
	 */
	if(c == 0xe0){
		esc1 = 1;
		return;
	} else if(c == 0xe1){
		esc2 = 2;
		return;
	}

	keyup = c&0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		print("unknown key %ux\n", c|keyup);
		return;
	}

	if(esc1){
		c = kbtabesc1[c];
		esc1 = 0;
	}
	else if(esc2){
		esc2--;
		return;
	}
	else if(shift)
		c = kbtabshift[c];
	else
		c = kbtab[c];

	if(caps && c<='z' && c>='a')
		c += 'A' - 'a';

	/*
	 *  keyup only important for shifts
	 */
	if(keyup){
		switch(c){
		case Shift:
			shift = 0;
			break;
		case Ctrl:
			ctl = 0;
			break;
		}
		return;
	}
	/*
 	 *  normal character
	 */
	if(!(c & Spec)){
		if(ctl)
			c &= 0x1f;
		if(!collecting){
			kbdputc(kbdq, c);
			return;
		}
		kc[nk++] = c;
		c = latin1(kc, nk);
		if(c < -1)	/* need more keystrokes */
			return;
		if(c != -1)	/* valid sequence */
			kbdputc(kbdq, c);
		else	/* dump characters */
			for(i=0; i<nk; i++)
				kbdputc(kbdq, kc[i]);
		nk = 0;
		collecting = 0;
		return;
	}
	else {
		switch(c){
		case Caps:
			caps ^= 1;
			return;
		case Num:
			num ^= 1;
			return;
		case Shift:
			shift = 1;
			return;
		case Latin:
			collecting = 1;
			nk = 0;
			return;
		case Ctrl:
			ctl = 1;
			return;
		}
	}
	kbdputc(kbdq, c);
}

void
lights(int l)
{
	USED(l);
}

static void
empty(void)
{
	int i;

	/*
	 *  empty the buffer
	 */
	kdbdly(20);
	while(KBDCTL & Sobf){
		i = KBDDAT;
		USED(i);
		kdbdly(1);
	}
}

/*
 *  send a command to the mouse
 */
static int
mousecmd(int cmd)
{
	int tries;
	unsigned int c;

	c = 0;
	tries = 0;
	do{
		if(tries++ > 2)
			break;
		OUTWAIT;
		KBDCTL = 0xD4;
		OUTWAIT;
		KBDDAT = cmd;
		OUTWAIT;
		kbdwait();
		c = KBDDAT;
	} while(c == 0xFE || c == 0);
	if(c != 0xFA){
		/*print("mouse returns %2.2ux to the %2.2ux command\n", c, cmd);/**/
		return -1;
	}
	return 0;
}

int
kbdinit(void)
{
	int i;

	kbdq = qopen(4*1024, 0, 0, 0);
	if(kbdq == nil)
		panic("kbdinit");
	qnoblock(kbdq, 1);

	/*
	 *  empty the buffer
	 */
	while(KBDCTL & Sobf){
		i = KBDDAT;
		USED(i);
	}


	/* wait for a quiescent controller */
	OUTWAIT;
	KBDCTL = 0x20;
	if(kbdwait() == 0) {
		print("kbdinit: can't read ccc\n");
		ccc = 0;
	} else
		ccc = KBDDAT;

	/* enable kbd xfers and interrupts */
	ccc &= ~Ckbddis;
	ccc |= Csf | Ckbdint | Cscs1 | Cmouseint;

	OUTWAIT;
	KBDCTL = 0x60;
	OUTWAIT;
	KBDDAT = ccc;
	OUTWAIT;

	mousecmd(0xEA);	/* streaming */
	mousecmd(0xE8);	/* set resolution */
	mousecmd(3);
	mousecmd(0xF4);	/* enabled */

	return 1;
}

void
mousectl(char* field[], int)
{
	int s;

	if(strncmp(field[0], "reset", 5) == 0){
		s = splhi();
		mousecmd(0xF6);
		mousecmd(0xEA);	/* streaming */
		mousecmd(0xE8);	/* set resolution */
		mousecmd(3);
		mousecmd(0xF4);	/* enabled */
		splx(s);
	}
	else if(strcmp(field[0], "accelerated") == 0){
		s = splhi();
		mousecmd(0xE7);
		splx(s);
	}
	else
		error(Ebadctl);
}
