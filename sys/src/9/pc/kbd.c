#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"io.h"

#include	<libg.h>
#include	<gnot.h>
#include	"screen.h"

enum {
	Data=		0x60,	/* data port */

	Status=		0x64,	/* status port */
	 Inready=	0x01,	/*  input character ready */
	 Outbusy=	0x02,	/*  output busy */
	 Sysflag=	0x04,	/*  system flag */
	 Cmddata=	0x08,	/*  cmd==0, data==1 */
	 Inhibit=	0x10,	/*  keyboard/mouse inhibited */
	 Minready=	0x20,	/*  mouse character ready */
	 Rtimeout=	0x40,	/*  general timeout */
	 Parity=	0x80,

	Cmd=		0x64,	/* command port (write only) */

	Spec=	0x80,

	PF=	Spec|0x20,	/* num pad function key */
	View=	Spec|0x00,	/* view (shift window up) */
	KF=	Spec|0x40,	/* function key */
	Shift=	Spec|0x60,
	Break=	Spec|0x61,
	Ctrl=	Spec|0x62,
	Latin=	Spec|0x63,
	Caps=	Spec|0x64,
	Num=	Spec|0x65,
	Middle=	Spec|0x66,
	No=	0x00,		/* peter */

	Home=	KF|13,
	Up=	KF|14,
	Pgup=	KF|15,
	Print=	KF|16,
	Left=	View,
	Right=	View,
	End=	'\r',
	Down=	View,
	Pgdown=	View,
	Ins=	KF|20,
	Del=	0x7F,
};

uchar kbtab[] = 
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

uchar kbtabshift[] =
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

uchar kbtabesc1[] =
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

/*
 *  keyboard input q
 */
KIOQ	kbdq;

static int mousebuttons;
static int keybuttons;

/*
 *  predeclared
 */
static void	kbdintr(Ureg*);

enum
{
	/* what kind of mouse */
	Mouseother=	0,
	Mouseserial=	1,
	MousePS2=	2,
};
static int mousetype;

/*
 *  wait for output no longer busy
 */
static int
outready(void)
{
	int tries;

	for(tries = 0; (inb(Status) & Outbusy); tries++)
		if(tries > 1000)
			return -1;
	return 0;
}

/*
 *  wait for input
 */
static int
inready(void)
{
	int tries;

	for(tries = 0; !(inb(Status) & Inready); tries++)
		if(tries > 1000)
			return -1;
	return 0;
}

/*
 *  send a command to the mouse
 */
static int
mousecmd(int cmd)
{
	unsigned int c;
	int tries;

	c = 0;
	do{
		for(tries=0; tries < 10; tries++){
			if(outready() < 0)
				return -1;
			outb(Cmd, 0xD4);
			if(outready() < 0)
				return -1;
			outb(Data, cmd);
			if(inready() < 0)
				return -1;
			c = inb(Data);
		}
	} while(c == 0xFE);
	if(c != 0xFA)
		return -1;
	return 0;
}

/*
 *  ask 8042 to enable the use of address bit 20
 */
void
i8042a20(void)
{
	outready();
	outb(Cmd, 0xD1);
	outready();
	outb(Data, 0xDF);
	outready();
}

/*
 *  ask 8042 to reset the machine
 */
void
i8042reset(void)
{
	outready();
	outb(Cmd, 0xD1);
	outready();
	outb(Data, 0xDE);
	outready();
}

void
kbdinit(void)
{
	int c;

	setvec(Kbdvec, kbdintr);

	/* wait for a quiescent controller */
	while((c = inb(Status)) & (Outbusy | Inready))
		if(c & Inready)
			inb(Data);

	/* enable kbd xfers and interrupts */
	outb(Cmd, 0x60);
	if(outready() < 0)
		print("kbd init failed\n");
	outb(Data, 0x65);
}

/*
 *  setup a serial mouse
 */
void
mouseserial(int port)
{
	if(mousetype)
		return;

	/* set up /dev/eia0 as the mouse */
	uartspecial(port, 0, &mouseq, 1200);
	mousetype = Mouseserial;
}

/*
 *  set up a ps2 mouse
 */
void
mouseps2(void)
{
	if(mousetype)
		return;

	bigcursor();
	setvec(Mousevec, kbdintr);

	/* enable kbd/mouse xfers and interrupts */
	outb(Cmd, 0x60);
	if(outready() < 0)
		print("kbd init failed\n");
	outb(Data, 0x47);
	if(outready() < 0)
		print("kbd init failed\n");
	outb(Cmd, 0xA8);

	/* make mouse streaming, enabled */
	mousecmd(0xEA);
	mousecmd(0xF4);
	mousetype = MousePS2;
}

/*
 *  turn mouse acceleration on/off
 */
void
mouseaccelerate(int on)
{

	switch(mousetype){
	case MousePS2:
		if(on)
			mousecmd(0xE7);
		else
			mousecmd(0xE6);
		break;
	}
}

/*
 *  set mouse resolution
 */
void
mouseres(int res)
{

	switch(mousetype){
	case MousePS2:
		mousecmd(0xE8);
		mousecmd(res);
		break;
	}
}

static int shift;

/*
 *  mouse message is three bytes
 *
 *	byte 0 -	0 0 SDY SDX 1 M R L
 *	byte 1 -	DX
 *	byte 2 -	DY
 *
 *  shift & left button is the same as middle button
 */
static void
mymouseputc(int c)
{
	static short msg[3];
	static int nb;
	static uchar b[] = {0, 1, 4, 5, 2, 3, 6, 7, 0, 1, 2, 5, 2, 3, 6, 7 };
	static lastdx, lastdy;
	extern Mouseinfo mouse;

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

		mousebuttons = b[(msg[0]&7) | (shift ? 8 : 0)];
		mouse.newbuttons = mousebuttons | keybuttons;
		mouse.dx = msg[1];
		mouse.dy = -msg[2];
		mouse.track = 1;
		spllo();		/* mouse tracking kills uart0 */
		mouseclock();
	}
}

/*
 *  Ctrl key used as middle button pressed
 */
static void
mbon(int val)
{
	keybuttons |= val;
	mouse.newbuttons = mousebuttons | keybuttons;
	mouse.dx = 0;
	mouse.dy = 0;
	mouse.track = 1;
	spllo();		/* mouse tracking kills uart0 */
	mouseclock();
}
static void
mboff(int val)
{
	keybuttons &= ~val;
	mouse.newbuttons = mousebuttons | keybuttons;
	mouse.dx = 0;
	mouse.dy = 0;
	mouse.track = 1;
	spllo();		/* mouse tracking kills uart0 */
	mouseclock();
}

/*
 *  keyboard interrupt
 */
int
kbdintr0(void)
{
	int s, c, i, nk;
	static int esc1, esc2;
	static int caps;
	static int ctl;
	static int num;
	static int lstate;
	static uchar kc[5];
	int keyup;

	/*
	 *  get status
	 */
	s = inb(Status);
	if(!(s&Inready))
		return -1;

	/*
	 *  get the character
	 */
	c = inb(Data);

	/*
	 *  if it's the mouse...
	 */
	if(s & Minready){
		mymouseputc(c);
		return 0;
	}

	/*
	 *  e0's is the first of a 2 character sequence
	 */
	if(c == 0xe0){
		esc1 = 1;
		return 0;
	} else if(c == 0xe1){
		esc2 = 2;
		return 0;
	}

	keyup = c&0x80;
	c &= 0x7f;
	if(c > sizeof kbtab){
		print("unknown key %ux\n", c|keyup);
		return 0;
	}

	if(esc1){
		c = kbtabesc1[c];
		esc1 = 0;
	} else if(esc2){
		esc2--;
		return 0;
	} else if(shift)
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
		case KF|1:
			mboff(4);
			break;
		case KF|2:
			mboff(2);
			break;
		case KF|3:
			mboff(1);
			break;
		}
		return 0;
	}

	/*
 	 *  normal character
	 */
	if(!(c & Spec)){
		if(ctl)
			c &= 0x1f;
		switch(lstate){
		case 1:
			kc[0] = c;
			lstate = 2;
			if(c == 'X')
				lstate = 3;
			break;
		case 2:
			kc[1] = c;
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
			kc[lstate-2] = c;
			lstate++;
			break;
		case 6:
			kc[4] = c;
			c = unicode(kc);
			nk = 5;
			goto putit;
		default:
			kbdputc(&kbdq, c);
			break;
		}
		return 0;
	} else {
		switch(c){
		case Caps:
			caps ^= 1;
			return 0;
		case Num:
			num ^= 1;
			return 0;
		case Shift:
			shift = 1;
			return 0;
		case Latin:
			lstate = 1;
			return 0;
		case Ctrl:
			ctl = 1;
			return 0;
		case KF|1:
			mbon(4);
			return 0;
		case KF|2:
			mbon(2);
			return 0;
		case KF|3:
			mbon(1);
			return 0;
		}
	}
	kbdputc(&kbdq, c);
	return 0;
}

void
kbdintr(Ureg *ur)
{
	USED(ur);
	kbdintr0();
}
